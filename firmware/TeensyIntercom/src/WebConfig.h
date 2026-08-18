#pragma once
//
// Built-in configuration web page.
//
// Requests are accumulated across successive update() calls rather than
// read in a blocking loop, so serving the page never delays a button press.
// This is a plain-HTTP, unauthenticated page intended for a trusted LAN --
// it can be turned off entirely with the 'web' setting.
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
