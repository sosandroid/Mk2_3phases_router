/**
 * @file _global.h
 * @author Robin Emley (www.Mk2PVrouter.co.uk)
 * @author Frederic Metrich (frederic.metrich@live.fr)
 * @author Emmanuel Havet (@sosandroid)
 * @brief _global.h - Most of the configuration parameters are here
 * @date 2022-12-29
 */


//--------------------------------------------------------------------------------------------------
// constants which must be set individually for each system
//
constexpr uint8_t NO_OF_PHASES{3};    /**< number of phases of the main supply. */
constexpr uint8_t NO_OF_DUMPLOADS{4}; /**< number of dump loads connected to the diverter */
constexpr uint8_t DATALOG_PERIOD_IN_SECONDS{5}; /**< Period of datalogging in seconds */
constexpr uint8_t PERSISTENCE_FOR_POLARITY_CHANGE{2};    /**< allows polarity changes to be confirmed */
constexpr uint8_t SUPPLY_FREQUENCY{50};          /**< number of cycles/s of the grid power supply */
constexpr int8_t REQUIRED_EXPORT_IN_WATTS{20};   /**< when set to a negative value, this acts as a PV generator */
constexpr uint16_t WORKING_ZONE_IN_JOULES{3600}; /**< number of joule for 1Wh */
constexpr uint16_t DATALOG_PERIOD_IN_MAINS_CYCLES{DATALOG_PERIOD_IN_SECONDS * SUPPLY_FREQUENCY}; /**< Period of datalogging in cycles */

//
// Calibration values
//-------------------
// Three calibration values are used in this sketch: f_powerCal, f_phaseCal and f_voltageCal.
// With most hardware, the default values are likely to work fine without
// need for change. A compact explanation of each of these values now follows:

// When calculating real power, which is what this code does, the individual
// conversion rates for voltage and current are not of importance. It is
// only the conversion rate for POWER which is important. This is the
// product of the individual conversion rates for voltage and current. It
// therefore has the units of ADC-steps squared per Watt. Most systems will
// have a power conversion rate of around 20 (ADC-steps squared per Watt).
//
// powerCal is the RECIPR0CAL of the power conversion rate. A good value
// to start with is therefore 1/20 = 0.05 (Watts per ADC-step squared)
//
constexpr float f_powerCal[NO_OF_PHASES]{0.05000f, 0.05000f, 0.05000f};


//--------------------------------------------------------------------------------------------------
// f_phaseCal is used to alter the phase of the voltage waveform relative to the current waveform.
// The algorithm interpolates between the most recent pair of voltage samples according to the value of f_phaseCal.
//
//    With f_phaseCal = 1, the most recent sample is used.
//    With f_phaseCal = 0, the previous sample is used
//    With f_phaseCal = 0.5, the mid-point (average) value in used
//
// NB. Any tool which determines the optimal value of f_phaseCal must have a similar
// scheme for taking sample values as does this sketch.
//
constexpr float f_phaseCal{1}; /**< Nominal values only */


//--------------------------------------------------------------------------------------------------
// When using integer maths, calibration values that have been supplied in floating point form need to be rescaled.
constexpr int16_t i_phaseCal{256}; /**< to avoid the need for floating-point maths (f_phaseCal * 256) */
constexpr uint8_t p_phaseCal{8};   /**< to speed up math (i_phaseCal = 1 << p_phaseCal) */

//--------------------------------------------------------------------------------------------------
// For datalogging purposes, f_voltageCal has been added too. Because the range of ADC values is
// similar to the actual range of volts, the optimal value for this cal factor is likely to be
// close to unity.
constexpr float f_voltageCal[NO_OF_PHASES]{0.803f, 0.803f, 0.803f}; /**< compared with Fluke 77 meter */
//--------------------------------------------------------------------------------------------------

// --------------  general global variables -----------------
//
// Some of these variables are used in multiple blocks so cannot be static.
// For integer maths, some variables need to be 'int32_t'
//
bool beyondStartUpPeriod{false};        /**< start-up delay, allows things to settle */
constexpr uint16_t initialDelay{3000};  /**< in milli-seconds, to allow time to open the Serial monitor */
constexpr uint16_t startUpPeriod{3000}; /**< in milli-seconds, to allow LP filter to settle */

// -------------- DC OFFSET -----------------
int32_t l_DCoffset_V[NO_OF_PHASES]; /**< <--- for LPF */

