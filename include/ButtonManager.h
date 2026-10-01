#pragma once

#include <Arduino.h>
#include "DebouncedButton.h"

class ButtonManager {
 public:
  static constexpr uint8_t count = 3;
  void begin();
  // Bit 0 = A1, bit 1 = A2, bit 2 = A3; one event per stable press.
  uint8_t poll(uint32_t now);

 private:
  static const uint8_t pins_[count];
  DebouncedButton buttons_[count];
};
