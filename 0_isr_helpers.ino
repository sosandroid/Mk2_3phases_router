/**
 * @file isr_helpers.ino
 * @author Robin Emley (www.Mk2PVrouter.co.uk)
 * @author Frederic Metrich (frederic.metrich@live.fr)
 * @author Emmanuel Havet (@sosandroid)
 * @brief isr_helpers - all functions needed by ISR fonction
 * @date 2022-12-29
 */
 
 /* -----------------------------------------------------------
   Start of various helper functions which are used by the ISR
*/

/*!
 *  @defgroup TimeCritical Time critical functions Group
 *  Functions used by the ISR
 */

/**
 * @brief Process the calculation for the actual current raw sample for the specific phase
 *
 * @param phase the phase number [0..NO_OF_PHASES[
 * @param rawSample the current sample for the specified phase
 *
 * @ingroup TimeCritical
 */
void processCurrentRawSample(const uint8_t phase, const int16_t rawSample)
{
  // remove most of the DC offset from the current sample (the precise value does not matter)
  int32_t sampleIminusDC = ((int32_t)(rawSample - l_DCoffset_I_nom)) << 8;
  //
  // phase-shift the voltage waveform so that it aligns with the grid current waveform
  int32_t phaseShiftedSampleVminusDC = l_lastSampleVminusDC[phase] + (((l_sampleVminusDC[phase] - l_lastSampleVminusDC[phase]) << p_phaseCal) >> 8);
  //
  // calculate the "real power" in this sample pair and add to the accumulated sum
  int32_t filtV_div4 = phaseShiftedSampleVminusDC >> 2; // reduce to 16-bits (now x64, or 2^6)
  int32_t filtI_div4 = sampleIminusDC >> 2;             // reduce to 16-bits (now x64, or 2^6)
  int32_t instP = filtV_div4 * filtI_div4;              // 32-bits (now x4096, or 2^12)
  instP >>= 12;                                         // scaling is now x1, as for Mk2 (V_ADC x I_ADC)

  l_sumP[phase] += instP;               // cumulative power, scaling as for Mk2 (V_ADC x I_ADC)
  l_sumP_atSupplyPoint[phase] += instP; // cumulative power, scaling as for Mk2 (V_ADC x I_ADC)
}

/**
 * @brief Process the current voltage raw sample for the specific phase
 *
 * @param phase the phase number [0..NO_OF_PHASES[
 * @param rawSample the current sample for the specified phase
 *
 * @ingroup TimeCritical
 */
void processVoltageRawSample(const uint8_t phase, const int16_t rawSample)
{
  processPolarity(phase, rawSample);
  confirmPolarity(phase);
  //
  processRawSamples(phase); // deals with aspects that only occur at particular stages of each mains cycle
  //
  processVoltage(phase);

  if (phase == 0)
    ++i_sampleSetsDuringThisDatalogPeriod;
}

/**
 * @brief Process with the polarity for the actual voltage sample for the specific phase
 *
 * @param phase the phase number [0..NO_OF_PHASES[
 * @param rawSample the current sample for the specified phase
 *
 * @ingroup TimeCritical
 */
void processPolarity(const uint8_t phase, const int16_t rawSample)
{
  l_lastSampleVminusDC[phase] = l_sampleVminusDC[phase]; // required for phaseCal algorithm
  // remove DC offset from each raw voltage sample by subtracting the accurate value
  // as determined by its associated LP filter.
  l_sampleVminusDC[phase] = (((int32_t)rawSample) << 8) - l_DCoffset_V[phase];
  polarityOfMostRecentSampleV[phase] = (l_sampleVminusDC[phase] > 0) ? Polarities::POSITIVE : Polarities::NEGATIVE;
}

/**
 * @brief This routine prevents a zero-crossing point from being declared until a certain number
 *        of consecutive samples in the 'other' half of the waveform have been encountered.
 *
 * @param phase the phase number [0..NO_OF_PHASES[
 *
 * @ingroup TimeCritical
 */
