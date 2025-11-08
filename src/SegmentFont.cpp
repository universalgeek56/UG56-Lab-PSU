// SegmentFont.cpp (v11.7)
// Retro 7-segment font for OLED with embedded decimal point.
// -------------------------------------------------------------------------
// Usage:
//   drawSegmentStringSmart("12.3", x, y, h, w, invert, u8g2);
//   drawSegmentBlinkRange("36.00", x, y, h, w, invert, start, len, u8g2);
//   formatSegmentValue(val, 3);
//
// Features:
//   — Dot is attached to the previous character, like a real 7-segment display
//   — Colon ":" draws two dots centered (for clocks)
//   — Character width is constant; spacing depends on segment width

#include <U8g2lib.h>
#include <Arduino.h>
#include "SegmentFont.h"


// === 7-segment character table ===
const SegChar segmentChars[] = {
    {'0', SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F},
    {'1', SEG_B | SEG_C},
    {'2', SEG_A | SEG_B | SEG_G | SEG_E | SEG_D},
    {'3', SEG_A | SEG_B | SEG_C | SEG_D | SEG_G},
    {'4', SEG_F | SEG_G | SEG_B | SEG_C},
    {'5', SEG_A | SEG_F | SEG_G | SEG_C | SEG_D},
    {'6', SEG_A | SEG_F | SEG_G | SEG_E | SEG_C | SEG_D},
    {'7', SEG_A | SEG_B | SEG_C},
    {'8', SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G},
    {'9', SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G},
    {'A', SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G},
    {'b', SEG_C | SEG_D | SEG_E | SEG_F | SEG_G},
    {'C', SEG_A | SEG_D | SEG_E | SEG_F},
    {'c', SEG_G | SEG_E | SEG_D},
    {'d', SEG_B | SEG_C | SEG_D | SEG_E | SEG_G},
    {'E', SEG_A | SEG_D | SEG_E | SEG_F | SEG_G},
    {'F', SEG_A | SEG_E | SEG_F | SEG_G},
    {'L', SEG_D | SEG_E | SEG_F},
    {'P', SEG_A | SEG_B | SEG_E | SEG_F | SEG_G},
    {'U', SEG_B | SEG_C | SEG_D | SEG_E | SEG_F},
    {'r', SEG_E | SEG_G},
    {'n', SEG_C | SEG_E | SEG_G},
    {'o', SEG_C | SEG_D | SEG_E | SEG_G},
    {'-', SEG_G},
    {' ', 0},
    {'I', SEG_E | SEG_F},
};

// === Draw one segment by index ===
void drawSegment(uint8_t seg, int x, int y, uint8_t h, uint8_t w, bool inv, U8G2& u8g2) {
    switch(seg) {
        case 0: u8g2.drawBox(x + w, y, h, w); break;
        case 1: u8g2.drawBox(x + w + h, y + w, w, h); break;
        case 2: u8g2.drawBox(x + w + h, y + w + h + w, w, h); break;
        case 3: u8g2.drawBox(x + w, y + 2 * h + 2 * w, h, w); break;
        case 4: u8g2.drawBox(x, y + w + h + w, w, h); break;
        case 5: u8g2.drawBox(x, y + w, w, h); break;
        case 6: u8g2.drawBox(x + w, y + h + w, h, w); break;
    }
}

// === Draw colon ":" ===
void drawColon(int x, int y, uint8_t h, uint8_t w, bool inv, U8G2& u8g2) {
    int cy = y + h + w;
    int top = cy - w - 1;
    int bottom = cy + w + 1;
    u8g2.drawBox(x, top, w, w);
    u8g2.drawBox(x, bottom, w, w);
}

// === Draw one character with optional embedded dot ===
void drawCharSmart(char ch, bool withDot, int x, int y, uint8_t h, uint8_t w, bool inv, U8G2& u8g2) {
    char uc = (ch == 'c') ? 'c' : toupper(ch);
    for (const SegChar& sc : segmentChars) {
        if (sc.c == uc) {
            if (inv) u8g2.setDrawColor(0);
            for (uint8_t i = 0; i < 7; i++)
                if (sc.mask & (1 << i)) drawSegment(i, x, y, h, w, inv, u8g2);
            if (withDot) {
                int dotX = x + h + 2 * w + (w + 1);
                int dotY = y + 2 * h + 2 * w;
                u8g2.drawBox(dotX, dotY, w, w);
            }
            if (inv) u8g2.setDrawColor(1);
            return;
        }
    }
}

// === Draw string in 7-segment style, supporting embedded dots ===
void drawSegmentStringSmart(const char* str, int x, int y, uint8_t h, uint8_t w, bool inv, U8G2& u8g2) {
    const int charWidth = h + 2 * w;
    const int spacing = w + 1;

    while (*str) {
        if (*str == ':') {
            drawColon(x, y, h, w, inv, u8g2);
            x += charWidth + spacing;
            str++;
            continue;
        }

        bool hasDot = (*(str + 1) == '.');
        drawCharSmart(*str, hasDot, x, y, h, w, inv, u8g2);

        x += charWidth + spacing;
        if (hasDot) x += spacing, str++;
        str++;
    }
}

