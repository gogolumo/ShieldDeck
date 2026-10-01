#pragma once

#include <stdint.h>

// Pure timing logic, shared by the Arduino driver and host-side tests.
class DebouncedButton {
 public:
  static constexpr uint32_t debounceMs = 25;
  bool isPressed() const { return stablePressed_; }
  bool isReleased() const { return !rawPressed_ && !stablePressed_ && armed_; }

  void begin(bool pressed, uint32_t now) {
    rawPressed_ = stablePressed_ = pressed;
    changedAt_ = now;
    armed_ = false;  // Require a stable release after boot.
  }

  bool update(bool pressed, uint32_t now) {
    if (pressed != rawPressed_) {
      rawPressed_ = pressed;
      changedAt_ = now;
    }
    if (uint32_t(now - changedAt_) < debounceMs) return false;
    if (!rawPressed_) armed_ = true;
    if (rawPressed_ == stablePressed_) return false;
    stablePressed_ = rawPressed_;
    if (!stablePressed_ || !armed_) return false;
    armed_ = false;
    return true;
  }

 private:
  uint32_t changedAt_ = 0;
  bool rawPressed_ = false;
  bool stablePressed_ = false;
  bool armed_ = false;
};
