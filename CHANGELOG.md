# Changelog

## 2.0.0 — 2026-06-20
- Rewrote firmware as a modular sketch (Config.h, ButtonPanel, MqttManager, StatusLed, Watchdog) instead of one monolithic file
- Each button is now debounced independently, so simultaneous presses are all reported (previously only one button at a time could be tracked)
- Replaced the blocking MQTT reconnect loop with a paced, non-blocking reconnect — button reads never stall while the broker is unreachable
- Replaced delay()-based status blinking with a non-blocking LED sequencer
- Added a hardware watchdog (Teensy 4 WDT1) that resets the board if the firmware ever hangs
- No changes to pin assignments, MQTT topics, HA discovery payloads/unique IDs, or the secrets.h interface — drop-in compatible with units already deployed

## 1.0.0 — 2025-08-25
- Initial public release
- Arduino IDE 2.3.6 build instructions
- Home Assistant MQTT discovery
- Wiring diagram, system overview, photos, STL, timelapse
