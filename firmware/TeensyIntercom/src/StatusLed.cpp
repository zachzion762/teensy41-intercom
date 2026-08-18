#include "StatusLed.h"

namespace {
const uint16_t kOnMs       = 80;
const uint16_t kOffMs      = 120;
const uint16_t kPauseMs    = 700;
const uint16_t kSteadyMs   = 150;
}  // namespace

void StatusLed::begin(uint8_t pin) {
  pin_ = pin;
  pinMode(pin_, OUTPUT);
  digitalWrite(pin_, LOW);
}

void StatusLed::setPattern(uint8_t count) {
  if (count == count_) return;
  count_    = count;
  emitted_  = 0;
  inPause_  = false;
  on_       = false;
  lastEdge_ = millis();
  digitalWrite(pin_, LOW);
}

void StatusLed::update(uint32_t now) {
  if (count_ == 0) {
    if (now - lastEdge_ >= kSteadyMs) {
      lastEdge_ = now;
      on_       = !on_;
      digitalWrite(pin_, on_ ? HIGH : LOW);
    }
    return;
  }

  if (inPause_) {
    if (now - lastEdge_ >= kPauseMs) {
      inPause_  = false;
      emitted_  = 0;
      lastEdge_ = now;
      on_       = true;
      digitalWrite(pin_, HIGH);
    }
    return;
  }

  const uint16_t interval = on_ ? kOnMs : kOffMs;
  if (now - lastEdge_ < interval) return;

  lastEdge_ = now;
  if (on_) {
    on_ = false;
    digitalWrite(pin_, LOW);
    if (++emitted_ >= count_) inPause_ = true;
  } else {
    on_ = true;
    digitalWrite(pin_, HIGH);
  }
}
