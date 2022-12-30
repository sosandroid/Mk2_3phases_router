/**
 * @file Mk2_3phase_RFdatalog_temp.ino
 * @author Robin Emley (www.Mk2PVrouter.co.uk)
 * @author Frederic Metrich (frederic.metrich@live.fr)
 * @author Emmanuel Havet (@sosandroid)
 * @brief conf_pinout.h - Most of the configuration parameters here
 * @date 2022-12-29
 */

// ----------- Pinout assignments  -----------
//
// digital pins:
// D0 & D1 are reserved for the Serial i/f
// D2 is for the RFM12B
#ifdef OFF_PEAK_TARIFF
    constexpr uint8_t offPeakForcePin{3}; /**< for 3-phase PCB, off-peak trigger */
#endif

#ifdef FORCE_PIN_PRESENT
    constexpr uint8_t forcePin{4};
#endif

#ifdef TEMP_SENSOR
    constexpr uint8_t tempSensorPin{/*4*/}; /**< for 3-phase PCB, sensor pin */
#endif

constexpr uint8_t physicalLoadPin[NO_OF_DUMPLOADS]{5, 6, 7}; /**< for 3-phase PCB, Load #1/#2/#3 (Rev 2 PCB) */
// D8 is not in use
constexpr uint8_t watchDogPin{9};
// D10 is for the RFM12B
// D11 is for the RFM12B
// D12 is for the RFM12B
// D13 is for the RFM12B

// analogue input pins
constexpr uint8_t sensorV[NO_OF_PHASES]{0, 2, 4}; /**< for 3-phase PCB, voltage measurement for each phase */
constexpr uint8_t sensorI[NO_OF_PHASES]{1, 3, 5}; /**< for 3-phase PCB, current measurement for each phase */