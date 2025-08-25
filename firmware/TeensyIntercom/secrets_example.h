#pragma once
#include <IPAddress.h>

// Broker address (pick one)
#define MQTT_HOST_IP   IPAddress(192,168,1,10)
// or:
// #define MQTT_HOST_NAME "homeassistant.local"

#define MQTT_PORT 1883
#define MQTT_USER "CHANGE_ME"
#define MQTT_PASS "CHANGE_ME"

// Unique suffix for HA unique_id/device id (anything unique, not secret)
#define DEVICE_UNIQ_SUFFIX "ABC123"

// Network (DHCP by default). For static, uncomment and set:
// #define STATIC_NET
// static const IPAddress T_IP  (192,168,1,231);
// static const IPAddress T_MASK(255,255,255,0);
// static const IPAddress T_GW  (192,168,1,1);
// static const IPAddress T_DNS (192,168,1,1);

// Optional fixed MAC (normally not needed)
// #define USE_FIXED_MAC
// static const uint8_t FIXED_MAC[6] = {0x02,0x12,0x34,0x56,0x78,0x9A};
