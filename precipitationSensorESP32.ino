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
 * The recommended radar sensor moduls are the IPM-170 and RSM-1700 because of their
 * symmetric radiation pattern.
 * Mounted in the middle of a 1m long vertival tube facing upwards is the recommended setup.
 * The IPM-165 or CDM-324 may be used instead, if direction independency
 * does not matter.
 * However, to estimate the rain level, the tube setup with an IPM-170 or RSM-1700
 * is highly recommended. (rain level estimation is yet to be evaluated!)
 *
 * Using a 0° tilted radar sensor with the following FFT settings, the hydrometeor speed
 * can be resolved in 0,12 m/s steps within a speed-range of 0,12...31,61 m/s.
 * Using a 45° tilted radar sensor, the hydrometeor speed can be resolved in 0,18 m/s steps
 * within a speed-range of 0,18...47,71 m/s.
 * 
 * ADC sampling rate:     10240 sps
 * Sampling period:       50 ms
 * FFT points/samples:    512
 * FFT resolution:        20 Hz
 * FFT frequency range:   DC...5100 Hz
 *
 * Arduino IDE board settings:
 * ===========================
 * ESP32 CPU clock speed: 80 MHz
 * 
 * Version history:
 * ================
 * Version v0.1 (16.07.2017)
 * -) Initial version
 * 
 * Version v0.2 (18.07.2017)
 * -) Hamming window option added
 * -) Changed bin masking to be 1 by default and changed binMaskCycles_table values
 * -) Statistics values magSum and magAVG added
 * -) Renamed doStatistics() to updateStatistics()
 * -) Added finalizeStatistics() for AVG calculation etc.
 * -) Added parameters to publish_compact()
 * 
 * Version v0.3 (18.07.2017)
 * -) Changed "magAVG" from uint16_t to float
 * -) Removed "magSum" from publishing since it does not add additional information
 * -) Renamed readings to begin with their cathegory-type: "detectionsInGroup"->"GroupDetections", "peakInGroup"->"GroupMagPeak", "magAVGinGroup"->"GroupMagAVG"
 * -) Hamming window option removed
 * 
 * Version v0.4 (19.07.2017)
 * -) maskCycle table values now represent 1m avg FOV
 * -) FOV_m parameter added to set the average FOV size
 * -) Bin-groups can now be defined with individual first- und last-bin boundary and thus also overlap
 * -) Statistics functions reworked
 * 
 * Version v0.5 
 * -) Web-frontend added by HCS, thanks!
 * 
 * Version v0.6 (23.07.2017)
 * -) Increased sample-ringbuffer size from 1024 to 2048 samples
 * -) TODO: check variable usage in ISR&loop - really atomic?
 * -) Added buffer overflow check and handling
 * -) Added ringbuffer overflow counter to publishCompact()
 * -) Added "magAVG" and "magPeak" that hold the average and peak value calculated from all FFT-bins
 * -) Modification in satatistics structure
 * 
 * Version v0.7 (29.07.2017)
 * -) Added magAVGkorr and GroupMagAVGkorr that hold the FOV-time corrected magAVG values
 * -) Added threshold to bin struct to set the threshold individually (TODO: calc threshold based on noise figure)
 *    Should hopefully never be necessary when the planned low-noise preamp design is used
 * -) magAVGkorr and GroupMagAVGkorr added to published data. All AVG-parameters are now formated to always have 4 decimals.
 * -) Replaced previous detection algorithm with a new "mask-less" approach.
 * -) ADCpeak is now limited to 0...100%.
 * 
 * Version v0.8 (31.07.2017)
 * -) Added function StartCapture(() and StopCapture(()
 * -) Important calculation bugfix in snapshotAvailable()
 * 
 * Version v0.8.1 (05.08.2017)
 * -) Added Tools class with helpers like GetChipId() ...
 * -) Added ChipID and Revision to the hardware page and as serial output
 * -) OTA MDS name is now precipitationSensor_<ChipID>
 * -) Default hostname in the setup is now precipitationSensor - LaCrosseGateway was wrong :-)
 * -) AccessPoint name is now ps-<ChipID>
 * -) Added watchdog triggering on GPIO32
 *
 * Version v0.8.2 (11.08.2017)
 * -) Refactored the publisher
 * -) Added "OTA upload"
 *
 * Version v0.9.0 (11.08.2017)
 * -) Added "AddBinMagAVGReading" to publish functions.
 * -) Added "binThreshFactorNormalized[]" to individually adapt threshold for each bin dependign on preamp used
 * -) TODO: Fill binThreshFactorNormalized[] with real values
 * -) Added "magAVGkorr" and "magAVGkorrThresh" to each bin
 * -) Added "Bins MA", "Bins MAK", "Bins MAKT" and "Bins Thresh" publish functions to internal value debugging (DO NOT ACTIVATE MORE THAN ONE AT A TIME)
 * -) Added "magAVGkorrThresh" to binand group statistics
 * -) Changed "threshold" type from uint32_t to float (enter float value in web-frontend now)
 * -) Changed threshold factor table entries (TODO: check if automatic/dynamic and temperature compensated generation is necessary)
 * 
 * Version v0.9.1 (19.08.2017)
 * -) 8 decimals for GroupMagAVGkorrThresh
 * -) DataPort
 * -) FHEM module
 * 
 * Version v0.10.0 (20.08.2017)
 * -) !! removed all detection-count related parameters !! -> use magAVG... parameters from now on
 * -) Statistics.ino converted to C++ class
 * -) Added GlobalDefines.h
 * -) process.ino and fft.ino and parts of precipitationSensor.ino combined in SigProc.cpp
 * -) TODO: Permanently store calibration data
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

