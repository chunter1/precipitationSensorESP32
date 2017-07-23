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
 * This project uses an IMP-165 radarsensor with preamp and an ESP8266 to classify
 * the type of precipitation/hydrometeors by measuring the speed/doppler frequency.
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
\***************************************************************************/

#define PROGNAME                         "precipitationSensor"
#define PROGVERS                         "0.6"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "StateManager.h"
#include "Settings.h"
#include "WebFrontend.h"
#include "AccessPoint.h"

#define DEFAULT_SSID                      "SSID"
#define DEFAULT_PASSWORD                  "PASSWORD"
#define DEFAULT_FHEM_IP                   "192.168.1.100"
#define DEFAULT_FHEM_PORT                 8083
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
AccessPoint accessPoint(IPAddress(192, 168, 222, 1), IPAddress(192, 168, 222, 1), IPAddress(255, 255, 225, 0), "precipitationSensor");

// FHEM server settings
IPAddress serverIP;
uint16_t serverPORT = 0;

volatile uint32_t samplePtrIn;
volatile uint32_t samplePtrOut;
volatile int16_t sampleRB[RINGBUFFER_SIZE];
volatile uint32_t RBoverflow;

uint publishInterval;
uint detectionTreshold;    // Detection threshold highly depends on gain and sensor->drop distance

uint32_t snapshotCtr;
uint8_t snapProcessed_flag;
uint8_t clippingCtr;
uint32_t RBoverflowCtr;
uint16_t ADCpeakSample;

int16_t re[NR_OF_FFT_SAMPLES];
int16_t im[NR_OF_FFT_SAMPLES];

// =========== statistics variables

uint32_t totalDetectionsCtr;
int32_t ADCoffset;

float magAVG;
uint16_t magPeak;

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

struct FFT_BIN {
  uint32_t magSum;
  float magAVG;
  uint16_t magPeak;
  uint32_t detections;
} bin[NR_OF_BINS];

struct FFT_BIN_GROUP {
  uint32_t magSum;
  float magAVG;
  uint16_t magPeak;
  uint32_t detections;
} binGroup[NR_OF_BIN_GROUPS];

IPAddress IPAddressFromString(String ipString) {
  byte o1p = ipString.indexOf(".", 0);
  byte o2p = ipString.indexOf(".", o1p + 1);
  byte o3p = ipString.indexOf(".", o2p + 1);
  byte o4p = ipString.indexOf(".", o3p + 1);

  return IPAddress(strtol(ipString.substring(0, o1p).c_str(), NULL, 10),
    strtol(ipString.substring(o1p + 1, o2p).c_str(), NULL, 10),
    strtol(ipString.substring(o2p + 1, o3p).c_str(), NULL, 10),
    strtol(ipString.substring(o3p + 1, o4p).c_str(), NULL, 10));
}

