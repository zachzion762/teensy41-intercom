#pragma once
//
// Home Assistant MQTT discovery + button state publishing.
//
// All topics and identifiers are derived from runtime config, so a single
// firmware image can serve any base topic, discovery prefix, or device name.
//

#include <Arduino.h>
#include <PubSubClient.h>

#include "IntercomConfig.h"

class HaPublisher {
 public:
  HaPublisher(PubSubClient& mqtt, const IntercomConfig& cfg, uint8_t buttonCount)
      : mqtt_(mqtt), cfg_(cfg), buttonCount_(buttonCount) {}

  void availabilityTopic(char* out, size_t outLen) const;
  void stateTopic(uint8_t idx, char* out, size_t outLen) const;

  // Retained per-button discovery config. Returns false if any publish was
  // rejected -- almost always because the PubSubClient buffer is too small.
  bool publishDiscovery();

  void publishState(uint8_t idx, bool on);
  void publishAllOff();
  void publishAvailability(bool online);

  // Clears retained discovery configs so entities disappear from Home
  // Assistant instead of lingering as "unavailable".
  void clearDiscovery();

 private:
  void deviceId(char* out, size_t outLen) const;

  PubSubClient&         mqtt_;
  const IntercomConfig& cfg_;
  uint8_t               buttonCount_;
};
