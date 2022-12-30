/**
 * @file Mk2_3phases_router.ino
 * @author Robin Emley (www.Mk2PVrouter.co.uk)
 * @author Frederic Metrich (frederic.metrich@live.fr)
 * @author Emmanuel Havet (sosandroid@github)
 * @brief Mk2_3phases_router.ino - A photovoltaic energy diverter.
 * @date 2022-12-29
 *
 * @mainpage A 3-phase photovoltaic router/diverter
 *
 * @section description Description
 * Mk2_3phase_Router.ino - Arduino program that maximizes the use of home photovoltaic production
 * by monitoring energy consumption and diverting power to one or more resistive charge(s) when needed.
 * In the absence of such a system, surplus energy flows away to the grid and is of no benefit to the PV-owner.
 *
 * @section history History
 * __Issue 1 was released in January 2015.__
 *
 * This sketch provides continuous monitoring of real power on three phases.
 * Surplus power is diverted to multiple loads in sequential order. A suitable
 * output-stage is required for each load; this can be either triac-based, or a
 * Solid State Relay.
 *
 * Datalogging of real power and Vrms is provided for each phase.
 * The presence or absence of the RFM12B needs to be set at compile time
 *
 * __January 2016, renamed as Mk2_3phase_RFdatalog_2 with these changes:__
 * - Improved control of multiple loads has been imported from the
 *     equivalent 1-phase sketch, Mk2_multiLoad_wired_6.ino
 * - the ISR has been upgraded to fix a possible timing anomaly
 * - variables to store ADC samples are now declared as "volatile"
 * - for RF69 RF module is now supported
 * - a performance check has been added with the result being sent to the Serial port
 * - control signals for loads are now active-high to suit the latest 3-phase PCB
 *
 * __February 2016, renamed as Mk2_3phase_RFdatalog_3 with these changes:__
 * - improvements to the start-up logic. The start of normal operation is now
 *    synchronized with the start of a new mains cycle.
 * - reduce the amount of feedback in the Low Pass Filter for removing the DC content
 *     from the SampleV stream. This resolves an anomaly which has been present since
 *     the start of this project. Although the amount of feedback has previously been
 *     excessive, this anomaly has had minimal effect on the system's overall behaviour.
 * - The reported power at each of the phases has been inverted. These values are now in
 *     line with the Open Energy Monitor convention, whereby import is positive and
 *     export is negative.
 *
 * __February 2020: updated to Mk2_3phase_RFdatalog_3a with these changes:__
 * - removal of some redundant code in the logic for determining the next load state.
 *
 *      Robin Emley
 *      www.Mk2PVrouter.co.uk
 *
 * __October 2019, renamed as Mk2_3phase_RFdatalog_temp with these changes:__
 * - This sketch has been restructured in order to make better use of the ISR.
 * - All of the time-critical code is now contained within the ISR and its helper functions.
 * - Values for datalogging are transferred to the main code using a flag-based handshake mechanism.
 * - The diversion of surplus power can no longer be affected by slower
 * activities which may be running in the main code such as Serial statements and RF.
 * - Temperature sensing is supported. A pullup resistor (4K7 or similar) is required for the Dallas sensor.
 * - The output mode, i.e. NORMAL or ANTI_FLICKER, is now set at compile time.
 * - Also:
 *   - The ADC is now in free-running mode, at ~104 µs per conversion.
 *   - a persistence check has been added for zero-crossing detection (polarityConfirmed)
 *   - a lowestNoOfSampleSetsPerMainsCycle check has been added, to detect any disturbances
 *   - Vrms has been added to the datalog payload (as Vrms x 100)
 *   - temperature has been added to the datalog payload (as degrees C x 100)
 *   - the phaseCal/f_voltageCal mechanism has been modified to be the same for all phases
 *   - RF capability made switchable so that the code will continue to run
 *     when an RF module is not fitted. Dataloging can then take place via the Serial port.
 *   - temperature capability made switchable so that the code will continue to run w/o sensor.
 *   - priority pin changed to handle peak/off-peak tariff
 *   - add rotating load priorities: this sketch is intended to control a 3-phase
 *     water heater which is composed of 3 independent heating elements wired in WYE
 *     with a neutral wire. The router will control each element in a specific order.
 *     To ensure that in average (over many days/months), each load runs the same time,
 *     each day, the router will rotate the priorities.
 *   - most functions have been splitted in one or more sub-functions. This way, each function
 *     has a specific task and is much smaller than before.
 *   - renaming of most of the variables with a single-letter prefix to identify its type.
 *   - direct port manipulation to save code size and speed-up performance
 *
 * __January 2020, changes:__
 * - This sketch has been again re-engineered. All 'defines' have been removed except
 *   the ones for compile-time optional functionalities.
 * - All constants have been replaced with constexpr initialized at compile-time
 * - all number-types have been replaced with fixed width number types
 * - old fashion enums replaced by scoped enums with fixed types
 * - off-peak tariff made switchable at compile-time
 * - rotation of load priorities made switchable at compile-time
 * - enhanced configuration for override specific loads during off-peak period
 *
 * __April 2020, changes:__
 * - Fix a bug in the load level calculation
 *
 * __May 2020, changes:__
 * - Fix a bug in the initialization of off-peak offsets
 * - added detailed configuration on start-up with build timestamp
 *
 * __June 2020, changes:__
 * - Add force pin for full power through overwrite switch
 * - Add priority rotation for single tariff
 *
 * __October 2020, changes:__
 * - Moving some part around (calibration values toward beginning of the sketch)
 * - renaming some preprocessor defines
 * - system/user specific data moved toward beginning of the sketch
 *
 * __January 2021, changes:__
 * - Further optimization
 * - now it's possible to specify the override period in minutes and hours
 * - initialization of runtime parameters for override period at compile-time
 *
 * __February 2021, changes:__
 * - Added temperature threshold for off-peak period
 *
 * __April 2021, renamed as Mk2_3phase_RFdatalog_temp with these changes:__
 * - since this sketch is under source control, no need to write the version in the name
 * - rename function 'checkLoadPrioritySelection' to function's job
 * - made forcePin presence configurable
 * - added WatchDog LED (blink 1s ON/ 1s OFF)
 * - code enhanced to support 6 loads
 *
 * @author Fred Metrich
 * @copyright Copyright (c) 2022
 *
 * __December 2022, renamed as Mk2_3phases_router with these changes:__
 * - Attemps to make the code more readable and more maintanable. Whole bunchs of code are beeing sent to external files
 * - All project settings and constants are sent to relevant header files located in "confs/"
 * - Added some .ino files to be merged automatically by Arduino's IDE at compile time
 *
 */
 
 
