#pragma once
//
// LEGACY -- this file is no longer needed.
//
// Firmware 2.x stores the broker address, credentials, and network settings in
// EEPROM. Configure a board over USB serial ('wizard') or its built-in web
// page instead. See docs/quickstart.md.
//
// It is kept only for upgrades: if you build from source and a secrets.h is
// still present next to the sketch, these values seed the defaults on first
// boot, so an existing board keeps its Home Assistant entities. Once the
// config has been saved to EEPROM, the file can be deleted.
//
#include <IPAddress.h>

// Broker address (pick one)
#define MQTT_HOST_IP   IPAddress(192,168,1,10)
// or:
// #define MQTT_HOST_NAME "homeassistant.local"

#define MQTT_PORT 1883
#define MQTT_USER "CHANGE_ME"
#define MQTT_PASS "CHANGE_ME"

// Unique suffix for the HA unique_id / device id (anything unique, not secret).
// Keep your existing value here to preserve entities across the 1.x -> 2.x upgrade.
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
