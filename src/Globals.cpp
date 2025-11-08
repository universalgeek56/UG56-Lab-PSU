#include "Globals.h"

// Measured parameters
float labV_meas = 0.0f;        // Measured voltage (V)
float labI_meas = 0.0f;        // Measured current (A)
float labQ_meas = 0.0f;        // Measured power (W)
float labTemp_ntc = 25.0f;     // NTC temperature (°C)

// Setpoints and limits
float labV_set = 5.0f;         // Voltage setpoint (V)
float labI_set = 2.0f;       // Current limit (A)
float labI_cut = 2.0f;        // Fuse current limit (A)
float rampedVset = labV_set;   // Ramped voltage setpoint
float rampedIset = labI_set;   // Ramped voltage setpoint

// PID parameters (CV)
float Kp = 3.0f;               // Proportional gain
float Ki = 1.0f;               // Integral gain
float Kd = 0.05f;              // Derivative gain
float integralLimit = 50.0f;   // Integral limit
float dutyMin = 5.0f;          // Minimum PWM duty cycle (%)
float dutyMax = 100.0f;        // Maximum PWM duty cycle (%)
bool invertPwmSignal = false;  // PWM signal inversion

// PID parameters (CC)
bool modeCCActive = false;     // Current control mode
float Kp_I = 3.0f;             // Proportional gain for current
float Ki_I = 1.0f;             // Integral gain for current
float Kd_I = 0.1f;             // Derivative gain for current
float integralLimit_I = 50.0f; // Integral limit for current
float prevIError = 0.0f;       // Previous current error
float deltaVMax = 0.05f;       // Max voltage change per step (V)

// Protection settings
float tempLimitC = 70.0f;      // Overheat threshold (°C)
float tempDiffC = 5.0f;        // Temperature hysteresis (°C)
float VdevLimit = 1.0f;        // Voltage deviation limit (V)
float IdevLimit = 0.05f;       // Current deviation limit (5%)
bool tempFaultActive = false;  // Overheat protection
bool errorOverCurrent = false; // Overcurrent error
bool errorFuseBlown = false;   // Fuse blown error
bool errorSensorFail = false;  // Sensor failure
bool errorInaInitFail = false; // INA226 initialization failure
bool errorWifiInitFail = false; // WiFi initialization failure
bool errorSsd1306InitFail = false; // SSD1306 initialization failure
bool errorPwmInitFail = false; // PWM initialization failure
bool errorVoutOverLimit = false; // Output voltage over limit
bool errorOverPower = false;   // Over power error
bool errorVoltageDev = false;  // Voltage deviation error
bool errorCurrentDev  = false; // Current deviation error
bool errorPowerOverLimit = false; // Power over limit
bool errorLedcInitFail = false; // LEDC initialization failure
bool errorPidDivergence = false; // Voltage PID divergence error
bool errorLowMemory = false;    // Low memory error
bool errorHighCpuTemp = false; // High CPU temperature error
bool errorPidCurrentDivergence = false; // Current PID divergence error

// Error flags for ErrMgr
bool* errorFlags[MAX_ERRORS] = {
  &tempFaultActive,          // Bit 0: Overheat
  &errorOverCurrent,         // Bit 1: Overcurrent
  &errorFuseBlown,           // Bit 2: Fuse blown
  &errorSensorFail,          // Bit 3: Sensor failure
  &errorInaInitFail,         // Bit 4: INA226 init failure
  &errorWifiInitFail,        // Bit 5: WiFi init failure
  &errorSsd1306InitFail,     // Bit 6: SSD1306 init failure
  &errorPwmInitFail,         // Bit 7: PWM init failure
  &errorVoutOverLimit,       // Bit 8: Voltage out of limits
  &errorOverPower,           // Bit 9: Over power
  &errorVoltageDev,          // Bit 10: Voltage deviation
  &errorCurrentDev,          // Bit 11: Current over limit
  &errorPowerOverLimit,      // Bit 12: Power over limit
  &errorLedcInitFail,        // Bit 13: LEDC init failure
  &errorPidDivergence,       // Bit 14: Voltage PID divergence
  &errorLowMemory,           // Bit 15: Low memory
  &errorHighCpuTemp,         // Bit 16: High CPU temperature
  &errorPidCurrentDivergence // Bit 17: Current PID divergence
  // Remaining elements are nullptr
};

