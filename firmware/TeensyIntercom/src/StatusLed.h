#pragma once
//
// Non-blocking status LED.
//
// The original firmware blinked with delay(), which stalled the main loop
// for ~600 ms out of every 2 s and swallowed button presses. This drives the
// same blink patterns from millis() instead, so the loop never stops.
//

#include <Arduino.h>

class StatusLed {
 public:
  void begin(uint8_t pin);

  // count > 0 : blink count times, then pause, and repeat.
  // count == 0: steady 150 ms on/off flash (used for "not configured").
  void setPattern(uint8_t count);

  void update(uint32_t now);

 private:
  uint8_t  pin_      = 0;
  uint8_t  count_    = 255;  // force the first setPattern() to take effect
  uint8_t  emitted_  = 0;
  bool     on_       = false;
  bool     inPause_  = false;
  uint32_t lastEdge_ = 0;
};
