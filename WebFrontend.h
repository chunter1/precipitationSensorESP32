#ifndef _WEBFRONTEND_h
#define _WEBFRONTEND_h

#include "Arduino.h"
#include "WiFi.h"
#include "ESP32WebServer.h"
#include "StateManager.h"
#include "Settings.h"
#include "BME280.h"

class WebFrontend {
 public:
   WebFrontend(int port, Settings *settings);
   void Handle();
   void Begin(StateManager *stateManager, BME280 *bme280);
   ESP32WebServer *GetWebServer();
   typedef void CommandCallbackType(String);
   typedef String HardwareCallbackType();
   void SetCommandCallback(CommandCallbackType callback);
   void SetHardwareCallback(HardwareCallbackType callback);
   void SetPassword(String password);

private:
  int m_port;
  ESP32WebServer m_webserver;
  StateManager *m_stateManager;
  BME280 *m_bme280;
  CommandCallbackType *m_commandCallback;
  HardwareCallbackType *m_hardwareCallback;
  String m_password;
  String GetNavigation();
  String GetTop();
  String GetBottom();
  String GetRedirectToRoot(String message = "");
  bool IsAuthentified();
  String GetDisplayName();
  String BuildHardwareRow(String text1, String text2, String text3 = "");
  Settings *m_settings;
  String GetBinGroupRow(byte nbr, uint16_t lastBin);
};

#endif

