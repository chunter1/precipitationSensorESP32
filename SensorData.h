#ifndef __SENSORDATA__h
#define __SENSORDATA__h

#include "Arduino.h"

struct FFT_BIN {
  //uint32_t threshold;
  float threshold;
  uint32_t magSum;
  float magAVG;
  float magAVGkorr;
  float magAVGkorrThresh;
  uint16_t magPeak;
  uint32_t detections;
};

struct FFT_BIN_GROUP {
  uint32_t magSum;
  float magAVG;
  float magAVGkorr;
  float magAVGkorrThresh;
  uint16_t magPeak;
  uint32_t detections;
};

class SensorData {
public:
  FFT_BIN_GROUP *binGroup = NULL;
  FFT_BIN *bin = NULL;

  uint32_t totalDetectionsCtr;
  int32_t ADCoffset;
  float magAVG;
  float magAVGkorr;
  float magAVGkorrThresh;
  uint16_t magPeak;
  uint32_t RBoverflowCtr;
  uint16_t ADCpeakSample;
  uint32_t snapshotCtr;
  uint8_t clippingCtr;


  SensorData(uint nrOfBins, byte nrOfBinGroups);

private:

};

#endif
