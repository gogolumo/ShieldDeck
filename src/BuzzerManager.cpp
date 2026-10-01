#include "BuzzerManager.h"

namespace {
constexpr uint8_t buzzerPin = 3;
constexpr uint32_t testDurationMs = 40;
}

void BuzzerManager::begin() {
  // PNP transistor: HIGH disables current through the buzzer.
  digitalWrite(buzzerPin, HIGH);
  pinMode(buzzerPin, OUTPUT);
  active_ = false;
}

void BuzzerManager::startTest(uint32_t now) {
  if (active_) return;
  startedAt_ = now;
  active_ = true;
  digitalWrite(buzzerPin, LOW);
  if (Serial.availableForWrite() >= 24) Serial.println(F("DIAG|BUZZER|LOW"));
}

void BuzzerManager::update(uint32_t now) {
  if (!active_ || uint32_t(now - startedAt_) < testDurationMs) return;
  digitalWrite(buzzerPin, HIGH);  // Switch off before any diagnostic output.
  active_ = false;
  if (Serial.availableForWrite() >= 36) {
    Serial.print(F("DIAG|BUZZER|HIGH|"));
    Serial.println(uint32_t(now - startedAt_));
  }
}