void confirmPolarity(const uint8_t phase)
{
  static uint8_t count[NO_OF_PHASES]{};

  if (polarityOfMostRecentSampleV[phase] == polarityConfirmedOfLastSampleV[phase])
  {
    count[phase] = 0;
    return;
  }

  ++count[phase];
  if (count[phase] > PERSISTENCE_FOR_POLARITY_CHANGE)
  {
    count[phase] = 0;
    polarityConfirmed[phase] = polarityOfMostRecentSampleV[phase];
  }
}

/**
 * @brief Process the calculation for the current voltage sample for the specific phase
 *
 * @param phase the phase number [0..NO_OF_PHASES[
 *
 * @ingroup TimeCritical
 */
void processVoltage(const uint8_t phase)
{
  // for the Vrms calculation (for datalogging only)
  int32_t filtV_div4 = l_sampleVminusDC[phase] >> 2; // reduce to 16-bits (now x64, or 2^6)
  int32_t inst_Vsquared = filtV_div4 * filtV_div4;   // 32-bits (now x4096, or 2^12)
  inst_Vsquared >>= 12;                              // scaling is now x1 (V_ADC x I_ADC)
  l_sum_Vsquared[phase] += inst_Vsquared;            // cumulative V^2 (V_ADC x I_ADC)
  //
  // store items for use during next loop
  l_cumVdeltasThisCycle[phase] += l_sampleVminusDC[phase];          // for use with LP filter
  polarityConfirmedOfLastSampleV[phase] = polarityConfirmed[phase]; // for identification of half cycle boundaries
  ++n_samplesDuringThisMainsCycle[phase];                           // for real power calculations
}

/**
 * @brief This routine is called by the ISR when a pair of V & I sample becomes available.
 *
 * @param phase the phase number [0..NO_OF_PHASES[
 *
 * @ingroup TimeCritical
 */
void processRawSamples(const uint8_t phase)
{
  // The raw V and I samples are processed in "phase pairs"
  if (Polarities::POSITIVE == polarityConfirmed[phase])
  {
    // the polarity of this sample is positive
    if (Polarities::POSITIVE != polarityConfirmedOfLastSampleV[phase])
    {
      if (beyondStartUpPeriod)
      {
        // This is the start of a new +ve half cycle, for this phase, just after the zero-crossing point.
        processPlusHalfCycle(phase);
      }
      else
        processStartUp(phase);
    }

    // still processing samples where the voltage is POSITIVE ...
    // check to see whether the trigger device can now be reliably armed
    if (beyondStartUpPeriod && (0 == phase) &&
        (2 == n_samplesDuringThisMainsCycle[0])) // lower value for larger sample set
    {
      // This code is executed once per 20mS, shortly after the start of each new mains cycle on phase 0.
      processStartNewCycle();
    }
  }
  else
  {
    // the polarity of this sample is negative
    if (Polarities::NEGATIVE != polarityConfirmedOfLastSampleV[phase])
    {
      // This is the start of a new -ve half cycle (just after the zero-crossing point)
      processMinusHalfCycle(phase);
    }
  }
}
// end of processRawSamples()

/**
 * @brief Process the startup period for the router.
 *
 * @param phase the phase number [0..NO_OF_PHASES[
 *
 * @ingroup TimeCritical
 */
void processStartUp(const uint8_t phase)
{
  // wait until the DC-blocking filters have had time to settle
  if (millis() <= (initialDelay + startUpPeriod))
    return; // still settling, do nothing

  // the DC-blocking filters have had time to settle
  beyondStartUpPeriod = true;
  l_sumP[phase] = 0;
  l_sumP_atSupplyPoint[phase] = 0;
  n_samplesDuringThisMainsCycle[phase] = 0;
  i_sampleSetsDuringThisDatalogPeriod = 0;

  n_lowestNoOfSampleSetsPerMainsCycle = UINT8_MAX;
  // can't say "Go!" here 'cos we're in an ISR!
}

/**
 * @brief This code is executed once per 20mS, shortly after the start of each new
 *        mains cycle on phase 0.
 * @details Changing the state of the loads  is a 3-part process:
 *          - change the LOGICAL load states as necessary to maintain the energy level
 *          - update the PHYSICAL load states according to the logical -> physical mapping
 *          - update the driver lines for each of the loads.
 *
 * @ingroup TimeCritical
 */
