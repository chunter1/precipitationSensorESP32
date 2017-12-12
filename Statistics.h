#ifndef __STATISTICS__h
#define __STATISTICS__h

#include "Arduino.h"
#include "GlobalDefines.h"
#include "Settings.h"
#include "SensorData.h"

class Statistics {
public:
  void Begin(Settings *settings, SensorData *sensorData);
  void Calibrate();
  void Calc();
  void Finalize();
  void ResetPreciAmountAcc();
  void Reset();

private:
  SensorData *m_sensorData;
  Settings *m_settings;
  float thresholdOffset;
  float countThreshold;

  void filterMagMaxGroup();
  float noiseDebiasing(float scaleStart, float scaleStop, float scaleStep);
};

#endif
