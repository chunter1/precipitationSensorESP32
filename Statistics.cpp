#include "Statistics.h"

/* The following table holds the number of snapshots (a 25ms) that the "same" drop is
 * visible to the radar sensor, based on an average 1m FOV with a 0° sensor tilt
 */
const float dropInFOVsnapshots_table[NR_OF_BINS] = {
  1.00, 322.67, 161.33, 107.56, 80.67, 64.53, 53.78, 46.10,
  40.33, 35.85, 32.27, 29.33, 26.89, 24.82, 23.05, 21.51,
  20.17, 18.98, 17.93, 16.98, 16.13, 15.37, 14.67, 14.03,
  13.44, 12.91, 12.41, 11.95, 11.52, 11.13, 10.76, 10.41,
  10.08, 9.78, 9.49, 9.22, 8.96, 8.72, 8.49, 8.27,
  8.07, 7.87, 7.68, 7.50, 7.33, 7.17, 7.01, 6.87,
  6.72, 6.59, 6.45, 6.33, 6.21, 6.09, 5.98, 5.87,
  5.76, 5.66, 5.56, 5.47, 5.38, 5.29, 5.20, 5.12,
  
  5.04, 4.96, 4.89, 4.82, 4.75, 4.68, 4.61, 4.54,
  4.48, 4.42, 4.36, 4.30, 4.25, 4.19, 4.14, 4.08,
  4.03, 3.98, 3.93, 3.89, 3.84, 3.80, 3.75, 3.71,
  3.67, 3.63, 3.59, 3.55, 3.51, 3.47, 3.43, 3.40,
  3.36, 3.33, 3.29, 3.26, 3.23, 3.19, 3.16, 3.13,
  3.10, 3.07, 3.04, 3.02, 2.99, 2.96, 2.93, 2.91,
  2.88, 2.86, 2.83, 2.81, 2.78, 2.76, 2.73, 2.71,
  2.69, 2.67, 2.64, 2.62, 2.60, 2.58, 2.56, 2.54,
  
  2.52, 2.50, 2.48, 2.46, 2.44, 2.43, 2.41, 2.39,
  2.37, 2.36, 2.34, 2.32, 2.30, 2.29, 2.27, 2.26,
  2.24, 2.23, 2.21, 2.20, 2.18, 2.17, 2.15, 2.14,
  2.12, 2.11, 2.10, 2.08, 2.07, 2.06, 2.04, 2.03,
  2.02, 2.00, 1.99, 1.98, 1.97, 1.96, 1.94, 1.93,
  1.92, 1.91, 1.90, 1.89, 1.88, 1.87, 1.85, 1.84,
  1.83, 1.82, 1.81, 1.80, 1.79, 1.78, 1.77, 1.76,
  1.75, 1.74, 1.73, 1.73, 1.72, 1.71, 1.70, 1.69,
  
  1.68, 1.67, 1.66, 1.65, 1.65, 1.64, 1.63, 1.62,
  1.61, 1.61, 1.60, 1.59, 1.58, 1.57, 1.57, 1.56,
  1.55, 1.54, 1.54, 1.53, 1.52, 1.51, 1.51, 1.50,
  1.49, 1.49, 1.48, 1.47, 1.47, 1.46, 1.45, 1.45,
  1.44, 1.43, 1.43, 1.42, 1.42, 1.41, 1.40, 1.40,
  1.39, 1.38, 1.38, 1.37, 1.37, 1.36, 1.36, 1.35,
  1.34, 1.34, 1.33, 1.33, 1.32, 1.32, 1.31, 1.31,
  1.30, 1.30, 1.29, 1.29, 1.28, 1.28, 1.27, 1.27
  };

/* 
 * TEST values based on LME49720 preamp - need to be replaced by worst case values
 */
const float magAVGkorrThresholds_table[NR_OF_BIN_GROUPS] = {
  0.0107, 0.0258, 0.0334, 0.0447, 0.0528, 0.0592, 0.0665, 0.0710,
  0.0873, 0.0849, 0.0954, 0.1078, 0.1578, 0.1154, 0.1455, 0.1570,
  0.1771, 0.1781, 0.1531, 0.2957, 0.1976, 0.1804, 0.1985, 0.1958,
  0.2363, 0.2294, 0.2495, 0.2784, 0.3136, 0.3140, 0.3972, 0.4420
};

void Statistics::Begin(Settings *settings, SensorData *sensorData) {
  m_settings = settings;
  m_sensorData = sensorData;
  detectionThreshold = m_settings->GetFloat("DetectionThreshold", DEFAULT_DETECTION_TRESHOLD);

  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++) {
    m_sensorData->binGroup[binGroupNr].threshold = magAVGkorrThresholds_table[binGroupNr] * detectionThreshold;
  }

  Reset();
}

float Statistics::Calibrate()
{
  float magAVGkorrMin = ~0;
  
//  m_sigProc->StopCapture();

  // Find minimum magAVGkorr value
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++) {
    if (m_sensorData->binGroup[binGroupNr].magAVGkorr < magAVGkorrMin) {
      magAVGkorrMin = m_sensorData->binGroup[binGroupNr].magAVGkorr;
    }
  }

  if (magAVGkorrMin == 0)
    magAVGkorrMin = 1;

  // Calculate normalized threshold-factors and update thresholds
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++) {
    m_sensorData->binGroup[binGroupNr].threshold = m_sensorData->binGroup[binGroupNr].magAVGkorr / magAVGkorrMin;     // TODO: reboot is needed or values need to be multiplied with detectionThreshold to be valid!
    //Serial.printf("%f ", m_sensorData->binGroup[binGroupNr].threshold);
  }

