#include <QNEthernet.h>
#include "secrets.h"      // <-- this file is local-only; not committed
#include "Config.h"
#include "ButtonPanel.h"
#include "MqttManager.h"
#include "StatusLed.h"
#include "Watchdog.h"

using namespace qindesign::network;

static ButtonPanel  buttons;
static MqttManager  mqttMgr;
static StatusLed    led;
static WatchdogGuard watchdog;

static void beginEthernet() {
#if defined(USE_FIXED_MAC)
  Ethernet.setMACAddress(FIXED_MAC);
#endif

#if defined(STATIC_NET)
  Ethernet.begin(T_IP, T_MASK, T_GW);
  Ethernet.setDNSServerIP(T_DNS);
#else
  Ethernet.begin();  // DHCP by default
#endif
}

void setup() {
  watchdog.begin();

  led.begin();
  buttons.begin();
  beginEthernet();

  // Wait up to LINK_WAIT_TIMEOUT_MS for link; keep feeding the watchdog
  // and blinking the LED while we do.
  uint32_t t0 = millis();
  while (!Ethernet.linkState() && (millis() - t0 < cfg::LINK_WAIT_TIMEOUT_MS)) {
    watchdog.feed();
    led.update(1);
  }

  mqttMgr.begin();
}

void loop() {
  watchdog.feed();

  buttons.update();

  bool linkUp = Ethernet.linkState();
  if (linkUp) mqttMgr.loop(buttons);

  if (mqttMgr.connected()) {
    for (int i = 0; i < cfg::NUM_BUTTONS; i++) {
      if (buttons.changed(i)) mqttMgr.publishButtonState(i, buttons.isPressed(i));
    }
  }

  led.update(!linkUp ? 1 : (!mqttMgr.connected() ? 2 : 3));
}
