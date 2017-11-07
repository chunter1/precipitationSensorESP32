#include "WebFrontend.h"
#include "Settings.h"
#include "StateManager.h"
#include "Help.h"
#include "Tools.h"
#include "GlobalDefines.h"

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

String WebFrontend::GetBinGroupRow(byte nbr, uint16_t lastBin) {
  String result = "";
  String bgtKey = "BG" + String(nbr) + "T";
  
  result += "<tr><td><label>Bin group ";
  result += String(nbr);
  result += ":</label></td><td><label>";
  if (nbr == 0) {
    result += "1 ";
  }
  result += "to:&nbsp;</label><input name='" + bgtKey + "' size='8' maxlength='3' Value='";
  result += m_settings->Get(bgtKey, String(defaultBinGroupBoundary[nbr]));

  result += F("'></input></td></tr>");

  return result;
}

String GetOption(String option, String defaultValue) {
  String result = "";

  result += F("<option value='");
  result += option;
  if (defaultValue == option) {
    result += F("' selected>");
  }
  else {
    result += F("'>");
  }
  result += option;
  result += F("</option>");

  return result;
}


void WebFrontend::Begin(StateManager *stateManager, BME280 *bme280) {
  m_stateManager = stateManager;
  m_bme280 = bme280;

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

  m_webserver.on("/help", [this]() {
    if (IsAuthentified()) {
      String result;

      m_webserver.setContentLength(CONTENT_LENGTH_UNKNOWN);
      m_webserver.send(200);

      result += GetTop();
      result += GetNavigation();
      result += FPSTR(help);

      result += "<h3>Bin speeds</h3>";
      result += "<style>table{border-collapse: collapse; width: 100 % ;}td, th{border: 1px solid #dddddd;text - align: left;padding: 8px;}</style>";
      result += "<Table>";
      uint angle = m_settings->GetUInt("SMA", 45);
      for (float i = 1; i < m_settings->BaseData.NrOfBins; i++) {
        float freq = i * 20.0;
        float mps = ((freq * (0.3 / 24.15)) / 2.0) / cos((PI * angle) / 180);

        result += "<tr>";
        result += "<td> Bin " + String(i, 0);
        result += "<td>" + String(freq, 0) + " Hz</td>";
        result += "<td>" + String(mps, 2) + " m/s</td>";
        result += "<td>" + String(mps * 3.6, 1) + " km/h</td>";
        result += "</tr>";

        if ((int)i % 32 == 0) {
          m_webserver.sendContent(result);
          result = "";
        }

      }
      result += "</Table>";

      result += GetBottom();
      m_webserver.sendContent(result);

      m_webserver.client().stop();
    }
  });

  m_webserver.on("/hardware", [this]() {
    if (IsAuthentified()) {
      String result;
      result += GetTop();
      result += GetNavigation();
      result += "<br><table cellspacing='3'>"; 
      result += BuildHardwareRow("ESP32:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;", "present&nbsp;:-)", "SDK:&nbsp;" + String(ESP.getSdkVersion()) + "&nbsp;&nbsp;free heap:&nbsp;" + String(ESP.getFreeHeap()));
      result += BuildHardwareRow("WiFi:", String(WiFi.RSSI()) + "&nbsp;dBm", "Mode: " + WifiModeToString(WiFi.getMode()) + "&nbsp;&nbsp;&nbsp;Time to connect: " + String(m_stateManager->GetWiFiConnectTime(), 1) + " s");
      result += BuildHardwareRow("Chip ID:", Tools::GetChipId());
      result += BuildHardwareRow("Chip Revision:", Tools::GetChipRevision());

      String bme = "missing";
      if (m_bme280->IsPresent()) {
        m_bme280->Measure();
        bme = "T:" + String(m_bme280->Values.Temperature, 1);
        bme += " H:" + String(m_bme280->Values.Humidity);
        bme += " P:" + String(m_bme280->Values.Pressure);
      }
      result += BuildHardwareRow("BME280:", bme);

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

      // Check the bin numbers if they are valid
      for (byte nbr = 0; nbr < m_settings->BaseData.NrOfBinGroups; nbr++) {
        String bgT = m_webserver.arg("BG" + String(nbr) + "T");
        long bgTI = bgT.toInt();

        String all = bgT;
        bool noDigit = false;
        for (byte c = 0; c < all.length(); c++) {
          if(!isDigit(all[c])) {
            noDigit = true;
            break;
          }
        }
        if (noDigit || bgT.length() == 0 || bgTI < 1 || bgTI > 511) {
          saveIt = false;
          String content = GetTop();
          content += F("<div align=center>");
          content += F("<br><br><h2><font color='red'>");
          content += F("Bin numbers must be in the range 1 ... 511</font></h2>");
          content += F("</div>");
          content += GetBottom();
          m_webserver.send(200, "text/html", content);
          break;
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
      ////m_settings->Clear();
      ////m_settings->Read();
 
      m_webserver.setContentLength(CONTENT_LENGTH_UNKNOWN);
      m_webserver.send(200, "text/html", "");

      m_webserver.sendContent(GetTop() + GetNavigation());

      bool hasNoConfiguration = m_settings->Get("PublishInterval", "???") == "???";

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
      data += m_settings->Get("HostName", "PrecipitationSensor");
      data += F("'><label>&nbsp;&nbsp;Startup-delay (s): </label><input name='StartupDelay' size='3' maxlength='2' Value='");
      data += m_settings->Get("StartupDelay", "5");
      data += F("'> </td></tr>");

      // FHEM connection
      data += F("<tr><td></td><td><br>Connection to FHEM</td></tr>");
      data += F("<tr><td><label>FHEM-Address: </label></td><td><input name='fhemIP' size='27' maxlength='15' Value='");
      data += m_settings->Get("fhemIP", "");
      data += F("'><label>&nbsp;&nbsp;Port: </label><input name='fhemPort' size='12' maxlength='5' Value='");
      data += m_settings->Get("fhemPort", "8083");
      data += F("'></td></tr>");

      data += F("<tr><td><label>Dummy prefix:</label></td><td><input name='DPR' size='30' maxlength='25' Value='");
      data += m_settings->Get("DPR", "PRECIPITATION_SENSOR");
      data += F("'></td></tr>");

      // Measurement settings
      data += F("<tr><td></td><td><br>Measurement options</td></tr>");
      data += F("<tr><td> <label>ADC Pin:</label></td><td>");
      data += F("<select name='ADCPIN' style='width:50px'>");
      String value = m_settings->Get("ADCPIN", "33");
      data += GetOption("32", value);
      data += GetOption("33", value);
      data += GetOption("34", value);
      data += GetOption("35", value);
      data += GetOption("36", value);
      data += GetOption("37", value);
      data += GetOption("38", value);
      data += GetOption("39", value);
      data += F("</select>&nbsp;&nbsp;");
      data += F("<tr><td> <label>Mounting angle: </label></td><td><input name='SMA' size='5' maxlength='3' Value='");
      data += m_settings->Get("SMA", "0");
      data += F("'><label>&nbsp;degree&nbsp;(0 is facing up-/downward)</td></tr>");
      data += F("<tr><td> <label>Publish interval: </label></td><td><input name='PublishInterval' size='5' maxlength='4' Value='");
      data += m_settings->Get("PublishInterval", "60");
      data += F("'><label>&nbsp;&nbsp;Threshold Factor: </label><input name='ThresholdFactor' size='10' maxlength='6' Value='");
      data += m_settings->Get("ThresholdFactor", "2");
      data += F("'><tr><td> <label>Altitude: </label></td><td><input name='Altitude' size='5' maxlength='5' Value='");
      data += m_settings->Get("Altitude", "0");
      data += F("'></td></tr>");

      // Data port
      data += F("<tr><td><label>Publish:</label></td><td>");
      data += F("Data port for FHEM is 81");
      data += F("</td></tr>");

      // Flags
      data += F("<tr><td><label>Optional data: </label></td><td>");
      data += F("<input name='PubBM' type='checkbox' value='true' ");
      data += m_settings->GetBool("PubBM", false) ? "checked" : "";
      data += F(">Bins mag&nbsp;&nbsp;&nbsp;");

      data += F("<input name='PubBG' type='checkbox' value='true' ");
      data += m_settings->GetBool("PubBG", false) ? "checked" : "";
      data += F(">Bin groups&nbsp;&nbsp;&nbsp;");

      data += F("<input name='PubBMA' type='checkbox' value='true' ");
      data += m_settings->GetBool("PubBMA", false) ? "checked" : "";
      data += F(">Bins MA&nbsp;&nbsp;&nbsp;");

      data += F("<input name='PubBMAK' type='checkbox' value='true' ");
      data += m_settings->GetBool("PubBMAK", false) ? "checked" : "";
      data += F(">Bins MAK&nbsp;&nbsp;&nbsp;");

      data += F("<input name='PubGCAL' type='checkbox' value='true' ");
      data += m_settings->GetBool("PubGCAL", false) ? "checked" : "";
      data += F(">Groups Cal&nbsp;&nbsp;&nbsp;");

      data += F("</td></tr>");

      // Bin group boundaries
      data += F("<tr><td></td><td><br>Bin group boundaries</td></tr>");
      for (byte i = 0; i < m_settings->BaseData.NrOfBinGroups; i++) {
        data += GetBinGroupRow(i, 256);
      }



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
  result += F("<a href='help'>Help</a>&nbsp;&nbsp;");
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



