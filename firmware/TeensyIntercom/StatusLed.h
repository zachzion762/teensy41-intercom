#pragma once
#include <Arduino.h>

// Blinks the status LED N times per cycle without ever calling delay(),
// so the main loop keeps reading buttons and servicing MQTT while it blinks.
//   1 blink  = no Ethernet link
//   2 blinks = link up, MQTT not connected
//   3 blinks = link up, MQTT connected (all good)
class StatusLed {
 public:
  void begin();
  void update(int pattern);

 private:
  enum class Phase { Idle, LedOn, LedOff };

  Phase    phase_        = Phase::Idle;
  int      pattern_      = 1;
  int      blinksLeft_   = 0;
  uint32_t lastEventTime_ = 0;
  uint32_t cycleStart_   = 0;
};
