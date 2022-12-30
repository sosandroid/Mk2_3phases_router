/**
 * @file ds18b20.h
 * @author Robin Emley (www.Mk2PVrouter.co.uk)
 * @author Frederic Metrich (frederic.metrich@live.fr)
 * @author Emmanuel Havet (@sosandroid)
 * @brief conf_ds18b20.h - Most of the configuration parameters here
 * @date 2022-12-29
 */
 
 // Dallas DS18B20 commands
constexpr uint8_t SKIP_ROM{0xcc};
constexpr uint8_t CONVERT_TEMPERATURE{0x44};
constexpr uint8_t READ_SCRATCHPAD{0xbe};
constexpr int16_t UNUSED_TEMPERATURE{30000};     /**< this value (300C) is sent if no sensor has ever been detected */
constexpr int16_t OUTOFRANGE_TEMPERATURE{30200}; /**< this value (302C) is sent if the sensor reports < -55C or > +125C */
constexpr int16_t BAD_TEMPERATURE{30400};        /**< this value (304C) is sent if no sensor is present or the checksum is bad (corrupted data) */
constexpr int16_t TEMP_RANGE_LOW{-5500};
constexpr int16_t TEMP_RANGE_HIGH{12500};