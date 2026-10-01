#include <Arduino.h>
#include "ButtonManager.h"
#include "LedManager.h"
#include "DisplayManager.h"
#include "BuzzerManager.h"

namespace {
ButtonManager buttons;
LedManager leds;
DisplayManager display;
BuzzerManager buzzer;
bool startupTestPending = true;
uint32_t presses[ButtonManager::count] = {};
uint32_t lastHeartbeat = 0;
uint8_t pendingEvents = 0;
bool scanReportPending = false;

void reportHeartbeat(uint32_t now) {
  if (scanReportPending && Serial.availableForWrite() >= 32) {
    Serial.print(F("DIAG|SCAN_MAX_US|"));
    Serial.println(display.maxScanGapUs());
    scanReportPending = false;
    return;
  }
  // Skip rather than block when the Serial TX buffer is busy.
  if (uint32_t(now - lastHeartbeat) < 2000 || Serial.availableForWrite() < 58) return;
  lastHeartbeat = now;
  Serial.print(F("DIAG|ALIVE|"));
  Serial.print(now);
  for (uint8_t i = 0; i < ButtonManager::count; ++i) {
    Serial.print('|');
    Serial.print(presses[i]);
  }
  Serial.println();
  scanReportPending = true;
}
}  // namespace

void setup() {
  buzzer.begin();
  leds.begin();
  buttons.begin();
  Serial.begin(115200);
  Serial.println(F("DIAG|SHIELDDECK|BUZZER|4"));
  for (uint8_t i = 0; i < ButtonManager::count; ++i) {
    Serial.print(F("DIAG|INPUT|"));
    Serial.print(i + 1);
    Serial.print('|');
    Serial.println(digitalRead(A1 + i) == LOW ? F("LOW") : F("HIGH"));
  }
  // Start scanning after the boot diagnostics, which can briefly fill Serial TX.
  display.begin();
  display.setDigits(1, 2, 3, 4);
}

void loop() {
  display.update(micros());
  const uint32_t now = millis();
  buzzer.update(now);
  if (startupTestPending && now >= 1500) {
    startupTestPending = false;
    leds.startTest(now);
  }
  leds.update(now);
  const uint8_t events = buttons.poll(now);
  if ((events & 1U) != 0) {
    startupTestPending = false;
    leds.startTest(now);  // Diagnostic-only S1 replay, not a macro mapping.
  }
  if ((events & 2U) != 0) buzzer.startTest(now);  // Diagnostic-only S2 beep.
  pendingEvents |= events;
  for (uint8_t i = 0; i < ButtonManager::count; ++i) {
    if ((events & (1U << i)) != 0) ++presses[i];
    if ((pendingEvents & (1U << i)) == 0 || Serial.availableForWrite() < 13) continue;
    Serial.print(F("BTN|"));
    Serial.print(i + 1);
    Serial.println(F("|PRESS"));
    pendingEvents &= uint8_t(~(1U << i));
  }
  reportHeartbeat(now);
}
