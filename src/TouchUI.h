#pragma once

#include <Arduino.h>

// Touch UI manager for handling touch buttons and LEDs
namespace TouchUI {

struct SmartTouch {
  uint8_t pin;              // Touch pin number
  bool calibrating = true;  // Calibration in progress
  unsigned long lastCalib = 0; // Last calibration timestamp
  float calibSum = 0;       // Sum of calibration samples
  int calibCount = 0;       // Number of calibration samples
  int calibRaw[150];        // Calibration samples (size matches CALIB_SAMPLES)
  float baseline = 0;       // Baseline touch value
  float filtered = 0;       // Filtered touch value
  float diff = 0;           // Difference from baseline
  float maxDiff = 0;        // Maximum difference observed
  float pressAvg = 0;       // Average value during press
  float threshold = 0;      // Touch detection threshold
  float lastAmps[8];        // Amplitude history (size matches AMPS_HISTORY)
  uint8_t idx = 0;          // Current index in lastAmps
  bool pressed = false;      // Button pressed state
  bool ledState = false;    // LED state
  unsigned long lastChange = 0; // Last state change timestamp
  unsigned long lastInit = 0;   // Last initialization timestamp
  int raw = 0;              // Raw touchRead value
};

extern SmartTouch btns[2];  // Array of two touch buttons

void begin();               // Initialize touch buttons and LEDs
void update();              // Update touch button states and LEDs
bool getButtonPressed(int index); // Get pressed state for button index
void updateDebugVars();     // Update debugVars with touch parameters

} // namespace TouchUI