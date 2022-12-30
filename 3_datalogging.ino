/**
 * @file datalogging.ino
 * @author Robin Emley (www.Mk2PVrouter.co.uk)
 * @author Frederic Metrich (frederic.metrich@live.fr)
 * @author Emmanuel Havet (@sosandroid)
 * @brief datalogging - all function related to datalogging
 * @date 2022-12-29
 */

/**
 * @brief Prints data logs to the Serial output in text or json format
 *
 * @param bOffPeak true if off-peak tariff is active
 */
void sendResults(bool bOffPeak)
{
  uint8_t phase;

#ifdef RF_PRESENT
  send_rf_data(); // *SEND RF DATA*
#endif

#if defined SERIALOUT && !defined EMONESP
  Serial.print(copyOf_energyInBucket_main / SUPPLY_FREQUENCY);
  Serial.print(F(", P:"));
  Serial.print(tx_data.power);

  for (phase = 0; phase < NO_OF_PHASES; ++phase)
  {
    Serial.print(F(", P"));
    Serial.print(phase + 1);
    Serial.print(F(":"));
    Serial.print(tx_data.power_L[phase]);
  }
  for (phase = 0; phase < NO_OF_PHASES; ++phase)
  {
    Serial.print(F(", V"));
    Serial.print(phase + 1);
    Serial.print(F(":"));
    Serial.print((float)tx_data.Vrms_L_x100[phase] / 100);
  }

#ifdef TEMP_SENSOR
  Serial.print(", temperature ");
  Serial.print((float)tx_data.temperature_x100 / 100);
#endif
  Serial.println(F(")"));
#endif // if defined SERIALOUT && !defined EMONESP

#if defined EMONESP && !defined SERIALOUT
  StaticJsonDocument<200> doc;
  char strPhase[]{"L0"};
  char strLoad[]{"LOAD_0"};

  for (phase = 0; phase < NO_OF_PHASES; ++phase)
  {
    doc[strPhase] = tx_data.power_L[phase];
    ++strPhase[1];
  }

  for (uint8_t i = 0; i < NO_OF_DUMPLOADS; ++i)
  {
    doc[strLoad] = (100 * copyOf_countLoadON[i]) / DATALOG_PERIOD_IN_MAINS_CYCLES;
    ++strLoad[5];
  }

#ifdef OFF_PEAK_TARIFF
  doc["OFF_PEAK_TARIFF"] = bOffPeak ? true : false;
#endif

  // Generate the minified JSON and send it to the Serial port.
  //
  serializeJson(doc, Serial);

  // Start a new line
  Serial.println();
  delay(50);
#endif // if defined EMONESP && !defined SERIALOUT

#if defined SERIALPRINT && !defined EMONESP
  Serial.print(copyOf_energyInBucket_main / SUPPLY_FREQUENCY);
  Serial.print(F(", P:"));
  Serial.print(tx_data.power);

  for (phase = 0; phase < NO_OF_PHASES; ++phase)
  {
    Serial.print(F(", P"));
    Serial.print(phase + 1);
    Serial.print(F(":"));
    Serial.print(tx_data.power_L[phase]);
  }
  for (phase = 0; phase < NO_OF_PHASES; ++phase)
  {
    Serial.print(F(", V"));
    Serial.print(phase + 1);
    Serial.print(F(":"));
    Serial.print((float)tx_data.Vrms_L_x100[phase] / 100);
  }

#ifdef TEMP_SENSOR
  Serial.print(", temperature ");
  Serial.print((float)tx_data.temperature_x100 / 100);
#endif // TEMP_SENSOR
  Serial.print(F(", (minSampleSets/MC "));
  Serial.print(copyOf_lowestNoOfSampleSetsPerMainsCycle);
  Serial.print(F(", #ofSampleSets "));
  Serial.print(copyOf_sampleSetsDuringThisDatalogPeriod);
#ifndef OFF_PEAK_TARIFF
#ifdef PRIORITY_ROTATION
  Serial.print(F(", NoED "));
  Serial.print(absenceOfDivertedEnergyCount);
#endif // PRIORITY_ROTATION
#endif // OFF_PEAK_TARIFF
  Serial.println(F(")"));
#endif // if defined SERIALPRINT && !defined EMONESP
}

/**
 * @brief Prints the load priorities to the Serial output.
 *
 */
void logLoadPriorities()
{
#ifdef DEBUGGING
  Serial.println(F("loadPriority: "));
  for (const auto loadPrioAndState : loadPrioritiesAndState)
  {
    Serial.print(F("\tload "));
    Serial.println(loadPrioAndState);
  }
#endif
}


/**
 * @brief Print the configuration during start
 *
 */
