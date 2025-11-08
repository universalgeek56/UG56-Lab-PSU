#include "Config.h"
#include "DisplayManager.h"
#include "SegmentFont.h"
#include "Globals.h"
#include "EncoderManager.h"
#include "ErrMgr.h"

namespace DisplayManager {

// Display object
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, SCL_PIN, SDA_PIN, U8X8_PIN_NONE);

// Smoothed display values
float smoothV = labV_meas; // Smoothed voltage
float smoothI = labI_meas; // Smoothed current
float smoothP = labQ_meas; // Smoothed power
const float alphaDisplay = 0.4f; // Smoothing factor

// Format unit for display (V, mV, A, mA, W, mW)
const char* formatUnit(float val, const char* type, bool forceFullUnit = false) {
  if (forceFullUnit) {
    if (strcmp(type, "VOLTAGE") == 0) return " V";
    if (strcmp(type, "CURRENT") == 0) return " A";
    if (strcmp(type, "POWER") == 0) return " W";
    return "";
  }
  const float TH_MILLI_TO_UNITS = 0.995f;
  bool highUnit = (val >= TH_MILLI_TO_UNITS);
  if (strcmp(type, "VOLTAGE") == 0) return highUnit ? " V" : "mV";
  if (strcmp(type, "CURRENT") == 0) return highUnit ? " A" : "mA";
  if (strcmp(type, "POWER") == 0) return highUnit ? " W" : "mW";
  return "";
}

// Error table
struct ErrorEntry {
  uint32_t mask;
  const char* text;
};

static const ErrorEntry errorTable[] = {
  { 1UL << 0, "Overheat" },           // tempFaultActive
  { 1UL << 1, "Overcurrent" },        // errorOverCurrent
  { 1UL << 2, "Fuse Blown" },         // errorFuseBlown
  { 1UL << 3, "Sensor Fail" },        // errorSensorFail
  { 1UL << 4, "INA226 Init Fail" },   // errorInaInitFail
  { 1UL << 5, "WiFi Init Fail" },     // errorWifiInitFail
  { 1UL << 6, "SSD1306 Init Fail" },  // errorSsd1306InitFail
  { 1UL << 7, "PWM Init Fail" },      // errorPwmInitFail
  { 1UL << 8, "Vout Over Limit" },    // errorVoutOverLimit
  { 1UL << 9, "Over Power" },         // errorOverPower
  { 1UL << 10, "Voltage Deviation" }, // errorVoltageDev
  { 1UL << 11, "Current Deviation" },// errorCurrentOverLimit
  { 1UL << 12, "Power Over Limit" },  // errorPowerOverLimit
  { 1UL << 13, "LEDC Init Fail" },    // errorLedcInitFail
  { 1UL << 14, "PID Divergence" },    // errorPidDivergence
  { 1UL << 15, "Low Memory" },        // errorLowMemory
  { 1UL << 16, "High CPU Temp" },     // errorHighCpuTemp
  { 1UL << 17, "Current PID Div" }    // errorPidCurrentDivergence
};

const int errorCount = sizeof(errorTable) / sizeof(errorTable[0]);

// Get cycling active error string
String getActiveErrorString() {
  static uint32_t lastUpdate = 0;
  static int currentIndex = 0;
  int activeErrors[errorCount];
  int activeCount = 0;

  for (int i = 0; i < errorCount; i++) {
    if (errorCode & errorTable[i].mask) {
      activeErrors[activeCount++] = i;
    }
  }

  if (activeCount == 0) {
    currentIndex = 0;
    lastUpdate = millis();
    return "No Errors";
  }

  if (millis() - lastUpdate >= 2000) {
    lastUpdate = millis();
    currentIndex = (currentIndex + 1) % activeCount;
  }

  return String(errorTable[activeErrors[currentIndex]].text);
}

void begin() {
  display.begin();
  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tf);
  display.drawStr(0, 12, "Booting...");
  display.sendBuffer();
  delay(1000);
}

// Debug print to display
void debugPrint(const char* message, int line = 1) {
  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tf);
  display.drawStr(0, 12 * line, message);
  display.sendBuffer();
  delay(2000); // Allow time to read
}

// Smooth display values
void updateDisplaySmoothing() {
  smoothV = alphaDisplay * labV_meas + (1.0f - alphaDisplay) * smoothV;
  smoothI = alphaDisplay * labI_meas + (1.0f - alphaDisplay) * smoothI;
  smoothP = alphaDisplay * labQ_meas + (1.0f - alphaDisplay) * smoothP;
}

