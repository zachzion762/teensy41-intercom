#include "HaPublisher.h"

#include <stdio.h>

#include "BoardConfig.h"

void HaPublisher::deviceId(char* out, size_t outLen) const {
  snprintf(out, outLen, "teensy-intercom-%s", cfg_.deviceId);
}

void HaPublisher::availabilityTopic(char* out, size_t outLen) const {
  snprintf(out, outLen, "%s/availability", cfg_.baseTopic);
}

void HaPublisher::stateTopic(uint8_t idx, char* out, size_t outLen) const {
  snprintf(out, outLen, "%s/button/%u", cfg_.baseTopic, idx + 1);
}

bool HaPublisher::publishDiscovery() {
  char devId[64];
  deviceId(devId, sizeof(devId));

  char availTopic[96];
  availabilityTopic(availTopic, sizeof(availTopic));

  bool ok = true;
  for (uint8_t i = 0; i < buttonCount_; i++) {
    const uint8_t n = i + 1;

    char discTopic[192];
    snprintf(discTopic, sizeof(discTopic), "%s/binary_sensor/%s_%u/config",
             cfg_.discoveryPrefix, devId, n);

    char stTopic[96];
    stateTopic(i, stTopic, sizeof(stTopic));

    char uniqueId[96];
    snprintf(uniqueId, sizeof(uniqueId), "%s_btn_%u", devId, n);

    char payload[768];
    snprintf(payload, sizeof(payload),
      "{"
        "\"name\":\"Button %u\","
        "\"uniq_id\":\"%s\","
        "\"stat_t\":\"%s\","
        "\"pl_on\":\"ON\",\"pl_off\":\"OFF\","
        "\"avty_t\":\"%s\",\"pl_avail\":\"online\",\"pl_not_avail\":\"offline\","
        "\"dev\":{\"ids\":[\"%s\"],\"name\":\"%s\",\"mf\":\"PJRC\","
                 "\"mdl\":\"Teensy 4.1\",\"sw\":\"" INTERCOM_FW_VERSION "\"}"
      "}",
      n, uniqueId, stTopic, availTopic, devId, cfg_.deviceName);

    if (!mqtt_.publish(discTopic, payload, true)) ok = false;
  }
  return ok;
}

void HaPublisher::clearDiscovery() {
  char devId[64];
  deviceId(devId, sizeof(devId));

  for (uint8_t i = 0; i < buttonCount_; i++) {
    char discTopic[192];
    snprintf(discTopic, sizeof(discTopic), "%s/binary_sensor/%s_%u/config",
             cfg_.discoveryPrefix, devId, i + 1);
    mqtt_.publish(discTopic, "", true);
  }
}

void HaPublisher::publishState(uint8_t idx, bool on) {
  char topic[96];
  stateTopic(idx, topic, sizeof(topic));
  mqtt_.publish(topic, on ? "ON" : "OFF", false);
}

void HaPublisher::publishAllOff() {
  for (uint8_t i = 0; i < buttonCount_; i++) publishState(i, false);
}

void HaPublisher::publishAvailability(bool online) {
  char topic[96];
  availabilityTopic(topic, sizeof(topic));
  mqtt_.publish(topic, online ? "online" : "offline", true);
}
