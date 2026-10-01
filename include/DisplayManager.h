#pragma once

#include <Arduino.h>

class DisplayManager {
 public:
  void begin();
  // Each argument is a decimal digit; values >9 blank that position.
  void setDigits(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth);
  void update(uint32_t nowUs);
  uint32_t maxScanGapUs() const { return maxScanGapUs_; }

 private:
  static void writeFrame(uint8_t segments, uint8_t digitMask);
  uint8_t segments_[4] = {0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t nextDigit_ = 0;
  uint32_t lastScanUs_ = 0;
  uint32_t maxScanGapUs_ = 0;
  bool scanned_ = false;
};
