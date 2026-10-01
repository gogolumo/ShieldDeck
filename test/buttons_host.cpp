#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "DebouncedButton.h"

int main() {
  DebouncedButton button;
  button.begin(false, 0);
  assert(!button.update(false, 25));
  // Contact bounce, then a stable press: exactly one event.
  assert(!button.update(true, 30));
  assert(!button.update(false, 35));
  assert(!button.update(true, 40));
  assert(!button.update(true, 64));
  assert(button.update(true, 65));
  assert(!button.update(true, 10000));  // Hold does not repeat.
  // Release bounce must not rearm the button.
  assert(!button.update(false, 10001));
  assert(!button.update(true, 10005));
  assert(!button.update(true, 10030));
  assert(!button.update(false, 10040));
  assert(!button.update(false, 10065));
  assert(!button.update(true, 10070));
  assert(button.update(true, 10095));

  button.begin(true, 0);  // Boot while held.
  assert(!button.update(true, 1000));
  assert(!button.update(false, 1001));
  assert(!button.update(false, 1026));
  assert(!button.update(true, 1030));
  assert(button.update(true, 1055));

  button.begin(false, UINT32_MAX - 100);
  assert(!button.update(false, UINT32_MAX - 75));
  assert(!button.update(true, UINT32_MAX - 10));
  assert(!button.update(true, 13));
  assert(button.update(true, 14));  // millis() rollover.
  assert(!button.update(true, 15));
  puts("PASS: bounce, release rearm, hold, boot-held, millis rollover");
}
