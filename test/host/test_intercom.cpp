//
// Host tests for the hardware-independent parts of the firmware:
// configuration parsing, EEPROM persistence, button debouncing, and the
// status LED timing. Run with `make -C test/host`.
//
#include <stdio.h>
#include <string.h>

#include "ButtonPanel.h"
#include "EEPROM.h"
#include "IntercomConfig.h"
#include "StatusLed.h"

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const char* what) {
  g_checks++;
  if (!ok) {
    g_failures++;
    printf("  FAIL: %s\n", what);
  }
}

void checkStr(const char* got, const char* want, const char* what) {
  g_checks++;
  if (strcmp(got, want) != 0) {
    g_failures++;
    printf("  FAIL: %s (got \"%s\", want \"%s\")\n", what, got, want);
  }
}

// --------------------------------------------------------------------------

void testDefaults() {
  printf("defaults\n");
  IntercomConfig cfg;
  config::setDefaults(cfg);

  check(cfg.mqttPort == 1883, "default port is 1883");
  check(cfg.useDhcp == 1, "DHCP on by default");
  check(cfg.webEnabled == 1, "web config on by default");
  checkStr(cfg.baseTopic, "intercom", "default base topic");
  checkStr(cfg.discoveryPrefix, "homeassistant", "default discovery prefix");
  check(!config::isConfigured(cfg), "a default config is not yet usable");
}

void testSetField() {
  printf("setField\n");
  IntercomConfig cfg;
  config::setDefaults(cfg);
  char err[64];

  check(config::setField(cfg, "host", "192.168.1.10", err, sizeof(err)), "set host");
  checkStr(cfg.mqttHost, "192.168.1.10", "host stored");
  check(config::isConfigured(cfg), "host + port makes it usable");

  check(config::setField(cfg, "host", "homeassistant.local", err, sizeof(err)),
        "hostnames are accepted too");

  check(config::setField(cfg, "port", "8883", err, sizeof(err)), "set port");
  check(cfg.mqttPort == 8883, "port stored");
  check(!config::setField(cfg, "port", "0", err, sizeof(err)), "port 0 rejected");
  check(!config::setField(cfg, "port", "70000", err, sizeof(err)), "port 70000 rejected");
  check(cfg.mqttPort == 8883, "rejected port left the old value alone");

  check(config::setField(cfg, "ip", "10.0.0.5", err, sizeof(err)), "set static ip");
  check(cfg.ip[0] == 10 && cfg.ip[3] == 5, "ip octets stored");
  check(!config::setField(cfg, "ip", "10.0.0.999", err, sizeof(err)), "octet > 255 rejected");
  check(!config::setField(cfg, "ip", "10.0.0", err, sizeof(err)), "short ip rejected");
  check(!config::setField(cfg, "ip", "not-an-ip", err, sizeof(err)), "garbage ip rejected");

  check(config::setField(cfg, "dhcp", "off", err, sizeof(err)), "dhcp off");
  check(cfg.useDhcp == 0, "dhcp flag cleared");
  check(config::setField(cfg, "dhcp", "yes", err, sizeof(err)), "dhcp yes");
  check(cfg.useDhcp == 1, "dhcp flag set");
  check(!config::setField(cfg, "dhcp", "maybe", err, sizeof(err)), "bad bool rejected");

  check(config::setField(cfg, "mac", "02:12:34:56:78:9a", err, sizeof(err)), "set mac");
  check(cfg.useFixedMac == 1 && cfg.mac[0] == 0x02 && cfg.mac[5] == 0x9a, "mac stored");
  check(config::setField(cfg, "mac", "", err, sizeof(err)), "empty mac clears the override");
  check(cfg.useFixedMac == 0, "fixed-mac flag cleared");
  check(!config::setField(cfg, "mac", "zz:12:34:56:78:9a", err, sizeof(err)), "bad mac rejected");

  check(!config::setField(cfg, "base_topic", "", err, sizeof(err)), "empty base topic rejected");
  check(!config::setField(cfg, "nonsense", "x", err, sizeof(err)), "unknown key rejected");
  check(err[0] != '\0', "an error message is reported");

  // Long values must be truncated, not overflowed.
  char longValue[256];
  memset(longValue, 'a', sizeof(longValue) - 1);
  longValue[sizeof(longValue) - 1] = '\0';
  check(config::setField(cfg, "host", longValue, err, sizeof(err)), "overlong host accepted");
  check(strlen(cfg.mqttHost) == sizeof(cfg.mqttHost) - 1, "overlong host truncated");
}

