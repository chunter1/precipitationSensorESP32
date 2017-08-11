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
 * The recommended radar sensor moduls are the IPM-170 and RSM-1700 because of their
 * symmetric radiation pattern.
 * Mounted in a vertival tube, facing upwards is the recommended setup.
 * The IPM-165 (or CDM-324, not yet tested) may be used instead, if direction independency
 * does not matter.
 * Working on 24 GHz, the radar sensor gives a doppler frequency of 161 Hz per 1 m/s speed.
 * Using a 45° tilted radar sensor with the following FFT settings, the hydrometeor speed
 * can be resolved in 0,18 m/s steps within a speed-range of 0,18...47,71 m/s.
 * FFT frequency bin range (0=DC, 1=20 Hz, ... , 255=5100 Hz)
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
 * -) Added function startCapture() and stopCapture()
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
 * Version v0.8.2 11.08.2017)
 * -) Refactored the publisher
 * -) Added "OTA upload"
 *
 \***************************************************************************/

#define PROGNAME                         "precipitationSensor"
#define PROGVERS                         "0.8.2"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
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

#define DEFAULT_SSID                      "SSID"
#define DEFAULT_PASSWORD                  "PASSWORD"
#define DEFAULT_PUBLISH_INTERVAL          60
#define DEFAULT_DETECTION_TRESHOLD        30
                         
#define FOV_m                             0.5                              // Average FOV size in meter

#define ADC_PIN_NR                        34
#define DEBUG_GPIO_ISR                    5
#define DEBUG_GPIO_MAIN                   23

#define RINGBUFFER_SIZE                   (NR_OF_FFT_SAMPLES << 2)

#define NR_OF_FFT_SAMPLES_bit             9                                 // DO NOT MODIFY - Number of samples in [bits] e.g. 9 bits = 512 samples
#define NR_OF_FFT_SAMPLES                 (1 << NR_OF_FFT_SAMPLES_bit)
#define NR_OF_BINS                        (NR_OF_FFT_SAMPLES >> 1)
#define NR_OF_BIN_GROUPS                  32

hw_timer_t * timer = NULL;
StateManager stateManager;
Settings settings;
WebFrontend frontend(80, &settings);
AccessPoint accessPoint(IPAddress(192, 168, 222, 1), IPAddress(192, 168, 222, 1), IPAddress(255, 255, 225, 0), "ps");
Watchdog watchdog;
OTAUpdate ota;
Publisher publisher;
SensorData sensorData(NR_OF_BINS, NR_OF_BIN_GROUPS);

volatile uint32_t samplePtrIn;
volatile uint32_t samplePtrOut;
volatile int16_t sampleRB[RINGBUFFER_SIZE];
volatile uint32_t RBoverflow;

uint publishInterval;
uint detectionTreshold;    // Detection threshold highly depends on gain and sensor->drop distance
uint mountingAngle;
uint8_t snapProcessed_flag;

int16_t re[NR_OF_FFT_SAMPLES];
int16_t im[NR_OF_FFT_SAMPLES];

struct BIN_GROUP_BOUNDARIES {
  uint8_t firstBin[NR_OF_BIN_GROUPS] = {
    1, 8, 16, 24, 32, 40, 48, 56,
    64, 72, 80, 88, 96, 104, 112, 120,
    128, 136, 144, 152, 160, 168, 176, 184,
    192, 200, 208, 216, 224, 232, 240, 248
  };
  uint8_t lastBin[NR_OF_BIN_GROUPS] = {
    7,  15,  23,  31,  39,  47,  55,  63,
    71,  79,  87,  95, 103, 111, 119, 127,
    135, 143, 151, 159, 167, 175, 183, 191,
    199, 207, 215, 223, 231, 239, 247, 255
  };
} binGroupBoundaries;

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

    delay(5);
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
    stopCapture();
  });


  return result;
}

// *****************************************
// *****************************************
// *****************************************
void IRAM_ATTR onTimer()
{
  int16_t data;
  uint32_t diff;

  data = analogRead(ADC_PIN_NR);
  
  //digitalWrite(DEBUG_GPIO_ISR, HIGH);

  sampleRB[samplePtrIn] = data - 2048;
  samplePtrIn = (++samplePtrIn) & (RINGBUFFER_SIZE - 1);

  if (samplePtrIn == samplePtrOut)
    RBoverflow = 1;

  //digitalWrite(DEBUG_GPIO_ISR, LOW);  
}

// *****************************************
// *****************************************
// *****************************************
void startCapture()
{
  samplePtrIn = 0;
  samplePtrOut = 0;
  timerAlarmEnable(timer);
  while (samplePtrIn < NR_OF_FFT_SAMPLES) {}
}

