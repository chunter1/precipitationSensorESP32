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

const uint8_t binMaskCyclesPerMeter_table[] = {
  0, 228, 114, 76, 57, 46, 38, 33, 
  29, 25, 23, 21, 19, 18, 16, 15, 
  14, 13, 13, 12, 11, 11, 10, 10, 
  10, 9, 9, 8, 8, 8, 8, 7, 
  7, 7, 7, 7, 6, 6, 6, 6,
  6, 6, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1
};
const uint16_t SIZE_OF_LOCK_CYCLE_TABLE = sizeof(binMaskCyclesPerMeter_table) / sizeof(binMaskCyclesPerMeter_table[0]);

// **************************************************
// **************************************************
// **************************************************
void updateStatistics()
{
  static uint8_t binMaskCycles[NR_OF_BINS];
  uint16_t mag[NR_OF_BINS];
  uint16_t magPeak;


  // Bin-level statistics (exclude DC-bin)
  for (uint16_t binNr = 1; binNr < NR_OF_BINS; binNr++)
  {
    mag[binNr] = sqrt(pow(re[binNr], 2) + pow(im[binNr], 2));

    // peak
    if (mag[binNr] > bin[binNr].magPeak)
      bin[binNr].magPeak = mag[binNr];

    // sum (for AVG)
    bin[binNr].magSum += mag[binNr];

    //binMaskCycles[binNr] = 0;                 // disable masking for testing

    // speed-dependent masking
    if (binMaskCycles[binNr] > 0)
    {
      binMaskCycles[binNr]--;
    } else {
      if (mag[binNr] > DETECTION_THRESHOLD)
      {
        bin[binNr].detections++;
        totalDetectionsCtr++;
        
        // always mask out bin in next cycle
        binMaskCycles[binNr] = 1;

        // avoid multiple detections/counts of same "signal-event" spread across multiple FFT windows due to low-speed particles
        if (binNr < SIZE_OF_LOCK_CYCLE_TABLE)
          binMaskCycles[binNr] += FOV_m * binMaskCyclesPerMeter_table[binNr];
      }
    }
  }
}

// **************************************************
// **************************************************
// **************************************************
void finalizeStatistics()
{
  uint32_t magSumGroup;
  
  // bin-level statistic
  for (uint16_t binNr = 0; binNr < NR_OF_BINS; binNr++)
  {
    bin[binNr].magAVG = bin[binNr].magSum / snapshotCtr;
  }
  
  // group-level statistics
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++)
  {
    magSumGroup = 0;
    binGroup[binGroupNr].magPeak = 0;
    binGroup[binGroupNr].detections = 0;
    
    // scan through all bins within the group
    for (uint16_t binNr = binGroupBoundaries.firstBin[binGroupNr]; binNr <= binGroupBoundaries.lastBin[binGroupNr]; binNr++)
    {
      magSumGroup += bin[binNr].magSum;

      if (bin[binNr].magPeak > binGroup[binGroupNr].magPeak) 
        binGroup[binGroupNr].magPeak = bin[binNr].magPeak;

      binGroup[binGroupNr].detections += bin[binNr].detections;
    }
    
    binGroup[binGroupNr].magAVG = (float)magSumGroup / snapshotCtr / (binGroupBoundaries.lastBin[binGroupNr] - binGroupBoundaries.firstBin[binGroupNr] + 1);
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
    bin[binNr].magSum = 0;
    bin[binNr].magAVG = 0;
    bin[binNr].magPeak = 0;
    bin[binNr].detections = 0;
  }

  // group-level statistics
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++)
  {
    binGroup[binGroupNr].magSum = 0;
    binGroup[binGroupNr].magAVG = 0;
    binGroup[binGroupNr].magPeak = 0;
    binGroup[binGroupNr].detections = 0;
  }

  ADCpeakSample = 0;
  clippingCtr = 0;
  totalDetectionsCtr = 0;
}

