#include "ButtonPanel.h"

void ButtonPanel::begin() {
  for (int i = 0; i < cfg::NUM_BUTTONS; i++) {
    pinMode(cfg::BUTTON_PINS[i], INPUT_PULLUP);
  }
}

void ButtonPanel::update() {
  uint32_t now = millis();
  for (int i = 0; i < cfg::NUM_BUTTONS; i++) {
    bool raw = (digitalRead(cfg::BUTTON_PINS[i]) == LOW);
    changedFlags_[i] = false;

    if (raw != lastRead_[i]) {
      lastRead_[i]        = raw;
      lastChangeTime_[i]  = now;
    }

    if ((now - lastChangeTime_[i]) >= cfg::DEBOUNCE_MS && raw != stable_[i]) {
      stable_[i]       = raw;
      changedFlags_[i] = true;
    }
  }
}