void printConfiguration()
{
  Serial.println();
  Serial.println();
  Serial.println(F("----------------------------------"));
  Serial.print(F("Sketch ID: "));
  Serial.println(__FILE__);
  Serial.print(F("Build on "));
  Serial.print(__DATE__);
  Serial.print(F(" "));
  Serial.println(__TIME__);

  Serial.println(F("ADC mode:       free-running"));

  Serial.println(F("Electrical settings"));
  for (uint8_t phase = 0; phase < NO_OF_PHASES; ++phase)
  {
    Serial.print(F("\tf_powerCal for L"));
    Serial.print(phase + 1);
    Serial.print(F(" =    "));
    Serial.println(f_powerCal[phase], 5);

    Serial.print(F("\tf_voltageCal, for Vrms_L"));
    Serial.print(phase + 1);
    Serial.print(F(" =    "));
    Serial.println(f_voltageCal[phase], 5);
  }

  Serial.print(F("\tf_phaseCal for all phases =     "));
  Serial.println(f_phaseCal);

  Serial.print(F("\tExport rate (Watts) = "));
  Serial.println(REQUIRED_EXPORT_IN_WATTS);

  Serial.print(F("\tzero-crossing persistence (sample sets) = "));
  Serial.println(PERSISTENCE_FOR_POLARITY_CHANGE);

  printParamsForSelectedOutputMode();

  Serial.print("Temperature capability ");
#ifdef TEMP_SENSOR
  Serial.println(F("is present"));
#else
  Serial.println(F("is NOT present"));
#endif

  Serial.print("Dual-tariff capability ");
#ifdef OFF_PEAK_TARIFF
  Serial.println(F("is present"));
  printOffPeakConfiguration();
#else
  Serial.println(F("is NOT present"));
#endif

  Serial.print("Load rotation feature ");
#ifdef PRIORITY_ROTATION
  Serial.println(F("is present"));
#else
  Serial.println(F("is NOT present"));
#endif

  Serial.print("RF capability ");
#ifdef RF_PRESENT
  Serial.print(F("IS present, Freq = "));
  if (FREQ == RF12_433MHZ)
    Serial.println(F("433 MHz"));
  else if (FREQ == RF12_868MHZ)
    Serial.println(F("868 MHz"));
  rf12_initialize(nodeID, FREQ, networkGroup); // initialize RF
#else
  Serial.println(F("is NOT present"));
#endif

  Serial.print("Datalogging capability ");
#ifdef SERIALPRINT
  Serial.println(F("is present"));
#else
  Serial.println(F("is NOT present"));
#endif
}

#ifdef OFF_PEAK_TARIFF
/**
 * @brief Print the settings for off-peak period
 *
 */
void printOffPeakConfiguration()
{
  Serial.print(F("\tDuration of off-peak period is "));
  Serial.print(ul_OFF_PEAK_DURATION);
  Serial.println(F(" hours."));

  Serial.print(F("\tTemperature threshold is "));
  Serial.print(uiTemperature);
  Serial.println(F("°C."));

  for (uint8_t i = 0; i < NO_OF_DUMPLOADS; ++i)
  {
    Serial.print(F("\tLoad #"));
    Serial.print(i + 1);
    Serial.println(F(":"));

    Serial.print(F("\t\tStart "));
    if (rg_ForceLoad[i].getStartOffset() >= 0)
    {
      Serial.print(rg_ForceLoad[i].getStartOffset());
      Serial.print(F(" hours/minutes after begin of off-peak period "));
    }
    else
    {
      Serial.print(-rg_ForceLoad[i].getStartOffset());
      Serial.print(F(" hours/minutes before the end of off-peak period "));
    }
    if (rg_ForceLoad[i].getDuration() == UINT16_MAX)
      Serial.println(F("till the end of the period."));
    else
    {
      Serial.print(F("for a duration of "));
      Serial.print(rg_ForceLoad[i].getDuration());
      Serial.println(F(" hour/minute(s)."));
    }
    Serial.print(F("\t\tCalculated offset in seconds: "));
    Serial.println(rg_OffsetForce[i][0] / 1000);
    Serial.print(F("\t\tCalculated duration in seconds: "));
    Serial.println(rg_OffsetForce[i][1] / 1000);
  }
}
#endif

/**
 * @brief Print the settings used for the selected output mode.
 *
 */
void printParamsForSelectedOutputMode()
{
#ifndef NO_OUTPUT
  // display relevant settings for selected output mode
  Serial.print("Output mode:    ");
  if (OutputModes::NORMAL == outputMode)
    Serial.println("normal");
  else
  {
    Serial.println("anti-flicker");
    Serial.print("\toffsetOfEnergyThresholds  = ");
    Serial.println(f_offsetOfEnergyThresholdsInAFmode);
  }
  Serial.print(F("\tf_capacityOfEnergyBucket_main = "));
  Serial.println(f_capacityOfEnergyBucket_main);
  Serial.print(F("\tf_lowerEnergyThreshold   = "));
  Serial.println(f_lowerThreshold_default);
  Serial.print(F("\tf_upperEnergyThreshold   = "));
  Serial.println(f_upperThreshold_default);
#endif
}

#ifdef RF_PRESENT
/**
 * @brief Send the logging data through RF.
 * @details For better performance, the RFM12B needs to remain in its
 *          active state rather than being periodically put to sleep.
 *
 */
void send_rf_data()
{
  // check whether it's ready to send, and an exit route if it gets stuck
  uint8_t i = 10;
  do
  {
    rf12_recvDone();
  } while (!rf12_canSend() && --i)

      rf12_sendNow(0, &tx_data, sizeof tx_data);
}
#endif // #ifdef RF_PRESENT