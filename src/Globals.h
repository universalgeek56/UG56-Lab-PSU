#pragma once

#include <Arduino.h>

// Global variables and constants for system-wide use

// Measured parameters
extern float labV_meas;     // Measured voltage (V)
extern float labI_meas;     // Measured current (A)
extern float labQ_meas;     // Measured power (W)
extern float labTemp_ntc;   // NTC temperature (°C)

// Setpoints and limits
extern float labV_set;      // Voltage setpoint (V)
extern float labI_set;    // Current limit (A)
extern float labI_cut;     // Fuse current limit (A)
extern float rampedVset;   // Ramped voltage setpoint
extern float rampedIset;   // Ramped current setpoint

// PID parameters (CV)
extern float Kp;            // Proportional gain
extern float Ki;            // Integral gain
extern float Kd;            // Derivative gain
extern float integralLimit; // Integral limit
extern float dutyMin;       // Minimum PWM duty cycle (%)
extern float dutyMax;       // Maximum PWM duty cycle (%)
extern bool invertPwmSignal; // Invert PWM signal

// PID parameters (CC)
extern float Kp_I;          // Proportional gain for current
extern float Ki_I;          // Integral gain for current
extern float Kd_I;          // Derivative gain for current
extern float integralLimit_I; // Integral limit for current
extern float prevIError;    // Previous current error
extern float deltaVMax;     // Max voltage change per step

// Protection settings
extern float tempLimitC;    // Temperature limit (°C)
extern float tempDiffC;     // Temperature difference threshold (°C)
extern float VdevLimit;     // Voltage deviation limit (V)
extern float IdevLimit;     // Voltage deviation limit (%)
extern bool tempFaultActive; // Overheat fault
extern bool errorOverCurrent; // Overcurrent error
extern bool errorFuseBlown;  // Fuse blown error
extern bool errorSensorFail; // Sensor failure
extern bool errorInaInitFail; // INA226 initialization failure
extern bool errorWifiInitFail; // WiFi initialization failure
extern bool errorSsd1306InitFail; // SSD1306 initialization failure
extern bool errorPwmInitFail; // PWM initialization failure
extern bool errorVoutOverLimit; // Output voltage over limit
extern bool errorOverPower;  // Over power error
extern bool errorVoltageDev; // Voltage deviation error
extern bool errorCurrentDev; // Current over limit
extern bool errorPowerOverLimit; // Power over limit
extern bool errorLedcInitFail; // LEDC initialization failure
extern bool errorPidDivergence; // PID divergence error
extern bool errorLowMemory;  // Low memory error
extern bool errorHighCpuTemp; // High CPU temperature error
extern bool errorPidCurrentDivergence; // Current PID divergence error

// Error flags for ErrMgr
#define MAX_ERRORS 32
extern bool* errorFlags[MAX_ERRORS]; // Array of error flag pointers
extern uint32_t errorCode;          // Combined error code

// Operating modes and control
extern bool modeAuto;               // Automatic mode
extern bool manualOutputEnable;     // Manual output enable
extern bool outputActive;           // Output active state
extern bool isCC;                   // Current control mode flag

// Display settings
extern bool mainScreenVoltage;      // Show voltage on main screen
extern bool editingValue;           // Editing mode active
extern volatile bool configMenuRequested; // Config menu request
extern volatile bool miniMenuRequested;  // Mini menu request

// Main screen footer
extern unsigned long footerActiveUntil; // Footer active timestamp
extern const unsigned long IDLE_TIMEOUT_MS; // Idle timeout (ms)

// Encoder and editing
extern bool encoderAutoApply;       // Auto-apply encoder changes
extern const unsigned long LONG_PRESS_MS; // Long press duration (ms)

// System limits
extern float systemVoutMin;         // Minimum output voltage (V)
extern float systemVoutMax;         // Maximum output voltage (V)
extern float systemIlimitMax;       // Maximum current limit (A)
extern float systemPowerMax;        // Maximum power limit (W)


// Storage settings
extern bool needSave;               // Settings need saving
extern bool isSaving;               // Saving in progress
extern unsigned long lastSaveTime;  // Last save timestamp
extern unsigned long saveIndicatorTimeout; // Save indicator timeout
extern bool settingsLoaded;         // Settings loaded flag
extern uint8_t settingsVersion;     // Settings version

// Communication status
extern bool wsConnected;            // WebSocket connection status

// Miscellaneous
extern bool inEdit;                 // Editing state
extern bool ledState;               // LED state

// Theme settings
extern uint16_t themeHue;           // Theme hue (0-360)
extern uint8_t themeSat;            // Theme saturation (0-255)
extern uint8_t themeVal;            // Theme value (0-255)

// Wi-Fi and OTA settings
extern bool wifiEnabled;            // WiFi enabled
extern bool wifiConnected;          // WiFi connected
extern bool otaEnabled;             // OTA enabled
extern bool apMode;                 // Access point mode
extern char wifiSSID[32];           // WiFi SSID
extern char wifiPass[32];           // WiFi password
extern char wifiIP[16];             // WiFi IP address
extern int wifiRSSI;                // WiFi signal strength (dBm)
extern char apSSID[32];             // AP SSID
extern char apPass[32];             // AP password
extern char otaHostname[32];        // OTA hostname

// Local timers
extern unsigned long lastSend;       // Last WebSocket send timestamp
extern unsigned long lastBlink;      // Last LED blink timestamp
extern unsigned long lastDisplayUpdate; // Last display update timestamp
extern unsigned long lastUpdateDcControl; // Last DC control update timestamp
extern unsigned long lastWiFiReconnect; // Last WiFi reconnect timestamp
extern unsigned long lastOTACheck;   // Last OTA check timestamp
extern unsigned long lastActivityTime; // Last user activity timestamp

// Debug variables for /charts
extern int dbgMode;                 // Debug mode: 0 = off, 1 = sensor setup, 2 = voltage PID, 3 = current PID, ...
extern float debugVars[6];          // Debug variables array
extern bool debugEnabled;           // Debug mode enabled
extern float fixedThreshold;        // Fixed threshold for debugging

// Update intervals (ms)
constexpr unsigned long DISPLAY_UPDATE_INTERVAL = 200;    // Display update interval
constexpr unsigned long WEBSOCKET_SEND_INTERVAL = 500;    // WebSocket send interval
constexpr unsigned long LED_BLINK_INTERVAL = 1000;        // LED blink interval
constexpr unsigned long DC_CONTROL_UPDATE_INTERVAL = 35;  // DC control update interval

// Touch UI constants
constexpr unsigned long CALIB_INTERVAL = 50;   // Calibration interval (ms)
constexpr unsigned long CALIB_SAMPLES = 150;   // Number of calibration samples
constexpr unsigned long AMPS_HISTORY = 8;      // Amplitude history points
constexpr float ALPHA = 0.1f;                 // Touch smoothing filter coefficient
constexpr float BASE_ALPHA = 0.01f;           // Baseline adjustment coefficient
constexpr unsigned long MIN_TOGGLE_MS = 300;  // Minimum toggle interval (ms)