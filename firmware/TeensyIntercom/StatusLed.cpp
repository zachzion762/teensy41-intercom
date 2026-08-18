#include "StatusLed.h"
#include "Config.h"

static constexpr uint16_t ON_MS  = 80;
static constexpr uint16_t OFF_MS = 120;

void StatusLed::begin() {
  pinMode(cfg::LED_PIN, OUTPUT);
  digitalWrite(cfg::LED_PIN, LOW);
  // Backdate cycleStart_ so the first update() fires a blink cycle immediately
  // instead of waiting out a full STATUS_BLINK_INTERVAL_MS after boot.
  cycleStart_ = millis() - cfg::STATUS_BLINK_INTERVAL_MS;
}

void StatusLed::update(int pattern) {
  pattern_       = pattern;
  uint32_t now   = millis();

  if (phase_ == Phase::Idle) {
    if (now - cycleStart_ < cfg::STATUS_BLINK_INTERVAL_MS) return;
    cycleStart_    = now;
    blinksLeft_    = pattern_;
    phase_         = Phase::LedOn;
    lastEventTime_ = now;
    digitalWrite(cfg::LED_PIN, HIGH);
    return;
  }

  if (phase_ == Phase::LedOn) {
    if (now - lastEventTime_ < ON_MS) return;
    digitalWrite(cfg::LED_PIN, LOW);
    lastEventTime_ = now;
    phase_         = Phase::LedOff;
    return;
  }

  // Phase::LedOff
  if (now - lastEventTime_ < OFF_MS) return;
  blinksLeft_--;
  if (blinksLeft_ > 0) {
    digitalWrite(cfg::LED_PIN, HIGH);
    lastEventTime_ = now;
    phase_         = Phase::LedOn;
  } else {
    phase_ = Phase::Idle;
  }
}
