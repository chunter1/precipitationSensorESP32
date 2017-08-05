#ifndef _WEBFRONTEND_h
#define _WEBFRONTEND_h

#include "Arduino.h"
#include "WiFi.h"
#include "ESP32WebServer.h"
#include "StateManager.h"
#include "Settings.h"

class WebFrontend {
 public:
   WebFrontend(int port, Settings *settings);
   void Handle();
   void Begin(StateManager *stateManager);
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
  String GetBinGroupRow(byte nbr);
};

#endif

