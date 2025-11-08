#pragma once
#include <Arduino.h>

extern const char index_html[] PROGMEM;
extern const char charts_html[] PROGMEM;
extern const char settings_html[] PROGMEM;
extern const char system_html[] PROGMEM;

namespace WebInterface {
  void begin();
  void update();
}