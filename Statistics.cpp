#include "Statistics.h"

/* The following table holds the number of snapshots (a 25ms) that the "same" drop is
 * visible to the radar sensor, based on an average 1m FOV with a 0° sensor tilt
 */
const float dropInFOVsnapshots
[NR_OF_BINS] = {
  1.0000, 322.6667, 161.3333, 107.5556, 80.6667, 64.5333, 53.7778, 46.0952,
  40.3333, 35.8519, 32.2667, 29.3333, 26.8889, 24.8205, 23.0476, 21.5111,
  20.1667, 18.9804, 17.9259, 16.9825, 16.1333, 15.3651, 14.6667, 14.0290,
  13.4444, 12.9067, 12.4103, 11.9506, 11.5238, 11.1264, 10.7556, 10.4086,
  10.0833, 9.7778, 9.4902, 9.2190, 8.9630, 8.7207, 8.4912, 8.2735,
  8.0667, 7.8699, 7.6825, 7.5039, 7.3333, 7.1704, 7.0145, 6.8652,
  6.7222, 6.5850, 6.4533, 6.3268, 6.2051, 6.0881, 5.9753, 5.8667,
  5.7619, 5.6608, 5.5632, 5.4689, 5.3778, 5.2896, 5.2043, 5.1217,
  5.0417, 4.9641, 4.8889, 4.8159, 4.7451, 4.6763, 4.6095, 4.5446,
  4.4815, 4.4201, 4.3604, 4.3022, 4.2456, 4.1905, 4.1368, 4.0844,
  4.0333, 3.9835, 3.9350, 3.8876, 3.8413, 3.7961, 3.7519, 3.7088,
  3.6667, 3.6255, 3.5852, 3.5458, 3.5072, 3.4695, 3.4326, 3.3965,
  3.3611, 3.3265, 3.2925, 3.2593, 3.2267, 3.1947, 3.1634, 3.1327,
  3.1026, 3.0730, 3.0440, 3.0156, 2.9877, 2.9602, 2.9333, 2.9069,
  2.8810, 2.8555, 2.8304, 2.8058, 2.7816, 2.7578, 2.7345, 2.7115,
  2.6889, 2.6667, 2.6448, 2.6233, 2.6022, 2.5813, 2.5608, 2.5407,
  2.5208, 2.5013, 2.4821, 2.4631, 2.4444, 2.4261, 2.4080, 2.3901,
  2.3725, 2.3552, 2.3382, 2.3213, 2.3048, 2.2884, 2.2723, 2.2564,
  2.2407, 2.2253, 2.2100, 2.1950, 2.1802, 2.1655, 2.1511, 2.1369,
  2.1228, 2.1089, 2.0952, 2.0817, 2.0684, 2.0552, 2.0422, 2.0294,
  2.0167, 2.0041, 1.9918, 1.9796, 1.9675, 1.9556, 1.9438, 1.9321,
  1.9206, 1.9093, 1.8980, 1.8869, 1.8760, 1.8651, 1.8544, 1.8438,
  1.8333, 1.8230, 1.8127, 1.8026, 1.7926, 1.7827, 1.7729, 1.7632,
  1.7536, 1.7441, 1.7348, 1.7255, 1.7163, 1.7072, 1.6982, 1.6894,
  1.6806, 1.6718, 1.6632, 1.6547, 1.6463, 1.6379, 1.6296, 1.6214,
  1.6133, 1.6053, 1.5974, 1.5895, 1.5817, 1.5740, 1.5663, 1.5588,
  1.5513, 1.5439, 1.5365, 1.5292, 1.5220, 1.5149, 1.5078, 1.5008,
  1.4938, 1.4869, 1.4801, 1.4734, 1.4667, 1.4600, 1.4535, 1.4469,
  1.4405, 1.4341, 1.4277, 1.4214, 1.4152, 1.4090, 1.4029, 1.3968,
  1.3908, 1.3848, 1.3789, 1.3730, 1.3672, 1.3615, 1.3557, 1.3501,
  1.3444, 1.3389, 1.3333, 1.3278, 1.3224, 1.3170, 1.3117, 1.3063,
  1.3011, 1.2959, 1.2907, 1.2855, 1.2804, 1.2754, 1.2703, 1.2654,
  1.2604, 1.2555, 1.2506, 1.2458, 1.2410, 1.2363, 1.2316, 1.2269,
  1.2222, 1.2176, 1.2130, 1.2085, 1.2040, 1.1995, 1.1951, 1.1907,
  1.1863, 1.1819, 1.1776, 1.1733, 1.1691, 1.1649, 1.1607, 1.1565,
  1.1524, 1.1483, 1.1442, 1.1402, 1.1362, 1.1322, 1.1282, 1.1243,
  1.1204, 1.1165, 1.1126, 1.1088, 1.1050, 1.1013, 1.0975, 1.0938,
  1.0901, 1.0864, 1.0828, 1.0792, 1.0756, 1.0720, 1.0684, 1.0649,
  1.0614, 1.0579, 1.0545, 1.0510, 1.0476, 1.0442, 1.0409, 1.0375,
  1.0342, 1.0309, 1.0276, 1.0243, 1.0211, 1.0179, 1.0147, 1.0115,
  1.0083, 1.0052, 1.0021, 0.9990, 0.9959, 0.9928, 0.9898, 0.9867,
  0.9837, 0.9807, 0.9778, 0.9748, 0.9719, 0.9690, 0.9661, 0.9632,
  0.9603, 0.9575, 0.9546, 0.9518, 0.9490, 0.9462, 0.9435, 0.9407,
  0.9380, 0.9353, 0.9326, 0.9299, 0.9272, 0.9245, 0.9219, 0.9193,
  0.9167, 0.9141, 0.9115, 0.9089, 0.9064, 0.9038, 0.9013, 0.8988,
  0.8963, 0.8938, 0.8913, 0.8889, 0.8864, 0.8840, 0.8816, 0.8792,
  0.8768, 0.8744, 0.8721, 0.8697, 0.8674, 0.8651, 0.8627, 0.8604,
  0.8582, 0.8559, 0.8536, 0.8514, 0.8491, 0.8469, 0.8447, 0.8425,
  0.8403, 0.8381, 0.8359, 0.8338, 0.8316, 0.8295, 0.8274, 0.8252,
  0.8231, 0.8210, 0.8190, 0.8169, 0.8148, 0.8128, 0.8107, 0.8087,
  0.8067, 0.8047, 0.8027, 0.8007, 0.7987, 0.7967, 0.7947, 0.7928,
  0.7908, 0.7889, 0.7870, 0.7851, 0.7832, 0.7813, 0.7794, 0.7775,
  0.7756, 0.7738, 0.7719, 0.7701, 0.7683, 0.7664, 0.7646, 0.7628,
  0.7610, 0.7592, 0.7574, 0.7557, 0.7539, 0.7521, 0.7504, 0.7486,
  0.7469, 0.7452, 0.7435, 0.7418, 0.7401, 0.7384, 0.7367, 0.7350,
  0.7333, 0.7317, 0.7300, 0.7284, 0.7267, 0.7251, 0.7235, 0.7218,
  0.7202, 0.7186, 0.7170, 0.7154, 0.7139, 0.7123, 0.7107, 0.7092,
  0.7076, 0.7061, 0.7045, 0.7030, 0.7014, 0.6999, 0.6984, 0.6969,
  0.6954, 0.6939, 0.6924, 0.6909, 0.6895, 0.6880, 0.6865, 0.6851,
  0.6836, 0.6822, 0.6807, 0.6793, 0.6779, 0.6765, 0.6750, 0.6736,
  0.6722, 0.6708, 0.6694, 0.6680, 0.6667, 0.6653, 0.6639, 0.6626,
  0.6612, 0.6599, 0.6585, 0.6572, 0.6558, 0.6545, 0.6532, 0.6519,
  0.6505, 0.6492, 0.6479, 0.6466, 0.6453, 0.6440, 0.6428, 0.6415,
  0.6402, 0.6389, 0.6377, 0.6364, 0.6352, 0.6339, 0.6327, 0.6314
};

