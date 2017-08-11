#ifndef _OTAUPDATE_H
#define _OTAUPDATE_H

#include "Arduino.h"
#include "WiFi.h"
#include "ESP32WebServer.h"
#include "Update.h"

class OTAUpdate {
public:
  typedef void OtaStartCallbackType();

  OTAUpdate();
  void Begin(ESP32WebServer *server, OtaStartCallbackType otaCallback);
  void Handle();

private:
  ESP32WebServer *m_server;
  String m_log;
  OtaStartCallbackType *m_otaStartCallback;
  void Log(String text, bool newLine = true);
};


#endif

