#pragma once
//
// USB serial configuration console.
//
// This is the bootstrap path: it works before the network does, so a board
// with no stored config can still be set up from any serial monitor at
// 115200 baud. Reading is non-blocking -- one line is assembled across
// successive update() calls.
//

#include <Arduino.h>

#include "AppHooks.h"
#include "IntercomConfig.h"

class SerialConsole {
 public:
  void begin(IntercomConfig& cfg, const AppHooks& hooks);
  void update();

  void printBanner();

 private:
  void handleLine(char* line);
  void handleCommand(char* line);
  void handleWizardAnswer(const char* answer);
  void startWizard();
  void promptWizardStep();
  void prompt();
  void printHelp();

  IntercomConfig* cfg_   = nullptr;
  AppHooks    hooks_;
  char            buf_[160];
  size_t          len_        = 0;
  bool            inWizard_   = false;
  uint8_t         wizardStep_ = 0;
};
