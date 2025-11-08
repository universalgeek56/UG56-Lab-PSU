#pragma once

#include <Arduino.h>
#include <functional>

namespace EncoderManager {

// Screen states for UI navigation
enum class ScreenState {
  MainIdle,     // Main screen, idle state
  MainEditing,  // Main screen, editing parameter
  ConfigIdle,   // Config menu, idle state
  ConfigEditing // Config menu, editing item
};

// Menu item structure for configuration pages
struct MenuItem {
  const char* label;                    // Page title
  std::function<String()> line1;        // First center line
  std::function<String()> line2;        // Second center line
  std::function<bool()> boolValue;      // Boolean state for footer
  std::function<void(bool)> setFunc;    // Action handler for button press
  const char* label1;                   // Left button label
  const char* label2;                   // Right button label
};

// Draft index control for menu navigation
uint8_t getDraftIndex();              // Get current draft index (0 = left, 1 = right)
void setDraftIndex(uint8_t idx);      // Set draft index

// Accessor functions
ScreenState getScreenState();         // Get current screen state
int getCurrentMenuIndex();            // Get current menu index
MenuItem* getMenuItems();             // Get menu items array
int getMenuCount();                   // Get number of menu items

// Core lifecycle
void begin();                         // Initialize encoder and button
void update();                        // Update encoder and button state

// Editing state
void beginEdit(float* target);        // Start editing a parameter
void cancelEdit();                    // Cancel editing
float getCurrentStep();               // Get current step size
const char* getActiveParamName();     // Get current parameter name
float* getEditTarget();               // Get current edit target
const char* getEditType();            // Get edit type (VOLTAGE, CURRENT, NONE)
bool isEditing();                     // Check if in editing mode

} // namespace EncoderManager