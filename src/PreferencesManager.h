#pragma once

#include <Arduino.h>

// Preferences manager for saving and loading system settings to NVS
struct LabSettings {
  float Kp;                // Proportional gain (CV)
  float Ki;                // Integral gain (CV)
  float Kd;                // Derivative gain (CV)
  float Kp_I;              // Proportional gain (CC)
  float Ki_I;              // Integral gain (CC)
  float Kd_I;              // Derivative gain (CC)
  float integralLimit;     // Integral limit (CV)
  float integralLimit_I;   // Integral limit (CC)
  float tempLimitC;        // Overheat threshold (°C)
  float tempDiffC;         // Temperature hysteresis (°C)
  float VdevLimit;         // Voltage deviation limit (V)
  float IdevLimit;         // Current deviation limit (%)
  uint16_t themeHue;       // Theme hue (0-360)
  bool wifiEnabled;        // WiFi enabled
  bool apMode;             // Access point mode
  char wifiSSID[32];       // WiFi SSID
  char wifiPass[32];       // WiFi password
  bool otaEnabled;         // OTA enabled
  uint8_t settingsVersion; // Settings version
};

namespace PreferencesManager {
  void begin();                // Initialize NVS and load settings
  void save();                 // Save settings to NVS
  void apply(const LabSettings& settings); // Apply settings to global variables
  void update();               // Update settings with delayed save
  LabSettings& get();          // Get current settings
} // namespace PreferencesManager