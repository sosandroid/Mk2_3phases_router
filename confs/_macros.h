/**
 * @file _macros.h
 * @author Robin Emley (www.Mk2PVrouter.co.uk)
 * @author Frederic Metrich (frederic.metrich@live.fr)
 * @author Emmanuel Havet (@sosandroid)
 * @brief Setting here the general beavior of the router 
 * @date 2022-12-29
 */

/* Defines here the general beahvior of the router */
//--------------------------------------------------------------------------------------------------
// Hardware settings
// #define TEMP_SENSOR ///< this line must be commented out if the temperature sensor is not present
// #define RF_PRESENT ///< this line must be commented out if the RFM12B module is not present
// #define EMONESP ///< Uncomment if an ESP WiFi module is used
// #define SERIALOUT ///< Uncomment if a wired serial connection is used

// Features settings
// #define PRIORITY_ROTATION ///< this line must be commented out if you want fixed priorities
// #define OFF_PEAK_TARIFF ///< this line must be commented out if there's only one single tariff each day
// #define FORCE_PIN_PRESENT ///< this line must be commented out if there's no force pin

// Output messages settings
#define DEBUGGING   ///< enable this line to include debugging print statements
#define SERIALPRINT ///< include 'human-friendly' print statement for commissioning - comment this line to exclude.
//--------------------------------------------------------------------------------------------------

 
 //--------------------------------------------------------------------------------------------------
#ifdef EMONESP
#undef SERIALPRINT // Must not corrupt serial output to emonHub with 'human-friendly' printout
#undef SERIALOUT
#undef DEBUGGING
#include <ArduinoJson.h>
#endif

#ifdef SERIALOUT
#undef EMONESP
#undef SERIALPRINT // Must not corrupt serial output to emonHub with 'human-friendly' printout
#undef DEBUGGING
#endif
//--------------------------------------------------------------------------------------------------