// Combined error code
uint32_t errorCode = 0;

// Operating modes and control
bool modeAuto = true;          // Automatic mode
bool manualOutputEnable = false; // Manual output enable
bool outputActive = false;     // Output state
bool isCC = false;             // Current control mode flag

// Display settings
bool mainScreenVoltage = true; // Show voltage on main screen
bool editingValue = false;     // Editing mode
volatile bool configMenuRequested = false; // Config menu request
volatile bool miniMenuRequested = false;   // Mini menu request

// Main screen footer
unsigned long footerActiveUntil = 0; // Footer active timestamp
const unsigned long IDLE_TIMEOUT_MS = 10000; // Idle timeout (ms)

// Encoder and editing
bool encoderAutoApply = true;  // Auto-apply encoder changes
const unsigned long LONG_PRESS_MS = 800; // Long press duration (ms)

// System limits
float systemVoutMin = 1.5f;    // Minimum output voltage (V)
float systemVoutMax = 36.0f;   // Maximum output voltage (V)
float systemIlimitMax = 3.2f;  // Maximum current limit (A)
float systemPowerMax = 100.0f; // Maximum power (W)

// Storage settings
bool needSave = false;         // Settings need saving
bool isSaving = false;         // Saving in progress
unsigned long lastSaveTime = 0; // Last save timestamp
unsigned long saveIndicatorTimeout = 0; // Save indicator timeout
bool settingsLoaded = false;   // Settings loaded
uint8_t settingsVersion = 1;   // Settings version

// Communication status
bool wsConnected = false;      // WebSocket connection status

// Miscellaneous
bool inEdit = false;           // Editing state
bool ledState = false;         // LED state

// Theme settings
uint16_t themeHue = 180;       // Theme hue (0-360)
uint8_t themeSat = 100;        // Theme saturation (0-100)
uint8_t themeVal = 50;         // Theme value (0-100)

// Wi-Fi and OTA settings
bool wifiEnabled = true;       // WiFi enabled
bool wifiConnected = false;    // WiFi connected
bool otaEnabled = true;        // OTA enabled
bool apMode = false;           // Access point mode
char wifiSSID[32] = "ASUS";    // WiFi SSID
char wifiPass[32] = "RememberToChange!7"; // WiFi password
char wifiIP[16] = "0.0.0.0";   // WiFi IP address
int wifiRSSI = -100;           // WiFi signal strength (dBm)
char apSSID[32] = "PSU_AP";    // AP SSID
char apPass[32] = "12345678"; // AP password
char otaHostname[32] = "Lab_PSU"; // OTA hostname

// Local timers
unsigned long lastSend = 0;    // Last WebSocket send timestamp
unsigned long lastBlink = 0;   // Last LED blink timestamp
unsigned long lastDisplayUpdate = 0; // Last display update timestamp
unsigned long lastUpdateDcControl = 0; // Last DC control update timestamp
unsigned long lastWiFiReconnect = 0; // Last WiFi reconnect timestamp
unsigned long lastOTACheck = 0; // Last OTA check timestamp
unsigned long lastActivityTime = 0; // Last activity timestamp

// Debug variables for /charts
int dbgMode = 0;               // Debug mode: 0 = off, 1 = sensor setup, 2 = voltage PID, 3 = current PID, ...
float debugVars[6] = {0};      // Debug array
bool debugEnabled = false;     // Debug mode
float fixedThreshold = 200.0f; // Touch threshold

