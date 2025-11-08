#pragma once

#include <U8g2lib.h>
#include "Globals.h"
#include "SegmentFont.h"
#include "Config.h"
#include "EncoderManager.h"

namespace DisplayManager {

// Display object
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C display;

// Smoothed display values
extern float smoothV;         // Smoothed voltage
extern float smoothI;         // Smoothed current
extern float smoothP;         // Smoothed power
extern const float alphaDisplay; // Smoothing factor

// Idle timeout
extern const unsigned long IDLE_TIMEOUT_MS;

// Display coordinates and sizes
// Main screen
constexpr int MAIN_VAL_X = 54;   // Main value X position
constexpr int MAIN_VAL_Y = 16;   // Main value Y position
constexpr int MAIN_H = 14;       // Main value height
constexpr int MAIN_W = 2;        // Main value width
constexpr int MAIN_UNIT_X = 116; // Main unit X position
constexpr int MAIN_UNIT_Y = 50;  // Main unit Y position

// Left values (current, power)
constexpr int LEFT_H = 7;        // Left value height
constexpr int LEFT_W = 1;        // Left value width
constexpr int LEFT_VAL_X = 0;    // Left value X position
constexpr int LEFT_UNIT_X = 36;  // Left unit X position
constexpr int TOP_Y = 16;        // Top value Y position
constexpr int BOTTOM_Y = 36;     // Bottom value Y position

// Footer
constexpr int FOOT_CELL1_X = 0;       // First footer cell X (Vset/active param)
constexpr int FOOT_CELL2_X = 70;      // Second footer cell X (IL/IF/step)
constexpr int FOOT_VAL_Y = 55;        // Footer value Y position
constexpr int FOOT_UNIT_Y = 64;       // Footer unit Y position
constexpr int FOOT_LABEL_OFFSET = 0;   // Label offset
constexpr int FOOT_VALUE_OFFSET = 30;  // Value offset from label

// Small segments
constexpr int SMALL_H = 3; // Small segment height
constexpr int SMALL_W = 1; // Small segment width

// Value widths
constexpr int MAIN_VAL_WIDTH = 3;      // Main value width
constexpr int LEFT_VAL_WIDTH = 3;      // Left value width
constexpr int FOOT_UNIT_OFFSET_CELL1 = 24; // Unit offset for first cell
constexpr int FOOT_UNIT_OFFSET_CELL2 = 17; // Unit offset for second cell
constexpr int FOOT_VAL_WIDTH_CELL1 = 4;    // First cell value width (Vset, Ilim, Ifus)
constexpr int FOOT_VAL_WIDTH_CELL2 = 3;    // Second cell value width (steps, currents)

// Function declarations
void begin();                      // Initialize display
void debugPrint(const char* message, int line);
void updateMainScreen();           // Update main screen
void drawHeader();                 // Draw header with status
void drawValueWithUnit(float val, int xVal, int yVal, int h, int w,
                       const char* type, int xUnit, int yUnit, int width,
                       bool forceFullUnit); // Draw value with unit
void drawMenuPage(int pageIndex);  // Draw menu page
void drawFooterStandard(float Vset, float Iset, float Icut); // Draw standard footer
void drawFooterActiveParam(float val, const char* name, float step, const char* type); // Draw active parameter footer
inline void drawFooterActiveParam(float val, const char* name, const char* type) {
  drawFooterActiveParam(val, name, 0, type); // Simplified footer for boolean params
}
void update();                     // Main display update
String getActiveErrorString();     // Get cycling active error string
String decodeErrorString();        // Decode error string for display

} // namespace DisplayManager