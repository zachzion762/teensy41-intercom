#include "IntercomConfig.h"

#include <EEPROM.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "BoardConfig.h"

// Optional backwards compatibility: if an old-style secrets.h is still
// sitting in the sketch folder, its values seed the defaults on first boot.
// New installs do not need this file at all.
#if defined(__has_include)
#  if __has_include("../secrets.h")
#    include "../secrets.h"
#    define INTERCOM_HAVE_LEGACY_SECRETS 1
#  endif
#endif

namespace {

const int kEepromAddr = 0;

void copyStr(char* dst, size_t dstLen, const char* src) {
  if (dstLen == 0) return;
  strncpy(dst, src ? src : "", dstLen - 1);
  dst[dstLen - 1] = '\0';
}

uint32_t crc32(const uint8_t* data, size_t len) {
  uint32_t crc = 0xFFFFFFFFUL;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t b = 0; b < 8; b++) {
      crc = (crc >> 1) ^ (0xEDB88320UL & (-(int32_t)(crc & 1)));
    }
  }
  return ~crc;
}

// CRC covers every byte of the struct except the trailing crc field itself.
uint32_t computeCrc(const IntercomConfig& cfg) {
  return crc32(reinterpret_cast<const uint8_t*>(&cfg),
               sizeof(IntercomConfig) - sizeof(cfg.crc));
}

bool parseIP(const char* s, uint8_t out[4]) {
  unsigned v[4];
  char extra;
  if (sscanf(s, "%u.%u.%u.%u%c", &v[0], &v[1], &v[2], &v[3], &extra) != 4) return false;
  for (int i = 0; i < 4; i++) {
    if (v[i] > 255) return false;
    out[i] = (uint8_t)v[i];
  }
  return true;
}

bool parseMac(const char* s, uint8_t out[6]) {
  unsigned v[6];
  char extra;
  int n = sscanf(s, "%x:%x:%x:%x:%x:%x%c", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &extra);
  if (n != 6) {
    n = sscanf(s, "%x-%x-%x-%x-%x-%x%c", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &extra);
    if (n != 6) return false;
  }
  for (int i = 0; i < 6; i++) {
    if (v[i] > 255) return false;
    out[i] = (uint8_t)v[i];
  }
  return true;
}

bool parseBool(const char* s, uint8_t& out) {
  if (!strcasecmp(s, "1") || !strcasecmp(s, "on") || !strcasecmp(s, "true") ||
      !strcasecmp(s, "yes") || !strcasecmp(s, "y")) {
    out = 1;
    return true;
  }
  if (!strcasecmp(s, "0") || !strcasecmp(s, "off") || !strcasecmp(s, "false") ||
      !strcasecmp(s, "no") || !strcasecmp(s, "n")) {
    out = 0;
    return true;
  }
  return false;
}

