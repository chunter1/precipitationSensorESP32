#include "WebFrontend.h"
#include "Settings.h"
#include "StateManager.h"

WebFrontend::WebFrontend(int port, Settings *settings) : m_webserver(port) {
  m_port = port;
  m_password = "";
  m_commandCallback = nullptr;
  m_hardwareCallback = nullptr;
  m_settings = settings;
}

void WebFrontend::Handle() {
  m_webserver.handleClient();
}

String WifiModeToString(WiFiMode_t mode) {
  switch (mode) {
  case WiFiMode_t::WIFI_AP:
    return "Accespoint";
    break;
  case WiFiMode_t::WIFI_AP_STA:
    return "Accespoint + Station";
    break;
  case WiFiMode_t::WIFI_OFF:
    return "Off";
    break;
  case WiFiMode_t::WIFI_STA:
    return "Station";
    break;
  default:
    return "";
    break;
  }
}

ESP32WebServer *WebFrontend::GetWebServer() {
  return &m_webserver;
}

void WebFrontend::SetCommandCallback(CommandCallbackType callback) {
  m_commandCallback = callback;
}

void WebFrontend::SetHardwareCallback(HardwareCallbackType callback) {
  m_hardwareCallback = callback;
}

void WebFrontend::SetPassword(String password) {
  m_password = password;
}

bool WebFrontend::IsAuthentified() {
  bool result = false;
  if (m_password.length() > 0) {
    if (m_webserver.hasHeader("Cookie")) {
      String cookie = m_webserver.header("Cookie");
      if (cookie.indexOf("ESPSESSIONID=1") != -1) {
        result = true;
      }
    }
    if (!result) {
      String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
      m_webserver.sendContent(header);
    }
  }
  else {
    result = true;
  }

  return result;
}

