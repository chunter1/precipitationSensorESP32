#include "AccessPoint.h"
#include "Tools.h"

AccessPoint::AccessPoint(IPAddress ip, IPAddress gateway, IPAddress subnet, String ssidPrefix) {
  m_ip = ip;
  m_gateway = gateway;
  m_subnet = subnet;
  m_ssidPrefix = ssidPrefix;
}

void AccessPoint::Begin(int autoClose) {
  Serial.println("Starting access point ...");

  WiFi.mode(WIFI_STA);
  String ssid = m_ssidPrefix + "-" + Tools::GetChipId();
  WiFi.softAPConfig(m_ip, m_ip, m_subnet);
  WiFi.softAP(ssid.c_str());
  Serial.println("running, SSID=" + ssid);

  m_autoClose = autoClose;
  m_startMillis = millis();
  m_running = true;
}

void AccessPoint::End() {
  Serial.println("Closing  access point");
  m_running = false;
  m_autoClose = 0;
  WiFi.mode(WIFI_MODE_STA);
  }

void AccessPoint::Handle() {
  if(m_autoClose > 0 && m_running) {
    if(millis() - m_startMillis > (uint)m_autoClose * 1000) {
      End();
    }
  } 
}
