#ifndef __SENSORDATA__h
#define __SENSORDATA__h

#include "Arduino.h"

struct FFT_BIN {
  uint16_t mag;
  uint32_t magSum;
  float magAVG;
  float magAVGkorr;
  uint16_t magMax;
};

struct FFT_BIN_GROUP {
  uint8_t firstBin;  
  uint8_t lastBin;
  float magAVG;
  uint16_t magThresh;
  float magAVGkorr;
  float magAVGkorrDom;
  uint16_t magMax;
  uint16_t magAboveThreshCnt;
};

class SensorData {
public:
  FFT_BIN_GROUP *binGroup = NULL;
  FFT_BIN *bin = NULL;

  int32_t ADCoffset;
  float magAVG;
  float magAVGkorr;
  uint16_t magMax;
  uint32_t RbOvCtr;
  uint16_t ADCpeakSample;
  uint32_t snapshotCtr;
  uint8_t clippingCtr;
  uint8_t dominantGroup;


  SensorData(uint nrOfBins, byte nrOfBinGroups);

private:

};

#endif