void TryConnectWIFI(String ctSSID, String ctPass, byte nbr, uint timeout, String staticIP, String staticMask, String staticGW, String hostName) {
  if (ctSSID.length() > 0 && ctSSID != "---") {
    unsigned long startMillis = millis();

    WiFi.begin(ctSSID.c_str(), ctPass.c_str());
    if (staticIP.length() > 7 && staticMask.length() > 7) {
      if (staticGW.length() < 7) {
        WiFi.config(IPAddressFromString(staticIP), IPAddressFromString(staticGW), (uint32_t)0);
      }
      else {
        WiFi.config(IPAddressFromString(staticIP), IPAddressFromString(staticGW), IPAddressFromString(staticMask));
      }
    }

    WiFi.setHostname(hostName.c_str());

    Serial.println("Connect " + String(timeout) + " seconds to an AP (SSID " + String(nbr) + ")");
    digitalWrite(DEBUG_GPIO_ISR, HIGH);
    byte retryCounter = 0;
    while (retryCounter < timeout * 2 && WiFi.status() != WL_CONNECTED) {
      retryCounter++;
      delay(500);
      Serial.print(".");
      digitalWrite(DEBUG_GPIO_ISR, retryCounter % 2 == 0);
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
void setup()
{
  uint32_t startProcessTS;
  
  Serial.begin(115200);
  delay(10);
  Serial.println();

  // Get the settings
  settings.BaseData.NrOfBins = NR_OF_BINS;
  settings.BaseData.NrOfBinGroups = NR_OF_BIN_GROUPS;
  settings.Read();

  // Get the FHEM connection from the settings
  serverIP = IPAddressFromString(settings.Get("fhemIP", DEFAULT_FHEM_IP));
  serverPORT = settings.GetUInt("fhemPort", DEFAULT_FHEM_PORT);
  Serial.println("FHEM IP=" + serverIP.toString());
  Serial.println("FHEM Port=" + String(serverPORT));

  // Get the measurement settings
  publishInterval = settings.GetUInt("PublishInterval", DEFAULT_PUBLISH_INTERVAL);
  detectionTreshold = settings.GetUInt("DetectionThreshold", DEFAULT_DETECTION_TRESHOLD);

  // Get the groups from the settings
  for (byte nbr = 0; nbr < settings.BaseData.NrOfBinGroups; nbr++) {
    byte groupSize = settings.BaseData.NrOfBins / settings.BaseData.NrOfBinGroups;
    binGroupBoundaries.firstBin[nbr] = settings.GetUInt("BG" + String(nbr) + "F", nbr * groupSize);
    binGroupBoundaries.lastBin[nbr]  = settings.GetUInt("BG" + String(nbr) + "T", nbr * groupSize + groupSize - 1);

    Serial.println(String(binGroupBoundaries.firstBin[nbr]) + " / " + String(binGroupBoundaries.lastBin[nbr]));

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

  ArduinoOTA.setHostname("precipitationSensorESP32");
  ArduinoOTA.onStart([]() { timerAlarmDisable(timer);});
  ArduinoOTA.onEnd([]() {});
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {});
  ArduinoOTA.onError([](ota_error_t error) {});
  ArduinoOTA.begin();

  timer = timerBegin(0, 868, true);
  settings.SetTimer(timer);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 9, true);

  startProcessTS = millis() + (settings.GetUInt("StartupDelay", 5) * 1000);
  while (millis() < startProcessTS) {
    ArduinoOTA.handle();
  }

  timerAlarmEnable(timer);
  while (samplePtrIn < NR_OF_FFT_SAMPLES);

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

  // bufferPtrIn must be at least (NR_OF_FFT_SAMPLES / 2) ahead
  if ((diff + 1) >= (NR_OF_FFT_SAMPLES >> 1))
    return 1;

  return 0;
}

// *****************************************
// *****************************************
// *****************************************
void loop()
{
  stateManager.SetLoopStart();

  ArduinoOTA.handle();
  frontend.Handle();
  accessPoint.Handle();

  while (snapshotAvailable())
  {
    //digitalWrite(DEBUG_GPIO_MAIN, HIGH);

    processSamples(samplePtrOut);

    // check if no ringbuffer overflow occurred before or during sample processing
    if (!RBoverflow)
    {
      updateStatistics();
      snapshotCtr++;      

      if (snapshotCtr >= (40 * publishInterval))                  // 1/0,025 ms = 40 snapshots/s (ringbuffer overflows ignored)
      {
        finalizeStatistics();
  
        //consoleOut_samples(0, 30);
        //consoleOut_bins(240, 255);
        
        publish_compact("PRECIPITATION_SENSOR");
        //publish_bins_count("PRECIPITATION_SENSOR_BINS_COUNT");
        //publish_bins_mag("PRECIPITATION_SENSOR_BINS_MAG");
        //publish_binGroups("PRECIPITATION_SENSOR_BIN_GROUPS");
      
        resetStatistics();
        
        RBoverflowCtr = 0;
        snapshotCtr = 0;
      }
    } else {
      Serial.println("RINGBUFFER OVERFLOW!");
      RBoverflowCtr++;
      RBoverflow = 0;
    }

    samplePtrOut = (samplePtrOut + (NR_OF_FFT_SAMPLES >> 1)) & (RINGBUFFER_SIZE - 1);

    //digitalWrite(DEBUG_GPIO_MAIN, LOW);
  }

  stateManager.SetLoopEnd();
} 