void Statistics::Begin(Settings *settings, SensorData *sensorData) {
  m_settings = settings;
  m_sensorData = sensorData;
  thresholdOffset = m_settings->GetFloat("ThresholdOffset", DEFAULT_THRESHOLD_OFFSET);
  Reset();
}

void Statistics::Calibrate() {
  // store current magMax values as calibration reference
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++) {
    // avoid threshold to be 0
    if (m_sensorData->binGroup[binGroupNr].magMax > 0) {
      m_sensorData->binGroup[binGroupNr].magThresh = m_sensorData->binGroup[binGroupNr].magMax;
    } else {
      m_sensorData->binGroup[binGroupNr].magThresh = 1;
    }
  }
}

void Statistics::ResetPreciAmountAcc() {
  m_sensorData->preciAmountAcc = 0;
}

void Statistics::Calc() {
  ////uint8_t aboveThresh;
  
  for (uint16_t binNr = 0; binNr < NR_OF_BINS; binNr++) {    
    if (m_sensorData->bin[binNr].mag > m_sensorData->bin[binNr].magMax) {
      m_sensorData->bin[binNr].magMax = m_sensorData->bin[binNr].mag;
    }
  }
    
  // ignore magnitudes below threshold
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++) { 
    ////aboveThresh = 0;
    // scan through all bins within the group
    for (uint16_t binNr = m_sensorData->binGroup[binGroupNr].firstBin; binNr <= m_sensorData->binGroup[binGroupNr].lastBin; binNr++) {
      if (m_sensorData->bin[binNr].mag > (m_sensorData->binGroup[binGroupNr].magThresh + thresholdOffset)) {
        m_sensorData->bin[binNr].magSum += m_sensorData->bin[binNr].mag;
        m_sensorData->binGroup[binGroupNr].magAboveThreshCnt += 1 / dropInFOVsnapshots[binNr];    // TODO: Use precalculated values
        ////aboveThresh = 1;
      }
    }
    ////if (aboveThresh) {
    ////  m_sensorData->binGroup[binGroupNr].magAboveThreshCnt++;
    ////}
  }
}

