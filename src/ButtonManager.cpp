#include "ButtonManager.h"

const uint8_t ButtonManager::pins_[ButtonManager::count] = {A1, A2, A3};

void ButtonManager::begin() {
  for (uint8_t i = 0; i < count; ++i) {
    pinMode(pins_[i], INPUT_PULLUP);
    buttons_[i].begin(digitalRead(pins_[i]) == LOW, millis());
  }
}

uint8_t ButtonManager::poll(uint32_t now) {
  uint8_t presses = 0;
  for (uint8_t i = 0; i < count; ++i) {
    if (buttons_[i].update(digitalRead(pins_[i]) == LOW, now)) {
      presses |= uint8_t(1U << i);
    }
  }
  return presses;
}
