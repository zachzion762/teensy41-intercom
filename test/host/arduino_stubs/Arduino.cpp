#include "Arduino.h"

#include <stdarg.h>

#include <deque>
#include <map>

namespace {
uint32_t g_millis = 0;
std::map<uint8_t, int> g_pins;
std::string g_output;
std::deque<char> g_input;
}  // namespace

volatile uint32_t SCB_AIRCR = 0;
SerialStub Serial;

namespace ardstub {

void setMillis(uint32_t ms) { g_millis = ms; }
void advance(uint32_t ms) { g_millis += ms; }
void setPin(uint8_t pin, int level) { g_pins[pin] = level; }
void resetPins() { g_pins.clear(); }

std::string takeOutput() {
  std::string out = g_output;
  g_output.clear();
  return out;
}

}  // namespace ardstub

void pinMode(uint8_t pin, uint8_t mode) {
  // INPUT_PULLUP idles HIGH, matching a released button.
  if (mode == INPUT_PULLUP && g_pins.find(pin) == g_pins.end()) g_pins[pin] = HIGH;
}

void digitalWrite(uint8_t, uint8_t) {}

int digitalRead(uint8_t pin) {
  auto it = g_pins.find(pin);
  return it == g_pins.end() ? HIGH : it->second;
}

uint32_t millis() { return g_millis; }

void delay(uint32_t ms) { g_millis += ms; }

size_t Print::writeStr(const char* s) {
  g_output += s;
  return strlen(s);
}

int Print::printf(const char* fmt, ...) {
  char buf[512];
  va_list ap;
  va_start(ap, fmt);
  const int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  writeStr(buf);
  return n;
}

int SerialStub::available() { return (int)g_input.size(); }

int SerialStub::read() {
  if (g_input.empty()) return -1;
  const char c = g_input.front();
  g_input.pop_front();
  return c;
}

void SerialStub::feed(const char* text) {
  for (const char* p = text; *p; p++) g_input.push_back(*p);
}
