#include "DisplayManager.h"

#include <avr/pgmspace.h>

namespace {
constexpr uint8_t latchPin = 4;
constexpr uint8_t clockPin = 7;
constexpr uint8_t dataPin = 8;
constexpr uint32_t scanIntervalUs = 2000;
// Bits 0..6 = a..g, bit 7 = decimal point. Common-anode segments: LOW = on.
// These patterns come from the segment wiring, not the order of the digits.
const uint8_t decimalSegments[] PROGMEM = {
    0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};
}

void DisplayManager::begin() {
  const uint8_t pins[] = {latchPin, clockPin, dataPin};
  for (uint8_t pin : pins) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }
  writeFrame(0xFF, 0x00);
  nextDigit_ = 0;
  lastScanUs_ = micros();
  maxScanGapUs_ = 0;
  scanned_ = false;
}

void DisplayManager::setDigits(uint8_t first, uint8_t second,
                               uint8_t third, uint8_t fourth) {
  const uint8_t digits[] = {first, second, third, fourth};
  for (uint8_t i = 0; i < 4; ++i) {
    segments_[i] = digits[i] < 10 ? pgm_read_byte(&decimalSegments[digits[i]]) : 0xFF;
  }
}

void DisplayManager::writeFrame(uint8_t segments, uint8_t digitMask) {
  digitalWrite(latchPin, LOW);
  // First byte travels through U2 to segment register U3.
  // Second byte stays in U2: QA..QD select digits 1..4, active HIGH.
  shiftOut(dataPin, clockPin, MSBFIRST, segments);
  shiftOut(dataPin, clockPin, MSBFIRST, digitMask);
  digitalWrite(latchPin, HIGH);
}

void DisplayManager::update(uint32_t nowUs) {
  const uint32_t gap = uint32_t(nowUs - lastScanUs_);
  if (gap < scanIntervalUs) return;
  if (scanned_ && gap > maxScanGapUs_) maxScanGapUs_ = gap;
  scanned_ = true;
  lastScanUs_ = nowUs;
  writeFrame(0xFF, 0x00);  // Latch a blank frame before enabling the next digit.
  writeFrame(segments_[nextDigit_], uint8_t(1U << nextDigit_));
  nextDigit_ = (nextDigit_ + 1) % 4;
}