// In this sketch, the ADC is free-running with a cycle time of ~104uS.
 
static_assert(__cplusplus >= 201703L, "**** Please define 'gnu++17' in 'platform.txt' ! ****");
#include <Arduino.h> // may not be needed, but it's probably a good idea to include this

/** --------------------------------------
  Global configuration - macros and their linked interoperability (emonesp, serial, debug)
*/
#include "confs/_macros.h"
/** --------------------------------------
  Global configuration - constants and global variables
*/
#include "confs/_global.h"
/** --------------------------------------
   Pinout definition 
*/
#include "confs/pinout.h"
/** --------------------------------------
  Definitions of enumerated types, struct types and their very first use
*/
#include "confs/enums.h"
/** --------------------------------------
   RF configuration 
*/
#ifdef RF_PRESENT
    #include "confs/rf.h"
#endif   // #ifdef RF_PRESENT

/** --------------------------------------
  Temperature sensor
*/
#ifdef TEMP_SENSOR
    #include <OneWire.h> // for temperature sensing
    #include "confs/ds18b20.h"//for ds18b20 sensor
	OneWire oneWire(tempSensorPin);
#endif // #ifdef TEMP_SENSOR

/** --------------------------------------
  Off-peak tariff management
*/
#ifndef OFF_PEAK_TARIFF
    #ifdef PRIORITY_ROTATION
        #include "confs/priority_rotation.h"
    #endif // #ifdef PRIORITY_ROTATION
