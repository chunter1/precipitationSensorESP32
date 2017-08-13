/***************************************************************************\
 * 
 * Arduino project "precipitationSensor" © Copyright huawatuam@gmail.com 
 * 
 * This program is free software. You can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version. This program is distributed in
 * the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
 * PURPOSE. See the GNU General Public License for more details.
 * You received a copy of the GNU General Public License along with this
 * program in file 'License.txt'.
 * Additional information about licensing can be found at:
 * http://www.gnu.org/licenses
 * 
\***************************************************************************/


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

// **************************************************
// **************************************************
// **************************************************
void updateStatistics()
{
  for (uint16_t binNr = 0; binNr < NR_OF_BINS; binNr++)
  {
    uint16_t mag = sqrt(pow(re[binNr], 2) + pow(im[binNr], 2));

    if (mag > sensorData.bin[binNr].magPeak)
      sensorData.bin[binNr].magPeak = mag;

    sensorData.bin[binNr].magSum += mag;

    // TODO: Evalute correction factor
    if (mag > (sensorData.bin[binNr].threshold * 2.4))
      sensorData.bin[binNr].detections++;
  }
}

// **************************************************
// **************************************************
// **************************************************
void finalizeStatistics()
{
  uint32_t magSumGroup;
  float magSumGroupKorr;
  float magSumGroupKorrThresh;


  // bin-level statistics
  sensorData.magAVG = 0;
  sensorData.magAVGkorr = 0;
  sensorData.magAVGkorrThresh = 0;
  sensorData.magPeak = 0;
  sensorData.totalDetectionsCtr = 0;
  
  for (uint16_t binNr = 0; binNr < NR_OF_BINS; binNr++)
  {
    sensorData.bin[binNr].magAVG = (float)sensorData.bin[binNr].magSum / sensorData.snapshotCtr;
    sensorData.bin[binNr].magAVGkorr = sensorData.bin[binNr].magAVG / dropInFOVsnapshots_table[binNr];
    sensorData.bin[binNr].detections /= dropInFOVsnapshots_table[binNr];

    if (binNr > 0)
    {
      sensorData.magAVG += sensorData.bin[binNr].magSum;
      sensorData.magAVGkorr += sensorData.bin[binNr].magSum / dropInFOVsnapshots_table[binNr];
      sensorData.totalDetectionsCtr += sensorData.bin[binNr].detections;
      
      if (sensorData.bin[binNr].magPeak > sensorData.magPeak)
        sensorData.magPeak = sensorData.bin[binNr].magPeak;

      if (sensorData.bin[binNr].magAVG > sensorData.bin[binNr].threshold) {
        sensorData.bin[binNr].magAVGkorrThresh = (sensorData.bin[binNr].magAVG - sensorData.bin[binNr].threshold) / dropInFOVsnapshots_table[binNr];
        sensorData.magAVGkorrThresh += sensorData.bin[binNr].magAVGkorrThresh;
      }
    }
  }
  
  sensorData.magAVG = sensorData.magAVG / sensorData.snapshotCtr / (NR_OF_BINS - 1);
  sensorData.magAVGkorr = sensorData.magAVGkorr / sensorData.snapshotCtr / (NR_OF_BINS - 1);
 
  
  // group-level statistics
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++)
  {
    magSumGroup = 0;
    magSumGroupKorr = 0;
    magSumGroupKorrThresh = 0;
    sensorData.binGroup[binGroupNr].magPeak = 0;
    sensorData.binGroup[binGroupNr].detections = 0;
    
    // scan through all bins within the group
    for (uint16_t binNr = binGroupBoundaries.firstBin[binGroupNr]; binNr <= binGroupBoundaries.lastBin[binGroupNr]; binNr++)
    {
      magSumGroup += sensorData.bin[binNr].magSum;
      magSumGroupKorr += ((float)sensorData.bin[binNr].magSum / dropInFOVsnapshots_table[binNr]);
      
      if (sensorData.bin[binNr].magAVG > sensorData.bin[binNr].threshold)
        magSumGroupKorrThresh += (sensorData.bin[binNr].magAVG - sensorData.bin[binNr].threshold) / dropInFOVsnapshots_table[binNr];

      if (sensorData.bin[binNr].magPeak > sensorData.binGroup[binGroupNr].magPeak)
        sensorData.binGroup[binGroupNr].magPeak = sensorData.bin[binNr].magPeak;
      
      sensorData.binGroup[binGroupNr].detections += sensorData.bin[binNr].detections;
    }
    
    sensorData.binGroup[binGroupNr].magAVG = (float)magSumGroup / sensorData.snapshotCtr / (binGroupBoundaries.lastBin[binGroupNr] - binGroupBoundaries.firstBin[binGroupNr] + 1);
    sensorData.binGroup[binGroupNr].magAVGkorr = (float)magSumGroupKorr / sensorData.snapshotCtr / (binGroupBoundaries.lastBin[binGroupNr] - binGroupBoundaries.firstBin[binGroupNr] + 1);
    sensorData.binGroup[binGroupNr].magAVGkorrThresh = (float)magSumGroupKorrThresh / sensorData.snapshotCtr / (binGroupBoundaries.lastBin[binGroupNr] - binGroupBoundaries.firstBin[binGroupNr] + 1);
  }
}

// **************************************************
// **************************************************
// **************************************************
void resetStatistics()
{
  // bin-level statistic
  for (uint16_t binNr = 0; binNr < NR_OF_BINS; binNr++)
  {
    sensorData.bin[binNr].magSum = 0;
    sensorData.bin[binNr].magAVG = 0;
    sensorData.bin[binNr].magAVGkorr = 0;
    sensorData.bin[binNr].magAVGkorrThresh = 0;
    sensorData.bin[binNr].magPeak = 0;
    sensorData.bin[binNr].detections = 0;
  }

  // group-level statistics
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++)
  {
    sensorData.binGroup[binGroupNr].magSum = 0;
    sensorData.binGroup[binGroupNr].magAVG = 0;
    sensorData.binGroup[binGroupNr].magAVGkorr = 0;
    sensorData.binGroup[binGroupNr].magAVGkorrThresh = 0;
    sensorData.binGroup[binGroupNr].magPeak = 0;
    sensorData.binGroup[binGroupNr].detections = 0;
  }

  sensorData.magAVG = 0;
  sensorData.magAVGkorr = 0;
  sensorData.magAVGkorrThresh = 0;
  sensorData.magPeak = 0;
  sensorData.ADCpeakSample = 0;
  sensorData.clippingCtr = 0;
  sensorData.totalDetectionsCtr = 0;
}
