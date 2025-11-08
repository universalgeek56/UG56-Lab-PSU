#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "TouchUI.h"
#include "Config.h"
#include "Globals.h"

namespace TouchUI {

constexpr uint8_t inactiveVal = 5; // LED brightness when inactive
Adafruit_NeoPixel leds(NUM_LEDS, LED_UI_PIN, NEO_GRB + NEO_KHZ800);

// Touch button configurations
SmartTouch btns[2] = {
  {TOUCH1_PAD}, // Button 1
  {TOUCH2_PAD}  // Button 2
};

// Read touch value with median filter
static int medianTouchRead(uint8_t pin) {
  int readings[3];
  for (int i = 0; i < 3; i++) {
    readings[i] = touchRead(pin);
    yield();
  }
  // Sort readings
  if (readings[0] > readings[1]) { int t = readings[0]; readings[0] = readings[1]; readings[1] = t; }
  if (readings[1] > readings[2]) { int t = readings[1]; readings[1] = readings[2]; readings[2] = t; }
  if (readings[0] > readings[1]) { int t = readings[0]; readings[0] = readings[1]; readings[1] = t; }
  // Return median if in valid range [10000, 20000]
  return (readings[1] >= 10000 && readings[1] <= 20000) ? readings[1] : 0;
}

// Calibrate touch button
void calibrateButton(SmartTouch& b) {
  if (!b.calibrating) return;

  unsigned long now = millis();
  // Set red LED during calibration
  for (int i = 0; i < min(NUM_LEDS, 2); i++) {
    leds.setPixelColor(i, leds.ColorHSV(0, 255, 30));
  }
  leds.show();

  if (now - b.lastCalib < CALIB_INTERVAL) return;
  b.lastCalib = now;

  int value = medianTouchRead(b.pin);
  if (value == 0) return; // Skip invalid values
  b.calibRaw[b.calibCount] = value;
  b.calibSum += value;
  b.calibCount++;

  if (b.calibCount >= CALIB_SAMPLES) {
    b.baseline = b.calibSum / (float)b.calibCount;
    b.threshold = fixedThreshold;
    b.filtered = b.baseline;
    b.pressAvg = b.baseline + b.threshold;
    for (int i = 0; i < AMPS_HISTORY; i++) b.lastAmps[i] = fixedThreshold;

    b.idx = 0;
    b.pressed = false;
    b.ledState = false;
    b.lastChange = now;
    b.lastInit = now;
    b.calibrating = false;

    b.ledState = (b.pin == TOUCH1_PAD) ? modeAuto : outputActive;
  }
}

// Update touch button state
void updateSmart(SmartTouch& b, int index) {
  if (b.calibrating) return;

  // Read and filter touch value
  b.raw = medianTouchRead(b.pin);
  if (b.raw == 0) return; // Skip invalid values
  b.filtered = b.filtered * (1 - ALPHA) + b.raw * ALPHA;
  b.diff = b.raw - b.filtered;

  // Adjust baseline if not pressed
  if (!b.pressed) {
    float newBaseline = b.baseline * (1 - BASE_ALPHA) + b.filtered * BASE_ALPHA;
    if (abs(newBaseline - b.baseline) < b.baseline * 0.05) {
      b.baseline = newBaseline;
    }
  }

  // Track max difference during press
  if (b.pressed && b.diff > b.maxDiff) {
    b.maxDiff = b.diff;
  }

  // Calculate average amplitude
  float sum = 0;
  for (int i = 0; i < AMPS_HISTORY; i++) sum += b.lastAmps[i];
  float avgAmp = sum / AMPS_HISTORY;

  b.pressAvg = b.baseline + avgAmp;
  b.threshold = avgAmp <= 0 ? fixedThreshold : avgAmp;

  unsigned long now = millis();

  // Handle press
  if (!b.pressed) {
    if (now - b.lastInit < 5000) return; // Ignore first 5 seconds
    if (b.diff > b.threshold && (now - b.lastChange) > MIN_TOGGLE_MS) {
      b.pressed = true;
      b.lastChange = now;

      if (index == 0) modeAuto = !modeAuto;
      else if (index == 1) manualOutputEnable = !manualOutputEnable;

      b.maxDiff = b.diff;
    }
  }
  // Handle release
  else {
    if (b.diff < b.threshold / 2 && (now - b.lastChange) > MIN_TOGGLE_MS) {
      b.pressed = false;
      b.lastChange = now;

      // Update amplitude history for strong presses
      if (b.maxDiff > b.threshold * 1.5) {
        float newAmp = 0.7f * avgAmp + 0.3f * b.maxDiff;
        b.lastAmps[b.idx] = newAmp;
        b.idx = (b.idx + 1) % AMPS_HISTORY;
      }

      b.maxDiff = 0;
    }
  }

  b.ledState = (index == 0) ? modeAuto : outputActive;
}

// Initialize touch buttons and LEDs
void begin() {
  leds.begin();
  leds.show();
  delay(1000); // Stabilize before calibration
  for (int i = 0; i < 2; i++) {
    btns[i].lastCalib = millis();
    btns[i].calibrating = true;
  }
}

// Update touch buttons and LEDs
void update() {

  for (int i = 0; i < 2; i++) {
    if (btns[i].calibrating) calibrateButton(btns[i]);
    else updateSmart(btns[i], i);
  }

  // Update LEDs
  for (int i = 0; i < min(NUM_LEDS, 2); i++) {
    uint32_t color;
    if (btns[i].ledState) {
      uint16_t hue16 = (uint32_t)themeHue * 65535 / 360;
      uint8_t sat8 = themeSat * 255 / 100;
      uint8_t val8 = themeVal * 255 / 100;
      color = leds.ColorHSV(hue16, sat8, val8);
    } else {
      color = leds.ColorHSV(themeHue * 65535 / 360, 255, inactiveVal * 255 / 100);
    }
    leds.setPixelColor(i, color);
  }
  leds.show();

  if (dbgMode == 1) updateDebugVars();
}

// Update debug variables for WebSocket
void updateDebugVars() {
  static unsigned long lastSend = 0;
  unsigned long now = millis();
  if (now - lastSend < WEBSOCKET_SEND_INTERVAL) return;
  lastSend = now;

  debugVars[0] = (float)btns[0].raw;    // TOUCH1_PAD raw value
  debugVars[1] = btns[0].diff;          // TOUCH1_PAD diff from baseline
  debugVars[2] = btns[0].threshold;     // TOUCH1_PAD threshold
  debugVars[3] = (float)btns[1].raw;    // TOUCH2_PAD raw value
  debugVars[4] = btns[1].diff;          // TOUCH2_PAD diff from baseline
  debugVars[5] = btns[1].threshold;     // TOUCH2_PAD threshold
}

// Get pressed state for button
bool getButtonPressed(int index) {
  if (index < 0 || index >= 2) return false;
  return btns[index].pressed;
}

} // namespace TouchUI