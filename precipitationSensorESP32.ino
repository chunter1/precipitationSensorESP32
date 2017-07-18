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
 * -) Changed bin masking to be 1 by default and changed binMaskCycles_table values
 * -) Statistics values magsAcc and magAVG added
 * -) Renamed doStatistics() to updateStatistics()
 * -) Added finalizeStatistics() for AVG calculation etc.
 * -) Added parameters to publish_compact()
 * 
\***************************************************************************/

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define START_DELAY_s                     5                                // Delay before application starts for "emergency"-OTA in case of blocking application code

#define PUBLISHING_INTERVAL_s             60
#define DETECTION_THRESHOLD               90                               // Detection threshold highly depends on gain and sensor->drop distance

#define ADC_PIN_NR                        34
#define DEBUG_GPIO_ISR                    5
#define DEBUG_GPIO_MAIN                   23

#define NR_OF_SAMPLES_bit                 9                                 // DO NOT MODIFY - Number of samples in [bits] e.g. 9 bits = 512 samples
#define NR_OF_SAMPLES                     (1 << NR_OF_SAMPLES_bit)
#define NR_OF_BINS                        (NR_OF_SAMPLES >> 1)


// Wifi settings
const char* ssid = "yourSSID";
const char* password = "yourPASSWORD";

// FHEM server settings
IPAddress serverIP(192, 168, 1, 100);
const uint16_t serverPORT = 8083;

hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

#define RINGBUFFER_SIZE     (NR_OF_SAMPLES * 2)

volatile int16_t sampleRB[RINGBUFFER_SIZE];
volatile uint16_t processSampleNr;

uint32_t snapshotCtr;
uint8_t snapProcessed_flag;
uint8_t clippingCtr;
uint16_t ADCpeakSample;

int16_t re[NR_OF_SAMPLES];
int16_t im[NR_OF_SAMPLES];

// =========== statistics variables

uint32_t totalDetectionsCtr;
uint16_t processSampleNr_tmp;
int32_t ADCoffset;

const uint8_t binGroupBoundary[] = {
  7,  15,  23,  31,  39,  47,  55,  63,
  71,  79,  87,  95, 103, 111, 119, 127,
  135, 143, 151, 159, 167, 175, 183, 191,
  199, 207, 215, 223, 231, 239, 247, 255
};
const uint8_t NR_OF_BIN_GROUPS = sizeof(binGroupBoundary) / sizeof(binGroupBoundary[0]);

struct FFT_BIN {
  uint16_t peakMag;                             // holds the peak-magnitude
  uint32_t detectionCtr;                        // counts when bin exceets detection threshold
} bin[NR_OF_BINS];

struct BINGROUP {
  uint16_t peakMag;                             // holds the magnitude value of the peak-bin
  uint32_t binDetectionCtr;                     // counts EVERY bin inside the group that exceets the detection threshold
  uint32_t magsAcc;                             // accumulated magnitudes
  uint16_t magAVG;
} binGroup[NR_OF_BIN_GROUPS];


// *****************************************
// *****************************************
// *****************************************
void IRAM_ATTR onTimer()
{
  int16_t data;
  static uint16_t sampleNr;

  data = analogRead(ADC_PIN_NR);

  //digitalWrite(DEBUG_GPIO_ISR, HIGH);

  sampleRB[sampleNr++] = data - 2048;
  
  if ((sampleNr == 256) || (sampleNr == 512) || (sampleNr == 768) || (sampleNr == 1024))
  {
    processSampleNr = sampleNr;
    xSemaphoreGiveFromISR(timerSemaphore, NULL);
  }

  if (sampleNr >= RINGBUFFER_SIZE)
    sampleNr = 0;

  //digitalWrite(DEBUG_GPIO_ISR, LOW);
}

// **************************************************
// **************************************************
// **************************************************
void WiFiStart()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// *****************************************
// *****************************************
// *****************************************
void setup()
{
  uint32_t startProcessTS;
  
  Serial.begin(115200);

  pinMode(DEBUG_GPIO_ISR, OUTPUT);
  pinMode(DEBUG_GPIO_MAIN, OUTPUT);

  analogSetAttenuation(ADC_6db);
  analogSetWidth(12);
  analogSetCycles(8);
  analogSetSamples(1);
  analogSetClockDiv(1);
  adcAttachPin(ADC_PIN_NR);

  Serial.print("\nConnecting to ");
  Serial.println(ssid);

  WiFiStart();

  ArduinoOTA.setHostname("precipitationSensorESP32");
  ArduinoOTA.onStart([]() { timerAlarmDisable(timer);});
  ArduinoOTA.onEnd([]() {});
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {});
  ArduinoOTA.onError([](ota_error_t error) {});
  ArduinoOTA.begin();

  timerSemaphore = xSemaphoreCreateBinary();

  timer = timerBegin(0, 868, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 9, true);

  startProcessTS = millis() + (START_DELAY_s * 1000);
  while (millis() < startProcessTS)
    ArduinoOTA.handle();

  timerAlarmEnable(timer);

  // fill buffer
  do {
    if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE)
    {
      portENTER_CRITICAL(&timerMux);
      processSampleNr_tmp = processSampleNr;
      portEXIT_CRITICAL(&timerMux);
    }
  } while(processSampleNr_tmp < NR_OF_SAMPLES);
}

// *****************************************
// *****************************************
// *****************************************
void loop()
{
  ArduinoOTA.handle();

  // new snapshot ready?
  if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE)
  {
    portENTER_CRITICAL(&timerMux);
    processSampleNr_tmp = processSampleNr;
    portEXIT_CRITICAL(&timerMux);

    if (processSampleNr_tmp)
    {
      //digitalWrite(DEBUG_GPIO_MAIN, HIGH);
    
      snapshotCtr++;
    
      processSamples(processSampleNr_tmp);
      updateStatistics();
    
      if (snapshotCtr >= (40 * PUBLISHING_INTERVAL_s))                  // 1/0,025 ms = 40 snapshots/s
      {
        finalizeStatistics();
        
        if (WiFi.status() != WL_CONNECTED)
          WiFiStart();

        //consoleOut_samples(0, 32);
        //consoleOut_bins(0, 40);
    
        publish_compact("PRECIPITATION_SENSOR");
        //publish_bins_count("PRECIPITATION_SENSOR_BINS_COUNT");
        //publish_bins_mag("PRECIPITATION_SENSOR_BINS_MAG");
        //publish_binGroups("PRECIPITATION_SENSOR_BIN_GROUPS");
    
        resetStatistics();
    
        snapshotCtr = 0;
      }
        
      //digitalWrite(DEBUG_GPIO_MAIN, LOW);
    }
  }
}