void WebFrontend::Begin(StateManager *stateManager) {
  m_stateManager = stateManager;

  const char *headerKeys[] = { "User-Agent", "Cookie" };
  m_webserver.collectHeaders(headerKeys, sizeof(headerKeys) / sizeof(char*));

  m_webserver.on("/", [this]() {
    if (IsAuthentified()) {
      String result;
      result += GetTop();
      result += GetNavigation();
      result += F("<br>");
      result += m_stateManager->GetHTML();
      result += GetBottom();
      m_webserver.send(200, "text/html", result);
    }
  });

  m_webserver.on("/reset", [this]() {
    if (IsAuthentified()) {
      m_webserver.send(200, "text/html", GetRedirectToRoot());
      delay(1000);
      ESP.restart();
    }
  });

  m_webserver.on("/state", [this]() {
    String result;
    result += m_stateManager->GetXML();
    m_webserver.send(200, "text/xml", result);
  });

  m_webserver.on("/hardware", [this]() {
    if (IsAuthentified()) {
      String result;
      result += GetTop();
      result += GetNavigation();
      result += "<br><table cellspacing='3'>"; 
      result += BuildHardwareRow("ESP32&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;", "present&nbsp;:-)", "SDK:&nbsp;" + String(ESP.getSdkVersion()) + "&nbsp;&nbsp;free heap:&nbsp;" + String(ESP.getFreeHeap()));
      result += BuildHardwareRow("WiFi", String(WiFi.RSSI()) + "&nbsp;dBm", "Mode: " + WifiModeToString(WiFi.getMode()) + "&nbsp;&nbsp;&nbsp;Time to connect: " + String(m_stateManager->GetWiFiConnectTime(), 1) + " s");
      if (m_hardwareCallback != nullptr) {
        String rawData = m_hardwareCallback();
        result += "<tr><td>";
        rawData.replace(" ", "&nbsp;");
        rawData.replace("\t", "</td><td>");
        rawData.replace("\n", "</td></tr><tr><td>");

        result += rawData;
        result += "</td></tr></table>";
      }

      result += GetBottom();
      m_webserver.send(200, "text/html", result);
    }
  });

  m_webserver.on("/save", [this]() {
    if (IsAuthentified()) {
      m_settings->Clear();

      for (byte i = 0; i < m_webserver.args(); i++) {
        m_settings->Add(m_webserver.argName(i), m_webserver.arg(i));
      }

      bool saveIt = true;
      if (m_webserver.hasArg("FrontPass1") && m_webserver.hasArg("FrontPass2")) {
        String fp1 = m_webserver.arg("FrontPass1");
        String fp2 = m_webserver.arg("FrontPass2");
        if (!fp1.equals(fp2)) {
          String content = GetTop();
          content += F("<div align=center>");
          content += F("<br><br><h2><font color='red'>");
          content += F("Passwords do nat match</font></h2>");
          content += F("</div>");
          content += GetBottom();
          m_webserver.send(200, "text/html", content);
          saveIt = false;
        }
      }

      if (m_webserver.hasArg("HostName")) {
        String hostname = m_webserver.arg("HostName");
        for (byte i = 0; i < hostname.length(); i++) {
          char ch = (char)hostname[i];
          if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '-' || ch == '_')) {
            saveIt = false;
            String content = GetTop();
            content += F("<div align=center>");
            content += F("<br><br><h2><font color='red'>");
            content += F("Allowed characters for hostname: 0...9, a...z, A...Z, - and _</font></h2>");
            content += F("</div>");
            content += GetBottom();
            m_webserver.send(200, "text/html", content);
            break;
          }

        }
      }
      
      if (saveIt) {
        String info = m_settings->Write();
        m_webserver.send(200, "text/html", GetRedirectToRoot("Settings saved<br>" + info));
        delay(1000);
        ESP.restart();
      }
    }
  });

  m_webserver.on("/setup", [this]() {
    if (IsAuthentified()) {
      String result;
      m_settings->Clear();
      m_settings->Read();
 
      m_webserver.setContentLength(CONTENT_LENGTH_UNKNOWN);
      m_webserver.send(200, "text/html", "");

      m_webserver.sendContent(GetTop() + GetNavigation());

      String data;
      data += F("<form method='get' action='save'><table>");
      data += F("<tr><td></td><td>Third parameter is the timeout. After timeout seconds, it will try to connect to SSID2 (if defined)</td></tr>");

      // ctSSID1 ctPASS1
      data += F("<tr><td><label>SSID / Password: </label></td><td><input name='ctSSID1' size='44' maxlength='32' Value='");
      data += m_settings->Get("ctSSID1", "");
      data += F("'>");
      data += F("&nbsp;&nbsp;<input type='password' name='ctPASS1' size='54' maxlength='63' Value='");
      data += m_settings->Get("ctPASS1", "");
      data += F("'>&nbsp;&nbsp;<input name='ctTO1' size='6' maxlength='4' Value='");
      data += m_settings->Get("ctTO1", "15");
      data += F("'> </td></tr>");
      
      // ctSSID2 ctPASS2
      data += F("<tr><td><label>SSID2 / Password2: </label> </td> <td> <input name='ctSSID2' size='44' maxlength='32' Value='");
      data += m_settings->Get("ctSSID2", "");
      data += F("'>");
      data += F("&nbsp;&nbsp;<input type='password' name='ctPASS2' size='54' maxlength='63' Value='");
      data += m_settings->Get("ctPASS2", "");
      data += F("'>&nbsp;&nbsp;<input name='ctTO2' size='6' maxlength='4' Value='");
      data += m_settings->Get("ctTO2", "15");
      data += F("'> </td></tr>");
      
      // Frontend Password
      data += F("<tr><td><label>Frontend password: </label> </td> <td>");
      data += F("<input name='FrontPass1' type='password' size='30' maxlength='60' Value='");
      data += m_settings->Get("FrontPass1", "");
      data += F("'> Retype: ");
      data += F("<input name='FrontPass2' type='password' size='30' maxlength='60' Value='");
      data += m_settings->Get("FrontPass2", "");
      data += F("'> (if empty, no login is required)</td></tr>");

      data += F("<tr><td></td><td><br>DHCP will be used, in case of one of the fields IP, mask or gateway remains empty</td></tr>");

      // staticIP, staticMask, staticGW, 
      data += F("<tr> <td> <label>IP-Address: </label></td><td><input name='StaticIP' size='27' maxlength='15' Value='");
      data += m_settings->Get("StaticIP", "");
      data += F("'><label>&nbsp;&nbsp;Netmask: </label><input name='StaticMask' size='27' maxlength='15' Value='");
      data += m_settings->Get("StaticMask", "");
      data += F("'><label>&nbsp;&nbsp;Gateway: </label><input name='StaticGW' size='27' maxlength='15' Value='");
      data += m_settings->Get("StaticGW", "");
      data += F("'></td></tr>");

      // HostName, startup-delay
      data += F("<tr><td><label>Hostname: </label></td><td><input name='HostName' size='27' maxlength='63' Value='");
      data += m_settings->Get("HostName", "precipitationSensor");
      data += F("'> </td></tr>");

      // FHEM connection
      data += F("<tr><td></td><td><br>Connection to FHEM</td></tr>");
      data += F("<tr><td> <label>FHEM-Address: </label></td><td><input name='fhemIP' size='27' maxlength='15' Value='");
      data += m_settings->Get("fhemIP", "");
      data += F("'><label>&nbsp;&nbsp;Port: </label><input name='fhemPort' size='12' maxlength='5' Value='");
      data += m_settings->Get("fhemPort", "8083");
      data += F("'></td></tr>");

      // Measurement settings
      data += F("<tr><td></td><td><br>Measurement options</td></tr>");
      data += F("<tr><td> <label>Publish interval: </label></td><td><input name='PublishInterval' size='10' maxlength='4' Value='");
      data += m_settings->Get("PublishInterval", "60");
      data += F("'><label>&nbsp;&nbsp;Detection threshold: </label><input name='DetectionThreshold' size='10' maxlength='5' Value='");
      data += m_settings->Get("DetectionThreshold", "90");
      data += F("'><label>&nbsp;&nbsp;First bin: </label><input name='FirstBin' size='10' maxlength='5' Value='");
      data += m_settings->Get("FirstBin", "???");
      data += F("'><label>&nbsp;&nbsp;Last bin: </label><input name='LastBin' size='10' maxlength='5' Value='");
      data += m_settings->Get("LastBin", "???");
      data += F("'></td></tr>");


      // Save button
      data += F("</table><br><input type='submit' Value='Save and restart' ></form>");

      m_webserver.sendContent(data);
      m_webserver.sendContent(GetBottom());
      m_webserver.client().stop();
    }
  });
  
  m_webserver.on("/login", [this]() {
    String msg;
    if (m_webserver.hasArg("DISCONNECT")) {
      m_webserver.sendContent(F("HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=0\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n"));
      return;
    }
    if (m_webserver.hasArg("PASSWORD")) {
      if (m_webserver.arg("PASSWORD") == m_password) {
        m_webserver.sendContent(F("HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=1\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n"));
        return;
      }
      msg = "Login failed";
    }
    String content = F("<html><body><form action='/login' method='POST'>");
    content += F("<DIV ALIGN=CENTER>");
    content += F("<br><br><h2>precipitationSensor V");
    content += m_stateManager->GetVersion();
    content += F("</h2>");
    content += F("Password: <input type='password' name='PASSWORD'>&nbsp;&nbsp;");
    content += F("<input type='submit' name='SUBMIT' value='Login'></form>");
    content += F("<br><br><h2> <font color='red'>");
    content += msg;
    content += F("</font></h2>");
    content += F("</div>");
    content += F("</body></html>");
    m_webserver.send(200, "text/html", content);
  });

  m_webserver.onNotFound([this](){
    m_webserver.send(404, "text/plain", "Not Found");
  });

  m_webserver.begin();
}



