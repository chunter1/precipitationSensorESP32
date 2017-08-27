#ifndef __STATISTICS__h
#define __STATISTICS__h

#include "Arduino.h"
#include "GlobalDefines.h"
#include "Settings.h"
#include "SensorData.h"

class Statistics {
public:
  void Begin(Settings *settings, SensorData *sensorData);
  float Calibrate();
  void Calc();
  void Finalize();
  void Reset();

private:
  SensorData *m_sensorData;
  Settings *m_settings;
  float detectionThreshold;
};


#endif