float detectionThreshold;
//uint mountingAngle;

/* The following table holds the normalized threshold factor for each FFT-BIN.
 * The values are based on a LME49720 preamp measured at room-temperature. 
 * TODO: automatically/dynamically generate values
 */
const float binThreshFactorNormalized[NR_OF_BINS] = {
// normalized LME outdoor 2nd order bandpass (see FHEM forum schematic)
  1.0000, 10.8734, 19.2322, 19.1900, 15.5119, 12.0158, 11.9288, 13.4776,
  15.2058, 7.3113, 6.5752, 5.4960, 5.8813, 4.7282, 3.7916, 3.5066,
  4.8813, 2.7361, 3.2005, 3.5409, 3.0897, 1.9446, 2.4617, 1.9446,
  2.7493, 2.0237, 1.9024, 1.4063, 2.0343, 1.9235, 2.9129, 1.6491,
  4.2005, 1.4301, 2.4723, 1.8470, 1.8681, 1.2084, 1.7256, 1.5937,
  2.2216, 1.4723, 1.6702, 1.3958, 1.6042, 1.5620, 1.1768, 1.2744,
  2.0897, 1.0554, 1.5066, 1.5620, 1.8153, 1.5277, 1.7599, 1.3193,
  2.3404, 1.5831, 1.6280, 1.3747, 1.5620, 2.3298, 2.4301, 1.2216,
  4.3193, 1.2533, 2.4195, 2.5277, 1.7045, 1.3852, 1.5937, 1.2427,
  2.1003, 1.1108, 1.6280, 1.4195, 1.6939, 1.4617, 1.2533, 1.0000,
  1.7704, 1.1108, 1.1873, 1.4195, 1.3509, 1.1214, 1.1979, 1.1214,
  1.7704, 1.3087, 1.4512, 1.0765, 1.4406, 1.5383, 1.8786, 1.0897,
  2.8259, 1.1768, 2.0554, 1.4063, 1.3404, 1.0237, 1.2639, 1.1873,
  1.8918, 1.2982, 1.4512, 1.2982, 1.9024, 1.6174, 1.6491, 1.2639,
  2.2744, 1.2427, 1.7045, 1.9446, 2.5172, 2.1003, 2.1003, 1.8364,
  3.8707, 3.0026, 2.4512, 2.5620, 3.3087, 5.6728, 5.5963, 1.8470,
  6.9367, 1.8259, 5.4855, 5.7071, 3.3087, 2.5726, 2.4960, 2.9894,
  3.8918, 1.7916, 2.1214, 2.1003, 2.5277, 1.9578, 1.6807, 1.2427,
  2.2533, 1.2216, 1.6491, 1.6280, 1.9024, 1.2982, 1.4512, 1.2982,
  1.9129, 1.1873, 1.2639, 1.0237, 1.3193, 1.4063, 2.0554, 1.1768,
  2.7810, 1.0897, 1.8786, 1.5383, 1.4195, 1.0765, 1.4512, 1.2850,
  1.7467, 1.1214, 1.1979, 1.1214, 1.3509, 1.4195, 1.2084, 1.0897,
  1.7467, 1.0000, 1.2533, 1.4617, 1.6939, 1.4195, 1.6042, 1.1108,
  2.1003, 1.2427, 1.5726, 1.3852, 1.6939, 2.4512, 2.3852, 1.2533,
  4.2216, 1.2216, 2.3852, 2.3087, 1.5488, 1.3193, 1.6280, 1.5620,
  2.2982, 1.2322, 1.7599, 1.5066, 1.8153, 1.5620, 1.4828, 1.0449,
  2.0897, 1.2533, 1.1319, 1.5620, 1.5488, 1.3747, 1.6702, 1.4301,
  2.1662, 1.5726, 1.7150, 1.2084, 1.7810, 1.8021, 2.3852, 1.3509,
  3.8259, 1.5620, 2.7493, 1.8786, 1.9446, 1.3298, 1.8153, 1.7467,
  2.5172, 1.7467, 2.2533, 1.8021, 2.3641, 2.4960, 2.3852, 1.9340,
  3.6174, 2.5383, 2.6069, 3.3958, 4.3536, 3.7045, 3.6834, 3.4749,
  7.0475, 5.8364, 4.7599, 5.1003, 6.1689, 11.3905, 10.8734, 3.6069
};

