#pragma once

#include <Arduino.h>

// One-shot polarity/pin-order smoke test; no companion status behavior yet.
class LedManager {
 public:
  void begin();
  void set(uint8_t number, bool on);
  void startTest(uint32_t now);
  void update(uint32_t now);

 private:
  void applyStep();
  uint32_t stepStartedAt_ = 0;
  uint8_t step_ = 8;  // 0..7 = alternating LOW/HIGH on D10..D13; 8 = idle.
};
