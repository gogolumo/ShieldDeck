#include "AppController.h"
#include "ProtocolFields.h"
#include <stdio.h>

void AppController::begin() {
  buzzer_.begin(false);
  leds_.begin();
  buttons_.begin();
  display_.begin();
  serial_.begin();
  disconnect(millis());
}

void AppController::disconnect(uint32_t now) {
  connection_ = Connection::Offline;
  sid_[0] = '\0';
  revision_ = sequence_ = 0;
  pendingButton_ = eligiblePresses_ = successAfterPress_ = 0;
  shownFor_ = 0;
  sound_ = false;
  buzzer_.stop();
  serial_.clearOutput();
  for (uint8_t i = 0; i < 3; ++i) { leds_.set(i + 1, false); pulseFor_[i] = 0; }
  display_.showOffline();
  lastHello_ = now - 1000;
}

void AppController::show(char prefix, uint16_t value, uint32_t duration, uint32_t now) {
  display_.showStatus(prefix, value);
  shownAt_ = now;
  shownFor_ = duration;
  successAfterPress_ = 0;
}

void AppController::pulse(uint8_t button, uint32_t duration, uint32_t now) {
  leds_.set(button, true);
  pulseAt_[button - 1] = now;
  pulseFor_[button - 1] = duration;
}

void AppController::sendButton(uint8_t button, uint32_t now) {
  if (pendingButton_) { show('E', 4, 1500, now); return; }
  if (sequence_ == 65535) { disconnect(now); return; }
  char line[64];
  ++sequence_;
  snprintf(line, sizeof(line), "BTN|%s|%u|%u|%u|SHORT", sid_, revision_, sequence_, button);
  if (!serial_.send(line)) { show('E', 4, 1500, now); return; }
  pendingButton_ = button;
  eventAt_ = now;
  acked_ = false;
  show('b', button, 500, now);
}

void AppController::handleLine(char* line, uint32_t now) {
  char* f[9];
  const uint8_t n = splitFields(line, f, 9);
  if (!n) return;
  char reply[80];
  if (n == 3 && strcmp(f[0], "WELCOME") == 0) {
    if (connection_ != Connection::Offline || !validSession(f[2])) return;
    if (strcmp(f[1], "1") != 0) { serial_.send("VERSION|1"); return; }
    strcpy(sid_, f[2]);
    connection_ = Connection::Syncing;
    lastPing_ = now;
    syncingAt_ = now;
    snprintf(reply, sizeof(reply), "READY|%s", sid_);
    serial_.send(reply);
    return;
  }
  if (n < 2 || connection_ == Connection::Offline || strcmp(f[1], sid_) != 0) return;
  if (n == 2 && strcmp(f[0], "PING") == 0) {
    lastPing_ = now;
    snprintf(reply, sizeof(reply), "PONG|%s", sid_);
    serial_.send(reply);
    return;
  }
  uint16_t rev = 0, seq = 0;
  if (n == 8 && strcmp(f[0], "CONFIG") == 0) {
    uint16_t profile, sound;
    if (!parseNumber(f[2], 1, 65535, rev) || !parseNumber(f[3], 1, 1, profile) ||
        !parseNumber(f[4], 0, 1, sound) || strcmp(f[5], "PULSE") ||
        strcmp(f[6], "PULSE") || strcmp(f[7], "PULSE")) return;
    if (rev < revision_) return;
    if (rev == revision_ && bool(sound) != sound_) { disconnect(now); return; }
    if (rev != revision_ && (pendingButton_ || !buttons_.allReleased())) {
      snprintf(reply, sizeof(reply), "REJECT|%s|%u|BUSY", sid_, rev);
      serial_.send(reply);
      return;
    }
    if (rev != revision_) {
      revision_ = rev;
      sound_ = sound;
      eligiblePresses_ = 0;
      pendingButton_ = 0;
      for (uint8_t i = 0; i < 3; ++i) { leds_.set(i + 1, false); pulseFor_[i] = 0; }
      show('P', 1, 0, now);
    }
    connection_ = Connection::Ready;
    snprintf(reply, sizeof(reply), "CONFIGURED|%s|%u", sid_, revision_);
    serial_.send(reply);
    return;
  }
  if (!pendingButton_ || n < 4 || !parseNumber(f[2], 1, 65535, rev) ||
      !parseNumber(f[3], 1, 65535, seq) || rev != revision_ || seq != sequence_) return;
  if (n == 4 && strcmp(f[0], "ACK") == 0) { acked_ = true; return; }
  if (n != 6 || strcmp(f[0], "RESULT") != 0) return;
  uint16_t code;
  if (!parseNumber(f[5], 0, 6, code)) return;
  const bool ok = strcmp(f[4], "OK") == 0 && code == 0;
  if (!ok && !(strcmp(f[4], "ERR") == 0 && code >= 1)) return;
  const uint8_t button = pendingButton_;
  pendingButton_ = 0;
  if (ok) {
    pulse(button, 150, now);
    if (sound_) buzzer_.startTest(now);
    // Keep b00n readable before showing 000n; neither represents persistent state.
    if (shownFor_ && uint32_t(now - shownAt_) < shownFor_) successAfterPress_ = button;
    else show('0', button, 500, now);
  } else {
    leds_.set(button, false);
    pulseFor_[button - 1] = 0;
    show('E', code, 1500, now);
  }
}

void AppController::update() {
  display_.update(micros());
  const uint32_t now = millis();
  buzzer_.update(now);
  const uint8_t presses = buttons_.poll(now);
  const uint8_t releases = buttons_.releases();
  if (connection_ != Connection::Offline && uint32_t(now - lastPing_) >= 3000) disconnect(now);
  if (connection_ == Connection::Syncing && uint32_t(now - syncingAt_) >= 3000) disconnect(now);
  // Expire pending BEFORE parsing input, so late results cannot resurrect it.
  if (pendingButton_ && uint32_t(now - eventAt_) >= (acked_ ? 30000UL : 1000UL)) {
    leds_.set(pendingButton_, false);
    pulseFor_[pendingButton_ - 1] = 0;
    pendingButton_ = 0;
    show('E', 1, 1500, now);
  }
  if (char* line = serial_.readLine(now)) handleLine(line, now);
  if (connection_ == Connection::Offline && uint32_t(now - lastHello_) >= 1000) {
    serial_.send("HELLO|SHIELDDECK|1");
    lastHello_ = now;
  }
  leds_.set(4, connection_ == Connection::Ready || ((now / 500) % 2));
  if (connection_ == Connection::Ready) {
    eligiblePresses_ |= presses;
    for (uint8_t i = 0; i < 3; ++i) {
      const uint8_t mask = 1U << i;
      if ((presses & mask) && !pendingButton_) {
        pulse(i + 1, 60, now);
        show('b', i + 1, 500, now);
      }
      if ((releases & mask) && (eligiblePresses_ & mask)) {
        eligiblePresses_ &= uint8_t(~mask);
        sendButton(i + 1, now);
      }
    }
    if (shownFor_ && uint32_t(now - shownAt_) >= shownFor_) {
      const uint8_t success = successAfterPress_;
      if (success) show('0', success, 500, now);
      else if (pendingButton_) show('b', pendingButton_, 0, now);
      else show('P', 1, 0, now);
    }
  } else eligiblePresses_ = 0;
  for (uint8_t i = 0; i < 3; ++i) {
    if (pulseFor_[i] && uint32_t(now - pulseAt_[i]) >= pulseFor_[i]) {
      leds_.set(i + 1, false);
      pulseFor_[i] = 0;
    }
  }
  serial_.flushOutput();
}
