#pragma once

#include <Arduino.h>

// INA226 sensor manager for voltage, current, and power measurements
namespace Ina226Manager {

  void begin();       // Initialize INA226 sensor
  void update();      // Update sensor measurements

  float getVoltage(); // Get voltage (V)
  float getCurrent(); // Get current (A)
  float getPower();   // Get power (W)

} // namespace Ina226Manager