#pragma once

#include <Arduino.h>
#include <U8g2lib.h> 

// === 7-segment font bits ===
enum SegmentBits {
    SEG_A = 1 << 0,
    SEG_B = 1 << 1,
    SEG_C = 1 << 2,
    SEG_D = 1 << 3,
    SEG_E = 1 << 4,
    SEG_F = 1 << 5,
    SEG_G = 1 << 6
};

// === Displayed character structure ===
struct SegChar {
    char c;        // character
    uint8_t mask;  // segment bitmask
};

// === Character table for 7-segment display ===
extern const SegChar segmentChars[];

// === Segment blinking ===
extern bool blinkState;       // current blink state
extern uint16_t blinkInterval; // blink interval in ms
void updateBlink();

// === Main drawing and formatting functions ===

// Draw a string on 7-segment style, with optional inversion
void drawSegmentStringSmart(const char* str, int x, int y, 
                            uint8_t h, uint8_t w, bool inv, U8G2& u8g2);

// Format a float value into string for display (default width = 3)
String formatSegmentValue(float val, int width = 3);

// Format a float value into string for display, optionally forcing full unit
String formatSegmentValue(float val, int width, bool forceFullUnit);


