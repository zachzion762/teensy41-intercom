#pragma once
//
// Debounced button scanner.
//
// Semantics match the original firmware: the panel reports a single active
// button at a time (lowest-numbered pressed pin wins). Debouncing is applied
// to the scan result, and update() never blocks.
//

#include <Arduino.h>

class ButtonPanel {
 public:
  void begin(const uint8_t* pins, uint8_t count, uint32_t debounceMs);

  // Returns true on the tick where the debounced selection changed.
  bool update(uint32_t now);

  int8_t active() const   { return stable_; }    // -1 when nothing is pressed
  int8_t previous() const { return previous_; }  // -1 when nothing was pressed
  uint8_t count() const   { return count_; }

 private:
  int8_t scan() const;

  const uint8_t* pins_       = nullptr;
  uint8_t        count_      = 0;
  uint32_t       debounceMs_ = 20;
  int8_t         stable_     = -1;
  int8_t         previous_   = -1;
  int8_t         lastRead_   = -1;
  uint32_t       lastChange_ = 0;
};
