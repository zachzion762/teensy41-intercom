#pragma once
#include <QNEthernet.h>
#include <PubSubClient.h>
#include "ButtonPanel.h"

// Owns the MQTT connection. Reconnects are paced (non-blocking) so a slow or
// unreachable broker never stalls button reads.
class MqttManager {
 public:
  void begin();
  void loop(ButtonPanel& panel);

  bool connected() const { return mqtt_.connected(); }
  void publishButtonState(int idx, bool pressed);

 private:
  void publishDiscovery();
  void publishAllStates(ButtonPanel& panel);

  qindesign::network::EthernetClient net_;
  PubSubClient mqtt_{net_};
  uint32_t lastReconnectAttempt_ = 0;
  uint32_t lastDiscovery_        = 0;
};