#else  // #ifndef OFF_PEAK_TARIFF
    constexpr uint8_t ul_OFF_PEAK_DURATION{8}; /**< Duration of the off-peak period in hours */
    constexpr uint8_t uiTemperature{100}; /**< the temperature threshold to stop forcing in °C */
    
    /** @brief Config parameters for forcing a load
     *  @details This class allows the user to define when and how long a load will be forced at
     *           full power during off-peak period.
     *
     *           For each load, the user defines a pair of values: pairForceLoad => { offset, duration }.
     *           The load will be started with full power at ('start_offpeak' + 'offset') for a duration of 'duration'
     *             - all values are in hours (if between -24 and 24) or in minutes.
     *             - if the offset is negative, it's calculated from the end of the off-peak period (ie -3 means 3 hours
     * back from the end).
     *             - to leave the load at full power till the end of the off-peak period, set the duration to 'UINT16_MAX'
     * (somehow infinite time)
     */
    class pairForceLoad
    {
    public:
      constexpr pairForceLoad() = default;
      constexpr pairForceLoad(int16_t _iStartOffset, uint16_t _uiDuration = UINT16_MAX)
          : iStartOffset(_iStartOffset), uiDuration(_uiDuration)
      {
      }
    
      constexpr int16_t getStartOffset() const
      {
        return iStartOffset;
      }
      constexpr uint16_t getDuration() const
      {
        return uiDuration;
      }
    
    private:
      int16_t iStartOffset{0};         /**< the start offset from the off-peak begin in hours or minutes */
      uint16_t uiDuration{UINT16_MAX}; /**< the duration for forcing the load in hours or minutes */
    };
    
    constexpr pairForceLoad rg_ForceLoad[NO_OF_DUMPLOADS] = {{-3, 2},    /**< force config for load #1 */
                                                             {-3, 120},  /**< force config for load #2 */
                                                             {-180, 2}}; /**< force config for load #3 */

	uint32_t ul_TimeOffPeak; /**< 'timestamp' for start of off-peak period */

	/**
	 * @brief Template class for Load-Forcing
	 * @details The array is initialized at compile time so it can be read-only and
	 *          the performance and code size are better
	 *
	 * @tparam N # of loads
	 */
	template <uint8_t N>
	class _rg_OffsetForce
	{
		public:
		  constexpr _rg_OffsetForce()
			  : _rg()
		  {
			constexpr uint16_t uiPeakDurationInSec{ul_OFF_PEAK_DURATION * 3600};
			// calculates offsets for force start and stop of each load
			for (uint8_t i = 0; i != N; ++i)
			{
			  const bool bOffsetInMinutes{rg_ForceLoad[i].getStartOffset() > 24 ||
										  rg_ForceLoad[i].getStartOffset() < -24};
			  const bool bDurationInMinutes{rg_ForceLoad[i].getDuration() > 24 &&
											UINT16_MAX != rg_ForceLoad[i].getDuration()};

			  _rg[i][0] = ((rg_ForceLoad[i].getStartOffset() >= 0) ? 0 : uiPeakDurationInSec) +
						  rg_ForceLoad[i].getStartOffset() * (bOffsetInMinutes ? 60ul : 3600ul);
			  _rg[i][0] *= 1000ul; // convert in milli-seconds

			  if (UINT8_MAX == rg_ForceLoad[i].getDuration())
				_rg[i][1] = rg_ForceLoad[i].getDuration();
			  else
				_rg[i][1] = _rg[i][0] + rg_ForceLoad[i].getDuration() * (bDurationInMinutes ? 60ul : 3600ul) * 1000ul;
			}
		  }
		  const uint32_t (&operator[](uint8_t i) const)[2]
		  {
			return _rg[i];
		  }

		private:
		  uint32_t _rg[N][2];
	};
	constexpr auto rg_OffsetForce = _rg_OffsetForce<NO_OF_DUMPLOADS>(); /**< start & stop offsets for each load */
#endif // #ifndef OFF_PEAK_TARIFF




/**
 * @brief set default threshold at compile time so the variable can be read-only
 *
 * @param lower True to set the lower threshold, false for higher
 * @return the corresponding threshold
 */
constexpr float initThreshold(const bool lower)
{
  return lower
             ? f_capacityOfEnergyBucket_main * (0.5f - ((OutputModes::ANTI_FLICKER == outputMode) ? f_offsetOfEnergyThresholdsInAFmode : 0))
             : f_capacityOfEnergyBucket_main * (0.5f + ((OutputModes::ANTI_FLICKER == outputMode) ? f_offsetOfEnergyThresholdsInAFmode : 0));
}

constexpr float f_lowerThreshold_default{initThreshold(true)};  /**< lower default threshold set accordingly to the output mode */
constexpr float f_upperThreshold_default{initThreshold(false)}; /**< upper default threshold set accordingly to the output mode */


