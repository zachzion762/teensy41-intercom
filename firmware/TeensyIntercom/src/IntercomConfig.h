#pragma once
//
// Runtime configuration stored in Teensy EEPROM.
//
// Nothing here is baked into the binary, which is what lets a single
// prebuilt .hex work on anyone's network: flash it, then set the broker
// details over USB serial or the built-in web page.
//

#include <Arduino.h>
#include <IPAddress.h>

static const uint32_t kConfigMagic   = 0x54494331UL;  // "TIC1"
static const uint16_t kConfigVersion = 1;

struct IntercomConfig {
  uint32_t magic;
  uint16_t version;
  uint16_t reserved;

  // MQTT broker
  char     mqttHost[64];   // IP address or hostname
  uint16_t mqttPort;
  char     mqttUser[33];
  char     mqttPass[65];

  // Identity / topics
  char     deviceId[33];         // unique suffix for HA unique_id (auto-derived from MAC if empty)
  char     deviceName[33];       // friendly name shown in Home Assistant
  char     baseTopic[33];        // e.g. "intercom"
  char     discoveryPrefix[33];  // e.g. "homeassistant"

  // Network
  uint8_t  useDhcp;
  uint8_t  ip[4];
  uint8_t  mask[4];
  uint8_t  gw[4];
  uint8_t  dns[4];
  uint8_t  useFixedMac;
  uint8_t  mac[6];

  // Services
  uint8_t  webEnabled;

  uint32_t crc;
};

namespace config {

// Fills cfg with built-in defaults (seeded from a legacy secrets.h when present).
void setDefaults(IntercomConfig& cfg);

// Loads from EEPROM. Returns false (and applies defaults) when no valid
// config is stored -- first boot, a failed CRC, or a version bump.
bool load(IntercomConfig& cfg);

bool save(const IntercomConfig& cfg);

// Wipes the stored config so the next boot starts from defaults.
void erase();

// True once there is enough information to actually reach a broker.
bool isConfigured(const IntercomConfig& cfg);

// Derives deviceId from the interface MAC when the user has not set one.
void ensureDeviceId(IntercomConfig& cfg, const uint8_t mac[6]);

// Applies a single "key = value" setting. Shared by the serial console and
// the web form so both accept exactly the same keys. Returns false and fills
// err on an unknown key or an unparseable value.
bool setField(IntercomConfig& cfg, const char* key, const char* value,
              char* err, size_t errLen);

// Reads a single field back as text (used to render the web form).
// Passwords come back as an empty string unless showSecrets is true.
void getField(const IntercomConfig& cfg, const char* key, char* out,
              size_t outLen, bool showSecrets);

// Human-readable dump for the serial console.
void print(Print& out, const IntercomConfig& cfg, bool showSecrets);

IPAddress toIP(const uint8_t octets[4]);

}  // namespace config
