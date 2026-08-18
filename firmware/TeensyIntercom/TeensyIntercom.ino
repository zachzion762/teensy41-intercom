//
// Teensy 4.1 Intercom -- 12-button MQTT panel with Home Assistant discovery.
//
// There is nothing to edit in this file. The broker address, credentials, and
// network settings are stored in EEPROM and set at runtime over USB serial or
// the built-in web page, so one firmware image works on any network.
//
// First-time setup:
//   1. Flash the board.
//   2. Open a serial monitor at 115200 baud and type 'wizard'
//      (or browse to the board's IP address once it has one).
//
// See docs/quickstart.md for the full walkthrough.
//

#include "src/IntercomApp.h"

IntercomApp app;

void setup() {
  app.begin();
}

void loop() {
  app.loop();
}