// === Draw string with blinking in specified character range ===
void drawSegmentBlinkRange(const char* str, int x, int y, uint8_t h, uint8_t w, bool inv,
                           uint8_t start, uint8_t len, U8G2& u8g2) {
    const int charWidth = h + 2 * w;
    const int spacing = w + 1;
    uint8_t i = 0;

    while (*str) {
        if (*str == ':') {
            if (!(i >= start && i < start + len && !blinkState))
                drawColon(x, y, h, w, inv, u8g2);
            x += charWidth + spacing;
            str++;
            i++;
            continue;
        }
        bool hasDot = (*(str + 1) == '.');
        bool inRange = (i >= start && i < start + len);
        bool show = !inRange || blinkState;
        if (show) drawCharSmart(*str, hasDot, x, y, h, w, inv, u8g2);

        x += charWidth + spacing;
        if (hasDot) x += spacing, str++;
        str++;
        i++;
    }
}

// === Format float value for segment display with dynamic decimal point ===
String formatSegmentValue(float val, int width, bool forceFullUnit = false) {
    if (width < 2 || width > 4) return "Err";

    const float TH_2DP_TO_1DP = 9.995f;
    const float TH_1DP_TO_0DP = 99.95f;
    const float TH_MILLI_TO_UNITS = 0.995f;

    char buf[16] = {0};

    if (forceFullUnit) {
        if (width == 3) {
            if (val >= TH_1DP_TO_0DP)
                snprintf(buf, sizeof(buf), "%3d", (int)roundf(val));
            else if (val >= TH_2DP_TO_1DP) {
                int t = (int)roundf(val * 10.0f);
                snprintf(buf, sizeof(buf), "%2d.%1d", t / 10, t % 10);
            } else {
                int h = (int)roundf(val * 100.0f);
                snprintf(buf, sizeof(buf), "%1d.%02d", h / 100, h % 100);
            }
            return String(buf);
        }
        if (width == 4) {
            if (val >= 999.5f) snprintf(buf, sizeof(buf), "%4d", (int)roundf(val));
            else if (val >= TH_1DP_TO_0DP) {
                int t = (int)roundf(val * 10.0f);
                if (t >= 10000) snprintf(buf, sizeof(buf), "%4d", t / 10);
                else snprintf(buf, sizeof(buf), "%3d.%1d", t / 10, t % 10);
            } else {
                int h = (int)roundf(val * 100.0f);
                if (h < 1000) snprintf(buf, sizeof(buf), " %1d.%02d", h / 100, h % 100);
                else snprintf(buf, sizeof(buf), "%2d.%02d", h / 100, h % 100);
            }
            return String(buf);
        }
        return "Err";
    }

    // Normal logic with milli units
    if (width == 3) {
        if (val >= TH_1DP_TO_0DP) snprintf(buf, sizeof(buf), "%3d", (int)roundf(val));
        else if (val >= TH_2DP_TO_1DP) {
            int t = (int)roundf(val * 10.0f);
            if (t >= 1000) snprintf(buf, sizeof(buf), "%3d", t / 10);
            else snprintf(buf, sizeof(buf), "%2d.%1d", t / 10, t % 10);
        } else if (val >= TH_MILLI_TO_UNITS) {
            int h = (int)roundf(val * 100.0f);
            snprintf(buf, sizeof(buf), "%1d.%02d", h / 100, h % 100);
        } else {
            int m = (int)roundf(val * 1000.0f);
            snprintf(buf, sizeof(buf), "%3d", m);
        }
        return String(buf);
    }

    if (width == 4) {
        if (val >= 999.5f) snprintf(buf, sizeof(buf), "%4d", (int)roundf(val));
        else if (val >= TH_1DP_TO_0DP) {
            int t = (int)roundf(val * 10.0f);
            if (t >= 10000) snprintf(buf, sizeof(buf), "%4d", t / 10);
            else snprintf(buf, sizeof(buf), "%3d.%1d", t / 10, t % 10);
        } else if (val >= TH_2DP_TO_1DP) {
            int h = (int)roundf(val * 100.0f);
            snprintf(buf, sizeof(buf), "%2d.%02d", h / 100, h % 100);
        } else if (val >= TH_MILLI_TO_UNITS) {
            int h = (int)roundf(val * 100.0f);
            snprintf(buf, sizeof(buf), " %1d.%02d", h / 100, h % 100);
        } else {
            int m = (int)roundf(val * 1000.0f);
            snprintf(buf, sizeof(buf), "%4d", m);
        }
        return String(buf);
    }

    return "Err";
}
