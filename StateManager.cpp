#include "StateManager.h"
#include "WiFi.h"

StateManager::StateManager() {
  m_roolOverIsPossible = false;
  m_uptimeDays = 0;
}

void StateManager::Begin(String version, String identity) {
  m_version = version;
  m_identity = identity;
}

void StateManager::SetLoopStart() {
  m_loopStartTime = micros();
}

void StateManager::SetLoopEnd() {
  m_loopCount++;
  uint32_t currentLoopTime = micros() - m_loopStartTime;
  m_loopTotalTime += currentLoopTime;

  if (currentLoopTime < m_loopMinTime) {
    m_loopMinTime = currentLoopTime;
  }
  if (currentLoopTime > m_loopMaxTime) {
    m_loopMaxTime = currentLoopTime;
  }

  if (millis() < m_loopMeasureStart) {
    m_loopMeasureStart = 0;
  }
  if (millis() > m_loopMeasureStart + 5000) {
    word loopAverage = m_loopTotalTime / m_loopCount;
    m_loopDurationMin = m_loopMinTime;
    m_loopDurationAvg = loopAverage;
    m_loopDurationMax = m_loopMaxTime;
    m_loopTotalTime = 0;
    m_loopCount = 0;
    m_loopMinTime = 64000;
    m_loopMaxTime = 0;
    m_loopMeasureStart = millis();
  }

}

void StateManager::SetHostname(String hostname){
  m_hostname = hostname;
}

String StateManager::GetHostname(){
  return m_hostname;
}

void StateManager::Handle(){
  unsigned long currentMillis = millis();
  
  // Handle 50 Days rollover for UpTime
  if(currentMillis >= 3000000000) { 
    m_roolOverIsPossible = true;
  }
  if(currentMillis <= 100000 && m_roolOverIsPossible) {
    m_uptimeDays += 50;
    m_roolOverIsPossible = false;
  }
}

String StateManager::GetVersion() {
  return m_version;
}

String StateManager::GetUpTime() {
  return m_values.Get("UpTimeText","");
}

void StateManager::Update() {
  m_values.Clear();

  unsigned long upTimeSeconds = millis() / 1000;
  String upTimeText(m_uptimeDays + upTimeSeconds / 86400);
  upTimeText += "Tg. ";
  upTimeText += String(upTimeSeconds / 3600 % 24);
  upTimeText += "Std. ";
  upTimeText += String((upTimeSeconds / 60) % 60);
  upTimeText += "Min. ";
  upTimeText += String(upTimeSeconds % 60);
  upTimeText += "Sek. ";

  
  m_values.Put("UpTimeSeconds", String(upTimeSeconds));
  m_values.Put("UpTimeText", upTimeText);
  m_values.Put("WIFI", WiFi.SSID());
  m_values.Put("MacAddress", WiFi.macAddress());
  m_values.Put("RSSI", WiFi.getMode() == WiFiMode_t::WIFI_OFF ? "Off" : String(WiFi.RSSI()));
  m_values.Put("FreeHeap", String(ESP.getFreeHeap()));
  m_values.Put("Version", GetVersion());
  m_values.Put("LD.Min (ms)", String((float)m_loopDurationMin / 1000.0));
  m_values.Put("LD.Avg (ms)", String((float)m_loopDurationAvg / 1000.0));
  m_values.Put("LD.Max (ms)", String((float)m_loopDurationMax / 1000.0));

}

String StateManager::GetHTML() {
  Update();
  String result = "<style>table{border:1px solid black;border-collapse:collapse;}tr,td{padding:4px;border:1px solid; Margin:5px}</style>";
  
  result += "<table>";
  for (uint i = 0; i < m_values.Size(); i++) {
    result += "<tr><td>";
    result += m_values.GetKeyAt(i);
    result += ": </td><td>";
    result += m_values.GetValueAt(i);
    result += "</td></tr>";
  }

  return result;
}

String StateManager::GetXML() {
  Update();
  String result = "";

  result += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  result += "<LGW>";
  for (uint i = 0; i < m_values.Size(); i++) {
    result += "<Info Key = \"";
    result += m_values.GetKeyAt(i);
    result += "\" Value=\"";
    result += m_values.GetValueAt(i);
    result += "\"/>";
  }
  result += "</LGW>";
  
  return result;
}

void StateManager::SetWiFiConnectTime(float connectTime) {
  m_wifiConnnectTime = connectTime;
}

float StateManager::GetWiFiConnectTime() {
  return m_wifiConnnectTime;
}
