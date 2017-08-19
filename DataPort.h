#ifndef _DATAPORT_h
#define _DATAPORT_h

#include "Arduino.h"
#include "WiFi.h"
#include "TypedQueue.h"
#include <vector>
#include <algorithm>

typedef String CommandCallbackType(String);

class DataPort {
 private:
   WiFiServer m_server;
   uint m_port;
   unsigned long m_lastMillis = 0;
   std::vector<WiFiClient> m_clients;
   bool m_initialized = false;
   TypedQueue<String> m_queue;
   bool m_enabled = false;
   void Dispatch(String data);

 public:
   DataPort();
   void Begin(uint port);
   bool Handle(CommandCallbackType* callback);
   void AddPayload(String payload);
   uint GetPort();
   bool IsEnabled();
};

#endif

