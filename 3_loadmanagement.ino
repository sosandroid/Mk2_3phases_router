/**
 * @file loadmanagement.ino
 * @author Robin Emley (www.Mk2PVrouter.co.uk)
 * @author Frederic Metrich (frederic.metrich@live.fr)
 * @author Emmanuel Havet (@sosandroid)
 * @brief load management functions
 * @date 2022-12-29
 */
 
 

/**
 * @brief This function set all 3 loads to full power.
 *
 * @return true if loads are forced
 * @return false
 */
bool forceFullPower()
{
#ifdef FORCE_PIN_PRESENT
  const uint8_t pinState{!!(PIND & bit(forcePin))};

  for (auto &bForceLoad : b_forceLoadOn)
    bForceLoad = !pinState;

  return !pinState;
#else
  return false;
#endif
}

/**
 * @brief This function changes the value of the load priorities.
 * @details Since we don't have access to a clock, we detect the offPeak start from the main energy meter.
 *          Additionally, when off-peak period starts, we rotate the load priorities for the next day.
 *
 * @param currentTemperature_x100 current temperature x 100 (default to 0 if deactivated)
 * @return true if off-peak tariff is active
 * @return false if on-peak tariff is active
 */
bool proceedLoadPrioritiesAndForcing(const int16_t currentTemperature_x100)
{
#ifdef OFF_PEAK_TARIFF
  uint8_t i;
  constexpr int16_t iTemperatureThreshold_x100{uiTemperature * 100};
  static uint8_t pinOffPeakState{HIGH};
  const uint8_t pinNewState{!!(PIND & bit(offPeakForcePin))};

  if (pinOffPeakState && !pinNewState)
  {
// we start off-peak period
#ifndef NO_OUTPUT
    Serial.println(F("Change to off-peak period!"));
#endif // NO_OUTPUT
    ul_TimeOffPeak = millis();

	waitPrioritiesRotated(false);
	// prints the (new) load priorities
	logLoadPriorities();
  }
  else
  {
    const auto ulElapsedTime{(uint32_t)(millis() - ul_TimeOffPeak)};

    for (i = 0; i < NO_OF_DUMPLOADS; ++i)
    {
      // for each load, if we're inside off-peak period and within the 'force period', trigger the ISR to turn the
      // load ON
      if (!pinOffPeakState && !pinNewState && (ulElapsedTime >= rg_OffsetForce[i][0]) &&
          (ulElapsedTime < rg_OffsetForce[i][1]))
        b_forceLoadOn[i] = currentTemperature_x100 <= iTemperatureThreshold_x100;
      else
        b_forceLoadOn[i] = false;
    }
  }
// end of off-peak period
#ifndef NO_OUTPUT
	if (!pinOffPeakState && pinNewState)
	Serial.println(F("Change to peak period!"));
#endif // NO_OUTPUT

	pinOffPeakState = pinNewState;
	return (LOW == pinOffPeakState);
#else // OFF_PEAK_TARIFF
	waitPrioritiesRotated(true);
	// prints the (new) load priorities
	logLoadPriorities();
	return false;
#endif // OFF_PEAK_TARIFF
}

/**
 * @brief This function waits for rotated priorities from ISR
 * @details According to working mode, we can discard the reset absenceOfDivertedEnergyCount
 *			false : not reseted
 *			true : reseted
 *
 * @param working mode boolean
 * @return none
 */
void waitPrioritiesRotated(bool mode=false) {
#ifdef PRIORITY_ROTATION
	if (mode && ROTATION_AFTER_CYCLES < absenceOfDivertedEnergyCount) {
		absenceOfDivertedEnergyCount = 0;
	}
	b_reOrderLoads = true;

	// waits till the priorities have been rotated from inside the ISR
	do {
		delay(10);
	} while (b_reOrderLoads);

#endif // PRIORITY_ROTATION
}

