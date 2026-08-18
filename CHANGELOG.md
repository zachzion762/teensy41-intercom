# Changelog

## 2.0.0 — unreleased

Install no longer requires a toolchain. One prebuilt firmware image now works
on any network.

### Added
- **Runtime configuration** stored in EEPROM (magic + version + CRC32 guarded).
  Broker address, credentials, network settings, topics, and device identity
  are all set at runtime — nothing is compiled in.
- **USB serial console** at 115200 baud with a guided `wizard`, plus `show`,
  `set`, `save`, `status`, `factory-reset`, and `reboot`.
- **Built-in web configuration page** on port 80, exposing the same settings.
  Can be disabled with `set web off`.
- **PlatformIO project** with pinned library versions (`platformio.ini`).
- **CI** that runs host tests, builds the firmware, and attaches a prebuilt
  `.hex` to tagged releases.
- **Host test suite** (`make -C test/host`) covering config parsing, EEPROM
  round-trips, CRC rejection, and button debouncing — no hardware required.
- **Home Assistant automation blueprint** for wiring a button to an action.
- Device ID derived automatically from the board's MAC address, so two panels
  on one broker no longer collide.
- Docs: quick start, configuration reference, Home Assistant guide.

### Fixed
- **Discovery payloads were silently dropped.** Home Assistant discovery
  messages are ~330 bytes but PubSubClient's default buffer is 256, so
  `publish()` returned false and the entities never appeared unless the library
  header had been hand-edited. The firmware now calls `setBufferSize()` at
  startup, and `status` reports whether discovery was accepted.
- **The status LED blocked the main loop.** Blinking used `delay()`, stalling
  the loop for ~600 ms out of every 2 s and swallowing button presses. Blink
  patterns are now driven from `millis()`.
- **MQTT reconnection blocked everything.** The retry loop spun inside
  `while (!mqtt.connected())`, so a broker outage froze the buttons, the serial
  console, and the web page. Reconnection now uses non-blocking exponential
  backoff (1 s → 15 s).
- The MQTT client ID was a fixed string, so two panels on one broker would
  disconnect each other. It now includes the unique device ID.

### Changed
- `secrets.h` is no longer required. If present when building from source, its
  values seed the defaults on first boot — including `DEVICE_UNIQ_SUFFIX`, so
  existing Home Assistant entities survive the upgrade.
- Firmware split out of the single `.ino` into `firmware/TeensyIntercom/src/`.
- Discovery entity names are now "Button N" under the device name, rather than
  "Intercom Button N" — Home Assistant already prefixes the device name.

## 1.0.0 — 2025-08-25
- Initial public release
- Arduino IDE 2.3.6 build instructions
- Home Assistant MQTT discovery
- Wiring diagram, system overview, photos, STL, timelapse