void formatIP(const uint8_t ip[4], char* out, size_t outLen) {
  snprintf(out, outLen, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

void fail(char* err, size_t errLen, const char* msg) {
  if (err && errLen) copyStr(err, errLen, msg);
}

}  // namespace

namespace config {

void setDefaults(IntercomConfig& cfg) {
  memset(&cfg, 0, sizeof(cfg));
  cfg.magic    = kConfigMagic;
  cfg.version  = kConfigVersion;
  cfg.mqttPort = 1883;
  cfg.useDhcp  = 1;
  cfg.webEnabled = 1;
  copyStr(cfg.deviceName, sizeof(cfg.deviceName), "Teensy Intercom");
  copyStr(cfg.baseTopic, sizeof(cfg.baseTopic), "intercom");
  copyStr(cfg.discoveryPrefix, sizeof(cfg.discoveryPrefix), "homeassistant");

#if defined(INTERCOM_HAVE_LEGACY_SECRETS)
#  if defined(MQTT_HOST_NAME)
  copyStr(cfg.mqttHost, sizeof(cfg.mqttHost), MQTT_HOST_NAME);
#  elif defined(MQTT_HOST_IP)
  {
    IPAddress legacy = MQTT_HOST_IP;
    snprintf(cfg.mqttHost, sizeof(cfg.mqttHost), "%u.%u.%u.%u",
             legacy[0], legacy[1], legacy[2], legacy[3]);
  }
#  endif
#  if defined(MQTT_PORT)
  cfg.mqttPort = MQTT_PORT;
#  endif
#  if defined(MQTT_USER)
  copyStr(cfg.mqttUser, sizeof(cfg.mqttUser), MQTT_USER);
#  endif
#  if defined(MQTT_PASS)
  copyStr(cfg.mqttPass, sizeof(cfg.mqttPass), MQTT_PASS);
#  endif
#  if defined(DEVICE_UNIQ_SUFFIX)
  // Keeping the old suffix keeps existing Home Assistant entities intact.
  copyStr(cfg.deviceId, sizeof(cfg.deviceId), DEVICE_UNIQ_SUFFIX);
#  endif
#  if defined(STATIC_NET)
  cfg.useDhcp = 0;
  for (int i = 0; i < 4; i++) {
    cfg.ip[i]   = T_IP[i];
    cfg.mask[i] = T_MASK[i];
    cfg.gw[i]   = T_GW[i];
    cfg.dns[i]  = T_DNS[i];
  }
#  endif
#  if defined(USE_FIXED_MAC)
  cfg.useFixedMac = 1;
  memcpy(cfg.mac, FIXED_MAC, sizeof(cfg.mac));
#  endif
#endif
}

bool load(IntercomConfig& cfg) {
  IntercomConfig stored;
  EEPROM.get(kEepromAddr, stored);

  const bool valid = stored.magic == kConfigMagic &&
                     stored.version == kConfigVersion &&
                     stored.crc == computeCrc(stored);
  if (!valid) {
    setDefaults(cfg);
    return false;
  }

  // Defend against a truncated write leaving unterminated strings.
  stored.mqttHost[sizeof(stored.mqttHost) - 1]               = '\0';
  stored.mqttUser[sizeof(stored.mqttUser) - 1]               = '\0';
  stored.mqttPass[sizeof(stored.mqttPass) - 1]               = '\0';
  stored.deviceId[sizeof(stored.deviceId) - 1]               = '\0';
  stored.deviceName[sizeof(stored.deviceName) - 1]           = '\0';
  stored.baseTopic[sizeof(stored.baseTopic) - 1]             = '\0';
  stored.discoveryPrefix[sizeof(stored.discoveryPrefix) - 1] = '\0';

  cfg = stored;
  return true;
}

bool save(const IntercomConfig& cfg) {
  IntercomConfig out = cfg;
  out.magic   = kConfigMagic;
  out.version = kConfigVersion;
  out.crc     = computeCrc(out);
  // EEPROM.put() only rewrites bytes that actually changed, so repeated
  // saves do not needlessly wear the emulated flash.
  EEPROM.put(kEepromAddr, out);

  IntercomConfig check;
  EEPROM.get(kEepromAddr, check);
  return check.crc == out.crc;
}

void erase() {
  IntercomConfig blank;
  memset(&blank, 0xFF, sizeof(blank));
  EEPROM.put(kEepromAddr, blank);
}

bool isConfigured(const IntercomConfig& cfg) {
  return cfg.mqttHost[0] != '\0' && cfg.mqttPort != 0;
}

void ensureDeviceId(IntercomConfig& cfg, const uint8_t mac[6]) {
  if (cfg.deviceId[0] != '\0') return;
  snprintf(cfg.deviceId, sizeof(cfg.deviceId), "%02x%02x%02x", mac[3], mac[4], mac[5]);
}

IPAddress toIP(const uint8_t octets[4]) {
  return IPAddress(octets[0], octets[1], octets[2], octets[3]);
}

bool setField(IntercomConfig& cfg, const char* key, const char* value,
              char* err, size_t errLen) {
  if (!key || !value) {
    fail(err, errLen, "missing key or value");
    return false;
  }

  if (!strcasecmp(key, "host")) {
    copyStr(cfg.mqttHost, sizeof(cfg.mqttHost), value);
    return true;
  }
  if (!strcasecmp(key, "port")) {
    long p = strtol(value, nullptr, 10);
    if (p < 1 || p > 65535) {
      fail(err, errLen, "port must be 1-65535");
      return false;
    }
    cfg.mqttPort = (uint16_t)p;
    return true;
  }
  if (!strcasecmp(key, "user")) {
    copyStr(cfg.mqttUser, sizeof(cfg.mqttUser), value);
    return true;
  }
  if (!strcasecmp(key, "pass")) {
    copyStr(cfg.mqttPass, sizeof(cfg.mqttPass), value);
    return true;
  }
  if (!strcasecmp(key, "device_id")) {
    copyStr(cfg.deviceId, sizeof(cfg.deviceId), value);
    return true;
  }
  if (!strcasecmp(key, "device_name")) {
    copyStr(cfg.deviceName, sizeof(cfg.deviceName), value);
    return true;
  }
  if (!strcasecmp(key, "base_topic")) {
    if (value[0] == '\0') {
      fail(err, errLen, "base_topic cannot be empty");
      return false;
    }
    copyStr(cfg.baseTopic, sizeof(cfg.baseTopic), value);
    return true;
  }
  if (!strcasecmp(key, "discovery_prefix")) {
    if (value[0] == '\0') {
      fail(err, errLen, "discovery_prefix cannot be empty");
      return false;
    }
    copyStr(cfg.discoveryPrefix, sizeof(cfg.discoveryPrefix), value);
    return true;
  }
  if (!strcasecmp(key, "dhcp")) {
    if (!parseBool(value, cfg.useDhcp)) {
      fail(err, errLen, "dhcp must be on/off");
      return false;
    }
    return true;
  }
  if (!strcasecmp(key, "web")) {
    if (!parseBool(value, cfg.webEnabled)) {
      fail(err, errLen, "web must be on/off");
      return false;
    }
    return true;
  }
  if (!strcasecmp(key, "ip") || !strcasecmp(key, "mask") ||
      !strcasecmp(key, "gw") || !strcasecmp(key, "dns")) {
    uint8_t parsed[4];
    if (!parseIP(value, parsed)) {
      fail(err, errLen, "expected a dotted IPv4 address");
      return false;
    }
    uint8_t* dst = !strcasecmp(key, "ip")   ? cfg.ip
                 : !strcasecmp(key, "mask") ? cfg.mask
                 : !strcasecmp(key, "gw")   ? cfg.gw
                                            : cfg.dns;
    memcpy(dst, parsed, 4);
    return true;
  }
  if (!strcasecmp(key, "mac")) {
    if (value[0] == '\0') {
      cfg.useFixedMac = 0;
      return true;
    }
    if (!parseMac(value, cfg.mac)) {
      fail(err, errLen, "expected aa:bb:cc:dd:ee:ff");
      return false;
    }
    cfg.useFixedMac = 1;
    return true;
  }

  fail(err, errLen, "unknown key");
  return false;
}

void getField(const IntercomConfig& cfg, const char* key, char* out,
              size_t outLen, bool showSecrets) {
  if (!out || outLen == 0) return;
  out[0] = '\0';

  if (!strcasecmp(key, "host"))                  copyStr(out, outLen, cfg.mqttHost);
  else if (!strcasecmp(key, "port"))             snprintf(out, outLen, "%u", cfg.mqttPort);
  else if (!strcasecmp(key, "user"))             copyStr(out, outLen, cfg.mqttUser);
  else if (!strcasecmp(key, "pass"))             copyStr(out, outLen, showSecrets ? cfg.mqttPass : "");
  else if (!strcasecmp(key, "device_id"))        copyStr(out, outLen, cfg.deviceId);
  else if (!strcasecmp(key, "device_name"))      copyStr(out, outLen, cfg.deviceName);
  else if (!strcasecmp(key, "base_topic"))       copyStr(out, outLen, cfg.baseTopic);
  else if (!strcasecmp(key, "discovery_prefix")) copyStr(out, outLen, cfg.discoveryPrefix);
  else if (!strcasecmp(key, "dhcp"))             copyStr(out, outLen, cfg.useDhcp ? "on" : "off");
  else if (!strcasecmp(key, "web"))              copyStr(out, outLen, cfg.webEnabled ? "on" : "off");
  else if (!strcasecmp(key, "ip"))               formatIP(cfg.ip, out, outLen);
  else if (!strcasecmp(key, "mask"))             formatIP(cfg.mask, out, outLen);
  else if (!strcasecmp(key, "gw"))               formatIP(cfg.gw, out, outLen);
  else if (!strcasecmp(key, "dns"))              formatIP(cfg.dns, out, outLen);
  else if (!strcasecmp(key, "mac")) {
    if (cfg.useFixedMac) {
      snprintf(out, outLen, "%02x:%02x:%02x:%02x:%02x:%02x",
               cfg.mac[0], cfg.mac[1], cfg.mac[2], cfg.mac[3], cfg.mac[4], cfg.mac[5]);
    }
  }
}

void print(Print& out, const IntercomConfig& cfg, bool showSecrets) {
  char buf[64];

  out.println(F("--- configuration ---"));
  out.print(F("  host             = ")); out.println(cfg.mqttHost);
  out.print(F("  port             = ")); out.println(cfg.mqttPort);
  out.print(F("  user             = ")); out.println(cfg.mqttUser);
  out.print(F("  pass             = "));
  if (showSecrets)          out.println(cfg.mqttPass);
  else if (cfg.mqttPass[0]) out.println(F("(set, hidden -- 'show secrets' to reveal)"));
  else                      out.println(F("(empty)"));
  out.print(F("  device_id        = ")); out.println(cfg.deviceId);
  out.print(F("  device_name      = ")); out.println(cfg.deviceName);
  out.print(F("  base_topic       = ")); out.println(cfg.baseTopic);
  out.print(F("  discovery_prefix = ")); out.println(cfg.discoveryPrefix);
  out.print(F("  dhcp             = ")); out.println(cfg.useDhcp ? F("on") : F("off"));
  if (!cfg.useDhcp) {
    formatIP(cfg.ip, buf, sizeof(buf));   out.print(F("  ip               = ")); out.println(buf);
    formatIP(cfg.mask, buf, sizeof(buf)); out.print(F("  mask             = ")); out.println(buf);
    formatIP(cfg.gw, buf, sizeof(buf));   out.print(F("  gw               = ")); out.println(buf);
    formatIP(cfg.dns, buf, sizeof(buf));  out.print(F("  dns              = ")); out.println(buf);
  }
  getField(cfg, "mac", buf, sizeof(buf), true);
  out.print(F("  mac              = ")); out.println(buf[0] ? buf : "(auto)");
  out.print(F("  web              = ")); out.println(cfg.webEnabled ? F("on") : F("off"));
}

}  // namespace config
