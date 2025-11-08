#pragma once

#include <Arduino.h>

// Wi-Fi and OTA manager for connectivity and firmware updates
namespace WifiOtaManager {
  void begin();       // Initialize Wi-Fi and OTA
  void update();      // Update Wi-Fi and OTA state
  bool isConnected(); // Check Wi-Fi connection status
} // namespace WifiOtaManager
