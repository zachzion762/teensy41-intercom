#pragma once
#include <Arduino.h>

// Central place for pin assignments, MQTT topics, and timing constants.
//
// IMPORTANT: BUTTON_PINS must match the wiring already deployed in the field.
// Do not reorder/renumber without rewiring the panel.
namespace cfg {

static constexpr int BUTTON_PINS[] = {1, 3, 5, 7, 9, 10, 12, 24, 26, 28, 30, 32};
static constexpr int NUM_BUTTONS = sizeof(BUTTON_PINS) / sizeof(BUTTON_PINS[0]);

static constexpr int LED_PIN = 13;

static constexpr uint32_t DEBOUNCE_MS = 20;
static constexpr uint32_t REDISCOVER_MS = 300000;           // 5 min
static constexpr uint32_t STATUS_BLINK_INTERVAL_MS = 2000;
static constexpr uint32_t MQTT_RECONNECT_INTERVAL_MS = 700;
static constexpr uint16_t MQTT_KEEPALIVE_S = 20;
static constexpr uint32_t LINK_WAIT_TIMEOUT_MS = 8000;
static constexpr uint8_t  WATCHDOG_TIMEOUT_S = 8;

// Topics / names (safe to keep public).
static constexpr const char* BASE_TOPIC  = "intercom";
static constexpr const char* AVAIL_TOPIC = "intercom/availability";
static constexpr const char* DISC_BASE   = "homeassistant/binary_sensor";
static constexpr const char* DEVICE_NAME = "teensy-intercom";

}  // namespace cfg
