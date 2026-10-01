#pragma once

#include <Arduino.h>

// Explicit, short hardware test. No sound at startup or on unrelated buttons.
class BuzzerManager {
 public:
  void begin(bool diagnostics = true);
  void stop();
  void startTest(uint32_t now);
  void update(uint32_t now);

 private:
  uint32_t startedAt_ = 0;
  bool active_ = false;
  bool diagnostics_ = true;
};