void processStartNewCycle()
{
  // Restrictions apply for the period immediately after a load has been switched.
  // Here the b_recentTransition flag is checked and updated as necessary.
  // if (b_recentTransition)
  //   b_recentTransition = (++postTransitionCount < POST_TRANSITION_MAX_COUNT);
  // for optimization, the next line is equivalent to the two lines above
  b_recentTransition &= (++postTransitionCount < POST_TRANSITION_MAX_COUNT);

  if (f_energyInBucket_main > f_midPointOfEnergyBucket_main)
  {
    // the energy state is in the upper half of the working range
    f_lowerEnergyThreshold = f_lowerThreshold_default; // reset the "opposite" threshold
    if (f_energyInBucket_main > f_upperEnergyThreshold)
    {
      // Because the energy level is high, some action may be required
      proceedHighEnergyLevel();
    }
  }
  else
  {
    // the energy state is in the lower half of the working range
    f_upperEnergyThreshold = f_upperThreshold_default; // reset the "opposite" threshold
    if (f_energyInBucket_main < f_lowerEnergyThreshold)
    {
      // Because the energy level is low, some action may be required
      proceedLowEnergyLevel();
    }
  }

  updatePhysicalLoadStates(); // allows the logical-to-physical mapping to be changed

  updatePortsStates(); // update the control ports for each of the physical loads

  // Now that the energy-related decisions have been taken, min and max limits can now
  // be applied  to the level of the energy bucket. This is to ensure correct operation
  // when conditions change, i.e. when import changes to export, and vice versa.
  //
  if (f_energyInBucket_main > f_capacityOfEnergyBucket_main)
    f_energyInBucket_main = f_capacityOfEnergyBucket_main;
  else if (f_energyInBucket_main < 0)
    f_energyInBucket_main = 0;
}

/**
 * @brief Process the start of a new -ve half cycle, for this phase, just after the zero-crossing point.
 *
 * @param phase the phase number [0..NO_OF_PHASES[
 *
 * @ingroup TimeCritical
 */
void processMinusHalfCycle(const uint8_t phase)
{
  // This is a convenient point to update the Low Pass Filter for removing the DC
  // component from the phase that is being processed.
  // The portion which is fed back into the integrator is approximately one percent
  // of the average offset of all the SampleVs in the previous mains cycle.
  //
  l_DCoffset_V[phase] += (l_cumVdeltasThisCycle[phase] >> 12);
  l_cumVdeltasThisCycle[phase] = 0;

  // To ensure that this LP filter will always start up correctly when 240V AC is
  // available, its output value needs to be prevented from drifting beyond the likely range
  // of the voltage signal.
  //
  if (l_DCoffset_V[phase] < l_DCoffset_V_min)
    l_DCoffset_V[phase] = l_DCoffset_V_min;
  else if (l_DCoffset_V[phase] > l_DCoffset_V_max)
    l_DCoffset_V[phase] = l_DCoffset_V_max;
}

/**
 * @brief Retrieve the next load that could be added (be aware of the order)
 *
 * @return The load number if successfull, NO_OF_DUMPLOADS in case of failure
 *
 * @ingroup TimeCritical
 */
uint8_t nextLogicalLoadToBeAdded()
{
  for (uint8_t index = 0; index < NO_OF_DUMPLOADS; ++index)
    if (0x00 == (loadPrioritiesAndState[index] & loadStateOnBit))
      return (index);

  return (NO_OF_DUMPLOADS);
}

/**
 * @brief Retrieve the next load that could be removed (be aware of the reverse-order)
 *
 * @return The load number if successfull, NO_OF_DUMPLOADS in case of failure
 *
 * @ingroup TimeCritical
 */
uint8_t nextLogicalLoadToBeRemoved()
{
  uint8_t index{NO_OF_DUMPLOADS};
  do
  {
    --index;
    if (loadPrioritiesAndState[index] & loadStateOnBit)
      return (index);
  } while (index);

  return (NO_OF_DUMPLOADS);
}

