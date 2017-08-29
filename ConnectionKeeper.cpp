#include "ConnectionKeeper.h"
#include "WiFi.h"

void ConnectionKeeper::Begin(CriticalActionCallbackType *criticalActionCallback) {
  m_criticalActionCallback = criticalActionCallback;
  m_isConnected = WiFi.status() == WL_CONNECTED;
}

bool ConnectionKeeper::IsConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void ConnectionKeeper::Handle() {
  if (!IsConnected()) {
    if (m_criticalActionCallback) {
      m_criticalActionCallback(true);
    }
   
    if (m_isConnected) {
      Serial.println("WiFi CONNECTION LOST");
      m_isConnected = false;
    }

    Serial.print("Try reconnect: ");
    byte retryCounter = 0;
    WiFi.reconnect();
    while (retryCounter < 20 && !IsConnected()) {
      retryCounter++;
      Serial.print(String(WiFi.status()) + " ");
      delay(500);
    }
    Serial.println();

    if (IsConnected()) {
      m_isConnected = true;
      Serial.println("Reconnect OK");
    }
    else {
      Serial.println("STILL NO CONNECTION");
    }

    delay(1000);

    if (m_criticalActionCallback) {
      m_criticalActionCallback(false);
    }
  }

}