void testGetField() {
  printf("getField\n");
  IntercomConfig cfg;
  config::setDefaults(cfg);
  char err[64], buf[72];

  config::setField(cfg, "host", "broker.lan", err, sizeof(err));
  config::setField(cfg, "pass", "hunter2", err, sizeof(err));
  config::setField(cfg, "gw", "192.168.4.1", err, sizeof(err));

  config::getField(cfg, "host", buf, sizeof(buf), false);
  checkStr(buf, "broker.lan", "host round-trips");

  config::getField(cfg, "port", buf, sizeof(buf), false);
  checkStr(buf, "1883", "port round-trips as text");

  config::getField(cfg, "gw", buf, sizeof(buf), false);
  checkStr(buf, "192.168.4.1", "gateway round-trips");

  config::getField(cfg, "pass", buf, sizeof(buf), false);
  checkStr(buf, "", "password hidden unless secrets are requested");

  config::getField(cfg, "pass", buf, sizeof(buf), true);
  checkStr(buf, "hunter2", "password readable with showSecrets");

  config::getField(cfg, "dhcp", buf, sizeof(buf), false);
  checkStr(buf, "on", "dhcp renders as on/off");

  config::getField(cfg, "unknown_key", buf, sizeof(buf), false);
  checkStr(buf, "", "unknown key yields an empty string");
}

void testPersistence() {
  printf("EEPROM persistence\n");
  EEPROM.wipe(0xFF);

  IntercomConfig loaded;
  check(!config::load(loaded), "a blank EEPROM reports no stored config");
  check(loaded.mqttPort == 1883, "blank load falls back to defaults");

  IntercomConfig cfg;
  config::setDefaults(cfg);
  char err[64];
  config::setField(cfg, "host", "192.168.1.10", err, sizeof(err));
  config::setField(cfg, "user", "mqtt-user", err, sizeof(err));
  config::setField(cfg, "pass", "s3cret", err, sizeof(err));
  config::setField(cfg, "device_id", "abc123", err, sizeof(err));
  config::setField(cfg, "dhcp", "off", err, sizeof(err));
  config::setField(cfg, "ip", "192.168.1.231", err, sizeof(err));

  check(config::save(cfg), "save reports success");

  IntercomConfig back;
  check(config::load(back), "stored config loads back");
  checkStr(back.mqttHost, "192.168.1.10", "host survives a round-trip");
  checkStr(back.mqttUser, "mqtt-user", "user survives a round-trip");
  checkStr(back.mqttPass, "s3cret", "password survives a round-trip");
  checkStr(back.deviceId, "abc123", "device id survives a round-trip");
  check(back.useDhcp == 0, "dhcp flag survives a round-trip");
  check(back.ip[3] == 231, "static ip survives a round-trip");

  // A single flipped byte must be caught by the CRC rather than loaded.
  uint8_t raw[sizeof(IntercomConfig)];
  EEPROM.get(0, raw);
  raw[8] ^= 0xFF;
  EEPROM.put(0, raw);
  IntercomConfig corrupt;
  check(!config::load(corrupt), "a corrupted config is rejected by the CRC");
  check(corrupt.mqttPort == 1883, "rejected config falls back to defaults");

  config::erase();
  IntercomConfig erased;
  check(!config::load(erased), "erase() clears the stored config");
}

