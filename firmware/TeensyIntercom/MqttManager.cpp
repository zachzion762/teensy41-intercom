#include "MqttManager.h"
#include "Config.h"
#include "secrets.h"

void MqttManager::begin() {
#if defined(MQTT_HOST_NAME)
  mqtt_.setServer(MQTT_HOST_NAME, MQTT_PORT);
#else
  mqtt_.setServer(MQTT_HOST_IP, MQTT_PORT);
#endif
  mqtt_.setKeepAlive(cfg::MQTT_KEEPALIVE_S);
}

void MqttManager::loop(ButtonPanel& panel) {
  if (mqtt_.connected()) {
    mqtt_.loop();
    if (millis() - lastDiscovery_ > cfg::REDISCOVER_MS) publishDiscovery();
    return;
  }

  // Pace reconnect attempts instead of blocking the caller until one succeeds.
  uint32_t now = millis();
  if (now - lastReconnectAttempt_ < cfg::MQTT_RECONNECT_INTERVAL_MS) return;
  lastReconnectAttempt_ = now;

  bool ok = mqtt_.connect(cfg::DEVICE_NAME, MQTT_USER, MQTT_PASS,
                          cfg::AVAIL_TOPIC, 1, true, "offline");
  if (ok) {
    mqtt_.publish(cfg::AVAIL_TOPIC, "online", true);
    publishDiscovery();
    publishAllStates(panel);
  }
}

void MqttManager::publishButtonState(int idx, bool pressed) {
  char topic[96];
  snprintf(topic, sizeof(topic), "%s/button/%d", cfg::BASE_TOPIC, idx + 1);
  mqtt_.publish(topic, pressed ? "ON" : "OFF", false);
}

void MqttManager::publishAllStates(ButtonPanel& panel) {
  for (int i = 0; i < cfg::NUM_BUTTONS; i++) {
    publishButtonState(i, panel.isPressed(i));
  }
}

void MqttManager::publishDiscovery() {
  char dev_id[96];
  snprintf(dev_id, sizeof(dev_id), "%s-%s", cfg::DEVICE_NAME, DEVICE_UNIQ_SUFFIX);

  for (int i = 0; i < cfg::NUM_BUTTONS; i++) {
    int  n = i + 1;
    char disc_topic[160];
    snprintf(disc_topic,  sizeof(disc_topic),  "%s/%s_%d/config", cfg::DISC_BASE, cfg::DEVICE_NAME, n);
    char state_topic[128];
    snprintf(state_topic, sizeof(state_topic), "%s/button/%d",    cfg::BASE_TOPIC, n);
    char unique_id[96];
    snprintf(unique_id,   sizeof(unique_id),   "%s_btn_%d",       dev_id, n);

    char payload[640];
    snprintf(payload, sizeof(payload),
      "{"
        "\"name\":\"Intercom Button %d\","
        "\"uniq_id\":\"%s\","
        "\"stat_t\":\"%s\","
        "\"pl_on\":\"ON\",\"pl_off\":\"OFF\","
        "\"avty_t\":\"%s\",\"pl_avail\":\"online\",\"pl_not_avail\":\"offline\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"Teensy Intercom\",\"mf\":\"PJRC\",\"mdl\":\"Teensy 4.1\",\"sw\":\"Intercom v2.0\"}"
      "}",
      n, unique_id, state_topic, cfg::AVAIL_TOPIC, dev_id);
    mqtt_.publish(disc_topic, payload, true);
  }
  lastDiscovery_ = millis();
}