#ifdef PRIORITY_ROTATION
    volatile bool b_reOrderLoads{false}; /**< async trigger for loads re-ordering */
#endif



static void processStartNewCycle();
static uint8_t nextLogicalLoadToBeAdded();
static uint8_t nextLogicalLoadToBeRemoved();
static void proceedHighEnergyLevel();
static void proceedLowEnergyLevel();
static void processDataLogging();
static bool proceedLoadPrioritiesAndForcing(const int16_t currentTemperature_x100);
static void sendResults(bool bOffPeak);
static void printConfiguration();
static void send_rf_data();



/**
 * @brief Interrupt Service Routine - Interrupt-Driven Analog Conversion.
 * @details An Interrupt Service Routine is now defined which instructs the ADC to perform a conversion
 *          for each of the voltage and current sensors in turn.
 *
 *          This Interrupt Service Routine is for use when the ADC is in the free-running mode.
 *          It is executed whenever an ADC conversion has finished, approx every 104 µs. In
 *          free-running mode, the ADC has already started its next conversion by the time that
 *          the ISR is executed. The ISR therefore needs to "look ahead".
 *
 *          At the end of conversion Type N, conversion Type N+1 will start automatically. The ISR
 *          which runs at this point therefore needs to capture the results of conversion Type N,
 *          and set up the conditions for conversion Type N+2, and so on.
 *
 *          By means of various helper functions, all of the time-critical activities are processed
 *          within the ISR.
 *
 *          The main code is notified by means of a flag when fresh copies of loggable data are available.
 *
 *          Keep in mind, when writing an Interrupt Service Routine (ISR):
 *            - Keep it short
 *            - Don't use delay ()
 *            - Don't do serial prints
 *            - Make variables shared with the main code volatile
 *            - Variables shared with main code may need to be protected by "critical sections"
 *            - Don't try to turn interrupts off or on
 *
 */
ISR(ADC_vect)
{
  static uint8_t sample_index{0};
  int16_t rawSample;

  switch (sample_index)
  {
  case 0:
    rawSample = ADC;           // store the ADC value (this one is for Voltage L1)
    ADMUX = 0x40 + sensorV[1]; // the conversion for I1 is already under way
    ++sample_index;            // increment the control flag
    //
    processVoltageRawSample(0, rawSample);
    break;
  case 1:
    rawSample = ADC;           // store the ADC value (this one is for Current L1)
    ADMUX = 0x40 + sensorI[1]; // the conversion for V2 is already under way
    ++sample_index;            // increment the control flag
    //
    processCurrentRawSample(0, rawSample);
    break;
  case 2:
    rawSample = ADC;           // store the ADC value (this one is for Voltage L2)
    ADMUX = 0x40 + sensorV[2]; // the conversion for I2 is already under way
    ++sample_index;            // increment the control flag
    //
    processVoltageRawSample(1, rawSample);
    break;
  case 3:
    rawSample = ADC;           // store the ADC value (this one is for Current L2)
    ADMUX = 0x40 + sensorI[2]; // the conversion for V3 is already under way
    ++sample_index;            // increment the control flag
    //
    processCurrentRawSample(1, rawSample);
    break;
  case 4:
    rawSample = ADC;           // store the ADC value (this one is for Voltage L3)
    ADMUX = 0x40 + sensorV[0]; // the conversion for I3 is already under way
    ++sample_index;            // increment the control flag
    //
    processVoltageRawSample(2, rawSample);
    break;
  case 5:
    rawSample = ADC;           // store the ADC value (this one is for Current L3)
    ADMUX = 0x40 + sensorI[0]; // the conversion for V1 is already under way
    sample_index = 0;          // reset the control flag
    //
    processCurrentRawSample(2, rawSample);
    break;
  default:
    sample_index = 0; // to prevent lockup (should never get here)
  }
} // end of ISR




/**
 * @brief Called once during startup.
 * @details This function initializes a couple of variables we cannot init at compile time and
 *          sets a couple of parameters for runtime.
 *
 */
