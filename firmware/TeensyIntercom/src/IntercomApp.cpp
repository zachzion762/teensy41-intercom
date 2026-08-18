#include "IntercomApp.h"

#include <stdio.h>

using qindesign::network::Ethernet;

namespace {

// Trampolines so the config front-ends can call back without knowing the
// application type.
void hookSave(void* ctx)                   { static_cast<IntercomApp*>(ctx)->saveConfig(); }
void hookReboot(void* ctx)                 { static_cast<IntercomApp*>(ctx)->reboot(); }
void hookFactoryReset(void* ctx)           { static_cast<IntercomApp*>(ctx)->factoryReset(); }
void hookStatus(void* ctx, Print& out)     { static_cast<IntercomApp*>(ctx)->printStatus(out); }

}  // namespace

IntercomApp::IntercomApp()
    : mqtt_(net_), ha_(mqtt_, cfg_, kButtonCount) {}

void IntercomApp::begin() {
  led_.begin(kStatusLedPin);
  led_.setPattern(1);
  buttons_.begin(kButtonPins, kButtonCount, kDebounceMs);

  const bool stored = config::load(cfg_);

  hooks_.onSave         = hookSave;
  hooks_.onReboot       = hookReboot;
  hooks_.onFactoryReset = hookFactoryReset;
  hooks_.printStatus    = hookStatus;
  hooks_.ctx            = this;

  console_.begin(cfg_, hooks_);
  // Give a USB serial monitor a moment to attach so the banner is not missed.
  const uint32_t t0 = millis();
  while (!Serial && (millis() - t0) < 1500) {
    led_.update(millis());
  }
  console_.printBanner();
  if (!stored) {
    Serial.println(F("(using defaults -- nothing valid was stored in EEPROM)"));
  }

  // The network comes up even without a broker configured, so the web page
  // is reachable on a fresh board.
  startNetwork();

  uint8_t mac[6] = {0};
  Ethernet.macAddress(mac);
  config::ensureDeviceId(cfg_, mac);

  // Discovery payloads are larger than PubSubClient's 256-byte default,
  // which would otherwise make publish() fail silently.
  if (!mqtt_.setBufferSize(kMqttBufferSize)) {
    Serial.println(F("WARNING: could not grow the MQTT buffer; discovery may fail"));
  }
  mqtt_.setKeepAlive(kMqttKeepAliveSec);

  if (cfg_.webEnabled) web_.begin(cfg_, hooks_, kWebPort);
}

void IntercomApp::startNetwork() {
  char hostname[64];
  snprintf(hostname, sizeof(hostname), "teensy-intercom-%s",
           cfg_.deviceId[0] ? cfg_.deviceId : "setup");
  Ethernet.setHostname(hostname);

  if (cfg_.useFixedMac) Ethernet.setMACAddress(cfg_.mac);

  if (cfg_.useDhcp) {
    netStarted_ = Ethernet.begin();
  } else {
    netStarted_ = Ethernet.begin(config::toIP(cfg_.ip),
                                 config::toIP(cfg_.mask),
                                 config::toIP(cfg_.gw),
                                 config::toIP(cfg_.dns));
  }

  // Wait briefly for link so the first MQTT attempt has a chance, but keep
  // the LED animating instead of blocking blind.
  const uint32_t t0 = millis();
  while (!Ethernet.linkState() && (millis() - t0) < kLinkWaitMs) {
    led_.update(millis());
    console_.update();
  }
}

