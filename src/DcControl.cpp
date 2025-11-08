#include <Arduino.h>
#include "DcControl.h"
#include "Globals.h"
#include "Config.h"

namespace DcControl {

// Control parameters
static float stepVset = 0.1f;         // Voltage ramp step
static float stepIset = 0.01f;        // Current ramp step
static float pwmDuty = 0.0f;          // PWM duty cycle
static unsigned long lastUpdate = 0;  // Last update timestamp

// PID CV parameters
static float integralV = 0.0f;    // voltage integral
static float prevErrorV = 0.0f;   // previous voltage error
static float derivativeV = 0.0f;  // voltage derivative

// PID CC parameters
static float integralI = 0.0f;    // current integral
static float prevErrorI = 0.0f;   // previous current error
static float derivativeI = 0.0f;  // current derivative

static float pidOutput = 0.0f;  // PID output (CV/CC)

// PWM configuration
const uint32_t pwmFreq = 9700;               // PWM frequency (Hz)
const uint8_t pwmBits = 12;                  // PWM resolution (bits)
const uint16_t pwmMax = (1 << pwmBits) - 1;  // Max PWM value

// Deadband for error calculations
const float deadbandV = 0.002f;
const float deadbandI = 0.002f;

static bool lastCC = false;
static bool softStartDone = false;

// Peak and RMS tracking
static float maxDeltaV = 0.0f;      // Max voltage deviation
static float maxDeltaI = 0.0f;      // Max current deviation
static unsigned long lastSend = 0;  // Last data send timestamp
static float sumV2 = 0.0f;          // Sum of squared voltage deviations
static float sumI2 = 0.0f;          // Sum of squared current deviations
static int rmsCount = 0;            // RMS sample count

// Send debug variables via WebSocket
void updateDebugVars() {
  unsigned long now = millis();
  if (now - lastSend < WEBSOCKET_SEND_INTERVAL) return;
  lastSend = now;

  if (dbgMode == 2) {                                               // voltage PID debug
    debugVars[0] = pwmDuty;                                         // PID PWM output
    debugVars[1] = integralV;                                       // voltage integral term
    debugVars[2] = prevErrorV;                                      // previous voltage error
    debugVars[3] = (rmsCount > 0) ? sqrt(sumV2 / rmsCount) : 0.0f;  // RMS deviation
    debugVars[4] = maxDeltaV;                                       // peak deviation
    debugVars[5] = derivativeV;                                     // derivative (rate of change)
  } else if (dbgMode == 3) {                                        // current PID debug
    debugVars[0] = pwmDuty;                                         // PID PWM output
    debugVars[1] = integralI;                                       // current integral term
    debugVars[2] = prevErrorI;                                      // previous current error
    debugVars[3] = (rmsCount > 0) ? sqrt(sumI2 / rmsCount) : 0.0f;  // RMS deviation
    debugVars[4] = maxDeltaI;                                       // peak deviation
    //debugVars[5] = derivativeI;                                     // derivative
    debugVars[5] = rampedIset;
  }

  // reset peaks and RMS after sending
  maxDeltaV = maxDeltaI = 0.0f;
  sumV2 = sumI2 = 0.0f;
  rmsCount = 0;
}

void begin() {
  pinMode(DC_CONTROL_PIN, OUTPUT);
  if (!ledcAttach(DC_CONTROL_PIN, pwmFreq, pwmBits)) {
    errorLedcInitFail = true;
    return;
  }
  ledcOutputInvert(DC_CONTROL_PIN, invertPwmSignal);
  rampedVset = labV_set;
  pwmDuty = dutyMin;
  uint16_t pwmValue = invertPwmSignal ? pwmMax - (uint16_t)(pwmDuty / 100.0f * pwmMax)
                                      : (uint16_t)(pwmDuty / 100.0f * pwmMax);
  ledcWrite(DC_CONTROL_PIN, pwmValue);
}

// void update() {
//     unsigned long now = millis();
//     if (now - lastUpdate < DC_CONTROL_UPDATE_INTERVAL) return;
//     lastUpdate = now;

//     // === SMOOTH SETPOINT RAMPING ===
//     if (rampedVset < labV_set) rampedVset = min(rampedVset + stepVset, labV_set);
//     else if (rampedVset > labV_set) rampedVset = max(rampedVset - stepVset, labV_set);

//     if (rampedIset < labI_set) rampedIset = min(rampedIset + stepIset, labI_set);
//     else if (rampedIset > labI_set) rampedIset = max(rampedIset - stepIset, labI_set);

//     // === AUTO CV/CC HANDOFF ===
//     const float switchHyst = (labI_set < 0.1f) ? 0.002f : 0.02f * max(labI_set, 0.25f);
//     bool nowCC = (labI_meas > labI_set + switchHyst);

//     // CV → CC transition
//     if (nowCC && !lastCC) {
//         rampedIset = labI_meas;
//         integralI = 0.0f;
//         prevErrorI = 0.0f;
//         softStartDone = true;
//     }

//     // CC → CV transition
//     if (!nowCC && lastCC) {
//         rampedVset = labV_meas;
//         integralV = 0.0f;
//         prevErrorV = 0.0f;
//         softStartDone = true;
//     }

//     lastCC = nowCC;

//     // === MANUAL MODE ===
//     if (!modeAuto) {
//         pwmDuty = dutyMax;
//         integralV = integralI = 0.0f;
//         errorPidDivergence = errorPidCurrentDivergence = false;
//     } else {
//         // === ERROR CALCULATION WITH DEADBAND ===
//         float errorV = rampedVset - labV_meas;
//         if (fabs(errorV) < deadbandV) errorV = 0.0f;
//         float errorI = rampedIset - labI_meas;
//         if (fabs(errorI) < deadbandI) errorI = 0.0f;

//         // === PID CONTROL BASED ON CURRENT MODE ===
//         if (nowCC) {
//             // CC MODE
//             float P = Kp_I * errorI;
//             integralI += Ki_I * errorI * (DC_CONTROL_UPDATE_INTERVAL / 1000.0f);
//             integralI = constrain(integralI, -integralLimit_I, integralLimit_I);
//             derivativeI = Kd_I * (errorI - prevErrorI) / (DC_CONTROL_UPDATE_INTERVAL / 1000.0f);
//             prevErrorI = errorI;
//             pidOutput = P + integralI + derivativeI;

//             isCC = true;
//             integralV = 0.0f;
//         } else {
//             // CV MODE
//             float P = Kp * errorV;
//             integralV += Ki * errorV * (DC_CONTROL_UPDATE_INTERVAL / 1000.0f);
//             integralV = constrain(integralV, -integralLimit, integralLimit);
//             derivativeV = Kd * (errorV - prevErrorV) / (DC_CONTROL_UPDATE_INTERVAL / 1000.0f);
//             prevErrorV = errorV;
//             pidOutput = P + integralV + derivativeV;

//             isCC = false;
//             integralI = 0.0f;
//         }

//         // === LIMIT CC AT HIGH VOLTAGE ===
//         if (isCC && labV_meas > labV_set * 1.05f) {
//             pwmDuty = constrain(pwmDuty, dutyMin, (labV_set / systemVoutMax) * 100.0f);
//             integralI = 0.0f;
//         }

//         // === PID DIVERGENCE ===
//         errorPidDivergence = fabs(pidOutput) > 1.5f * dutyMax && !isCC;
//         errorPidCurrentDivergence = fabs(pidOutput) > 1.5f * dutyMax && isCC;
//         if (errorPidDivergence || errorPidCurrentDivergence) {
//             if (isCC) integralI *= 0.5f;
//             else integralV *= 0.5f;
//         }

//         // === PWM UPDATE ===
//         pwmDuty += pidOutput;
//         pwmDuty = constrain(pwmDuty, dutyMin, dutyMax);

//         // === ANTI-WINDUP ===
//         if (isCC) {
//             if (pwmDuty >= dutyMax && errorI > 0) integralI = max(integralI - Ki_I * errorI * (DC_CONTROL_UPDATE_INTERVAL / 1000.0f), -integralLimit_I);
//             if (pwmDuty <= dutyMin && errorI < 0) integralI = min(integralI - Ki_I * errorI * (DC_CONTROL_UPDATE_INTERVAL / 1000.0f), integralLimit_I);
//         } else {
//             if (pwmDuty >= dutyMax && errorV > 0) integralV = max(integralV - Ki * errorV * (DC_CONTROL_UPDATE_INTERVAL / 1000.0f), -integralLimit);
//             if (pwmDuty <= dutyMin && errorV < 0) integralV = min(integralV - Ki * errorV * (DC_CONTROL_UPDATE_INTERVAL / 1000.0f), integralLimit);
//         }

//         // === PEAK/RMS TRACKING ===
//         float deltaV = labV_meas - labV_set;
//         float deltaI = labI_meas - labI_set;
//         if (fabs(deltaV) > maxDeltaV) maxDeltaV = fabs(deltaV);
//         if (fabs(deltaI) > maxDeltaI) maxDeltaI = fabs(deltaI);
//         sumV2 += deltaV * deltaV;
//         sumI2 += deltaI * deltaI;
//         rmsCount++;

//         // === SEND DEBUG ===
//         if (dbgMode == 2 || dbgMode == 3) updateDebugVars();
//     }

//     // === WRITE PWM ===
//     uint16_t pwmValue = invertPwmSignal ? pwmMax - (uint16_t)(pwmDuty / 100.0f * pwmMax)
//                                        : (uint16_t)(pwmDuty / 100.0f * pwmMax);
//     ledcWrite(DC_CONTROL_PIN, pwmValue);
// }

void update() {
  unsigned long now = millis();
  if (now - lastUpdate < DC_CONTROL_UPDATE_INTERVAL) return;
  lastUpdate = now;

  // Smooth voltage setpoint adjustment
  if (rampedVset < labV_set) {
    rampedVset += stepVset;
    if (rampedVset > labV_set) rampedVset = labV_set;
  } else if (rampedVset > labV_set) {
    rampedVset -= stepVset;
    if (rampedVset < labV_set) rampedVset = labV_set;
  }
  
  // Smooth current setpoint adjustment
  if (rampedIset < labI_set) {
    rampedIset += stepIset;
    if (rampedIset > labI_set) rampedIset = labI_set;
  } else if (rampedIset > labI_set) {
    rampedIset -= stepIset;
    if (rampedIset < labI_set) rampedIset = labI_set;
  }

  // Manual mode
  if (!modeAuto) {
    pwmDuty = dutyMax;
    integralV = integralI = 0.0f;
    errorPidDivergence = false;
    errorPidCurrentDivergence = false;
  } else {
    // Calculate errors with deadband
    float errorV = rampedVset - labV_meas;
    if (fabs(errorV) < deadbandV) errorV = 0.0f;
    float errorI = rampedIset - labI_meas;
    if (fabs(errorI) < deadbandI) errorI = 0.0f;

    // Adaptive hysteresis (2% of labI_set, min 2mA for low currents)
    const float switchHyst = (labI_set < 0.1f) ? 0.002f : 0.02f * max(labI_set, 0.25f);


    // Select mode (CV priority)
    if (labI_meas > rampedIset + switchHyst) {
      // CC mode
      float proportionalI = Kp_I * errorI;
      integralI += Ki_I * errorI * (DC_CONTROL_UPDATE_INTERVAL / 1000.0f);
      integralI = constrain(integralI, -integralLimit_I, integralLimit_I);
      derivativeI = Kd_I * (errorI - prevErrorI) / (DC_CONTROL_UPDATE_INTERVAL / 1000.0f);
      prevErrorI = errorI;
      pidOutput = proportionalI + integralI + derivativeI;
      isCC = true;
      integralV = 0.0f;  // Reset CV integral
    } else if (labI_meas < rampedIset - switchHyst || labI_meas < 0.005f || labV_meas >= labV_set + 0.5f) {
      // CV mode (based on current or voltage)
      float proportionalV = Kp * errorV;
      integralV += Ki * errorV * (DC_CONTROL_UPDATE_INTERVAL / 1000.0f);
      integralV = constrain(integralV, -integralLimit, integralLimit);
      derivativeV = Kd * (errorV - prevErrorV) / (DC_CONTROL_UPDATE_INTERVAL / 1000.0f);
      prevErrorV = errorV;
      pidOutput = proportionalV + integralV + derivativeV;
      isCC = false;
      integralI = 0.0f;  // Reset CC integral
    } else {
      // Hysteresis range: maintain current mode
      if (isCC) {
        float proportionalI = Kp_I * errorI;
        integralI += Ki_I * errorI * (DC_CONTROL_UPDATE_INTERVAL / 1000.0f);
        integralI = constrain(integralI, -integralLimit_I, integralLimit_I);
        derivativeI = Kd_I * (errorI - prevErrorI) / (DC_CONTROL_UPDATE_INTERVAL / 1000.0f);
        prevErrorI = errorI;
        pidOutput = proportionalI + integralI + derivativeI;
      } else {
        float proportionalV = Kp * errorV;
        integralV += Ki * errorV * (DC_CONTROL_UPDATE_INTERVAL / 1000.0f);
        integralV = constrain(integralV, -integralLimit, integralLimit);
        derivativeV = Kd * (errorV - prevErrorV) / (DC_CONTROL_UPDATE_INTERVAL / 1000.0f);
        prevErrorV = errorV;
        pidOutput = proportionalV + integralV + derivativeV;
      }
    }

    // Limit CC mode at high voltage
    if (isCC && labV_meas > labV_set * 1.05f) {
      pwmDuty = constrain(pwmDuty, dutyMin, (labV_set / systemVoutMax) * 100.0f);
      integralI = 0.0f;
    }

    // Check PID divergence
    errorPidDivergence = fabs(pidOutput) > 1.5f * dutyMax && !isCC;
    errorPidCurrentDivergence = fabs(pidOutput) > 1.5f * dutyMax && isCC;
    if (errorPidDivergence || errorPidCurrentDivergence) {
      if (isCC) {
        integralI *= 0.5f;
      } else {
        integralV *= 0.5f;
      }
    }

    // Incremental PWM adjustment
    pwmDuty += pidOutput;
    pwmDuty = constrain(pwmDuty, dutyMin, dutyMax);

    // Anti-windup for CV and CC modes
    if (isCC) {
      // CC mode: Prevent integral windup for current control
      if (pwmDuty >= dutyMax && errorI > 0) {
        integralI = max(integralI - Ki_I * errorI * (DC_CONTROL_UPDATE_INTERVAL / 1000.0f), -integralLimit_I);
      }
      if (pwmDuty <= dutyMin && errorI < 0) {
        integralI = min(integralI - Ki_I * errorI * (DC_CONTROL_UPDATE_INTERVAL / 1000.0f), integralLimit_I);
      }
    } else {
      // CV mode: Prevent integral windup for voltage control
      if (pwmDuty >= dutyMax && errorV > 0) {
        integralV = max(integralV - Ki * errorV * (DC_CONTROL_UPDATE_INTERVAL / 1000.0f), -integralLimit);
      }
      if (pwmDuty <= dutyMin && errorV < 0) {
        integralV = min(integralV - Ki * errorV * (DC_CONTROL_UPDATE_INTERVAL / 1000.0f), integralLimit);
      }
    }

    // Peak and RMS analysis
    float deltaV = labV_meas - labV_set;
    float deltaI = labI_meas - labI_set;
    if (fabs(deltaV) > maxDeltaV) maxDeltaV = fabs(deltaV);
    if (fabs(deltaI) > maxDeltaI) maxDeltaI = fabs(deltaI);
    sumV2 += deltaV * deltaV;
    sumI2 += deltaI * deltaI;
    rmsCount++;

    // Send debug data periodically
    if (dbgMode == 2 || dbgMode == 3) updateDebugVars();
  }

  // Update PWM output
  uint16_t pwmValue = invertPwmSignal ? pwmMax - (uint16_t)(pwmDuty / 100.0f * pwmMax)
                                      : (uint16_t)(pwmDuty / 100.0f * pwmMax);
  ledcWrite(DC_CONTROL_PIN, pwmValue);
}

}  // namespace DcControl
