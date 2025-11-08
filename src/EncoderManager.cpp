#include "EncoderManager.h"
#include "Globals.h"
#include "Config.h"
#include "ErrMgr.h"
#include "DisplayManager.h"
#include <RotaryEncoder.h>

namespace EncoderManager {

// Encoder instance (TWO03: 2 steps per detent)
RotaryEncoder encoder(ENC_DT, ENC_CLK, RotaryEncoder::LatchMode::TWO03);
// Other options (uncomment one if needed):
// RotaryEncoder encoder(ENC_DT, ENC_CLK, RotaryEncoder::LatchMode::FULL_STEP);
// RotaryEncoder encoder(ENC_DT, ENC_CLK, RotaryEncoder::LatchMode::HALF_STEP);
// RotaryEncoder encoder(ENC_DT, ENC_CLK, RotaryEncoder::LatchMode::FOUR03);

long encLastPos = 0;
static unsigned long lastEditUpdate = 0;

// Step sizes for parameter adjustments
constexpr float STEP_0_01 = 0.01f;
constexpr float STEP_0_1 = 0.1f;
constexpr float STEP_1 = 1.0f;
static float stepVset = STEP_0_01; // Voltage step
static float stepI = STEP_0_01;    // Current step
static float currentStep = STEP_0_01; // Current active step

// Main parameters (voltage/current setpoints & limits)
struct Param {
  const char* name;
  float* value;
  float minVal;
  float maxVal;
  const char* unit;
};

static Param params[] = {
  {"Vset", &labV_set, systemVoutMin, systemVoutMax, "V"},
  {"Iset", &labI_set, 0.0f, systemIlimitMax, "A"},
  {"Icut", &labI_cut, 0.0f, systemIlimitMax, "A"}
};
static const int paramCount = sizeof(params) / sizeof(params[0]);
static int currentParamIndex = 0;
static float* editTarget = params[0].value;

// Button handling
enum class ButtonEvent { None, Short, Long };
constexpr unsigned long BTN_DEBOUNCE_MS = 30; // Debounce time (ms)
constexpr unsigned long BTN_LONG_MS = 600;    // Long press threshold (ms)

// Screen state
static ScreenState currentState = ScreenState::MainIdle;
static bool editingValue = false;
static unsigned long lastActivityTime = 0;

// Config menu items
struct ConfigItem {
  enum class Type { Bool, Enum } type;
  void* value;
  const char* name;
  int enumCount = 0;
};

static ConfigItem configMenu[] = {
  {ConfigItem::Type::Bool, &apMode, "STA/AP"},
  {ConfigItem::Type::Bool, &wifiEnabled, "WiFi"},
  {ConfigItem::Type::Bool, &otaEnabled, "OTA"},
  {ConfigItem::Type::Bool, &mainScreenVoltage, "Main"}
};
static const int configCount = sizeof(configMenu) / sizeof(configMenu[0]);
static int currentConfigIndex = 0;

// Display menu pages
MenuItem menuItems[] = {
  {"Errors",
   []() { return String(""); },
   []() { return String(""); },
   []() { return true; },
   nullptr,
   "Exit", ""},
  {"WiFi:STA/AP",
   []() { return String("") + apSSID + " PASS:" + apPass; },
   []() { return apMode ? String("APIP: ") + wifiIP : String("APIP: N/A"); },
   []() { return apMode; },
   [](bool val) { apMode = val; },
   "AP", "STA"},
  {"WiFi: ON/OFF",
   []() { return String("SSID: ") + wifiSSID; },
   []() { return wifiConnected ? String("IP: ") + wifiIP : String("No IP"); },
   []() { return wifiEnabled; },
   [](bool val) { wifiEnabled = val; },
   "ON", "OFF"},
  {"OTA: ON/OFF",
   []() { return String("OTA Name: ") + otaHostname; },
   []() { return String("Pass: None / Any"); },
   []() { return otaEnabled; },
   [](bool val) { otaEnabled = val; },
   "ON", "OFF"},
  {"Main Parameter",
   []() { return String("Main parameter"); },
   []() { return String("on Home screen"); },
   []() { return mainScreenVoltage; },
   [](bool val) { mainScreenVoltage = val; },
   "Voltage", "Current"}
};
const int menuCount = sizeof(menuItems) / sizeof(menuItems[0]);
static int currentMenuIndex = 0;

// Draft index for bool menu pages (0 = left, 1 = right)
static uint8_t draftIndex = 0;

uint8_t getDraftIndex() { return draftIndex; }
void setDraftIndex(uint8_t idx) { if (idx < 2) draftIndex = idx; }

static void syncDraftWithPage() {
  MenuItem& page = menuItems[currentMenuIndex];
  bool val = page.boolValue ? page.boolValue() : false;
  draftIndex = val ? 0 : 1;
}

void handleDraftRotation(int8_t steps) {
  if (steps == 0) return;
  int newIndex = (int)draftIndex + (steps > 0 ? 1 : -1);
  newIndex = constrain(newIndex, 0, 1);
  draftIndex = (uint8_t)newIndex;
}

void confirmDraft() {
  MenuItem& page = menuItems[currentMenuIndex];
  if (page.setFunc) page.setFunc(draftIndex == 0);
  editingValue = false;
}

// Poll button with debounce and long press detection
static ButtonEvent pollButton() {
  static bool lastState = HIGH;
  static unsigned long lastChange = 0;
  static unsigned long pressTime = 0;
  static bool longReported = false;

  bool raw = digitalRead(ENC_SW);
  unsigned long now = millis();

  if (raw != lastState && now - lastChange >= BTN_DEBOUNCE_MS) {
    lastState = raw;
    lastChange = now;
    if (raw == LOW) {
      pressTime = now;
      longReported = false;
    } else if (!longReported) {
      return ButtonEvent::Short;
    }
  }

  if (lastState == LOW && !longReported && (now - pressTime >= BTN_LONG_MS)) {
    longReported = true;
    return ButtonEvent::Long;
  }

  return ButtonEvent::None;
}

// Start editing parameter
void beginEdit(float* target) {
  editTarget = target;
  editingValue = true;
  lastActivityTime = millis();
  encLastPos = encoder.getPosition();
  currentStep = (editTarget == &labV_set) ? stepVset : stepI;
  lastEditUpdate = millis();
}

// Cancel editing
void cancelEdit() { editingValue = false; }

// Cycle step size (0.01 -> 0.1 -> 1)
void cycleStep() {
  if (editTarget == &labV_set) {
    stepVset = (stepVset == STEP_0_01) ? STEP_0_1 : (stepVset == STEP_0_1) ? STEP_1 : STEP_0_01;
    currentStep = stepVset;
  } else {
    stepI = (stepI == STEP_0_01) ? STEP_0_1 : (stepI == STEP_0_1) ? STEP_1 : STEP_0_01;
    currentStep = stepI;
  }
}

// Move to next config item
void nextConfigItem() { currentConfigIndex = (currentConfigIndex + 1) % configCount; }

// Handle config encoder input
void handleConfigEncoder(int delta) {
  ConfigItem& item = configMenu[currentConfigIndex];
  switch (item.type) {
    case ConfigItem::Type::Bool:
      if (delta != 0) *(bool*)item.value = !*(bool*)item.value;
      break;
    case ConfigItem::Type::Enum:
      if (delta > 0 && *(int*)item.value < item.enumCount - 1) (*(int*)item.value)++;
      else if (delta < 0 && *(int*)item.value > 0) (*(int*)item.value)--;
      break;
  }
}

// Confirm config changes
void confirmConfig() { editingValue = false; }

// Initialize encoder and button
void begin() {
  pinMode(ENC_SW, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_CLK, INPUT_PULLUP);
  encoder.setPosition(0);
  encLastPos = encoder.getPosition();
  lastActivityTime = millis();
}

// Update encoder and button state
void update() {
  encoder.tick();
  unsigned long now = millis();
  int delta = encoder.getPosition() - encLastPos;
  encLastPos = encoder.getPosition();
  ButtonEvent btnEvt = pollButton();

  // Config menu handling
  if (currentState == ScreenState::ConfigIdle || currentState == ScreenState::ConfigEditing) {
    if (delta != 0) {
      if (currentState == ScreenState::ConfigIdle) {
        currentMenuIndex = (delta > 0) ? (currentMenuIndex + 1) % menuCount
                                       : (currentMenuIndex + menuCount - 1) % menuCount;
      } else {
        MenuItem& page = menuItems[currentMenuIndex];
        if (page.boolValue) handleDraftRotation(delta > 0 ? 1 : -1);
        else handleConfigEncoder(delta);
      }
      DisplayManager::drawMenuPage(currentMenuIndex);
      lastActivityTime = now;
    }

    if (btnEvt == ButtonEvent::Short) {
      MenuItem& page = menuItems[currentMenuIndex];
      if (currentState == ScreenState::ConfigIdle) {
        if (currentMenuIndex != 0) {
          currentState = ScreenState::ConfigEditing;
          editingValue = true;
          if (page.boolValue) syncDraftWithPage();
        }
        DisplayManager::drawMenuPage(currentMenuIndex);
      } else if (currentState == ScreenState::ConfigEditing) {
        if (page.setFunc && page.boolValue) confirmDraft();
        else confirmConfig();
        editingValue = false;
        currentState = ScreenState::ConfigIdle;
        DisplayManager::drawMenuPage(currentMenuIndex);
      }
      lastActivityTime = now;
    } else if (btnEvt == ButtonEvent::Long) {
      editingValue = false;
      currentState = ScreenState::MainIdle;
      DisplayManager::drawMenuPage(currentMenuIndex);
    }

    if (currentState == ScreenState::ConfigEditing && now - lastActivityTime > IDLE_TIMEOUT_MS) {
      MenuItem& page = menuItems[currentMenuIndex];
      if (page.setFunc && page.boolValue) confirmDraft();
      else confirmConfig();
      editingValue = false;
      currentState = ScreenState::ConfigIdle;
      DisplayManager::drawMenuPage(currentMenuIndex);
    }
    return;
  }

  // Main parameter handling
  if (delta != 0) {
    float step = (editTarget == &labV_set) ? stepVset : stepI;
    if (currentState == ScreenState::MainIdle) {
      beginEdit(editTarget);
      currentState = ScreenState::MainEditing;
      *editTarget += (delta > 0 ? step : -step);
    } else if (currentState == ScreenState::MainEditing) {
      if (now - lastEditUpdate > 50) {
        *editTarget += (delta > 0 ? step : -step);
        lastEditUpdate = now;
      }
    }
    if (editTarget) {
      *editTarget = constrain(*editTarget, params[currentParamIndex].minVal, params[currentParamIndex].maxVal);
    }
    lastActivityTime = now;
  }

  if (btnEvt == ButtonEvent::Short) {
    if (currentState == ScreenState::MainIdle) {
      currentParamIndex = (currentParamIndex + 1) % paramCount;
      editTarget = params[currentParamIndex].value;
    } else if (currentState == ScreenState::MainEditing) {
      cycleStep();
    }
    lastActivityTime = now;
  } else if (btnEvt == ButtonEvent::Long) {
    if (currentState == ScreenState::MainEditing) {
      cancelEdit();
      currentState = ScreenState::MainIdle;
    } else if (currentState == ScreenState::MainIdle) {
      cancelEdit();
      currentState = ScreenState::ConfigIdle;
      DisplayManager::drawMenuPage(currentMenuIndex);
    }
    lastActivityTime = now;
  }

  if (currentState == ScreenState::MainEditing && encoderAutoApply && now - lastActivityTime > IDLE_TIMEOUT_MS) {
    cancelEdit();
    currentState = ScreenState::MainIdle;
  }
}

// Accessor functions
const char* getActiveParamName() { return params[currentParamIndex].name; }
float getActiveParamValue() { return *params[currentParamIndex].value; }
float getCurrentStep() { return currentStep; }
float* getEditTarget() { return editTarget; }
bool isEditing() { return editingValue; }
ScreenState getScreenState() { return currentState; }
int getCurrentMenuIndex() { return currentMenuIndex; }
MenuItem* getMenuItems() { return menuItems; }
int getMenuCount() { return menuCount; }

const char* getEditType() {
  if (!editTarget) return "NONE";
  if (editTarget == &labV_set) return "VOLTAGE";
  if (editTarget == &labI_set || editTarget == &labI_cut) return "CURRENT";
  return "NONE";
}

} // namespace EncoderManager