#include "Ina226Manager.h"
#include "Globals.h"
#include "Config.h"
#include <Wire.h>
#include <INA226_WE.h>

namespace Ina226Manager {

static INA226_WE ina226(INA226_I2C_ADDRESS); // INA226 instance
static bool inaReady = false;                // Sensor readiness flag

// Initialize INA226 sensor
void begin() {
  inaReady = ina226.init();
  if (!inaReady) {
    errorInaInitFail = true; // Set error flag on initialization failure
    return;
  }
  errorInaInitFail = false;

  ina226.setAverage(INA226_AVERAGE_16); // Set measurement averaging
  ina226.setConversionTime(INA226_CONV_TIME_1100, INA226_CONV_TIME_1100); // Set conversion time
  ina226.setMeasureMode(INA226_CONTINUOUS); // Set continuous measurement mode
  ina226.setResistorRange(SHUNT_RESISTANCE_OHMS, SHUNT_MAX_CURRENT_A); // Set shunt range
}

// Update sensor measurements
void update() {
  if (!inaReady) return;

  labV_meas = ina226.getBusVoltage_V();     // Update voltage (V)
  labI_meas = ina226.getCurrent_mA() / 1000.0f; // Update current (A)
  labQ_meas = ina226.getBusPower() / 1000.0f;   // Update power (W)

  // Clamp small negative values to zero
  if (labV_meas > -0.01f && labV_meas < 0.0f) labV_meas = 0.0f;
  if (labI_meas > -0.01f && labI_meas < 0.0f) labI_meas = 0.0f;
  if (labQ_meas > -0.01f && labQ_meas < 0.01f) labQ_meas = 0.0f;
}

// Get voltage (V)
float getVoltage() { return labV_meas; }

// Get current (A)
float getCurrent() { return labI_meas; }

// Get power (W)
float getPower() { return labQ_meas; }

} // namespace Ina226Manager