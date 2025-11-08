#include <Arduino.h>
#include "OutputControl.h"
#include <math.h>

namespace OutputControl {
// Constants
static float voltageFiltered = 3.3f / 2;          // Initial NTC voltage filter
static constexpr float NTC_ALPHA = 0.05f;         // NTC smoothing factor
static constexpr uint8_t INA_CYCLES = 2;          // Cycles for fuse check (~70 ms)
static constexpr uint8_t NTC_CYCLES = 3;          // Cycles for temp fault (~105 ms)
static constexpr uint8_t VDEV_CYCLES = 3;         // Cycles for voltage deviation (~105 ms)
static constexpr uint8_t CDEV_CYCLES = 3;         // Cycles for current deviation (~105 ms)
static constexpr uint8_t START_CYCLES = 100;      // Startup delay (~1 s)

// Variables
static uint8_t fuseCount = 0;        // Fuse check counter
static uint8_t tempCount = 0;        // Temperature fault counter
static uint8_t vdevCount = 0;        // Voltage deviation counter
static uint8_t cdevCount = 0;        // Current deviation counter
static uint8_t startCount = 0;       // Startup counter
static bool isStarting = false;      // Startup flag

// Initialize MOSFET and reset variables
void begin() {
    pinMode(PROTECTION_MOSFET_PIN, OUTPUT);
    digitalWrite(PROTECTION_MOSFET_PIN, LOW);
    outputActive = false;
    voltageFiltered = 3.3f / 2;
    fuseCount = tempCount = vdevCount = cdevCount = startCount = 0;
    isStarting = true;
}

// Update system state periodically
void update() {
    unsigned long now = millis();
    static unsigned long lastUpdate = 0;
    if (now - lastUpdate < DC_CONTROL_UPDATE_INTERVAL) return;
    lastUpdate = now;

    // Handle startup delay
    if (isStarting) {
        startCount++;
        if (startCount >= START_CYCLES) {
            isStarting = false;
            startCount = START_CYCLES;
        }
    }

    // Read sensors and check errors
    readNTCTemperature();
    checkTemperature();
    checkFuse();
    checkSystemLimits();
    checkVoltageDeviation();
    checkCurrentDeviation();
    checkCriticalErrors();

    // Handle MOSFET enable/disable
    handleManualMOSFET();

}

// Read NTC temperature
void readNTCTemperature() {
    float voltage = analogReadMilliVolts(NTC_ADC_PIN) / 1000.0f;
    if (voltage < 0.01f || voltage > 3.3f) voltage = 3.3f / 2; // Protect against invalid ADC readings
    voltageFiltered = voltageFiltered * (1.0f - NTC_ALPHA) + voltage * NTC_ALPHA;
    float resistance = NTC_SERIES_RESISTOR * (3.3f / voltageFiltered - 1.0f);
    float steinhart = resistance / NTC_NOMINAL_RES;
    steinhart = log(steinhart) / NTC_BETA_COEFF + 1.0f / (NTC_NOMINAL_TEMP_C + 273.15f);
    labTemp_ntc = 1.0f / steinhart - 273.15f;
    errorSensorFail = (labTemp_ntc < 0.0f || labTemp_ntc > 100.0f);
}

// Check temperature errors
void checkTemperature() {
    if (labTemp_ntc >= tempLimitC) {
        tempCount++;
        if (tempCount >= NTC_CYCLES) {
            tempFaultActive = true;
            tempCount = NTC_CYCLES;
        }
    } else {
        tempCount = 0;
        tempFaultActive = false;
    }
}

// Check fuse overcurrent
void checkFuse() {
    if (labI_cut <= 0.0f) return;
    if (labI_meas < 0.0f || isnan(labI_meas)) return;

    if (labI_meas > labI_cut) {
        fuseCount++;
        if (fuseCount >= INA_CYCLES) {
            errorFuseBlown = true;
            fuseCount = INA_CYCLES;
        }
    } else {
        fuseCount = 0;
        errorFuseBlown = false;
    }
}

// Check system limits
void checkSystemLimits() {
    if (!isStarting) {
        errorOverCurrent = (labI_meas > systemIlimitMax && !isnan(labI_meas));
        errorOverPower   = (labQ_meas > systemPowerMax && !isnan(labQ_meas));
    } else {
        errorOverCurrent = false;
        errorOverPower   = false;
    }
    // Always check voltage
    errorVoutOverLimit = (!isStarting && (labV_meas > systemVoutMax || labV_meas < systemVoutMin) && !isnan(labV_meas));
}

// Check voltage deviation in auto mode
void checkVoltageDeviation() {
    if (!modeAuto || isStarting || isCC) {
        vdevCount = 0;
        errorVoltageDev = false;
        return;
    }

    if (fabs(labV_meas - rampedVset) > VdevLimit) {
        vdevCount++;
        if (vdevCount >= VDEV_CYCLES) errorVoltageDev = true;
    } else {
        vdevCount = 0;
        errorVoltageDev = false;
    }
}

// Check current deviation in auto mode
void checkCurrentDeviation() {
    if (!modeAuto) return;
    if (isnan(labI_meas) || isnan(labI_set)) return;

    if (labI_meas > rampedIset * (1.0f + IdevLimit)) {
        cdevCount++;
        if (cdevCount >= CDEV_CYCLES) errorCurrentDev = true;
    } else {
        cdevCount = 0;
        errorCurrentDev = false;
    }
}

// Check critical errors
void checkCriticalErrors() {
    bool criticalError = errorInaInitFail || errorLedcInitFail || errorSensorFail;
    if (criticalError) {
        // MOSFET disabled in handleManualMOSFET
    }
}

// Handle MOSFET manual control
void handleManualMOSFET() {
    if (!manualOutputEnable) {
        setProtectionMosfet(false);
        return;
    }

    bool hasError = tempFaultActive || errorFuseBlown || errorOverCurrent ||
                    errorOverPower || errorVoutOverLimit || errorVoltageDev ||
                    errorCurrentDev || errorSensorFail ||
                    errorInaInitFail || errorLedcInitFail;

    if (!hasError) {
        setProtectionMosfet(true);
    } else {
        setProtectionMosfet(false);
        manualOutputEnable = false;
    }
}

// Control MOSFET state
void setProtectionMosfet(bool enabled) {
    outputActive = enabled;
    digitalWrite(PROTECTION_MOSFET_PIN, enabled ? HIGH : LOW);
}

} // namespace OutputControl

