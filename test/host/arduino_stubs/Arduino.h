#pragma once
//
// Host-side stub of the Arduino / Teensyduino API surface used by this
// firmware. It exists so the pure-logic parts (configuration parsing, EEPROM
// CRC round-trips, debouncing, LED timing) can be compiled and tested on a
// normal machine, with no Teensy attached. It is NOT a general-purpose
// Arduino emulation -- it only covers what this project calls.
//
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <algorithm>
#include <string>

#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define F(x) (x)

typedef const char* __FlashStringHelper;

// ---- test controls -------------------------------------------------------
namespace ardstub {
void setMillis(uint32_t ms);
void advance(uint32_t ms);
void setPin(uint8_t pin, int level);
void resetPins();
std::string takeOutput();  // everything printed to Serial since the last call
}  // namespace ardstub

// ---- core API ------------------------------------------------------------
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
int digitalRead(uint8_t pin);
uint32_t millis();
void delay(uint32_t ms);

extern volatile uint32_t SCB_AIRCR;

class String {
 public:
  String() {}
  String(const char* s) : s_(s ? s : "") {}
  explicit String(char c) : s_(1, c) {}
  void reserve(unsigned n) { s_.reserve(n); }
  unsigned length() const { return (unsigned)s_.size(); }
  char operator[](unsigned i) const { return s_[i]; }
  String& operator+=(const String& o) { s_ += o.s_; return *this; }
  String& operator+=(const char* o) { s_ += o; return *this; }
  String& operator+=(char c) { s_ += c; return *this; }
  bool operator==(const char* o) const { return s_ == o; }
  const char* c_str() const { return s_.c_str(); }
  int indexOf(char c, int from = 0) const {
    auto p = s_.find(c, (size_t)from);
    return p == std::string::npos ? -1 : (int)p;
  }
  int indexOf(const char* c, int from = 0) const {
    auto p = s_.find(c, (size_t)from);
    return p == std::string::npos ? -1 : (int)p;
  }
  String substring(int a) const { return String(s_.substr((size_t)a).c_str()); }
  String substring(int a, int b) const {
    return String(s_.substr((size_t)a, (size_t)(b - a)).c_str());
  }
  void toLowerCase() { std::transform(s_.begin(), s_.end(), s_.begin(), ::tolower); }
  long toInt() const { return atol(s_.c_str()); }
  bool startsWith(const char* p) const { return s_.rfind(p, 0) == 0; }

 private:
  std::string s_;
};

class Print {
 public:
  virtual ~Print() {}
  virtual size_t writeStr(const char* s);
  size_t print(const char* s) { return writeStr(s ? s : ""); }
  size_t print(const String& s) { return writeStr(s.c_str()); }
  size_t print(char c) { char b[2] = {c, 0}; return writeStr(b); }
  size_t print(int v) { return printNum((long)v); }
  size_t print(unsigned v) { return printNum((long)v); }
  size_t print(long v) { return printNum(v); }
  size_t print(unsigned long v) { return printNum((long)v); }
  size_t println() { return writeStr("\n"); }
  size_t println(const char* s) { size_t n = print(s); return n + writeStr("\n"); }
  size_t println(const String& s) { size_t n = print(s); return n + writeStr("\n"); }
  size_t println(int v) { size_t n = print(v); return n + writeStr("\n"); }
  size_t println(unsigned v) { size_t n = print(v); return n + writeStr("\n"); }
  size_t println(long v) { size_t n = print(v); return n + writeStr("\n"); }
  size_t println(unsigned long v) { size_t n = print(v); return n + writeStr("\n"); }
  int printf(const char* fmt, ...);
  void flush() {}

 private:
  size_t printNum(long v) { char b[32]; snprintf(b, sizeof(b), "%ld", v); return writeStr(b); }
};

class Stream : public Print {
 public:
  virtual int available() { return 0; }
  virtual int read() { return -1; }
};

// Serial with an injectable input queue so the console can be driven by tests.
class SerialStub : public Stream {
 public:
  void begin(unsigned long) {}
  explicit operator bool() const { return true; }
  int available() override;
  int read() override;
  void feed(const char* text);  // queue input as if typed
};
extern SerialStub Serial;

class IPAddress {
 public:
  IPAddress() { memset(o_, 0, sizeof(o_)); }
  IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    o_[0] = a; o_[1] = b; o_[2] = c; o_[3] = d;
  }
  uint8_t operator[](int i) const { return o_[(size_t)i]; }

 private:
  uint8_t o_[4];
};

class Client {};
