#pragma once
//
// Byte-accurate stand-in for the Teensy EEPROM library, backed by a real
// array so CRC round-trips are exercised for real.
//
#include <string.h>

#include "Arduino.h"

class EEPROMClass {
 public:
  static const int kSize = 4284;

  template <typename T>
  T& get(int addr, T& value) {
    memcpy(&value, data_ + addr, sizeof(T));
    return value;
  }

  template <typename T>
  const T& put(int addr, const T& value) {
    memcpy(data_ + addr, &value, sizeof(T));
    return value;
  }

  uint16_t length() const { return kSize; }

  // test helper: simulate a blank/erased device
  void wipe(uint8_t fill = 0xFF) { memset(data_, fill, sizeof(data_)); }

 private:
  uint8_t data_[kSize] = {0};
};

extern EEPROMClass EEPROM;