/**
 * @brief Process the case of high energy level, some action may be required.
 *
 * @ingroup TimeCritical
 */
void proceedHighEnergyLevel()
{
  bool bOK_toAddLoad{true};
  auto tempLoad{nextLogicalLoadToBeAdded()};

  if (tempLoad >= NO_OF_DUMPLOADS)
    return;

  // a load which is now OFF has been identified for potentially being switched ON
  if (b_recentTransition)
  {
    // During the post-transition period, any increase in the energy level is noted.
    f_upperEnergyThreshold = f_energyInBucket_main;

    // the energy thresholds must remain within range
    if (f_upperEnergyThreshold > f_capacityOfEnergyBucket_main)
      f_upperEnergyThreshold = f_capacityOfEnergyBucket_main;

    // Only the active load may be switched during this period. All other loads must
    // wait until the recent transition has had sufficient opportunity to take effect.
    bOK_toAddLoad = (tempLoad == activeLoad);
  }

  if (bOK_toAddLoad)
  {
    loadPrioritiesAndState[tempLoad] |= loadStateOnBit;
    activeLoad = tempLoad;
    postTransitionCount = 0;
    b_recentTransition = true;
  }
}

/**
 * @brief Process the case of low energy level, some action may be required.
 *
 * @ingroup TimeCritical
 */
void proceedLowEnergyLevel()
{
  bool bOK_toRemoveLoad{true};
  auto tempLoad{nextLogicalLoadToBeRemoved()};

  if (tempLoad >= NO_OF_DUMPLOADS)
    return;

  // a load which is now ON has been identified for potentially being switched OFF
  if (b_recentTransition)
  {
    // During the post-transition period, any decrease in the energy level is noted.
    f_lowerEnergyThreshold = f_energyInBucket_main;

    // the energy thresholds must remain within range
    if (f_lowerEnergyThreshold < 0)
      f_lowerEnergyThreshold = 0;

    // Only the active load may be switched during this period. All other loads must
    // wait until the recent transition has had sufficient opportunity to take effect.
    bOK_toRemoveLoad = (tempLoad == activeLoad);
  }

  if (bOK_toRemoveLoad)
  {
    loadPrioritiesAndState[tempLoad] &= loadStateMask;
    activeLoad = tempLoad;
    postTransitionCount = 0;
    b_recentTransition = true;
  }
}

/**
 * @brief Process the lastest contribution after each phase specific new cycle
 *        additional processing is performed after each main cycle based on phase 0.
 *
 * @param phase the phase number [0..NO_OF_PHASES[
 *
 * @ingroup TimeCritical
 */
void processLatestContribution(const uint8_t phase)
{
  // for efficiency, the energy scale is Joules * SUPPLY_FREQUENCY
  // add the latest energy contribution to the main energy accumulator
  f_energyInBucket_main += (l_sumP[phase] / n_samplesDuringThisMainsCycle[phase]) * f_powerCal[phase];

  // apply any adjustment that is required.
  if (0 == phase)
  {
    b_newMainsCycle = true;                            //  a 50 Hz 'tick' for use by the main code
    f_energyInBucket_main -= REQUIRED_EXPORT_IN_WATTS; // energy scale is Joules x 50
  }
  // Applying max and min limits to the main accumulator's level
  // is deferred until after the energy related decisions have been taken
  //
}

/**
 * @brief Process the start of a new +ve half cycle, for this phase, just after the zero-crossing point.
 *
 * @param phase the phase number [0..NO_OF_PHASES[
 *
 * @ingroup TimeCritical
 */
void processPlusHalfCycle(const uint8_t phase)
{
  processLatestContribution(phase); // runs at 6.6 ms intervals

  // A performance check to monitor and display the minimum number of sets of
  // ADC samples per mains cycle, the expected number being 20ms / (104us * 6) = 32.05
  //
  if (0 == phase)
  {
    if (n_samplesDuringThisMainsCycle[phase] < n_lowestNoOfSampleSetsPerMainsCycle)
      n_lowestNoOfSampleSetsPerMainsCycle = n_samplesDuringThisMainsCycle[phase];

    processDataLogging();
  }

  l_sumP[phase] = 0;
  n_samplesDuringThisMainsCycle[phase] = 0;
}

