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

#define BARS    64

void consoleOut_samples(uint16_t startIdx, uint16_t stopIdx)
{
  Serial.printf("\nSnapshots: %d   ADC-offset: %d   Clipping: %d   ADCpeak: %d\n", sensorData.snapshotCtr, sensorData.ADCoffset, sensorData.clippingCtr, sensorData.ADCpeakSample);

  if ((startIdx >= RINGBUFFER_SIZE) || (stopIdx >= RINGBUFFER_SIZE))
  {
    Serial.println("Invalid sample index!");
    return;
  }
  
  for (uint16_t sampleNr = startIdx; sampleNr <= stopIdx; sampleNr++)
  {
    Serial.printf("SAMPLE %03d ", sampleNr);
    
    for (uint8_t x = 0; x < BARS; x++)
    {
      if (x < (((sampleRB[sampleNr] + 2048) * BARS) / 4096))
        Serial.printf("#");
      else
        Serial.printf("-");
    }

    Serial.printf(" %d\n", sampleRB[sampleNr]);
  }
}

// *****************************************
// *****************************************
// *****************************************
void consoleOut_bins(uint16_t startIdx, uint16_t stopIdx)
{
  Serial.printf("\nSnapshots: %d\n", sensorData.snapshotCtr);

  if ((startIdx >= NR_OF_BINS) || (stopIdx >= NR_OF_BINS))
  {
    Serial.println("Invalid bin index!");
    return;
  }

  for (uint16_t binNr = startIdx; binNr <= stopIdx; binNr++)
  {
    Serial.printf("BIN %03d ", binNr);
    
    for (uint8_t x = 0; x < BARS; x++)
    {
      if (x < ((sensorData.bin[binNr].magPeak * BARS) / 16383))
        Serial.printf("#");
      else
        Serial.printf("-");
    }

    Serial.printf(" %d\n", sensorData.bin[binNr].magPeak);
  }
}
