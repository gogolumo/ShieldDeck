#pragma once
#include "ButtonManager.h"
#include "BuzzerManager.h"
#include "DisplayManager.h"
#include "LedManager.h"
#include "SerialManager.h"

class AppController {
 public:
  void begin();
  void update();

 private:
  enum class Connection : uint8_t { Offline, Syncing, Ready };
  void disconnect(uint32_t now);
  void handleLine(char* line, uint32_t now);
  void show(char prefix, uint16_t value, uint32_t duration, uint32_t now);
  void pulse(uint8_t button, uint32_t duration, uint32_t now);
  void sendButton(uint8_t button, uint32_t now);
  ButtonManager buttons_;
  BuzzerManager buzzer_;
  DisplayManager display_;
  LedManager leds_;
  SerialManager serial_;
  Connection connection_ = Connection::Offline;
  char sid_[9] = {};
  uint16_t revision_ = 0;
  uint16_t sequence_ = 0;
  uint8_t pendingButton_ = 0;
  uint8_t eligiblePresses_ = 0;
  bool acked_ = false;
  bool sound_ = false;
  uint32_t lastPing_ = 0;
  uint32_t syncingAt_ = 0;
  uint32_t lastHello_ = 0;
  uint32_t eventAt_ = 0;
  uint32_t shownAt_ = 0;
  uint32_t shownFor_ = 0;
  uint32_t pulseAt_[3] = {};
  uint16_t pulseFor_[3] = {};
  uint8_t successAfterPress_ = 0;
};
