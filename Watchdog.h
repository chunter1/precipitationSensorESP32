#ifndef __WATCHDOG__h
#define __WATCHDOG__h

#include "Arduino.h"

class Watchdog {
public:
  void Begin(byte pin, uint interval);
  void Handle();

private:
  byte m_pin;
  uint m_interval;
  bool m_state;
  unsigned long m_lastToggle;

};

#endif
