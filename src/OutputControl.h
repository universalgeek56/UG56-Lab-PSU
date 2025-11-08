#pragma once

#include "Globals.h"
#include "Config.h"

namespace OutputControl {
  void begin();
  void update();
  void readNTCTemperature();
  void checkTemperature();
  void checkFuse();
  void checkSystemLimits();
  void checkVoltageDeviation();
  void checkCurrentDeviation();
  void checkCriticalErrors();
  void checkManualEnable();
  void checkManualDisable();
  void setProtectionMosfet(bool enabled);
  bool isProtectionMosfetOn();
  void handleError(const char* errorMsg);
  void resetErrors();
  void resetTempFault();

  // Новая функция для ручного MOSFET
  void handleManualMOSFET();
}


// namespace OutputControl {
//   void begin();                          // Initialize pins and variables
//   void update();                         // Periodic update (every 35 ms)
//   void readNTCTemperature();             // Read NTC temperature (XL6019E1)
//   void checkTemperature();               // Check overtemperature (70°C, 3 cycles)
//   void checkFuse();                     // Check user fuse (labI_fuse, 2 cycles)
//   void checkSystemLimits();             // Check system limits (current, power, voltage)
//   void checkVoltageDeviation();         // Check voltage deviation (VdevLimit, 3 cycles)
//   void checkCurrentDeviation();         // Check current deviation (CdevLimit, 3 cycles)
//   void checkCriticalErrors();           // Check critical errors (INA226, LEDC, NTC)
//   void checkManualEnable();             // Check manual enable (reset errors and check)
//   void checkManualDisable();            // Check manual disable (turn off output)
//   void setProtectionMosfet(bool enabled);// Control output MOSFET (IRLZ44N)
//   bool isProtectionMosfetOn();          // Check MOSFET state
//   void handleError(const char* errorMsg);// Handle errors (set errorSensorFail)
//   void resetErrors();                   // Reset all errors with recheck
//   void resetTempFault();                // Reset temperature fault
// } // namespace OutputControl