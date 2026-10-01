#include "SerialManager.h"
#include <string.h>

void SerialManager::begin() { Serial.begin(115200); }

char* SerialManager::readLine(uint32_t now) {
  if (delivered_) { length_ = 0; delivered_ = false; }
  if (length_ && uint32_t(now - lastByteAt_) >= 500) {
    length_ = 0;
    discard_ = true;  // Never execute the tail of a timed-out/oversize frame.
  }
  for (uint8_t budget = 0; budget < 16 && Serial.available(); ++budget) {
    const char ch = Serial.read();
    lastByteAt_ = now;
    if (ch == '\n') {
      if (discard_) { discard_ = false; length_ = 0; continue; }
      if (length_ && input_[length_ - 1] == '\r') --length_;
      if (!length_) continue;
      input_[length_] = '\0';
      delivered_ = true;
      return input_;
    }
    if (discard_) continue;
    if ((length_ && input_[length_ - 1] == '\r') ||
        (ch != '\r' && (ch < 32 || ch > 126)) || length_ >= 95) {
      discard_ = true;
      length_ = 0;
      continue;
    }
    input_[length_++] = ch;
  }
  return nullptr;
}

bool SerialManager::send(const char* line) {
  const size_t size = strlen(line);
  const uint8_t used = (head_ + sizeof(output_) - tail_) % sizeof(output_);
  if (size > 95 || size + 1 > sizeof(output_) - 1 - used) return false;
  for (size_t i = 0; i <= size; ++i) {
    output_[head_] = i == size ? '\n' : line[i];
    head_ = (head_ + 1) % sizeof(output_);
  }
  return true;
}

void SerialManager::flushOutput() {
  for (uint8_t budget = 0; budget < 16 && head_ != tail_ && Serial.availableForWrite(); ++budget) {
    Serial.write(output_[tail_]);
    tail_ = (tail_ + 1) % sizeof(output_);
  }
}

void SerialManager::clearOutput() { head_ = tail_ = 0; }