void IntercomApp::serviceMqtt(uint32_t now) {
  if (mqtt_.connected()) return;
  if (now < nextMqttTry_) return;

  mqtt_.setServer(cfg_.mqttHost, cfg_.mqttPort);

  char clientId[64];
  snprintf(clientId, sizeof(clientId), "teensy-intercom-%s", cfg_.deviceId);

  char availTopic[96];
  ha_.availabilityTopic(availTopic, sizeof(availTopic));

  const char* user = cfg_.mqttUser[0] ? cfg_.mqttUser : nullptr;
  const char* pass = cfg_.mqttPass[0] ? cfg_.mqttPass : nullptr;

  const bool ok = mqtt_.connect(clientId, user, pass, availTopic, 1, true, "offline");
  if (ok) {
    mqttBackoff_ = kMqttRetryMinMs;
    ha_.publishAvailability(true);
    discoveryOk_ = ha_.publishDiscovery();
    ha_.publishAllOff();
    lastDiscovery_ = now;

    Serial.print(F("MQTT connected to "));
    Serial.print(cfg_.mqttHost);
    Serial.print(':');
    Serial.println(cfg_.mqttPort);
    if (!discoveryOk_) {
      Serial.println(F("WARNING: discovery publish was rejected (MQTT buffer too small)"));
    }
  } else {
    Serial.print(F("MQTT connect failed, state="));
    Serial.print(mqtt_.state());
    Serial.print(F(", retrying in "));
    Serial.print(mqttBackoff_ / 1000);
    Serial.println(F("s"));

    nextMqttTry_ = now + mqttBackoff_;
    mqttBackoff_ = mqttBackoff_ * 2;
    if (mqttBackoff_ > kMqttRetryMaxMs) mqttBackoff_ = kMqttRetryMaxMs;
  }
}

void IntercomApp::publishChange() {
  const int8_t prev = buttons_.previous();
  const int8_t cur  = buttons_.active();

  if (prev >= 0 && prev != cur) ha_.publishState((uint8_t)prev, false);
  if (cur >= 0)                 ha_.publishState((uint8_t)cur, true);
}

void IntercomApp::updateLed(uint32_t now) {
  if (!config::isConfigured(cfg_))      led_.setPattern(0);
  else if (!Ethernet.linkState())       led_.setPattern(1);
  else if (!mqtt_.connected())          led_.setPattern(2);
  else                                  led_.setPattern(3);
  led_.update(now);
}

void IntercomApp::loop() {
  const uint32_t now = millis();

  console_.update();
  if (web_.running()) web_.update(now);

  if (buttons_.update(now) && mqtt_.connected()) publishChange();

  if (Ethernet.linkState() && config::isConfigured(cfg_)) {
    serviceMqtt(now);
    mqtt_.loop();

    if (mqtt_.connected() && (now - lastDiscovery_) > kRediscoverMs) {
      discoveryOk_   = ha_.publishDiscovery();
      lastDiscovery_ = now;
    }
  }

  updateLed(now);
}

void IntercomApp::saveConfig() {
  if (config::save(cfg_)) {
    Serial.println(F("Configuration saved to EEPROM."));
  } else {
    Serial.println(F("ERROR: could not write configuration to EEPROM."));
  }
}

void IntercomApp::factoryReset() {
  config::erase();
  Serial.println(F("Configuration erased. Rebooting..."));
  Serial.flush();
  reboot();
}

void IntercomApp::reboot() {
  Serial.flush();
  delay(50);
  SCB_AIRCR = 0x05FA0004;  // ARM Cortex-M system reset
  while (true) {
  }
}

void IntercomApp::printStatus(Print& out) {
  uint8_t mac[6] = {0};
  Ethernet.macAddress(mac);
  const IPAddress ip = Ethernet.localIP();

  out.println(F("--- status ---"));
  out.print(F("  firmware   = ")); out.println(F(INTERCOM_FW_VERSION));
  out.print(F("  configured = ")); out.println(config::isConfigured(cfg_) ? F("yes") : F("no"));
  out.print(F("  link       = ")); out.println(Ethernet.linkState() ? F("up") : F("down"));
  out.printf("  mac        = %02x:%02x:%02x:%02x:%02x:%02x\r\n",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  out.printf("  ip         = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
  out.print(F("  mqtt       = "));
  if (mqtt_.connected()) {
    out.println(F("connected"));
  } else {
    out.print(F("disconnected (state "));
    out.print(mqtt_.state());
    out.println(F(")"));
  }
  out.print(F("  discovery  = "));
  out.println(discoveryOk_ ? F("published") : F("REJECTED -- buffer too small"));
  out.print(F("  buttons    = ")); out.println(buttons_.count());
  out.print(F("  web config = "));
  if (web_.running()) {
    out.printf("http://%u.%u.%u.%u/\r\n", ip[0], ip[1], ip[2], ip[3]);
  } else {
    out.println(F("disabled"));
  }
  out.print(F("  uptime     = ")); out.print(millis() / 1000); out.println(F("s"));
}
