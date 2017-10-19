/***************************************************************************\
 * 
 * Arduino project "precipitationSensor" © Copyright huawatuam@gmail.com
 * 
 * This program is free software. You can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version. This program is distributed in
 * the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 * You received a copy of the GNU General Public License along with this
 * program in file 'License.txt'.
 * Additional information about licensing can be found at:
 * http://www.gnu.org/licenses
 * 
 * IDE download         : https://www.arduino.cc/en/Main/Software
 * FHEM forum thread    : https://forum.fhem.de/index.php/topic,73016.0.html
 * Contact              : huawatuam@gmail.com
 * 
 * Project description:
 * ====================
 * This project uses an radar sensor modul a preamp and an ESP32 to classify the type
 * of precipitation/hydrometeors by measuring the speed/doppler frequency.
 * 
 * The recommended radar sensor modules are the IPM-170 and RSM-1700 because of their
 * radial symmetric radiation pattern.
 * The IPM-165 or CDM-324 may be used instead, if direction independency does not matter.
 *
 * ADC sampling rate:     40960 sps (2x oversampling)
 * Sampling period:       50 ms
 * FFT points/samples:    1024
 * FFT resolution:        20 Hz
 * FFT frequency range:   DC...10220 Hz (limited by LPF to 8 kHz)
 * 
 * Arduino IDE board settings:
 * ===========================
 * ESP32 CPU clock speed: 80 MHz
 * 
 * Pins used:
 * ==========
 * 32 ... 39 Senor (ADC) (one of them, defined on the setup page)
 * 27        Watchdog
 * 21        SDA + 4k7 PU  BME280
 * 22        SCL + 4k7 PU  BME280
 *
 * Version history:
 * ================
 * See: https://github.com/chunter1/precipitationSensorESP32/commits/master
 *
 \***************************************************************************/

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include "GlobalDefines.h"
#include "StateManager.h"
#include "Settings.h"
#include "WebFrontend.h"
#include "AccessPoint.h"
#include "Tools.h"
#include "Watchdog.h"
#include <ArduinoOTA.h>
#include "OTAUpdate.h"
#include "Update.h"
#include "Publisher.h"
#include "SensorData.h"
#include "DataPort.h"
#include "Statistics.h"
#include "SigProc.h"
#include "ConnectionKeeper.h"
#include "Wire.h"
#include "BME280.h"

StateManager stateManager;
Settings settings;
WebFrontend frontend(80, &settings);
AccessPoint accessPoint(IPAddress(192, 168, 222, 1), IPAddress(192, 168, 222, 1), IPAddress(255, 255, 225, 0), "ps");
Watchdog watchdog;
OTAUpdate ota;
Publisher publisher;
SensorData sensorData(NR_OF_BINS, NR_OF_BIN_GROUPS);
DataPort dataPort;
Statistics statistics;
SigProc sigProc;
ConnectionKeeper connectionKeeper;
BME280 bme280;

float thresholdFactor;
byte adcPin;
//uint mountingAngle;

