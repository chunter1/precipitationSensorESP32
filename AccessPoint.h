#ifndef _ACCESSPOINT_h
#define _ACCESSPOINT_h

#include "Arduino.h"
#include "WiFi.h"

class AccessPoint {
 private:
   IPAddress m_ip;
   IPAddress m_gateway;
   IPAddress m_subnet;
   String m_ssidPrefix;
   bool m_running = false;
   int m_autoClose = 0;
   unsigned long m_startMillis = 0;
   
 public:
   AccessPoint(IPAddress ip, IPAddress gateway, IPAddress subnet, String ssidPrefix);
   void Begin(int autoClose = 0);
   void End();
   void Handle();

};





#endif

