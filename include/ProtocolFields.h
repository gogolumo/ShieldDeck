#pragma once
#include <stdint.h>
#include <string.h>

// In-place bounded splitting; empty fields stay visible and invalidate the line.
inline uint8_t splitFields(char* line, char** fields, uint8_t capacity) {
  uint8_t count = 0;
  char* start = line;
  for (char* p = line;; ++p) {
    if (*p != '|' && *p != '\0') continue;
    if (p == start || count == capacity) return 0;
    fields[count++] = start;
    const bool done = *p == '\0';
    *p = '\0';
    if (done) return count;
    start = p + 1;
  }
}

inline bool validSession(const char* text) {
  if (strlen(text) != 8) return false;
  for (uint8_t i = 0; i < 8; ++i) {
    if (!((text[i] >= '0' && text[i] <= '9') || (text[i] >= 'A' && text[i] <= 'F'))) return false;
  }
  return true;
}

inline bool parseNumber(const char* text, uint16_t min, uint16_t max, uint16_t& out) {
  if (!*text) return false;
  uint32_t value = 0;
  for (; *text; ++text) {
    if (*text < '0' || *text > '9') return false;
    value = value * 10 + (*text - '0');
    if (value > max) return false;  // Bound before the next multiplication.
  }
  if (value < min) return false;
  out = uint16_t(value);
  return true;
}
