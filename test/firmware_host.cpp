#include "AppController.h"
#include "ProtocolFields.h"
#include <cassert>
#include <iostream>

uint32_t fakeNow = 0;
uint8_t fakePins[32];
FakeSerial Serial;
AppController app;

void advance(uint32_t ms) {
  for (uint32_t i = 0; i < ms; ++i) { ++fakeNow; app.update(); }
}
std::string take() { std::string s=Serial.outgoing; Serial.outgoing.clear(); return s; }
std::string input(const std::string& line) {
  Serial.incoming += line + "\n";
  advance(30);
  return take();
}
void press(uint8_t pin) { fakePins[pin]=LOW; advance(30); }
void release(uint8_t pin) { fakePins[pin]=HIGH; advance(30); }
bool contains(const std::string& s, const char* text) { return s.find(text)!=std::string::npos; }

int main() {
  for (auto& pin : fakePins) pin=HIGH;
  app.begin(); advance(40);
  assert(contains(take(), "HELLO|SHIELDDECK|1"));
  press(A1); release(A1); assert(!contains(take(), "BTN|")); // Offline inputs ignored.
  assert(contains(input("WELCOME|1|ABCDEF12"), "READY|ABCDEF12"));
  press(A1);
  assert(contains(input("CONFIG|ABCDEF12|1|1|0|PULSE|PULSE|PULSE"), "REJECT|ABCDEF12|1|BUSY"));
  release(A1);
  assert(contains(input("CONFIG|ABCDEF12|1|1|0|PULSE|PULSE|PULSE"), "CONFIGURED|ABCDEF12|1"));
  assert(fakePins[13]==LOW); // LED4 connected, active-low.
  assert(contains(input("CONFIG|ABCDEF12|1|1|0|PULSE|PULSE|PULSE"), "CONFIGURED|ABCDEF12|1"));
  press(A1); advance(300); assert(!contains(take(), "BTN|"));
  release(A1); assert(contains(take(), "BTN|ABCDEF12|1|1|1|SHORT"));
  input("RESULT|DEADBEEF|1|1|OK|0");
  input("RESULT|ABCDEF12|1|9|OK|0");
  assert(!contains(input("CONFIG|ABCDEF12|2|1|0|PULSE|PULSE|PULSE"), "CONFIGURED"));
  input("ACK|ABCDEF12|1|1");
  input("RESULT|ABCDEF12|1|1|OK|0");
  assert(fakePins[10]==LOW);
  advance(180); assert(fakePins[10]==HIGH); // Momentary success, no fake state.
  input("RESULT|ABCDEF12|1|1|OK|0"); assert(fakePins[10]==HIGH); // Duplicate ignored.
  input("PING|ABCDEF12");
  press(A1); release(A1); assert(contains(take(), "|2|1|SHORT"));
  advance(1100); take();
  input("RESULT|ABCDEF12|1|2|OK|0"); assert(fakePins[10]==HIGH); // Late success ignored.
  // Oversize, control chars, empty field, and timed-out prefix cannot become a PING.
  assert(!contains(input(std::string(100, 'X')+"PING|ABCDEF12"), "PONG"));
  assert(!contains(input("PING||ABCDEF12"), "PONG"));
  assert(!contains(input(std::string("PING|ABCDEF12\0", 14)), "PONG"));
  Serial.incoming="PI"; advance(550);
  assert(!contains(input("NG|ABCDEF12"), "PONG"));
  assert(contains(input("PING|ABCDEF12\r"), "PONG|ABCDEF12"));
  advance(3100); assert(contains(take(), "HELLO|SHIELDDECK|1"));
  assert(fakePins[10]==HIGH && fakePins[11]==HIGH && fakePins[12]==HIGH && fakePins[3]==HIGH);
  assert(!contains(input("PING|ABCDEF12"), "PONG"));
  // A new session resets sequence but never imports old held input/result.
  press(A1);
  input("WELCOME|1|1234ABCD"); release(A1);
  assert(contains(input("CONFIG|1234ABCD|1|1|0|PULSE|PULSE|PULSE"), "CONFIGURED"));
  assert(!contains(take(), "BTN|"));
  press(A1); release(A1); assert(contains(take(), "BTN|1234ABCD|1|1|1|SHORT"));
  input("RESULT|1234ABCD|1|1|ERR|2"); assert(fakePins[10]==HIGH);
  uint16_t value;
  assert(!parseNumber("9999999999999999999999", 1, 65535, value));
  assert(!parseNumber("-1", 1, 65535, value));
  assert(parseNumber("65535", 1, 65535, value) && value==65535);
  std::cout << "PASS: firmware handshake, release-only, config busy, result matching, timeout, malformed frames, reconnect, momentary LEDs\n";
}