String WebFrontend::GetNavigation() {
  String result = "";
  result += F("<a href='/'>Home</a>&nbsp;&nbsp;");
  result += F("<a href='setup'>Setup</a>&nbsp;&nbsp;");
  result += F("<a href='hardware'>Hardware</a>&nbsp;&nbsp;");
  if (m_password.length() > 0) {
    result += F("<a href='login?DISCONNECT=YES'>Logout</a>&nbsp;&nbsp;");
  }
  result += F("<a href='reset'>Reboot</a>");
  result += F("<br>");
  
  return result;
}

String WebFrontend::GetDisplayName() {
  String result;
  result += m_stateManager->GetHostname();
  result += " (";
  result += WiFi.localIP().toString();
  result += ")";
  return result;
}

String WebFrontend::GetTop() {
  String result;
  result += F("<!DOCTYPE HTML><html>");
  result += F("<meta charset='utf-8'/>");
  result += "<head><title>";
  result += GetDisplayName();
  result += "</title></head>";
  result += F("<p>precipitationSensor V");
  result += m_stateManager->GetVersion();
  result += "&nbsp;&nbsp;&nbsp;";
  result += GetDisplayName();
  result += F("</p>");
  return result;
}

String WebFrontend::GetBottom() {
  String result;
  result += F("</html>");
  return result;
}

String WebFrontend::GetRedirectToRoot(String message) {
  String result;
  result += F("<html><head><meta http-equiv='refresh' content='5; URL=/'></head><body>");
  result += message;
  result += F("<br><br>Reboot, please wait a moment ...</body></html>");
  return result;
}

String WebFrontend::BuildHardwareRow(String text1, String text2, String text3) {
  return "<tr><td>" + text1 + "</td><td>" + text2 + "</td><td>" + text3 + "</td></tr>";
}


