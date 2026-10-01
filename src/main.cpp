#include <Arduino.h>
#include "ButtonManager.h"
#include "LedManager.h"

namespace {
ButtonManager buttons;
LedManager leds;
bool startupTestPending = true;
uint32_t presses[ButtonManager::count] = {};
uint32_t lastHeartbeat = 0;
uint8_t pendingEvents = 0;

void quietOutputs() {
  // VMA209 schematic: the buzzer uses a PNP transistor; HIGH is inactive.
  digitalWrite(3, HIGH);
  pinMode(3, OUTPUT);
  // Blank both 74HC595 outputs; no display scan is active in this test.
  pinMode(4, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  digitalWrite(4, LOW);
  shiftOut(8, 7, MSBFIRST, 0xFF);  // Segments inactive.
  shiftOut(8, 7, MSBFIRST, 0x00);  // No digit selected.
  digitalWrite(4, HIGH);
}

void reportHeartbeat(uint32_t now) {
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
}
}  // namespace

void setup() {
  quietOutputs();
  leds.begin();
  buttons.begin();
  Serial.begin(115200);
  Serial.println(F("DIAG|SHIELDDECK|BUTTONS_LEDS|2"));
  for (uint8_t i = 0; i < ButtonManager::count; ++i) {
    Serial.print(F("DIAG|INPUT|"));
    Serial.print(i + 1);
    Serial.print('|');
    Serial.println(digitalRead(A1 + i) == LOW ? F("LOW") : F("HIGH"));
  }
}

void loop() {
  const uint32_t now = millis();
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
