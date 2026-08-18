#pragma once
#include <Arduino.h>
#include "Config.h"

// Tracks all buttons independently (each pin debounced on its own), so
// multiple buttons held at once are all reported correctly.
class ButtonPanel {
 public:
  void begin();
  void update();

  bool isPressed(int idx) const { return stable_[idx]; }
  bool changed(int idx)   const { return changedFlags_[idx]; }

 private:
  bool     stable_[cfg::NUM_BUTTONS]         = {};
  bool     lastRead_[cfg::NUM_BUTTONS]       = {};
  bool     changedFlags_[cfg::NUM_BUTTONS]   = {};
  uint32_t lastChangeTime_[cfg::NUM_BUTTONS] = {};
};
