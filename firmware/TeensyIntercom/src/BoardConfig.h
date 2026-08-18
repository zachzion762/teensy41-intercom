#pragma once
//
// Compile-time board definition: pin map, LED, and timing constants.
// Everything a user normally needs to change lives in runtime config
// (see IntercomConfig.h) -- this file only describes the hardware.
//

#include <Arduino.h>

#define INTERCOM_FW_VERSION "2.0.0"

// Where users can obtain the complete corresponding source. Surfaced on the
// web page to satisfy AGPL-3.0 section 13 (QNEthernet is AGPL-licensed).
#define INTERCOM_SOURCE_URL "https://github.com/zachzion762/teensy41-intercom"

// Button pins. Each button connects its pin to GND; pins use INPUT_PULLUP,
// so a press reads LOW. Change this list (and kButtonCount follows) if you
// build a panel with a different number of buttons.
static const uint8_t kButtonPins[] = {1, 3, 5, 7, 9, 10, 12, 24, 26, 28, 30, 32};
static const uint8_t kButtonCount  = sizeof(kButtonPins) / sizeof(kButtonPins[0]);

static const uint8_t  kStatusLedPin = 13;

static const uint32_t kDebounceMs        = 20;
static const uint32_t kRediscoverMs      = 300000;  // re-announce HA discovery every 5 min
static const uint32_t kMqttRetryMinMs    = 1000;    // reconnect backoff floor
static const uint32_t kMqttRetryMaxMs    = 15000;   // reconnect backoff ceiling
static const uint32_t kLinkWaitMs        = 8000;    // how long to wait for link at boot
static const uint16_t kMqttBufferSize    = 1024;    // MUST exceed the largest discovery payload
static const uint16_t kMqttKeepAliveSec  = 20;
static const uint16_t kWebPort           = 80;
