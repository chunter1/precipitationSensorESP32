#include "SensorData.h"

SensorData::SensorData(uint nrOfBins, byte nrOfBinGroups) {
  bin = new FFT_BIN[nrOfBins];
  binGroup = new FFT_BIN_GROUP[nrOfBinGroups];
}

