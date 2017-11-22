#ifndef __SENSORDATA__h
#define __SENSORDATA__h

#include "Arduino.h"

struct FFT_BIN {
  uint16_t mag;
  uint16_t magMax;
  float magSum;
  float magAVG;
  float magAVGkorr;
};

struct FFT_BIN_GROUP {
  uint8_t firstBin;  
  uint8_t lastBin;
  uint16_t magMax;
  float magAVG;
  uint16_t magThresh;
  float magAboveThreshCnt;
  float magAboveThreshCntDom;
  float magAVGkorr;
  float magAVGkorrDom;
  float magAVGkorrDom2;
  float preciAmountFactor;
};

class SensorData {
public:
  FFT_BIN_GROUP *binGroup = NULL;
  FFT_BIN *bin = NULL;

  int16_t ADCoffset;
  float magAVG;
  float magAVGkorr;
  uint16_t magMax;
  uint32_t RbOvCtr;
  uint16_t ADCpeakSample;
  uint32_t snapshotCtr;
  uint8_t clippingCtr;
  uint8_t DomGroupMagAVGkorr;
  uint8_t DomGroupMagAboveThreshCnt;
  float preciAmount;
  float preciAmountAcc;

  SensorData(uint nrOfBins, byte nrOfBinGroups);

private:

};

#endif
