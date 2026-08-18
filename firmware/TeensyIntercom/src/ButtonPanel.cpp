#include "ButtonPanel.h"

void ButtonPanel::begin(const uint8_t* pins, uint8_t count, uint32_t debounceMs) {
  pins_       = pins;
  count_      = count;
  debounceMs_ = debounceMs;
  for (uint8_t i = 0; i < count_; i++) pinMode(pins_[i], INPUT_PULLUP);
}

int8_t ButtonPanel::scan() const {
  for (uint8_t i = 0; i < count_; i++) {
    if (digitalRead(pins_[i]) == LOW) return (int8_t)i;
  }
  return -1;
}

bool ButtonPanel::update(uint32_t now) {
  const int8_t cur = scan();
  if (cur != lastRead_) {
    lastRead_   = cur;
    lastChange_ = now;
  }

  if ((now - lastChange_) < debounceMs_ || cur == stable_) return false;

  previous_ = stable_;
  stable_   = cur;
  return true;
}
