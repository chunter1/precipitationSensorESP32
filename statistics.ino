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

const uint8_t binMaskCycles_table[] = {
  0, 161, 80, 53, 40, 32, 26, 23,
  20, 17, 16, 14, 13, 12, 11, 10,
  10, 9, 8, 8, 8, 7, 7, 7,
  6, 6, 6, 5, 5, 5, 5, 5,
  5, 4, 4, 4, 4, 4, 4, 4,
  4, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 2, 2,
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
  1, 1
};
const uint16_t SIZE_OF_LOCK_CYCLE_TABLE = sizeof(binMaskCycles_table) / sizeof(binMaskCycles_table[0]);

// **************************************************
// **************************************************
// **************************************************
void updateStatistics()
{
  static uint8_t binMaskCycles[NR_OF_BINS];
  uint16_t mag;
  uint16_t magMax;
  uint8_t lowerBinBoundary;
  uint8_t upperBinBoundary;
  

  // run through all bin-groups
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++)
  {
    magMax = 0;
    
    if (binGroupNr == 0)
      lowerBinBoundary = 0;
    else
      lowerBinBoundary = binGroupBoundary[binGroupNr - 1] + 1;

    upperBinBoundary = binGroupBoundary[binGroupNr];

    // scan all FFT-bins within the group
    for (uint16_t binNr = lowerBinBoundary; binNr <= upperBinBoundary; binNr++)
    {
      mag = sqrt(pow(re[binNr], 2) + pow(im[binNr], 2));

      if (mag > magMax)
        magMax = mag;

      if (mag > bin[binNr].peakMag)
        bin[binNr].peakMag = mag;

       binGroup[binGroupNr].magsAcc += mag;        // summ up the magnitude of all FFT-bins inside a group

      //binMaskCycles[binNr] = 0;                 // disable masking for testing

      // speed-dependent masking
      if (binMaskCycles[binNr] > 0)
      {
        binMaskCycles[binNr]--;
      } else {
        if (mag > DETECTION_THRESHOLD)
        {
          // ignore DC-bin
          if (binNr > 0)
          {
            bin[binNr].detectionCtr++;
            binGroup[binGroupNr].binDetectionCtr++;
            totalDetectionsCtr++;
          }

          // mask out bin in next cycle
          binMaskCycles[binNr] = 1;

          // avoid multiple detections/counts of same "signal-event" spread across multiple FFT windows due to low-speed particles
          if (binNr < SIZE_OF_LOCK_CYCLE_TABLE)
            binMaskCycles[binNr] += binMaskCycles_table[binNr];
        }
      }
    }

    if (magMax > binGroup[binGroupNr].peakMag)
      binGroup[binGroupNr].peakMag= magMax;
  }
}

// **************************************************
// **************************************************
// **************************************************
void finalizeStatistics()
{
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++)
  {
    if (binGroupNr == 0)
      binGroup[binGroupNr].magAVG = binGroup[binGroupNr].magsAcc / snapshotCtr / (binGroupBoundary[0] + 1);
    else
      binGroup[binGroupNr].magAVG = binGroup[binGroupNr].magsAcc / snapshotCtr / (binGroupBoundary[binGroupNr] - binGroupBoundary[binGroupNr - 1]);
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
    bin[binNr].detectionCtr = 0;
    bin[binNr].peakMag = 0;
  }

  // group-level statistics
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++)
  {
    binGroup[binGroupNr].peakMag = 0;
    binGroup[binGroupNr].binDetectionCtr = 0;
    binGroup[binGroupNr].magsAcc = 0;
    binGroup[binGroupNr].magAVG = 0;
  }

  ADCpeakSample = 0;
  clippingCtr = 0;
  totalDetectionsCtr = 0;
}

