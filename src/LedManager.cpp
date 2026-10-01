#include "LedManager.h"

void LedManager::begin() {
  for (uint8_t pin = 10; pin <= 13; ++pin) {
    digitalWrite(pin, HIGH);  // Load inactive level before enabling output.
    pinMode(pin, OUTPUT);
  }
  step_ = 8;
}

void LedManager::startTest(uint32_t now) {
  if (step_ < 8) return;  // Do not restart an in-progress sequence.
  step_ = 0;
  stepStartedAt_ = now;
  applyStep();
}

void LedManager::update(uint32_t now) {
  if (step_ >= 8) return;
  const uint32_t duration = (step_ % 2 == 0) ? 700 : 300;
  if (uint32_t(now - stepStartedAt_) < duration) return;
  ++step_;
  stepStartedAt_ = now;
  applyStep();
}

void LedManager::applyStep() {
  if (step_ >= 8) {
    if (Serial.availableForWrite() >= 16) Serial.println(F("DIAG|LED|DONE"));
    return;  // The last HIGH step has already switched D13 off.
  }
  const uint8_t pin = 10 + step_ / 2;
  const uint8_t level = (step_ % 2 == 0) ? LOW : HIGH;
  digitalWrite(pin, level);
  // Report commanded electrical level, not an unobserved physical ON/OFF.
  if (Serial.availableForWrite() < 24) return;
  Serial.print(F("DIAG|LED|"));
  Serial.print(pin);
  Serial.println(level == LOW ? F("|LOW") : F("|HIGH"));
}