void testDeviceId() {
  printf("device id derivation\n");
  IntercomConfig cfg;
  config::setDefaults(cfg);
  const uint8_t mac[6] = {0x04, 0xe9, 0xe5, 0x0a, 0xbb, 0xcc};

  config::ensureDeviceId(cfg, mac);
  checkStr(cfg.deviceId, "0abbcc", "device id derived from the MAC tail");

  config::ensureDeviceId(cfg, mac);
  checkStr(cfg.deviceId, "0abbcc", "derivation is stable across calls");

  IntercomConfig manual;
  config::setDefaults(manual);
  char err[64];
  config::setField(manual, "device_id", "myid", err, sizeof(err));
  config::ensureDeviceId(manual, mac);
  checkStr(manual.deviceId, "myid", "an explicit device id is never overwritten");
}

// Drives the panel the way the real main loop does: update() called
// continuously, in 1 ms slices, rather than once after a time jump.
bool run(ButtonPanel& panel, uint32_t ms) {
  bool changed = false;
  for (uint32_t i = 0; i < ms; i++) {
    ardstub::advance(1);
    if (panel.update(millis())) changed = true;
  }
  return changed;
}

void testButtonDebounce() {
  printf("button debounce\n");
  static const uint8_t pins[] = {1, 3, 5};
  ButtonPanel panel;
  ardstub::resetPins();
  ardstub::setMillis(1000);
  panel.begin(pins, 3, 20);

  check(!run(panel, 50), "idle panel reports no change");
  check(panel.active() == -1, "nothing pressed while idle");

  // Press button 2 (index 1). The change must not register until the
  // debounce window has elapsed.
  ardstub::setPin(3, LOW);
  check(!run(panel, 10), "press is still pending mid-debounce");
  check(run(panel, 20), "press registers after the debounce window");
  check(panel.active() == 1, "correct button reported");
  check(panel.previous() == -1, "nothing was previously active");

  check(!run(panel, 100), "a steady press is not re-reported");

  // Release.
  ardstub::setPin(3, HIGH);
  check(run(panel, 30), "release registers");
  check(panel.active() == -1, "nothing active after release");
  check(panel.previous() == 1, "previous button reported for the OFF publish");

  // A bounce shorter than the debounce window must be swallowed entirely.
  ardstub::setPin(5, LOW);
  run(panel, 5);
  ardstub::setPin(5, HIGH);
  run(panel, 5);
  check(!run(panel, 100), "a 10 ms bounce is ignored");
  check(panel.active() == -1, "bounce did not change the active button");

  // Lowest-numbered pressed pin wins, matching the original firmware.
  ardstub::setPin(5, LOW);
  check(run(panel, 30), "third button registers");
  check(panel.active() == 2, "third button active");

  ardstub::setPin(1, LOW);
  check(run(panel, 30), "adding a lower button changes the selection");
  check(panel.active() == 0, "lowest pressed index wins");
  check(panel.previous() == 2, "previous selection reported");

  // Releasing the lower button falls back to the one still held.
  ardstub::setPin(1, HIGH);
  check(run(panel, 30), "falling back to the held button is reported");
  check(panel.active() == 2, "still-held button becomes active again");
}

void testStatusLed() {
  printf("status LED\n");
  StatusLed led;
  ardstub::setMillis(0);
  led.begin(13);

  // The LED must never block: update() is pure bookkeeping, and a pattern
  // change takes effect without waiting for the previous cycle to finish.
  led.setPattern(3);
  for (int i = 0; i < 100; i++) {
    ardstub::advance(10);
    led.update(millis());
  }
  check(millis() == 1000, "1000 ms of updates consumed no extra time");

  led.setPattern(1);
  led.update(millis());
  led.setPattern(0);
  led.update(millis());
  check(true, "pattern switches do not hang");
}

}  // namespace

int main() {
  printf("Teensy Intercom host tests\n\n");
  testDefaults();
  testSetField();
  testGetField();
  testPersistence();
  testDeviceId();
  testButtonDebounce();
  testStatusLed();

  printf("\n%d checks, %d failures\n", g_checks, g_failures);
  if (g_failures == 0) printf("PASS\n");
  return g_failures == 0 ? 0 : 1;
}