// Draw header with parameter, error, and WiFi status
void drawHeader() {
  display.setFont(u8g2_font_5x8_tf);
  int y = 12;
  int x = 6;

  // Active parameter
  const char* param = EncoderManager::getActiveParamName();
  const char* headerParam = "----";
  if (strcmp(param, "Vset") == 0) headerParam = "VSET ";
  else if (strcmp(param, "Iset") == 0) headerParam = "ISET ";
  else if (strcmp(param, "Icut") == 0) headerParam = "ICUT ";
  int paramWidth = display.getStrWidth(headerParam);
  display.drawFrame(0, 2, paramWidth + 6, 14);
  display.drawStr(x, y, headerParam);
  x += paramWidth + 12;

  // Error status
  if (errorCode != 0 && (millis() / 500) % 2) {
    display.drawStr(x, y, "ERROR");
    x += display.getStrWidth("ERROR") + 10;
  } else {
    display.drawStr(x, y, "   ");
    x += display.getStrWidth("ERROR") + 10;
  }

  // WiFi status
  char wifiStr[16];
  if (!wifiEnabled) strcpy(wifiStr, "WIFI: OFF");
  else if (apMode) strcpy(wifiStr, "WIFI: AP");
  else if (!wifiConnected) strcpy(wifiStr, "WIFI: ...");
  else snprintf(wifiStr, sizeof(wifiStr), "WIFI:%ddb", wifiRSSI);
  display.drawStr(x, y, wifiStr);
}

// Draw value with unit
void drawValueWithUnit(float val, int xVal, int yVal, int h, int w,
                       const char* type, int xUnit, int yUnit, int width,
                       bool forceFullUnit) {
  String formatted = formatSegmentValue(val, width, forceFullUnit);
  drawSegmentStringSmart(formatted.c_str(), xVal, yVal, h, w, false, display);
  display.setFont(u8g2_font_6x12_tf);
  display.drawStr(xUnit, yUnit, formatUnit(val, type, forceFullUnit));
}

// Draw footer parameter
void drawFooterParam(const char* label, float value, const char* type,
                     int cellX, int valWidth,
                     bool blink = false, bool showUnit = true) {
  if (!blink || (millis() / 500) % 2) {
    display.setFont(u8g2_font_6x12_tf);
    display.drawStr(cellX + FOOT_LABEL_OFFSET, FOOT_UNIT_Y, label);
  }

  if (strcmp(type, "NONE") != 0 && showUnit) {
    int unitOffset = (valWidth == FOOT_VAL_WIDTH_CELL1) ? FOOT_UNIT_OFFSET_CELL1 : FOOT_UNIT_OFFSET_CELL2;
    drawValueWithUnit(value, cellX + FOOT_VALUE_OFFSET, FOOT_VAL_Y,
                      SMALL_H, SMALL_W, type,
                      cellX + FOOT_VALUE_OFFSET + unitOffset, FOOT_UNIT_Y,
                      valWidth, true);
  } else if (strcmp(type, "NONE") != 0) {
    drawSegmentStringSmart(formatSegmentValue(value, valWidth, true).c_str(),
                          cellX + FOOT_VALUE_OFFSET, FOOT_VAL_Y,
                          SMALL_H, SMALL_W, false, display);
  }
}

// Draw standard footer
void drawFooterStandard(float Vset, float Iset, float Icut) {
  drawFooterParam("Vset:", Vset, "VOLTAGE", FOOT_CELL1_X, FOOT_VAL_WIDTH_CELL1);
  if (Icut > Iset) {
    drawFooterParam("Iset:", Iset, "CURRENT", FOOT_CELL2_X, FOOT_VAL_WIDTH_CELL2);
  } else {
    drawFooterParam("Icut:", Icut, "CURRENT", FOOT_CELL2_X, FOOT_VAL_WIDTH_CELL2);
  }
}

// Draw footer for active parameter editing
void drawFooterActiveParam(float val, const char* name, float step, const char* type) {
  bool isBool = (strcmp(type, "NONE") == 0);
  char label[6];
  if (strcmp(name, "Vset") == 0) snprintf(label, sizeof(label), "Vset:");
  else if (strcmp(name, "Iset") == 0) snprintf(label, sizeof(label), "Iset:");
  else if (strcmp(name, "Icut") == 0) snprintf(label, sizeof(label), "Icut:");
  else snprintf(label, sizeof(label), "%s:", name);

  drawFooterParam(label, val, type, FOOT_CELL1_X, FOOT_VAL_WIDTH_CELL1, true);
  if (!isBool) {
    drawFooterParam("Step:", step, type, FOOT_CELL2_X, FOOT_VAL_WIDTH_CELL2, false, true);
  } else {
    drawFooterParam("", val, type, FOOT_CELL1_X, FOOT_VAL_WIDTH_CELL1, false, false);
  }
}

