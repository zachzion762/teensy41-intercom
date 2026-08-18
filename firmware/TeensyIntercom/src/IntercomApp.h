#pragma once
//
// Application wiring: network, MQTT, buttons, and the two config front-ends.
//
// Every step is non-blocking. In particular MQTT reconnection uses a backoff
// timer instead of the original blocking retry loop, so buttons, the serial
// console, and the web page all stay responsive while the broker is down.
//

#include <Arduino.h>
#include <PubSubClient.h>
#include <QNEthernet.h>

#include "AppHooks.h"
#include "BoardConfig.h"
#include "ButtonPanel.h"
#include "HaPublisher.h"
#include "IntercomConfig.h"
#include "SerialConsole.h"
#include "StatusLed.h"
#include "WebConfig.h"

class IntercomApp {
 public:
  IntercomApp();

  void begin();
  void loop();

  // Exposed for the console/web hooks.
  void saveConfig();
  void factoryReset();
  void reboot();
  void printStatus(Print& out);

 private:
  void startNetwork();
  void serviceMqtt(uint32_t now);
  void updateLed(uint32_t now);
  void publishChange();

  qindesign::network::EthernetClient net_;
  PubSubClient   mqtt_;
  IntercomConfig cfg_;
  HaPublisher    ha_;
  ButtonPanel    buttons_;
  StatusLed      led_;
  SerialConsole  console_;
  WebConfig      web_;
  AppHooks       hooks_;

  bool     netStarted_    = false;
  uint32_t lastDiscovery_ = 0;
  uint32_t nextMqttTry_   = 0;
  uint32_t mqttBackoff_   = kMqttRetryMinMs;
  bool     discoveryOk_   = true;
};
