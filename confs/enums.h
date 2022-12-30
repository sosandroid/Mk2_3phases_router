/**
 * @file enums.h
 * @author Robin Emley (www.Mk2PVrouter.co.uk)
 * @author Frederic Metrich (frederic.metrich@live.fr)
 * @author Emmanuel Havet (@sosandroid)
 * @brief enums.h - various enum type data and their definition for project use
 * @date 2022-12-29
 */
 
 // definitions of enumerated types

/** Polarities */
enum class Polarities : uint8_t
{
  NEGATIVE, /**< polarity is negative */
  POSITIVE  /**< polarity is positive */
};

/** Output modes */
enum class OutputModes : uint8_t
{
  ANTI_FLICKER, /**< Anti-flicker mode */
  NORMAL        /**< Normal mode */
};

/** Load state (for use if loads are active high (Rev 2 PCB)) */
enum class LoadStates : uint8_t
{
  LOAD_OFF, /**< load is OFF */
  LOAD_ON   /**< load is ON */
};
// enum loadStates {LOAD_ON, LOAD_OFF}; /**< for use if loads are active low (original PCB) */

constexpr uint8_t loadStateOnBit{0x80U}; /**< bit mask for load state ON */
constexpr uint8_t loadStateMask{0x7FU};  /**< bit mask for masking load state */

LoadStates physicalLoadState[NO_OF_DUMPLOADS]; /**< Physical state of the loads */
uint16_t countLoadON[NO_OF_DUMPLOADS];         /**< Number of cycle the load was ON (over 1 datalog period) */

constexpr OutputModes outputMode{OutputModes::NORMAL}; /**< Output mode to be used */

// Load priorities at startup
uint8_t loadPrioritiesAndState[NO_OF_DUMPLOADS]{0, 1, 2}; /**< load priorities and states. */

// For an enhanced polarity detection mechanism, which includes a persistence check
Polarities polarityOfMostRecentSampleV[NO_OF_PHASES];    /**< for zero-crossing detection */
Polarities polarityConfirmed[NO_OF_PHASES];              /**< for zero-crossing detection */
Polarities polarityConfirmedOfLastSampleV[NO_OF_PHASES]; /**< for zero-crossing detection */

/** @brief container for datalogging
 *  @details This class is used for datalogging.
 */
class PayloadTx_struct
{
public:
  int16_t power;                     /**< main power, import = +ve, to match OEM convention */
  int16_t power_L[NO_OF_PHASES];     /**< power for phase #, import = +ve, to match OEM convention */
  int16_t Vrms_L_x100[NO_OF_PHASES]; /**< average voltage over datalogging period (in 100th of Volt)*/
#ifdef TEMP_SENSOR
  int16_t temperature_x100{UNUSED_TEMPERATURE}; /**< temperature in 100th of °C */
#endif
};

PayloadTx_struct tx_data; /**< logging data */