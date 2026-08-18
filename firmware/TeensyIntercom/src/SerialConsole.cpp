#include "SerialConsole.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "BoardConfig.h"

namespace {

struct WizardStep {
  const char* key;
  const char* prompt;
  bool        staticOnly;  // only asked when DHCP is off
};

const WizardStep kWizard[] = {
  {"host",        "MQTT broker address (IP or hostname)",       false},
  {"port",        "MQTT port",                                  false},
  {"user",        "MQTT username (blank for none)",             false},
  {"pass",        "MQTT password (blank for none)",             false},
  {"dhcp",        "Use DHCP? (on/off)",                         false},
  {"ip",          "Static IP address",                          true},
  {"mask",        "Subnet mask",                                true},
  {"gw",          "Gateway",                                    true},
  {"dns",         "DNS server",                                 true},
  {"device_name", "Device name shown in Home Assistant",        false},
  {"base_topic",  "MQTT base topic",                            false},
};
const uint8_t kWizardSteps = sizeof(kWizard) / sizeof(kWizard[0]);

char* trim(char* s) {
  while (*s == ' ' || *s == '\t') s++;
  char* end = s + strlen(s);
  while (end > s && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) *--end = '\0';
  return s;
}

}  // namespace

void SerialConsole::begin(IntercomConfig& cfg, const AppHooks& hooks) {
  cfg_   = &cfg;
  hooks_ = hooks;
  Serial.begin(115200);
}

void SerialConsole::printBanner() {
  Serial.println();
  Serial.println(F("======================================"));
  Serial.println(F(" Teensy Intercom " INTERCOM_FW_VERSION));
  Serial.println(F("======================================"));
  if (!config::isConfigured(*cfg_)) {
    Serial.println(F("No broker configured yet."));
    Serial.println(F("Type 'wizard' to set this board up, or 'help' for commands."));
  } else {
    Serial.println(F("Type 'help' for commands."));
  }
  prompt();
}

void SerialConsole::prompt() {
  Serial.print(inWizard_ ? F("") : F("> "));
}

void SerialConsole::update() {
  while (Serial.available() > 0) {
    const int c = Serial.read();
    if (c < 0) break;

    if (c == '\n' || c == '\r') {
      if (len_ == 0) {
        // A bare newline still advances the wizard (accepting the default).
        if (inWizard_) {
          buf_[0] = '\0';
          handleLine(buf_);
        }
        continue;
      }
      buf_[len_] = '\0';
      len_       = 0;
      handleLine(buf_);
      continue;
    }

    if (c == 8 || c == 127) {  // backspace
      if (len_ > 0) len_--;
      continue;
    }

    if (len_ < sizeof(buf_) - 1) buf_[len_++] = (char)c;
  }
}

void SerialConsole::handleLine(char* line) {
  char* s = trim(line);
  if (inWizard_) {
    handleWizardAnswer(s);
  } else {
    if (*s) handleCommand(s);
    prompt();
  }
}

void SerialConsole::startWizard() {
  inWizard_   = true;
  wizardStep_ = 0;
  Serial.println();
  Serial.println(F("Setup wizard -- press Enter to keep the current value."));
  promptWizardStep();
}

void SerialConsole::promptWizardStep() {
  // Skip the static-IP questions while DHCP is enabled.
  while (wizardStep_ < kWizardSteps &&
         kWizard[wizardStep_].staticOnly && cfg_->useDhcp) {
    wizardStep_++;
  }

  if (wizardStep_ >= kWizardSteps) {
    inWizard_ = false;
    Serial.println();
    config::print(Serial, *cfg_, false);
    if (hooks_.onSave) hooks_.onSave(hooks_.ctx);
    Serial.println(F("Type 'reboot' to apply network changes."));
    prompt();
    return;
  }

  char current[72];
  config::getField(*cfg_, kWizard[wizardStep_].key, current, sizeof(current), false);

  Serial.println();
  Serial.print(kWizard[wizardStep_].prompt);
  if (current[0]) {
    Serial.print(F(" ["));
    Serial.print(current);
    Serial.print(F("]"));
  }
  Serial.print(F(": "));
}

void SerialConsole::handleWizardAnswer(const char* answer) {
  const WizardStep& step = kWizard[wizardStep_];

  if (answer[0] != '\0') {
    char err[64] = {0};
    if (!config::setField(*cfg_, step.key, answer, err, sizeof(err))) {
      Serial.println();
      Serial.print(F("  invalid: "));
      Serial.println(err);
      promptWizardStep();  // ask the same question again
      return;
    }
  } else if (!strcasecmp(step.key, "user") || !strcasecmp(step.key, "pass")) {
    // Empty input is a meaningful answer for the optional credentials.
    config::setField(*cfg_, step.key, "", nullptr, 0);
  }

  wizardStep_++;
  promptWizardStep();
}

void SerialConsole::printHelp() {
  Serial.println(F("Commands:"));
  Serial.println(F("  wizard              guided setup"));
  Serial.println(F("  show [secrets]      print current configuration"));
  Serial.println(F("  set <key> <value>   change one setting"));
  Serial.println(F("  save                write configuration to EEPROM"));
  Serial.println(F("  status              network / MQTT state"));
  Serial.println(F("  factory-reset       erase stored configuration"));
  Serial.println(F("  reboot              restart the board"));
  Serial.println();
  Serial.println(F("Keys: host port user pass device_id device_name base_topic"));
  Serial.println(F("      discovery_prefix dhcp ip mask gw dns mac web"));
}

void SerialConsole::handleCommand(char* line) {
  char* cmd = strtok(line, " ");
  if (!cmd) return;

  if (!strcasecmp(cmd, "help") || !strcmp(cmd, "?")) {
    printHelp();
    return;
  }

  if (!strcasecmp(cmd, "wizard")) {
    startWizard();
    return;
  }

  if (!strcasecmp(cmd, "show")) {
    char* arg = strtok(nullptr, " ");
    config::print(Serial, *cfg_, arg && !strcasecmp(arg, "secrets"));
    return;
  }

  if (!strcasecmp(cmd, "status")) {
    if (hooks_.printStatus) hooks_.printStatus(hooks_.ctx, Serial);
    return;
  }

  if (!strcasecmp(cmd, "set")) {
    char* key = strtok(nullptr, " ");
    if (!key) {
      Serial.println(F("usage: set <key> <value>"));
      return;
    }
    // The remainder of the line is the value, so passwords may contain spaces.
    char* value = strtok(nullptr, "");
    if (!value) value = const_cast<char*>("");
    value = trim(value);

    char err[64] = {0};
    if (config::setField(*cfg_, key, value, err, sizeof(err))) {
      Serial.println(F("ok (run 'save' to persist)"));
    } else {
      Serial.print(F("error: "));
      Serial.println(err);
    }
    return;
  }

  if (!strcasecmp(cmd, "save")) {
    if (hooks_.onSave) hooks_.onSave(hooks_.ctx);
    return;
  }

  if (!strcasecmp(cmd, "factory-reset")) {
    if (hooks_.onFactoryReset) hooks_.onFactoryReset(hooks_.ctx);
    return;
  }

  if (!strcasecmp(cmd, "reboot")) {
    if (hooks_.onReboot) hooks_.onReboot(hooks_.ctx);
    return;
  }

  Serial.print(F("unknown command: "));
  Serial.println(cmd);
  Serial.println(F("type 'help'"));
}
