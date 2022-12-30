/**
 * @file Mk2_3phase_RFdatalog_temp.ino
 * @author Robin Emley (www.Mk2PVrouter.co.uk)
 * @author Frederic Metrich (frederic.metrich@live.fr)
 * @author Emmanuel Havet (@sosandroid)
 * @brief priority_rotation.h - enums data
 * @date 2022-12-29
 */
 
constexpr uint32_t ROTATION_AFTER_CYCLES{8 * 3600 * SUPPLY_FREQUENCY}; /**< rotates load priorities after this period of inactivity */
volatile uint32_t absenceOfDivertedEnergyCount{0};                     /**< number of main cycles without diverted energy */