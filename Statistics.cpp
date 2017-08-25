#include "Statistics.h"

void Statistics::Begin(Settings *settings, SensorData *sensorData) {
  m_settings = settings;
  m_sensorData = sensorData;
  detectionThreshold = m_settings->GetFloat("DetectionThreshold", DEFAULT_DETECTION_TRESHOLD);
  Reset();
}

float Statistics::Calibrate()
{
  uint16_t magMin = ~0;
  
//  m_sigProc->StopCapture();

  // Find minimum magAVG value
  for (uint16_t binNr = 0; binNr < NR_OF_BINS; binNr++) {
    if (m_sensorData->bin[binNr].mag < magMin) {
      magMin = m_sensorData->bin[binNr].mag;
    }
  }

  if (magMin == 0)
    magMin = 1;

  // Calculate normalized threshold-factors and update thresholds
  for (uint16_t binNr = 0; binNr < NR_OF_BINS; binNr++) {
    m_sensorData->bin[binNr].threshold = (float)(detectionThreshold * m_sensorData->bin[binNr].mag) / magMin;
    //Serial.printf("%f ", m_sensorData->bin[binNr].threshold);
  }

/*
  Serial.println("");
  for (uint16_t binNr = 0; binNr < NR_OF_BINS; binNr++) {
    Serial.printf("%d ", m_sensorData->bin[binNr].mag);
  }
*/

//  ->StartCapture();

  return 1.0;   // TODO: replace "default" threshold factor?
}

void Statistics::Calc()
{
  for (uint16_t binNr = 0; binNr < NR_OF_BINS; binNr++)
  {
    if (m_sensorData->bin[binNr].mag > m_sensorData->bin[binNr].magPeak)
      m_sensorData->bin[binNr].magPeak = m_sensorData->bin[binNr].mag;

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

      if (m_sensorData->bin[binNr].magAVG > m_sensorData->bin[binNr].threshold) {
        m_sensorData->bin[binNr].magAVGkorrThresh = (m_sensorData->bin[binNr].magAVG - m_sensorData->bin[binNr].threshold) / dropInFOVsnapshots_table[binNr];
        m_sensorData->magAVGkorrThresh += m_sensorData->bin[binNr].magAVGkorrThresh;
      }
    }
  }
  
  m_sensorData->magAVG = m_sensorData->magAVG / m_sensorData->snapshotCtr / (NR_OF_BINS - 1);
  m_sensorData->magAVGkorr = m_sensorData->magAVGkorr / m_sensorData->snapshotCtr / (NR_OF_BINS - 1);
 
  
  // group-level statistics
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++) {
    magSumGroup = 0;
    magSumGroupKorr = 0;
    magSumGroupKorrThresh = 0;
    m_sensorData->binGroup[binGroupNr].magPeak = 0;
   
    // scan through all bins within the group
    for (uint16_t binNr = m_sensorData->binGroup[binGroupNr].firstBin; binNr <= m_sensorData->binGroup[binGroupNr].lastBin; binNr++) {
      magSumGroup += m_sensorData->bin[binNr].magSum;
      magSumGroupKorr += ((float)m_sensorData->bin[binNr].magSum / dropInFOVsnapshots_table[binNr]);
      
      if (m_sensorData->bin[binNr].magAVG > m_sensorData->bin[binNr].threshold) {
        magSumGroupKorrThresh += (m_sensorData->bin[binNr].magAVG - m_sensorData->bin[binNr].threshold) / dropInFOVsnapshots_table[binNr];
      }

      if (m_sensorData->bin[binNr].magPeak > m_sensorData->binGroup[binGroupNr].magPeak) {
        m_sensorData->binGroup[binGroupNr].magPeak = m_sensorData->bin[binNr].magPeak;
      }
    }

    m_sensorData->binGroup[binGroupNr].magAVG = (float)magSumGroup / m_sensorData->snapshotCtr / (m_sensorData->binGroup[binGroupNr].lastBin - m_sensorData->binGroup[binGroupNr].firstBin + 1);
    m_sensorData->binGroup[binGroupNr].magAVGkorr = (float)magSumGroupKorr / m_sensorData->snapshotCtr / (m_sensorData->binGroup[binGroupNr].lastBin - m_sensorData->binGroup[binGroupNr].firstBin + 1);
    m_sensorData->binGroup[binGroupNr].magAVGkorrThresh = (float)magSumGroupKorrThresh / m_sensorData->snapshotCtr / (m_sensorData->binGroup[binGroupNr].lastBin - m_sensorData->binGroup[binGroupNr].firstBin + 1);
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
