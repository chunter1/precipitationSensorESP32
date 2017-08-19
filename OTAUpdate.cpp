#include "OTAUpdate.h"
#include <ArduinoOTA.h>
#include "Tools.h"

OTAUpdate::OTAUpdate() {
  m_log = "";
}

void OTAUpdate::Log(String text, bool newLine) {
  m_log += text + (newLine ? "\n" : "");
  Serial.print(text + (newLine ? "\r\n" : ""));
}

static ESP32WebServer *webServer;
void OTAUpdate::Begin(ESP32WebServer *server, OtaStartCallbackType otaStartCallback) {
  // === Push ====================================================================================
  m_server = server;
  m_otaStartCallback = otaStartCallback;
  m_log = "";
  webServer = server;
  // Push 
  m_server->on("/ota/firmware.bin", HTTP_POST, [this]() {
    m_server->sendHeader("Connection", "close");
    m_server->sendHeader("Access-Control-Allow-Origin", "*");
    Log("");
    Log(Update.hasError() ? "ERROR: OTA update failed" : "OTA update finished");
    m_server->send(200, "text/plain", m_log);
    delay(2000);
    ESP.restart();
  }, [this]() {
    HTTPUpload &upload = m_server->upload();
    if (upload.status == UPLOAD_FILE_START) {
      if (m_otaStartCallback != nullptr) {
        m_otaStartCallback();
      }
      m_log = "";
      Log("Start receiving '", false);
      Log(upload.filename, false);
      Log("'");
      if (!Update.begin()){
        Update.printError(Serial);
        Log("ERROR: UPLOAD_FILE_START");
      }
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize){
        Update.printError(Serial);
        Log("ERROR: UPLOAD_FILE_WRITE");
      }
    }
    else if (upload.status == UPLOAD_FILE_END){
      if (Update.end(true)) { 
        Log("Firmware size: ", false);
        Log(String(upload.totalSize));
        Log("Rebooting ESP32 ...");
      }
      else {
        Update.printError(Serial);
        Log("ERROR: UPLOAD_FILE_END");
      }
    }
    yield();
  });

  // === Pull ====================================================================================
  ArduinoOTA.setHostname(String("precipitationSensor_" + Tools::GetChipId()).c_str());
  ArduinoOTA.onStart([this]() { 
    if (m_otaStartCallback != nullptr) {
      m_otaStartCallback();
    }
  });
  ArduinoOTA.onEnd([]() {});
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {});
  ArduinoOTA.onError([](ota_error_t error) {});
  ArduinoOTA.begin();
  
}

void OTAUpdate::Handle() {
  ArduinoOTA.handle();
}