// Define operating limits for the LP filters which identify DC offset in the voltage
// sample streams. By limiting the output range, these filters always should start up
// correctly.
constexpr int32_t l_DCoffset_V_min{(512L - 100L) * 256L}; /**< mid-point of ADC minus a working margin */
constexpr int32_t l_DCoffset_V_max{(512L + 100L) * 256L}; /**< mid-point of ADC plus a working margin */
constexpr int32_t l_DCoffset_I_nom{512L};                 /**< nominal mid-point value of ADC @ x1 scale */

/**< main energy bucket for 3-phase use, with units of Joules * SUPPLY_FREQUENCY */
constexpr float f_capacityOfEnergyBucket_main{(float)(WORKING_ZONE_IN_JOULES * SUPPLY_FREQUENCY)};
/**< for resetting flexible thresholds */
constexpr float f_midPointOfEnergyBucket_main{f_capacityOfEnergyBucket_main * 0.5f};
/**< threshold in anti-flicker mode - must not exceed 0.4 */
constexpr float f_offsetOfEnergyThresholdsInAFmode{0.1f};

// -------------- ENERGY DATA -----------------

float f_energyInBucket_main{0}; /**< main energy bucket (over all phases) */
float f_lowerEnergyThreshold;   /**< dynamic lower threshold */
float f_upperEnergyThreshold;   /**< dynamic upper threshold */

// for improved control of multiple loads
bool b_recentTransition{false};                 /**< a load state has been recently toggled */
uint8_t postTransitionCount;                    /**< counts the number of cycle since last transition */
constexpr uint8_t POST_TRANSITION_MAX_COUNT{3}; /**< allows each transition to take effect */
// constexpr uint8_t POST_TRANSITION_MAX_COUNT{50}; /**< for testing only */
uint8_t activeLoad{NO_OF_DUMPLOADS}; /**< current active load */

int32_t l_sumP[NO_OF_PHASES];                /**< cumulative power per phase */
int32_t l_sampleVminusDC[NO_OF_PHASES];      /**< for the phaseCal algorithm */
int32_t l_lastSampleVminusDC[NO_OF_PHASES];  /**< for the phaseCal algorithm */
int32_t l_cumVdeltasThisCycle[NO_OF_PHASES]; /**< for the LPF which determines DC offset (voltage) */
int32_t l_sumP_atSupplyPoint[NO_OF_PHASES];  /**< for summation of 'real power' values during datalog period */
int32_t l_sum_Vsquared[NO_OF_PHASES];        /**< for summation of V^2 values during datalog period */

uint8_t n_samplesDuringThisMainsCycle[NO_OF_PHASES]; /**< number of sample sets for each phase during each mains cycle */
uint16_t i_sampleSetsDuringThisDatalogPeriod;    /**< number of sample sets during each datalogging period */
uint8_t n_cycleCountForDatalogging{0};           /**< for counting how often datalog is updated */
uint8_t n_lowestNoOfSampleSetsPerMainsCycle; /**< For a mechanism to check the integrity of this code structure */

// for interaction between the main processor and the ISR
volatile bool b_datalogEventPending{false};   /**< async trigger to signal datalog is available */
volatile bool b_newMainsCycle{false};         /**< async trigger to signal start of new main cycle based on first phase */
volatile bool b_forceLoadOn[NO_OF_DUMPLOADS]; /**< async trigger to force specific load(s) to ON */
// since there's no real locking feature for shared variables, a couple of data
// generated from inside the ISR are copied from time to time to be passed to the
// main processor. When the data are available, the ISR signals it to the main processor.
volatile int32_t copyOf_sumP_atSupplyPoint[NO_OF_PHASES];   /**< copy of cumulative power per phase */
volatile int32_t copyOf_sum_Vsquared[NO_OF_PHASES];         /**< copy of for summation of V^2 values during datalog period */
volatile float copyOf_energyInBucket_main;                  /**< copy of main energy bucket (over all phases) */
volatile uint8_t copyOf_lowestNoOfSampleSetsPerMainsCycle;  /**<  */
volatile uint16_t copyOf_sampleSetsDuringThisDatalogPeriod; /**< copy of for counting the sample sets during each
                                                               datalogging period */
volatile uint16_t copyOf_countLoadON[NO_OF_DUMPLOADS]; /**< copy of number of cycle the load was ON (over 1 datalog period) */