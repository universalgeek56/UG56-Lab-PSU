#include "PreferencesManager.h"
#include "Globals.h"
#include <Preferences.h>

namespace PreferencesManager {

Preferences prefs;                // NVS instance
static LabSettings lastSavedSettings; // Last saved settings
static unsigned long lastChangeTime = 0; // Last change timestamp
constexpr unsigned long SAVE_DELAY_MS = 3000; // Save delay (ms)

// NVS namespace
#define PREFS_NAMESPACE "lab_psu"

// Initialize default settings
void initDefaults() {
  lastSavedSettings.Kp = 3.0f;
  lastSavedSettings.Ki = 1.0f;
  lastSavedSettings.Kd = 0.05f;
  lastSavedSettings.Kp_I = 3.0f;
  lastSavedSettings.Ki_I = 1.0f;
  lastSavedSettings.Kd_I = 0.1f;
  lastSavedSettings.integralLimit = 50.0f;
  lastSavedSettings.integralLimit_I = 50.0f;
  lastSavedSettings.tempLimitC = 70.0f;
  lastSavedSettings.tempDiffC = 5.0f;
  lastSavedSettings.VdevLimit = 1.0f;
  lastSavedSettings.IdevLimit = 0.05f;
  lastSavedSettings.themeHue = 180;
  lastSavedSettings.wifiEnabled = true;
  lastSavedSettings.apMode = false;
  strncpy(lastSavedSettings.wifiSSID, "ASUS", sizeof(lastSavedSettings.wifiSSID) - 1);
  lastSavedSettings.wifiSSID[sizeof(lastSavedSettings.wifiSSID) - 1] = '\0';
  strncpy(lastSavedSettings.wifiPass, "RememberToChange!7", sizeof(lastSavedSettings.wifiPass) - 1);
  lastSavedSettings.wifiPass[sizeof(lastSavedSettings.wifiPass) - 1] = '\0';
  lastSavedSettings.otaEnabled = true;
  lastSavedSettings.settingsVersion = 1;

  apply(lastSavedSettings);
}

// Apply settings to global variables
void apply(const LabSettings& settings) {
  Kp = settings.Kp;
  Ki = settings.Ki;
  Kd = settings.Kd;
  Kp_I = settings.Kp_I;
  Ki_I = settings.Ki_I;
  Kd_I = settings.Kd_I;
  integralLimit = settings.integralLimit;
  integralLimit_I = settings.integralLimit_I;
  tempLimitC = settings.tempLimitC;
  tempDiffC = settings.tempDiffC;
  VdevLimit = settings.VdevLimit;
  IdevLimit = settings.IdevLimit;
  themeHue = settings.themeHue;
  wifiEnabled = settings.wifiEnabled;
  apMode = settings.apMode;
  strncpy(wifiSSID, settings.wifiSSID, sizeof(wifiSSID) - 1);
  wifiSSID[sizeof(wifiSSID) - 1] = '\0';
  strncpy(wifiPass, settings.wifiPass, sizeof(wifiPass) - 1);
  wifiPass[sizeof(wifiPass) - 1] = '\0';
  otaEnabled = settings.otaEnabled;
  settingsVersion = settings.settingsVersion; // Sync settings version
}

// Initialize NVS and load settings
void begin() {
  prefs.begin(PREFS_NAMESPACE, false);
  size_t len = prefs.getBytes("settings", &lastSavedSettings, sizeof(LabSettings));
  if (len != sizeof(LabSettings) || lastSavedSettings.settingsVersion != settingsVersion) {
    initDefaults();
    prefs.putBytes("settings", &lastSavedSettings, sizeof(LabSettings));
  }
  apply(lastSavedSettings);
}

// Save settings to NVS
void save() {
  prefs.putBytes("settings", &lastSavedSettings, sizeof(LabSettings));
  needSave = false; // Use global needSave
}

// Update settings with delayed save
void update() {
  LabSettings current;
  memcpy(&current, &lastSavedSettings, sizeof(LabSettings));

  current.Kp = Kp;
  current.Ki = Ki;
  current.Kd = Kd;
  current.Kp_I = Kp_I;
  current.Ki_I = Ki_I;
  current.Kd_I = Kd_I;
  current.integralLimit = integralLimit;
  current.integralLimit_I = integralLimit_I;
  current.tempLimitC = tempLimitC;
  current.tempDiffC = tempDiffC;
  current.VdevLimit = VdevLimit;
  current.IdevLimit = IdevLimit;
  current.themeHue = themeHue;
  current.wifiEnabled = wifiEnabled;
  current.apMode = apMode;
  strncpy(current.wifiSSID, wifiSSID, sizeof(current.wifiSSID) - 1);
  current.wifiSSID[sizeof(current.wifiSSID) - 1] = '\0';
  strncpy(current.wifiPass, wifiPass, sizeof(current.wifiPass) - 1);
  current.wifiPass[sizeof(current.wifiPass) - 1] = '\0';
  current.otaEnabled = otaEnabled;
  current.settingsVersion = settingsVersion;

  if (memcmp(&current, &lastSavedSettings, sizeof(LabSettings)) != 0) {
    lastSavedSettings = current;
    lastChangeTime = millis();
    needSave = true; // Use global needSave
  }

  if (needSave && (millis() - lastChangeTime >= SAVE_DELAY_MS)) {
    save();
  }
}

// Get current settings
LabSettings& get() {
  return lastSavedSettings;
}

} // namespace PreferencesManager