#pragma once
#include <Watchdog_t4.h>

// Hardware watchdog (Teensy 4.x WDT1). If loop() ever stalls long enough
// that feed() stops being called, the chip resets itself instead of
// needing a field power-cycle.
class WatchdogGuard {
 public:
  void begin();
  void feed() { wdt_.feed(); }

 private:
  WDT_T4<WDT1> wdt_;
};
