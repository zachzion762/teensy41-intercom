#include "Watchdog.h"
#include "Config.h"

void WatchdogGuard::begin() {
  WDT_timings_t config  = {};
  config.trigger        = cfg::WATCHDOG_TIMEOUT_S - 2;  // early warning, seconds
  config.timeout        = cfg::WATCHDOG_TIMEOUT_S;      // hard reset, seconds
  wdt_.begin(config);
}
