#include "Watchdog.h"

void Watchdog::Begin(byte pin, uint interval) {
  m_pin = pin;
  m_interval = interval * 500;
  m_lastToggle = 0;
  
  pinMode(m_pin, OUTPUT);
  digitalWrite(m_pin, HIGH);
  delay(50);
  digitalWrite(m_pin, LOW);

  m_state = true;
}

void Watchdog::Handle() {
  if (millis() < m_lastToggle) {
    m_lastToggle = 0;
  }
  
  if (millis() > m_lastToggle + m_interval) {
    m_state = !m_state;
    digitalWrite(m_pin, m_state);
    
    m_lastToggle = millis();
  }
  
}

