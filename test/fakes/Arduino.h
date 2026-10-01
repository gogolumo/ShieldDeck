#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <sstream>

constexpr uint8_t LOW=0, HIGH=1, OUTPUT=1, INPUT_PULLUP=2, MSBFIRST=1;
constexpr uint8_t A1=15, A2=16, A3=17;
#define F(text) text
extern uint32_t fakeNow;
extern uint8_t fakePins[32];
inline uint32_t millis() { return fakeNow; }
inline uint32_t micros() { return fakeNow * 1000; }
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t pin, uint8_t value) { fakePins[pin] = value; }
inline int digitalRead(uint8_t pin) { return fakePins[pin]; }
inline void shiftOut(uint8_t, uint8_t, uint8_t, uint8_t) {}

struct FakeSerial {
  std::string incoming, outgoing;
  void begin(unsigned long) {}
  int available() { return int(incoming.size()); }
  int availableForWrite() { return 63; }
  char read() { char c=incoming[0]; incoming.erase(0,1); return c; }
  void write(char c) { outgoing += c; }
  template<class T> void print(T v) { std::ostringstream s; s << v; outgoing += s.str(); }
  void print(uint8_t v) { print(unsigned(v)); }
  template<class T> void println(T v) { print(v); println(); }
  void println() { outgoing += "\r\n"; }
};
extern FakeSerial Serial;