// Update main screen
void updateMainScreen() {
  updateDisplaySmoothing();
  display.clearBuffer();
  drawHeader();

  if (::mainScreenVoltage) {
    drawValueWithUnit(smoothV, MAIN_VAL_X, MAIN_VAL_Y, MAIN_H, MAIN_W,
                      "VOLTAGE", MAIN_UNIT_X, MAIN_UNIT_Y, MAIN_VAL_WIDTH, false);
    drawValueWithUnit(smoothI, LEFT_VAL_X, TOP_Y, LEFT_H, LEFT_W,
                      "CURRENT", LEFT_UNIT_X, TOP_Y + 17, LEFT_VAL_WIDTH, false);
  } else {
    drawValueWithUnit(smoothI, MAIN_VAL_X, MAIN_VAL_Y, MAIN_H, MAIN_W,
                      "CURRENT", MAIN_UNIT_X, MAIN_UNIT_Y, MAIN_VAL_WIDTH, false);
    drawValueWithUnit(smoothV, LEFT_VAL_X, TOP_Y, LEFT_H, LEFT_W,
                      "VOLTAGE", LEFT_UNIT_X, TOP_Y + 17, LEFT_VAL_WIDTH, false);
  }

  drawValueWithUnit(smoothP, LEFT_VAL_X, BOTTOM_Y, LEFT_H, LEFT_W,
                    "POWER", LEFT_UNIT_X, BOTTOM_Y + 17, LEFT_VAL_WIDTH, false);

  if (EncoderManager::getScreenState() == EncoderManager::ScreenState::MainEditing) {
    float step = EncoderManager::getCurrentStep();
    const char* type = EncoderManager::getEditType();
    drawFooterActiveParam(*EncoderManager::getEditTarget(),
                          EncoderManager::getActiveParamName(), step, type);
  } else {
    drawFooterStandard(labV_set, labI_set, labI_cut);
  }

  display.sendBuffer();
}

// Draw button with inversion and cursor
void drawButton(int x, int y, int w, int h, const char* text,
                bool active, bool draft = false, bool editing = false) {
  display.setFont(u8g2_font_6x12_tf);
  if (active) {
    display.setDrawColor(1);
    display.drawBox(x, y, w, h); // Active fill
    display.setDrawColor(0);     // Text black
  } else {
    display.setDrawColor(0);
    display.drawBox(x, y, w, h); // Inactive fill
    display.setDrawColor(1);     // Text white
  }

  // Center text
  int tw = display.getStrWidth(text);
  int th = 12;
  int textX = x + (w - tw) / 2;
  int textY = y + (h + th) / 2 - 3;
  display.drawStr(textX, textY, text);

  // Cursor for draft and editing
  if (draft && editing && (millis() / 500) % 2) {
    display.setDrawColor(1);
    display.drawFrame(x - 2, y - 2, w + 4, h + 4);
  }
  display.setDrawColor(1); // Reset color
}

// Count active errors
int activeErrorCount() {
  int count = 0;
  for (int i = 0; i < errorCount; i++) {
    if (errorFlags[i]) count++;
  }
  return count;
}

// Draw menu page
void drawMenuPage(int CurrentMenuIndex) {
  int menuCount = EncoderManager::getMenuCount();
  if (CurrentMenuIndex < 0 || CurrentMenuIndex >= menuCount) return;

  auto& page = EncoderManager::getMenuItems()[CurrentMenuIndex];
  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tf);

  // Header
  display.drawStr(2, 12, "Config");
  if (page.label) {
    display.drawStr(128 - display.getStrWidth(page.label) - 2, 12, page.label);
  }

  const int baseY = 24;
  const int lineHeight = 12;

  // Errors on first page
  if (CurrentMenuIndex == 0) {
    display.drawStr(2, baseY + lineHeight * 0, getActiveErrorString().c_str());
    char buf[24];
    snprintf(buf, sizeof(buf), "NTC Temp: %.1f C", labTemp_ntc);
    display.drawStr(2, baseY + lineHeight * 1 + 4, buf);
  } else {
    if (page.line1) display.drawStr(2, baseY + lineHeight * 0, page.line1().c_str());
    if (page.line2) display.drawStr(2, baseY + lineHeight * 1 + 4, page.line2().c_str());

    int halfWidth = 128 / 2 - 2;
    int yBtn = FOOT_UNIT_Y - 14;
    bool activeLeft = page.boolValue ? page.boolValue() : false;
    bool activeRight = !activeLeft;
    int draftIndex = EncoderManager::getDraftIndex();
    bool editing = EncoderManager::isEditing();

    drawButton(2, yBtn, halfWidth - 4, 12, page.label1, activeLeft, (draftIndex == 0), editing);
    drawButton(halfWidth + 2, yBtn, halfWidth - 4, 12, page.label2, activeRight, (draftIndex == 1), editing);
  }

  display.sendBuffer();
}

// Display update handler
void update() {
  unsigned long now = millis();
  auto state = EncoderManager::getScreenState();

  switch (state) {
    case EncoderManager::ScreenState::MainIdle:
    case EncoderManager::ScreenState::MainEditing:
      if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
        lastDisplayUpdate = now;
        updateDisplaySmoothing();
        updateMainScreen();
      }
      break;
    case EncoderManager::ScreenState::ConfigIdle:
    case EncoderManager::ScreenState::ConfigEditing:
      drawMenuPage(EncoderManager::getCurrentMenuIndex());
      break;
  }
}

} // namespace DisplayManager