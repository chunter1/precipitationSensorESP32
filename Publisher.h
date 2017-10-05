#ifndef __PUBLISHER__h
#define __PUBLISHER__h

#include "Arduino.h"
#include "WiFiClient.h"
#include "IPAddress.h"
#include "Settings.h"
#include "SensorData.h"
#include "StateManager.h"
#include "DataPort.h"
#include "BME280.h"

#define NR_OF_BARS 32

class Publisher {
public:
  void Begin(Settings *settings, DataPort *dataPort, BME280 *bme280);
  void Publish(SensorData *sensorData);

private:
  String m_dummyPrefix;
  String m_dummySuffix;
  Settings *m_settings;
  String m_readings;
  uint16_t m_nbrOfReadings;
  SensorData *m_sensorData;
  DataPort *m_dataPort;
  BME280 *m_bme280;

  void SendToDataPort();
  void AddReading(String name, String value);
  void AddReading(String name, uint32_t value);
  void AddReading(String name, int32_t value);
  void AddReading(String name, float value);
  void AddCommonReadings();
  void AddCompactReadings();
  void AddBinsCountReadings();
  void AddBinsMagReadings();
  void AddBinGroupsReadings();
  void AddBinMagAVGReading();
  void AddBinMagAVGkorrReading();
  void AddBinMagAVGkorrThreshReading();
  void AddGroupMagCalReading();
  void Transmit();

};


#endif
