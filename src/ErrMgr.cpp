#include "ErrMgr.h"
#include "Globals.h"
#include <Arduino.h>

namespace ErrMgr {

static unsigned long lastUpdate = 0;
uint32_t code = 0;

// Initialize error manager
void begin() {
  lastUpdate = millis();
}

void clear() {
  code = 0;       
  errorCode = 0;  
}

// Update error flags and code
void update() {
  unsigned long now = millis();
  if (now - lastUpdate < DC_CONTROL_UPDATE_INTERVAL) return;
  lastUpdate = now;

  static bool lastManualEnable = false;
  if (manualOutputEnable && !lastManualEnable) {
    clear();
  }
  lastManualEnable = manualOutputEnable;

  for (uint8_t i = 0; i < MAX_ERRORS; i++) {
    if (errorFlags[i] && *errorFlags[i]) {
      code |= (1UL << i);
    }
  }
  errorCode = code;
}

}  // namespace ErrMgr