void Statistics::Finalize() {
  uint32_t magSumGroup;
  float magSumGroupKorr;
  float magSumGroupKorrCal;
  float magSumGroupKorrCalThresh;
  float maxMagAVGkorr;
  float maxMagAboveThreshCnt;
  uint32_t nrOfBinsInGroup;

  // bin-level statistics
  m_sensorData->magAVG = 0;
  m_sensorData->magAVGkorr = 0;
  m_sensorData->magMax = 0;
  
  for (uint16_t binNr = 0; binNr < NR_OF_BINS; binNr++) {
    m_sensorData->bin[binNr].magAVG = m_sensorData->bin[binNr].magSum / m_sensorData->snapshotCtr;
    m_sensorData->bin[binNr].magAVGkorr = m_sensorData->bin[binNr].magAVG / dropInFOVsnapshots[binNr];

    if (binNr > 0) {
      m_sensorData->magAVG += m_sensorData->bin[binNr].magAVG;
      m_sensorData->magAVGkorr += m_sensorData->bin[binNr].magAVGkorr;
      
      if (m_sensorData->bin[binNr].magMax > m_sensorData->magMax) {
        m_sensorData->magMax = m_sensorData->bin[binNr].magMax;
      }
    }
  }
  m_sensorData->magAVG /= NR_OF_BINS - 1;
  m_sensorData->magAVGkorr /= NR_OF_BINS - 1;


  // group-level statistics
  m_sensorData->DomGroupMagAVGkorr = 0;
  m_sensorData->DomGroupMagAboveThreshCnt = 0;
  maxMagAVGkorr = 0;
  maxMagAboveThreshCnt = 0;
  
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++) {
    nrOfBinsInGroup = 0;
    magSumGroup = 0;
    magSumGroupKorr = 0;
    m_sensorData->binGroup[binGroupNr].magMax = 0;
   
    // scan through all bins within the group
    for (uint16_t binNr = m_sensorData->binGroup[binGroupNr].firstBin; binNr <= m_sensorData->binGroup[binGroupNr].lastBin; binNr++) {
      magSumGroup += m_sensorData->bin[binNr].magSum;
      magSumGroupKorr += m_sensorData->bin[binNr].magSum / dropInFOVsnapshots[binNr];
      
      if (m_sensorData->bin[binNr].magMax > m_sensorData->binGroup[binGroupNr].magMax) {
        m_sensorData->binGroup[binGroupNr].magMax = m_sensorData->bin[binNr].magMax;
      }
      nrOfBinsInGroup++;
    }
    m_sensorData->binGroup[binGroupNr].magAVG = magSumGroup / (float)nrOfBinsInGroup / (float)m_sensorData->snapshotCtr;
    m_sensorData->binGroup[binGroupNr].magAVGkorr = magSumGroupKorr / (float)nrOfBinsInGroup / (float)m_sensorData->snapshotCtr;

    if (m_sensorData->binGroup[binGroupNr].magAVGkorr > maxMagAVGkorr) {
      maxMagAVGkorr = m_sensorData->binGroup[binGroupNr].magAVGkorr;
      m_sensorData->DomGroupMagAVGkorr = binGroupNr;
    }

    if (m_sensorData->binGroup[binGroupNr].magAboveThreshCnt > maxMagAboveThreshCnt) {
      maxMagAboveThreshCnt = m_sensorData->binGroup[binGroupNr].magAboveThreshCnt; 
      m_sensorData->DomGroupMagAboveThreshCnt = binGroupNr;
    }

    m_sensorData->binGroup[binGroupNr].magAVGkorrDom = 0;
    m_sensorData->binGroup[binGroupNr].magAVGkorrDom2 = 0;
    m_sensorData->binGroup[binGroupNr].magAboveThreshCntDom = 0;
  }

  m_sensorData->binGroup[m_sensorData->DomGroupMagAVGkorr].magAVGkorrDom = m_sensorData->binGroup[m_sensorData->DomGroupMagAVGkorr].magAVGkorr;
  m_sensorData->binGroup[m_sensorData->DomGroupMagAboveThreshCnt].magAVGkorrDom2 = m_sensorData->binGroup[m_sensorData->DomGroupMagAboveThreshCnt].magAVGkorr;                  // TEST: dom index from count, value from magAVG
  m_sensorData->binGroup[m_sensorData->DomGroupMagAboveThreshCnt].magAboveThreshCntDom = m_sensorData->binGroup[m_sensorData->DomGroupMagAboveThreshCnt].magAboveThreshCnt;
    
  m_sensorData->preciAmount = 0;
  if (m_sensorData->DomGroupMagAboveThreshCnt < DOM_GROUP_RAIN_FIRST) {
    // snowing
  } else if (m_sensorData->DomGroupMagAboveThreshCnt <= DOM_GROUP_RAIN_LAST) {
    // raining
    for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++) {
      m_sensorData->preciAmount += m_sensorData->binGroup[binGroupNr].magAVGkorr * m_sensorData->binGroup[binGroupNr].preciAmountFactor;      
    }
    m_sensorData->preciAmountAcc += m_sensorData->preciAmount;  
  } else {
    // hailing
  }
}

void Statistics::Reset()
{
  // bin-level
  for (uint16_t binNr = 0; binNr < NR_OF_BINS; binNr++) {
    m_sensorData->bin[binNr].magSum = 0;
    m_sensorData->bin[binNr].magMax = 0;
  }

  // group-level
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++) {
    m_sensorData->binGroup[binGroupNr].magAboveThreshCnt = 0;
  }

  m_sensorData->ADCpeakSample = 0;
  m_sensorData->clippingCtr = 0;
  m_sensorData->RbOvCtr = 0;
  m_sensorData->snapshotCtr = 0;
}

