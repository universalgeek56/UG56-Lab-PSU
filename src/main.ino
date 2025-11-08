#include <WiFi.h>
#include <Wire.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "Config.h"
#include "Globals.h"
#include "WifiOtaManager.h"
#include "EncoderManager.h"
#include "DisplayManager.h"
#include "Ina226Manager.h"
#include "OutputControl.h"
#include "TouchUI.h"
#include "DcControl.h"
#include "WebInterface.h"
#include "PreferencesManager.h"
#include "ErrMgr.h"

// Initialize hardware and managers
void setup() {
  pinMode(PULSE_LED_PIN, OUTPUT);
  digitalWrite(PULSE_LED_PIN, LOW);
  Wire.begin(SDA_PIN, SCL_PIN);

  DisplayManager::begin();
  PreferencesManager::begin();
  EncoderManager::begin();
  Ina226Manager::begin();
  OutputControl::begin();
  TouchUI::begin();
  DcControl::begin();
  WifiOtaManager::begin();
  WebInterface::begin();
  ErrMgr::begin();
}

// Main loop for updating system components
void loop() {
  // Blink LED at specified interval
  if (millis() - lastBlink >= LED_BLINK_INTERVAL) {
    ledState = !ledState;
    digitalWrite(PULSE_LED_PIN, ledState);
    lastBlink = millis();
  }

  EncoderManager::update();
  Ina226Manager::update();
  OutputControl::update();
  TouchUI::update();
  DcControl::update();
  WifiOtaManager::update();
  WebInterface::update();
  DisplayManager::update();
  ErrMgr::update();
  PreferencesManager::update();
}