void TryConnectWIFI(String ctSSID, String ctPass, byte nbr, uint timeout, String staticIP, String staticMask, String staticGW, String hostName) {
  if (ctSSID.length() > 0 && ctSSID != "---") {
    unsigned long startMillis = millis();

    WiFi.disconnect(true);
    WiFi.begin(ctSSID.c_str(), ctPass.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(false);

    if (staticIP.length() > 7 && staticMask.length() > 7) {
      if (staticGW.length() < 7) {
        WiFi.config(Tools::IPAddressFromString(staticIP), Tools::IPAddressFromString(staticGW), (uint32_t)0);
      }
      else {
        WiFi.config(Tools::IPAddressFromString(staticIP), Tools::IPAddressFromString(staticGW), Tools::IPAddressFromString(staticMask));
      }
    }

    delay(50);
    Serial.println("Hostname: " + hostName);
    WiFi.setHostname(hostName.c_str());

    Serial.println("Connect " + String(timeout) + " seconds to an AP (SSID " + String(nbr) + ")");
    digitalWrite(DEBUG_GPIO_ISR, HIGH);
    byte retryCounter = 0;
    while (retryCounter < timeout * 2 && WiFi.status() != WL_CONNECTED) {
      retryCounter++;
      delay(500);
      Serial.print(".");
      digitalWrite(DEBUG_GPIO_ISR, retryCounter % 2 == 0);
      watchdog.Handle();
    }

    if (WiFi.status() == WL_CONNECTED) {
      stateManager.SetWiFiConnectTime((millis() - startMillis) / 1000.0);
    }

    digitalWrite(DEBUG_GPIO_ISR, LOW);
  }

}

static bool StartWifi(Settings *settings) {
  bool result = false;

  Serial.println("Start WIFI_STA");

  String hostName = settings->Get("HostName", "precipitationSensor");
  Serial.print("HostName is: ");
  Serial.println(hostName);
  stateManager.SetHostname(hostName);

  String staticIP = settings->Get("StaticIP", "");
  String staticMask = settings->Get("StaticMask", "");
  String staticGW = settings->Get("StaticGW", "");

  if (staticIP.length() < 7 || staticMask.length() < 7) {
    Serial.println("Using DHCP");
  }
  else {
    Serial.println("Using static IP");
    Serial.print("IP: ");
    Serial.println(staticIP);
    Serial.print("Mask: ");
    Serial.println(staticMask);
    Serial.print("Gateway: ");
    Serial.println(staticGW);
  }

  TryConnectWIFI(settings->Get("ctSSID1", DEFAULT_SSID), settings->Get("ctPASS1", DEFAULT_PASSWORD), 1, settings->GetInt("ctTO1", 15), staticIP, staticMask, staticGW, hostName);
  String ctSSID2 = settings->Get("ctSSID2", "---");
  if (WiFi.status() != WL_CONNECTED && ctSSID2.length() > 0 && ctSSID2 != "---") {
    TryConnectWIFI(ctSSID2, settings->Get("ctPASS2", "---"), 2, settings->GetInt("ctTO2", settings->GetInt("ctTO2", 15)), staticIP, staticMask, staticGW, hostName);
  }

  if (WiFi.status() == WL_CONNECTED) {
    result = true;
    Serial.println();
    Serial.println("connected :-)");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP: ");
    Serial.println(WiFi.localIP().toString());
  }
  else {
    Serial.println();
    Serial.println("We got no connection :-(");
    // Open access point for 15 minutes
    accessPoint.Begin(900);
  }

  Serial.println("Starting frontend");
  frontend.SetPassword(settings->Get("FrontPass1", ""));
  frontend.Begin(&stateManager, &bme280);
  
  Serial.println("Starting OTA");
  ota.Begin(frontend.GetWebServer(), [] {
    sigProc.StopCapture();
  });

  Serial.println("Starting data port");
  dataPort.Begin(81);

  return result;
}

void HandleCriticalAction(bool isCritical) {
  if (isCritical) {
    sigProc.StopCapture();
  }
  else {
    sigProc.StartCapture();
  }
}

void setup() {
  uint32_t startProcessTS;
  
  Serial.begin(115200);
  delay(50);
  Serial.println();

  Serial.println("Chip ID: " + Tools::GetChipId());
  Serial.println("Chip Revision: " + Tools::GetChipRevision());

  watchdog.Begin(27, 1);
  // Get the settings
  settings.Begin([](bool isCritical) {
    HandleCriticalAction(isCritical);
  });
  settings.BaseData.NrOfBins = NR_OF_BINS;
  settings.BaseData.NrOfBinGroups = NR_OF_BIN_GROUPS;
  settings.Read();

  // Get the measurement settings
  thresholdFactor = settings.GetFloat("ThresholdFactor", DEFAULT_THRESHOLD_FACTOR);
  //mountingAngle = settings.GetUInt("SMA", 45);

  // Get the group-boundaries from the settings
  for (byte nbr = 0; nbr < settings.BaseData.NrOfBinGroups; nbr++) {
    byte groupSize = settings.BaseData.NrOfBins / settings.BaseData.NrOfBinGroups;
    sensorData.binGroup[nbr].firstBin = settings.GetUInt("BG" + String(nbr) + "F", nbr == 0 ? 1 : (nbr * groupSize));
    sensorData.binGroup[nbr].lastBin = settings.GetUInt("BG" + String(nbr) + "T", nbr * groupSize + groupSize - 1);
  }

  settings.LoadCalibration(&sensorData);
  //// Test
  Serial.println("Ref: ");
  for (byte i = 0; i < NR_OF_BIN_GROUPS; i++) {
    Serial.print(String(sensorData.binGroup[i].magThresh) + " ");
  }
  Serial.println();

  stateManager.Begin(PROGVERS, PROGNAME);

  Wire.begin();
  Wire.setClock(800000);
  if (bme280.TryInitialize(0x76)) {
    Serial.println("BME280 detected");
    bme280.SetAltitudeAboveSeaLevel(settings.GetInt("Altitude", 0));
  }

  StartWifi(&settings);
  connectionKeeper.Begin([](bool isCritical) {
    HandleCriticalAction(isCritical);
  });

  pinMode(DEBUG_GPIO_ISR, OUTPUT);
  pinMode(DEBUG_GPIO_MAIN, OUTPUT);

  startProcessTS = millis() + (settings.GetUInt("StartupDelay", 5) * 1000);
  while (millis() < startProcessTS) {
    ota.Handle();
    watchdog.Handle();
  }

  // Initialize the publisher
  publisher.Begin(&settings, &dataPort, &bme280);

  // Initialize signal processing
  sigProc.Begin(&settings, &sensorData, &statistics, &publisher);

  // Initialize statistics
  statistics.Begin(&settings, &sensorData);
  
  // Go
  sigProc.StartCapture();

  Serial.println("Setup done");
}

String CommandHandler(String command) {
  String result;
  command = Tools::UTF8ToASCII(command);
  if (command.startsWith("alive")) {
    result = "alive";
  }
  else if (command.startsWith("thresholdFactor=")) {
    thresholdFactor = command.substring(9).toFloat();
    settings.Add("ThresholdFactor", thresholdFactor);
    Serial.println(thresholdFactor);
  }
  else if (command.startsWith("version")) {
    result = "version=" + String(PROGVERS);
  }
  else if (command.startsWith("uptime")) {
    result = "uptime=" +  stateManager.GetUpTime();
  }
  else if (command.startsWith("reboot")) {
    ESP.restart();
  }
  else if (command.startsWith("savesettings")) {
    settings.Write();
  }
  else if (command.startsWith("calibrate")) {
    statistics.Calibrate();
    settings.SaveCalibration(&sensorData);
  }

  return result;
}

void loop() {
  stateManager.SetLoopStart();
  
  watchdog.Handle();

  if (connectionKeeper.IsConnected()) {
    ota.Handle();
    if (dataPort.IsEnabled()) {
      dataPort.Handle(CommandHandler);
    }
  }

  if (accessPoint.IsRunning()) {
    accessPoint.Handle();
  }
  else {
    sigProc.Handle();
    connectionKeeper.Handle();
  }

  frontend.Handle();

  stateManager.SetLoopEnd();
} 
