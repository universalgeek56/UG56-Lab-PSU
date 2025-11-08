#pragma once

// DC control module for voltage and current regulation
namespace DcControl {

  void begin();  // Initialize DC control
  void update(); // Update DC control loop

}