void setup()
{
  delay(initialDelay); // allows time to open the Serial Monitor

  Serial.begin(9600); // initialize Serial interface, Do NOT set greater than 9600

#if !defined SERIALOUT && !defined EMONESP
  // On start, always display config info in the serial monitor
  printConfiguration();
#endif

  // initializes all loads to OFF at startup
  uint8_t i{NO_OF_DUMPLOADS};
  do
  {
    --i;
    if (physicalLoadPin[i] < 8)
      DDRD |= bit(physicalLoadPin[i]); // driver pin for Load #n
    else
      DDRB |= bit(physicalLoadPin[i] - 8); // driver pin for Load #n

    loadPrioritiesAndState[i] &= loadStateMask;
  } while (i);

  updatePhysicalLoadStates(); // allows the logical-to-physical mapping to be changed

  updatePortsStates(); // updates output pin states

#ifdef OFF_PEAK_TARIFF
  DDRD &= ~bit(offPeakForcePin);                     // set as input
  PORTD |= bit(offPeakForcePin);                     // enable the internal pullup resistor
  delay(100);                                        // allow time to settle
  uint8_t pinState{!!(PIND & bit(offPeakForcePin))}; // initial selection and

  ul_TimeOffPeak = millis();
#endif

#ifdef FORCE_PIN_PRESENT
  DDRD &= ~bit(forcePin); // set as input
  PORTD |= bit(forcePin); // enable the internal pullup resistor
  delay(100);             // allow time to settle
#endif

  DDRB |= bit(watchDogPin - 8);    // set as output
  setPinState(watchDogPin, false); // set to off

  for (auto &bForceLoad : b_forceLoadOn)
    bForceLoad = false;

  for (auto &DCoffset_V : l_DCoffset_V)
    DCoffset_V = 512L * 256L; // nominal mid-point value of ADC @ x256 scale

  // Set up the ADC to be free-running
  ADCSRA = bit(ADPS0) + bit(ADPS1) + bit(ADPS2); // Set the ADC's clock to system clock / 128
  ADCSRA |= bit(ADEN);                           // Enable the ADC

  ADCSRA |= bit(ADATE); // set the Auto Trigger Enable bit in the ADCSRA register. Because
  // bits ADTS0-2 have not been set (i.e. they are all zero), the
  // ADC's trigger source is set to "free running mode".

  ADCSRA |= bit(ADIE); // set the ADC interrupt enable bit. When this bit is written
  // to one and the I-bit in SREG is set, the
  // ADC Conversion Complete Interrupt is activated.

  ADCSRA |= bit(ADSC); // start ADC manually first time
  sei();               // Enable Global Interrupts

  logLoadPriorities();

#ifdef TEMP_SENSOR
  convertTemperature(); // start initial temperature conversion
#endif

#ifdef DEBUGGING
  Serial.print(F(">>free RAM = "));
  Serial.println(freeRam()); // a useful value to keep an eye on
  Serial.println(F("----"));
#endif
}



/**
 * @brief Main processor.
 * @details None of the workload in loop() is time-critical.
 *          All the processing of ADC data is done within the ISR.
 *
 */
void loop()
{
  static uint8_t perSecondTimer{0};
  static bool bOffPeak{false};
  static int16_t iTemperature_x100{0};

  if (b_newMainsCycle) // flag is set after every pair of ADC conversions
  {
    b_newMainsCycle = false; // reset the flag
    ++perSecondTimer;

    if (perSecondTimer >= SUPPLY_FREQUENCY)
    {
      perSecondTimer = 0;

      toggleWatchDogLED();

      if (!forceFullPower())
        bOffPeak = proceedLoadPrioritiesAndForcing(iTemperature_x100); // called every second
    }
  }

  if (b_datalogEventPending)
  {
    b_datalogEventPending = false;

    tx_data.power = 0;
    uint8_t phase{NO_OF_PHASES};
    do
    {
      --phase;

      tx_data.power_L[phase] =
          copyOf_sumP_atSupplyPoint[phase] / copyOf_sampleSetsDuringThisDatalogPeriod * f_powerCal[phase];
      tx_data.power_L[phase] *= -1;

      tx_data.power += tx_data.power_L[phase];

      tx_data.Vrms_L_x100[phase] =
          (int32_t)(100 * f_voltageCal[phase] *
                    sqrt(copyOf_sum_Vsquared[phase] / copyOf_sampleSetsDuringThisDatalogPeriod));
    } while (phase);

#ifdef TEMP_SENSOR
    iTemperature_x100 = readTemperature();
    tx_data.temperature_x100 = iTemperature_x100;
#endif

    sendResults(bOffPeak);

#ifdef TEMP_SENSOR
    convertTemperature(); // for use next time around
#endif
  }
} // end of loop()