void TryConnectWIFI(String ctSSID, String ctPass, byte nbr, uint timeout, String staticIP, String staticMask, String staticGW, String hostName) {
  if (ctSSID.length() > 0 && ctSSID != "---") {
    unsigned long startMillis = millis();

    WiFi.begin(ctSSID.c_str(), ctPass.c_str());
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
  frontend.Begin(&stateManager);
  
  Serial.println("Starting OTA");
  ota.Begin(frontend.GetWebServer(), [] {
    sigProc.StopCapture();
  });

  if (settings->GetBool("UseDataPort", false)) {
    Serial.println("Starting data port");
    dataPort.Begin(81);
  }

  return result;
}


void setup()
{
  uint32_t startProcessTS;
  
  Serial.begin(115200);
  delay(50);
  Serial.println();

  Serial.println("Chip ID: " + Tools::GetChipId());
  Serial.println("Chip Revision: " + Tools::GetChipRevision());

  watchdog.Begin(32, 1);

  // Get the settings
  settings.BaseData.NrOfBins = NR_OF_BINS;
  settings.BaseData.NrOfBinGroups = NR_OF_BIN_GROUPS;
  settings.Read();

  // Get the measurement settings
  detectionThreshold = settings.GetFloat("DetectionThreshold", DEFAULT_DETECTION_TRESHOLD);
  //mountingAngle = settings.GetUInt("SMA", 45);

  // Get the group-boundaries from the settings
  for (byte nbr = 0; nbr < settings.BaseData.NrOfBinGroups; nbr++) {
    byte groupSize = settings.BaseData.NrOfBins / settings.BaseData.NrOfBinGroups;
    sensorData.binGroup[nbr].firstBin = settings.GetUInt("BG" + String(nbr) + "F", nbr == 0 ? 1 : (nbr * groupSize));
    sensorData.binGroup[nbr].lastBin = settings.GetUInt("BG" + String(nbr) + "T", nbr * groupSize + groupSize - 1);
  }

  for (uint16_t binNr = 0; binNr < NR_OF_BINS; binNr++) {
/*    
    float thresh;
    String str;
    
    str = "TB" + String(binNr);
    Serial.println(str);
    
    // Get the measurement settings
    thresh = settings.GetFloat("TB" + String(binNr), DEFAULT_DETECTION_TRESHOLD);
    Serial.printf("%f\n", thresh);

    sensorData.bin[binNr].threshold = thresh * detectionThreshold;
*/    
    sensorData.bin[binNr].threshold = detectionThreshold * binThreshFactorNormalized[binNr];
  }

  stateManager.Begin(PROGVERS, PROGNAME);

  StartWifi(&settings);

  pinMode(DEBUG_GPIO_ISR, OUTPUT);
  pinMode(DEBUG_GPIO_MAIN, OUTPUT);

  startProcessTS = millis() + (settings.GetUInt("StartupDelay", 5) * 1000);
  while (millis() < startProcessTS) {
    ota.Handle();
    watchdog.Handle();
  }

  // Initialize the publisher
  publisher.Begin(&settings, &dataPort);

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
  else if (command.startsWith("treshold=")) {
    detectionThreshold = command.substring(9).toFloat();
    settings.Add("DetectionThreshold", detectionThreshold);
    Serial.println(detectionThreshold);
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
    for (uint16_t binNr = 0; binNr < 10; binNr++) {
      //settings.Add("TB" + String(binNr), sensorData.bin[binNr].threshold);
      settings.Add("TB" + String(binNr), binNr);
    }
    //settings.Write();
/*    
    // report it to fhem
    result = "treshold=" + String(detectionThreshold);
    // to make it persistent, you would do a "set Radar218 savesettings" in FHEM after the "set Radar218 calibrate" if results with the new treshold are fine 
*/    
  }

  return result;
}

void loop() {
  stateManager.SetLoopStart();

  ota.Handle();
  frontend.Handle();
  accessPoint.Handle();
  watchdog.Handle();

  if(dataPort.IsEnabled()) {
    dataPort.Handle(CommandHandler);
  }

  if (WiFi.status() == WL_CONNECTED) {
    ota.Handle();
  }

  sigProc.Handle();

  stateManager.SetLoopEnd();
} 
