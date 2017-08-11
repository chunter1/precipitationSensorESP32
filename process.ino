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

void processSamples(uint16_t firstSampleIdx)
{
  int16_t sample;
  uint16_t sampleABS;
  uint16_t lastSampleIdx;
  uint16_t RBidx;
  int32_t sampleSUM;

  // Do not modify the sampleRB content!
  
  lastSampleIdx = (firstSampleIdx + NR_OF_FFT_SAMPLES) & (RINGBUFFER_SIZE - 1);

  // calculate ADC offset
  sampleSUM = 0;
  for (uint16_t i = 0; i < NR_OF_FFT_SAMPLES; i++)
  {
    RBidx = (firstSampleIdx + i) & (RINGBUFFER_SIZE - 1);
    sampleSUM += sampleRB[RBidx];
  }
  sensorData.ADCoffset = sampleSUM >> NR_OF_FFT_SAMPLES_bit;


  // process each sample
  for (uint16_t i = 0; i < NR_OF_FFT_SAMPLES; i++)
  {
    RBidx = (firstSampleIdx + i) & (RINGBUFFER_SIZE - 1);
    sample = sampleRB[RBidx];

    if ((sample <= -2048) || (sample >= 2047))
      sensorData.clippingCtr++;

    sample -= sensorData.ADCoffset;

    sampleABS = abs(sample);
    if (sampleABS > sensorData.ADCpeakSample)
      sensorData.ADCpeakSample = sampleABS;

    re[i] = sample << 4;
    im[i] = 0;
  }

  hannWindow(re, NR_OF_FFT_SAMPLES_bit);

  FFT(re, im, NR_OF_FFT_SAMPLES_bit);
}

