/**
 * @file function_helpers.ino
 * @author Robin Emley (www.Mk2PVrouter.co.uk)
 * @author Frederic Metrich (frederic.metrich@live.fr)
 * @author Emmanuel Havet (@sosandroid)
 * @brief various helper functions
 * @date 2022-12-29
 */
 
/**
 * @brief update the control ports for each of the physical loads
 *
 */
void updatePortsStates()
{
  uint8_t i{NO_OF_DUMPLOADS};

  do
  {
    --i;
    // update the local load's state.
    if (LoadStates::LOAD_OFF == physicalLoadState[i])
      setPinState(physicalLoadPin[i], false);
    else
    {
      ++countLoadON[i];
      setPinState(physicalLoadPin[i], true);
    }
  } while (i);
}
 
 /**
 * @brief Toggle the watchdog LED
 *
 */
static void toggleWatchDogLED()
{
  PINB = bit(watchDogPin - 8); // toggle pin
}

/**
 * @brief Set the Pin state for the specified pin
 *
 * @param pin pin to change [2..13]
 * @param bState state to be set
 */
inline void setPinState(const uint8_t pin, bool bState)
{
  if (bState)
  {
    if (pin < 8)
      PORTD |= bit(pin);
    else
      PORTB |= bit(pin - 8);
  }
  else
  {
    if (pin < 8)
      PORTD &= ~bit(pin);
    else
      PORTB &= ~bit(pin - 8);
  }
}

/**
 * @brief Get the available RAM during setup
 *
 * @return int The amount of free RAM
 */
int freeRam()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}