// *****************************************
// *****************************************
// *****************************************
void stopCapture()
{
  timerAlarmDisable(timer);
}

// *****************************************
// *****************************************
// *****************************************
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
  publishInterval = settings.GetUInt("PublishInterval", DEFAULT_PUBLISH_INTERVAL);
  detectionTreshold = settings.GetUInt("DetectionThreshold", DEFAULT_DETECTION_TRESHOLD);
  mountingAngle = settings.GetUInt("SMA", 45);

  // Get the groups from the settings
  for (byte nbr = 0; nbr < settings.BaseData.NrOfBinGroups; nbr++) {
    byte groupSize = settings.BaseData.NrOfBins / settings.BaseData.NrOfBinGroups;
    binGroupBoundaries.firstBin[nbr] = settings.GetUInt("BG" + String(nbr) + "F", nbr == 0 ? 1 : (nbr * groupSize));
    binGroupBoundaries.lastBin[nbr]  = settings.GetUInt("BG" + String(nbr) + "T", nbr * groupSize + groupSize - 1);
  }

  stateManager.Begin(PROGVERS, PROGNAME);

  StartWifi(&settings);

  pinMode(DEBUG_GPIO_ISR, OUTPUT);
  pinMode(DEBUG_GPIO_MAIN, OUTPUT);

  analogSetAttenuation(ADC_6db);
  analogSetWidth(12);
  analogSetCycles(8);
  analogSetSamples(1);
  analogSetClockDiv(1);
  adcAttachPin(ADC_PIN_NR);

  timer = timerBegin(0, 868, true);
  settings.SetTimer(timer);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 9, true);

  startProcessTS = millis() + (settings.GetUInt("StartupDelay", 5) * 1000);
  while (millis() < startProcessTS) {
    ota.Handle();
    watchdog.Handle();
  }

  // Set threshold for each bin individually
  // TODO: Add formula that automatically adapts the threshold to the frequency dependent noise level
  for (uint16_t binNr = 0; binNr < NR_OF_BINS; binNr++)
  {
    if (binNr < 8)
      sensorData.bin[binNr].threshold = detectionTreshold * 3;     // quick & dirty test!
    else if (binNr < 16)
      sensorData.bin[binNr].threshold = detectionTreshold * 2;     // quick & dirty test!
    else
      sensorData.bin[binNr].threshold = detectionTreshold;
  }


  // Initialize the publisher
  publisher.Begin(&settings);
  
  // Go
  startCapture();

  Serial.println("Setup done");
}

// *****************************************
// *****************************************
// *****************************************
uint8_t snapshotAvailable()
{
  uint32_t samplePtrIn_tmp;
  uint32_t diff;

  samplePtrIn_tmp = samplePtrIn;

  if (samplePtrIn_tmp > samplePtrOut)
    diff = samplePtrIn_tmp - samplePtrOut;
  else
    diff = (samplePtrIn_tmp + RINGBUFFER_SIZE) - samplePtrOut;

  // bufferPtrIn must be at least NR_OF_FFT_SAMPLES ahead
  if (diff >= NR_OF_FFT_SAMPLES)
    return 1;

  return 0;
}

// *****************************************
// *****************************************
// *****************************************
void loop()
{
  stateManager.SetLoopStart();

  ota.Handle();
  frontend.Handle();
  accessPoint.Handle();
  watchdog.Handle();

  if (WiFi.status() == WL_CONNECTED) {
    ota.Handle();
  }

  while (snapshotAvailable())
  {
    //digitalWrite(DEBUG_GPIO_MAIN, HIGH);

    processSamples(samplePtrOut);
    samplePtrOut = (samplePtrOut + (NR_OF_FFT_SAMPLES >> 1)) & (RINGBUFFER_SIZE - 1);

    // check if no ringbuffer overflow occurred before or during sample processing
    if (!RBoverflow)
    {
      updateStatistics();
      sensorData.snapshotCtr++;

      if (sensorData.snapshotCtr >= (40 * publishInterval))                  // 1/0,025 ms = 40 snapshots/s (ringbuffer overflows ignored)
      {
        //stopCapture();

        finalizeStatistics();

        publisher.Publish(&sensorData);

        resetStatistics();
        
        sensorData.RBoverflowCtr = 0;
        sensorData.snapshotCtr = 0;
        
        //startCapture();
      }
    } else {
      Serial.println("RINGBUFFER OVERFLOW!");
      sensorData.RBoverflowCtr++;
      RBoverflow = 0;
    }

    //digitalWrite(DEBUG_GPIO_MAIN, LOW);    
  }

  stateManager.SetLoopEnd();
} 