/**
 * @brief This function provides the link between the logical and physical loads.
 * @details The array, logicalLoadState[], contains the on/off state of all logical loads, with
 *          element 0 being for the one with the highest priority. The array,
 *          physicalLoadState[], contains the on/off state of all physical loads.
 *
 *          The lowest 7 bits of element is the load number as defined in 'physicalLoadState'.
 *          The highest bit of each 'loadPrioritiesAndState' determines if the load is ON or OFF.
 *          The order of each element in 'loadPrioritiesAndState' determines the load priority.
 *          'loadPrioritiesAndState[i] & loadStateMask' will extract the load number at position 'i'
 *          'loadPrioritiesAndState[i] & loadStateOnBit' will extract the load state at position 'i'
 *
 *          Any other mapping relationships could be configured here.
 *
 * @ingroup TimeCritical
 */
void updatePhysicalLoadStates()
{
#ifdef PRIORITY_ROTATION
  if (b_reOrderLoads)
  {
    uint8_t i{NO_OF_DUMPLOADS - 1};
    const auto temp{loadPrioritiesAndState[i]};
    do
    {
      --i;
      loadPrioritiesAndState[i + 1] = loadPrioritiesAndState[i];
    } while (i);
    loadPrioritiesAndState[i] = temp;

    b_reOrderLoads = false;
  }

#ifndef OFF_PEAK_TARIFF
  if (0x00 == (loadPrioritiesAndState[0] & loadStateOnBit))
    ++absenceOfDivertedEnergyCount;
  else
    absenceOfDivertedEnergyCount = 0;
#endif // OFF_PEAK_TARIFF

#endif // PRIORITY_ROTATION

  uint8_t i{NO_OF_DUMPLOADS};
  do
  {
    --i;
    const auto iLoad{loadPrioritiesAndState[i] & loadStateMask};
    physicalLoadState[iLoad] = (loadPrioritiesAndState[i] & loadStateOnBit) || b_forceLoadOn[iLoad]
                                   ? LoadStates::LOAD_ON
                                   : LoadStates::LOAD_OFF;
  } while (i);
}

/**
 * @brief Process with data logging.
 * @details At the end of each datalogging period, copies are made of the relevant variables
 *          for use by the main code. These variable are then reset for use during the next
 *          datalogging period.
 *
 * @ingroup TimeCritical
 */
void processDataLogging()
{
  if (++n_cycleCountForDatalogging < DATALOG_PERIOD_IN_MAINS_CYCLES)
    return; // data logging period not yet reached

  n_cycleCountForDatalogging = 0;

  uint8_t phase{NO_OF_PHASES};
  do
  {
    --phase;
    copyOf_sumP_atSupplyPoint[phase] = l_sumP_atSupplyPoint[phase];
    l_sumP_atSupplyPoint[phase] = 0;

    copyOf_sum_Vsquared[phase] = l_sum_Vsquared[phase];
    l_sum_Vsquared[phase] = 0;
  } while (phase);

  uint8_t i{NO_OF_DUMPLOADS};
  do
  {
    --i;
    copyOf_countLoadON[i] = countLoadON[i];
    countLoadON[i] = 0;
  } while (i);

  copyOf_sampleSetsDuringThisDatalogPeriod = i_sampleSetsDuringThisDatalogPeriod; // (for diags only)
  copyOf_lowestNoOfSampleSetsPerMainsCycle = n_lowestNoOfSampleSetsPerMainsCycle; // (for diags only)
  copyOf_energyInBucket_main = f_energyInBucket_main;                             // (for diags only)

  n_lowestNoOfSampleSetsPerMainsCycle = UINT8_MAX;
  i_sampleSetsDuringThisDatalogPeriod = 0;

  // signal the main processor that logging data are available
  b_datalogEventPending = true;
}
/* End of helper functions which are used by the ISR
   -------------------------------------------------
*/