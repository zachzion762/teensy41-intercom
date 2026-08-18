#pragma once
//
// Built-in configuration web page.
//
// Requests are accumulated across successive update() calls rather than
// read in a blocking loop, so serving the page never delays a button press.
//
// Access is guarded by HTTP basic auth whenever a 'web_pass' is set. It is
// still plain HTTP -- credentials travel base64-encoded, not encrypted -- so
// this is LAN-grade protection, not internet-grade. The page can be turned
// off entirely with the 'web' setting.
//

#include <Arduino.h>
#include <QNEthernet.h>

#include "AppHooks.h"
#include "IntercomConfig.h"

class WebConfig {
 public:
  void begin(IntercomConfig& cfg, const AppHooks& hooks, uint16_t port);
  void update(uint32_t now);
  bool running() const { return running_; }

 private:
  void reset();
  bool requestComplete() const;
  void route();
  bool authorized() const;
  void sendAuthChallenge();
  void sendFormPage(const char* notice);
  void sendStatus(int code, const char* reason, const char* body);
  void applyForm(const String& body, char* notice, size_t noticeLen);

  qindesign::network::EthernetServer server_;
  qindesign::network::EthernetClient client_;
  IntercomConfig* cfg_     = nullptr;
  AppHooks        hooks_;
  String          req_;
  uint32_t        started_ = 0;
  bool            running_ = false;
  bool            rebootAfterResponse_ = false;
};
