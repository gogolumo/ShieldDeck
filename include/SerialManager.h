#pragma once
#include <Arduino.h>

class SerialManager {
 public:
  void begin();
  char* readLine(uint32_t now);
  bool send(const char* line);
  void flushOutput();
  void clearOutput();

 private:
  char input_[96] = {};
  uint8_t length_ = 0;
  bool discard_ = false;
  bool delivered_ = false;
  uint32_t lastByteAt_ = 0;
  char output_[192] = {};
  uint8_t head_ = 0;
  uint8_t tail_ = 0;
};