//  ->StartCapture();

  return 1.0;   // TODO: replace "default" threshold factor?
}

void Statistics::Calc()
{
  for (uint16_t binNr = 0; binNr < NR_OF_BINS; binNr++)
  {
    if (m_sensorData->bin[binNr].mag > m_sensorData->bin[binNr].magPeak) {
      m_sensorData->bin[binNr].magPeak = m_sensorData->bin[binNr].mag;
    }
    m_sensorData->bin[binNr].magSum += m_sensorData->bin[binNr].mag;
  }
}

void Statistics::Finalize()
{
  uint32_t magSumGroup;
  float magSumGroupKorr;
  float magSumGroupKorrThresh;

  // bin-level statistics
  m_sensorData->magAVG = 0;
  m_sensorData->magAVGkorr = 0;
  m_sensorData->magAVGkorrThresh = 0;
  m_sensorData->magPeak = 0;
  
  for (uint16_t binNr = 0; binNr < NR_OF_BINS; binNr++) {
    m_sensorData->bin[binNr].magAVG = (float)m_sensorData->bin[binNr].magSum /m_sensorData->snapshotCtr;
    m_sensorData->bin[binNr].magAVGkorr = m_sensorData->bin[binNr].magAVG / dropInFOVsnapshots_table[binNr];

    if (binNr > 0) {
      m_sensorData->magAVG += m_sensorData->bin[binNr].magSum;
      m_sensorData->magAVGkorr += m_sensorData->bin[binNr].magSum / dropInFOVsnapshots_table[binNr];
      
      if (m_sensorData->bin[binNr].magPeak > m_sensorData->magPeak) {
        m_sensorData->magPeak = m_sensorData->bin[binNr].magPeak;
      }
    }
  }
  
  m_sensorData->magAVG = m_sensorData->magAVG / m_sensorData->snapshotCtr / (NR_OF_BINS - 1);
  m_sensorData->magAVGkorr = m_sensorData->magAVGkorr / m_sensorData->snapshotCtr / (NR_OF_BINS - 1);

  
  // group-level statistics
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++) {
    magSumGroup = 0;
    magSumGroupKorr = 0;
    m_sensorData->binGroup[binGroupNr].magPeak = 0;
   
    // scan through all bins within the group
    for (uint16_t binNr = m_sensorData->binGroup[binGroupNr].firstBin; binNr <= m_sensorData->binGroup[binGroupNr].lastBin; binNr++) {
      magSumGroup += m_sensorData->bin[binNr].magSum;
      magSumGroupKorr += ((float)m_sensorData->bin[binNr].magSum / dropInFOVsnapshots_table[binNr]);

      if (m_sensorData->bin[binNr].magPeak > m_sensorData->binGroup[binGroupNr].magPeak) {
        m_sensorData->binGroup[binGroupNr].magPeak = m_sensorData->bin[binNr].magPeak;
      }
    }

    m_sensorData->binGroup[binGroupNr].magAVG = (float)magSumGroup / m_sensorData->snapshotCtr / (m_sensorData->binGroup[binGroupNr].lastBin - m_sensorData->binGroup[binGroupNr].firstBin + 1);
    m_sensorData->binGroup[binGroupNr].magAVGkorr = (float)magSumGroupKorr / m_sensorData->snapshotCtr / (m_sensorData->binGroup[binGroupNr].lastBin - m_sensorData->binGroup[binGroupNr].firstBin + 1);

    if (m_sensorData->binGroup[binGroupNr].magAVGkorr > m_sensorData->binGroup[binGroupNr].threshold) {
      m_sensorData->binGroup[binGroupNr].magAVGkorrThresh = m_sensorData->binGroup[binGroupNr].magAVGkorr - m_sensorData->binGroup[binGroupNr].threshold;
      m_sensorData->magAVGkorrThresh += m_sensorData->binGroup[binGroupNr].magAVGkorrThresh;
    }    
  }
}

void Statistics::Reset()
{
  // bin-level statistic
  for (uint16_t binNr = 0; binNr < NR_OF_BINS; binNr++) {
    m_sensorData->bin[binNr].magSum = 0;
    m_sensorData->bin[binNr].magAVG = 0;
    m_sensorData->bin[binNr].magAVGkorr = 0;
    m_sensorData->bin[binNr].magAVGkorrThresh = 0;
    m_sensorData->bin[binNr].magPeak = 0;
  }
  
  // group-level statistics
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++) {
    m_sensorData->binGroup[binGroupNr].magSum = 0;
    m_sensorData->binGroup[binGroupNr].magAVG = 0;
    m_sensorData->binGroup[binGroupNr].magAVGkorr = 0;
    m_sensorData->binGroup[binGroupNr].magAVGkorrThresh = 0;
    m_sensorData->binGroup[binGroupNr].magPeak = 0;
  }

  m_sensorData->magAVG = 0;
  m_sensorData->magAVGkorr = 0;
  m_sensorData->magAVGkorrThresh = 0;
  m_sensorData->magPeak = 0;
  m_sensorData->ADCpeakSample = 0;
  m_sensorData->clippingCtr = 0;
}
