/**
 * @file temperature.ino
 * @author Robin Emley (www.Mk2PVrouter.co.uk)
 * @author Frederic Metrich (frederic.metrich@live.fr)
 * @author Emmanuel Havet (@sosandroid)
 * @brief temperature related functions
 * @date 2022-12-29
 */
 
#ifdef TEMP_SENSOR
/**
 * @brief Convert the internal value read from the sensor to a value in °C.
 *
 */
void convertTemperature()
{
  oneWire.reset();
  oneWire.write(SKIP_ROM);
  oneWire.write(CONVERT_TEMPERATURE);
}

/**
 * @brief Read the temperature.
 *
 * @return The temperature in °C (x100).
 */
int16_t readTemperature()
{
  uint8_t buf[9];

  if (oneWire.reset())
  {
    oneWire.reset();
    oneWire.write(SKIP_ROM);
    oneWire.write(READ_SCRATCHPAD);
    for (auto &buf_elem : buf)
      buf_elem = oneWire.read();

    if (oneWire.crc8(buf, 8) != buf[8])
      return BAD_TEMPERATURE;

    // result is temperature x16, multiply by 6.25 to convert to temperature x100
    int16_t result = (buf[1] << 8) | buf[0];
    result = (result * 6) + (result >> 2);
    if (result <= TEMP_RANGE_LOW || result >= TEMP_RANGE_HIGH)
      return OUTOFRANGE_TEMPERATURE; // return value ('Out of range')

    return result;
  }
  return BAD_TEMPERATURE;
}
#endif // #ifdef TEMP_SENSOR

