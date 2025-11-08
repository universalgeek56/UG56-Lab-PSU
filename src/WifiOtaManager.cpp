#include "WifiOtaManager.h"
#include "Globals.h"
#include <WiFi.h>
#include <ArduinoOTA.h>

namespace WifiOtaManager {

constexpr unsigned long CHECK_INTERVAL_MS = 500;    // Config check interval (ms)
constexpr unsigned long STA_RECONNECT_MS = 5000;    // STA reconnect interval (ms)
constexpr unsigned long REBOOT_DELAY_MS = 5000;     // Delay before reboot (ms)
constexpr int WIFI_RSSI_INVALID = -100;             // Invalid RSSI value

// Internal state
static bool otaActive = false;                     // OTA active flag
static bool prevOtaEnabled = false;                // Previous OTA enabled state
static unsigned long lastCheck = 0;                // Last config check timestamp
static unsigned long lastSTAReconnect = 0;         // Last STA reconnect timestamp
static bool restartPending = false;                // Restart pending flag
static unsigned long restartStartTime = 0;         // Restart start timestamp
static bool initialized = false;                   // Initialization flag

// Wi-Fi configuration for change detection
struct WiFiConfig {
  bool apMode;
  char ssid[32];  // Match wifiSSID size in Globals.h
  char pass[32];  // Match wifiPass size in Globals.h

  bool operator==(const WiFiConfig& other) const {
    return apMode == other.apMode && strcmp(ssid, other.ssid) == 0 && strcmp(pass, other.pass) == 0;
  }
  bool operator!=(const WiFiConfig& other) const {
    return !(*this == other);
  }
};

static WiFiConfig prevConfig; // Initialized after loading settings

// Start OTA
static void startOTA() {
  if (!otaActive) {
    ArduinoOTA.setHostname(otaHostname);
    ArduinoOTA.begin();
    otaActive = true;
  }
}

// Stop OTA
static void stopOTA() {
  if (otaActive) {
    ArduinoOTA.end();
    otaActive = false;
  }
}

// Update STA connection info
static void updateSTAInfo() {
  if (WiFi.status() == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    snprintf(wifiIP, sizeof(wifiIP), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    wifiRSSI = WiFi.RSSI();
    wifiConnected = true;
  } else {
    wifiConnected = false;
    wifiRSSI = WIFI_RSSI_INVALID;
  }
}

// Setup Access Point mode
static void setupAP() {
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(apSSID, apPass)) {
    errorWifiInitFail = true; // Set error flag on failure
    return;
  }
  errorWifiInitFail = false;
  wifiConnected = true;
  IPAddress ip = WiFi.softAPIP();
  snprintf(wifiIP, sizeof(wifiIP), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  stopOTA(); // OTA not needed in AP mode
}

// Setup Station mode
static void setupSTA() {
  WiFi.mode(WIFI_STA);
  if (!WiFi.begin(wifiSSID, wifiPass)) {
    errorWifiInitFail = true; // Set error flag on failure
    return;
  }
  errorWifiInitFail = false;
  wifiConnected = false;
  if (otaEnabled) startOTA();
}

// Initialize Wi-Fi and OTA
void begin() {
  if (!wifiEnabled) {
    errorWifiInitFail = true;
    return;
  }

  prevConfig = {apMode, "", ""};
  strncpy(prevConfig.ssid, wifiSSID, sizeof(prevConfig.ssid) - 1);
  prevConfig.ssid[sizeof(prevConfig.ssid) - 1] = '\0';
  strncpy(prevConfig.pass, wifiPass, sizeof(prevConfig.pass) - 1);
  prevConfig.pass[sizeof(prevConfig.pass) - 1] = '\0';
  initialized = true;

  if (apMode) {
    setupAP();
  } else {
    setupSTA();
  }
}

// Update Wi-Fi and OTA state
void update() {
  if (!initialized) return;

  // Handle OTA enable/disable
  if (otaEnabled != prevOtaEnabled) {
    if (otaEnabled) startOTA();
    else stopOTA();
    prevOtaEnabled = otaEnabled;
  }

  if (otaActive) ArduinoOTA.handle();

  unsigned long now = millis();

  // Check for config changes
  if (now - lastCheck >= CHECK_INTERVAL_MS) {
    lastCheck = now;

    WiFiConfig currentConfig = {apMode, "", ""};
    strncpy(currentConfig.ssid, wifiSSID, sizeof(currentConfig.ssid) - 1);
    currentConfig.ssid[sizeof(currentConfig.ssid) - 1] = '\0';
    strncpy(currentConfig.pass, wifiPass, sizeof(currentConfig.pass) - 1);
    currentConfig.pass[sizeof(currentConfig.pass) - 1] = '\0';

    if (currentConfig != prevConfig && !restartPending) {
      prevConfig = currentConfig;
      restartPending = true;
      restartStartTime = now;
    }
  }

  // Handle delayed restart
  if (restartPending && now - restartStartTime >= REBOOT_DELAY_MS) {
    restartPending = false;
    ESP.restart();
  }

  // Handle STA reconnection
  if (!wifiEnabled || apMode) return;

  updateSTAInfo();
  if (!wifiConnected && now - lastSTAReconnect >= STA_RECONNECT_MS) {
    lastSTAReconnect = now;
    WiFi.begin(wifiSSID, wifiPass);
  }
}

// Check Wi-Fi connection status
bool isConnected() {
  return wifiConnected;
}

} // namespace WifiOtaManager