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

#define   NR_OF_BARS    32

WiFiClient client;
String url;

// **************************************************
// **************************************************
// **************************************************
void header_short(char *dummyName)
{
  if (!client.connect(serverIP, serverPORT)) 
    return;
  
  url = "/fhem?cmd=";
}

// **************************************************
// **************************************************
// **************************************************
void header(char *dummyName)
{
  char str[20];
  
  header_short(dummyName);

  url += "setreading%20";
  url += String(dummyName);
  url += "%20snapshots%20";
  url += String(snapshotCtr);
  url += "%3B";

  url += "setreading%20";
  url += String(dummyName);
  url += "%20ADCclipping%20";
  url += String(clippingCtr);
  url += "%3B";

  url += "setreading%20";
  url += String(dummyName);
  url += "%20ADCpeak%20";
  //url += String((100 * ADCpeakSample) / 2048);
  if (ADCpeakSample > 2048)
    ADCpeakSample = 2048;
  url += String((100 * ADCpeakSample) / 2048);
  url += "%3B";

  url += "setreading%20";
  url += String(dummyName);
  url += "%20detections%20";
  url += String(totalDetectionsCtr);
  url += "%3B";

  url += "setreading%20";
  url += String(dummyName);
  url += "%20ADCoffset%20";
  url += String(ADCoffset);
  url += "%3B";

  url += "setreading%20";
  url += String(dummyName);
  url += "%20RBoverflows%20";
  url += String(RBoverflowCtr);
  url += "%3B";

  url += "setreading%20";
  url += String(dummyName);
  url += "%20MagPeak%20";
  url += String(magPeak);
  url += "%3B";

  url += "setreading%20";
  url += String(dummyName);
  url += "%20MagAVG%20";  
  sprintf(str, "%.4f", magAVG);
  url += str;
  url += "%3B";

  url += "setreading%20";
  url += String(dummyName);
  url += "%20MagAVGkorr%20";  
  sprintf(str, "%.4f", magAVGkorr);
  url += str;
  url += "%3B";

  url += "setreading%20";
  url += String(dummyName);
  url += "%20state%20";
  url += String(totalDetectionsCtr);
  url += "%3B";
}

// **************************************************
// **************************************************
// **************************************************
void footer()
{
  client.print(String("POST ") + url + F("&XHR=1 HTTP/1.1\r\n") + F("Host: ") + serverIP + F("\r\n") + F("Connection: close\r\n\r\n"));
  client.stop();
  client.flush();   // needed here?
}

// **************************************************
// **************************************************
// **************************************************
void publish_compact(char* dummyName)
{
  char str[20];
  
  header(dummyName);

  url += "setreading%20";
  url += String(dummyName);
  url += "%20";

  url += "GroupDetections";
    
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++)
  {
    url += "%20";  
    url += String(binGroup[binGroupNr].detections);
  }
  url += "%3B";


  url += "setreading%20";
  url += String(dummyName);
  url += "%20";

  url += "GroupMagPeak%20";
    
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++)
  {
    url += "%20";
    url += String(binGroup[binGroupNr].magPeak);
  }
  url += "%3B";


  url += "setreading%20";
  url += String(dummyName);
  url += "%20";

  url += "GroupMagAVG%20";
    
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++)
  {
    url += "%20";
    char str[20];
    sprintf(str, "%.4f", binGroup[binGroupNr].magAVG);
    url += str;
  }
  url += "%3B";


  url += "setreading%20";
  url += String(dummyName);
  url += "%20";
  
  url += "GroupMagAVGkorr";
      
  for (uint8_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++)
  {
    url += "%20";
    sprintf(str, "%.4f", binGroup[binGroupNr].magAVGkorr);
    url += str;
  }
  url += "%3B";
      
  footer();
}

// **************************************************
// **************************************************
// **************************************************
void publish_binGroups(char* dummyName)
{
  const uint8_t NR_OF_TRANSFERS = 2;
  uint16_t magPeak;
  char str[12];
  
  // find peak value for scaling
  magPeak = 0;
  for (uint16_t binGroupNr = 0; binGroupNr < NR_OF_BIN_GROUPS; binGroupNr++)
  {
    if (binGroup[binGroupNr].magPeak > magPeak)
      magPeak = binGroup[binGroupNr].magPeak;
  }

  // transfer occurrence counter and maximum value of each FFT-bin (except DC)
  for (uint8_t transferNr = 0; transferNr < NR_OF_TRANSFERS; transferNr++)
  {
    if (transferNr ==0)
      header(dummyName);
    else
      header_short(dummyName);
    
    for (uint8_t binGroupNr = (NR_OF_BIN_GROUPS >> 1) * transferNr; binGroupNr < (NR_OF_BIN_GROUPS >> 1) * (transferNr + 1); binGroupNr++)
    {
      url += "setreading%20";
      url += String(dummyName);
      url += "%20";
  
      sprintf(str,"%02d%%20", binGroupNr);
      url += str;
  
      for (uint8_t x = 0; x < NR_OF_BARS; x++)
      {
        if ((x < ((NR_OF_BARS * binGroup[binGroupNr].magPeak) / magPeak)) && (magPeak > 0))
          url += "|";
        else
          url += ".";
      }
        
      url += "%20";
      url += String(binGroup[binGroupNr].magPeak);
  
      url += "%20";
      url += String(binGroup[binGroupNr].detections);
      
      url += "%3B";
    }
   
    footer();
  }
}

// **************************************************
// **************************************************
// **************************************************
void publish_bins_count(char *dummyName)
{
  const uint8_t NR_OF_TRANSFERS = 16;
  uint16_t countMax;
  char str[10];

  // find peak value for scaling
  countMax = 0;
  for (uint16_t binNr = 1; binNr < NR_OF_BINS; binNr++)
  {
    if (bin[binNr].detections > countMax)
      countMax = bin[binNr].detections;
  }

  // transfer occurrence counter and maximum value of each FFT-bin (except DC)
  for (uint8_t transferNr = 0; transferNr < NR_OF_TRANSFERS; transferNr++)
  {
    if (transferNr ==0)
      header(dummyName);
    else
      header_short(dummyName);
      
    for (uint16_t binNr = (NR_OF_BINS / NR_OF_TRANSFERS) * transferNr; binNr < ((NR_OF_BINS / NR_OF_TRANSFERS) * (transferNr + 1)); binNr++)
    {
      if (binNr == 0)
        continue;

      url += "setreading%20";
      url += String(dummyName);

      sprintf(str, "%%20%03d%%20", binNr);
      url += str;

      for (uint8_t x = 0; x < NR_OF_BARS; x++)
      {
        if (countMax > 0)
        {
          if (x < ((NR_OF_BARS * bin[binNr].detections) / countMax))
            url += "|";
          else
            url += ".";
        } else {
          url += "X";
        }
      }

      url += "%20";
      url += String(bin[binNr].detections);

      url += "%3B";      
    }

    footer();
  }
}

// **************************************************
// **************************************************
// **************************************************
void publish_bins_mag(char *dummyName)
{
  const uint8_t NR_OF_TRANSFERS = 16;
  uint16_t magPeak;
  char str[10];

  // find peak value for scaling
  magPeak = 0;
  for (uint16_t binNr = 1; binNr < NR_OF_BINS; binNr++)
  {
    if (bin[binNr].magPeak > magPeak)
      magPeak = bin[binNr].magPeak;
  }

  // transfer occurrence counter and maximum value of each FFT-bin (except DC)
  for (uint8_t transferNr = 0; transferNr < NR_OF_TRANSFERS; transferNr++)
  {
    if (transferNr ==0)
      header(dummyName);
    else
      header_short(dummyName);
      
    for (uint16_t binNr = (NR_OF_BINS / NR_OF_TRANSFERS) * transferNr; binNr < ((NR_OF_BINS / NR_OF_TRANSFERS) * (transferNr + 1)); binNr++)
    {
      if (binNr == 0)
        continue;

      url += "setreading%20";
      url += String(dummyName);

      sprintf(str, "%%20%03d%%20", binNr);
      url += str;

      for (uint8_t x = 0; x < NR_OF_BARS; x++)
      {
        if (magPeak > 0)
        {
          if (x < ((NR_OF_BARS * bin[binNr].magPeak) / magPeak))
            url += "|";
          else
            url += ".";
        } else {
          url += "X";
        }
      }

      url += "%20";
      url += String(bin[binNr].magPeak);
      
      url += "%20";
      url += String(bin[binNr].detections);

      url += "%3B";      
    }

    footer();
  }
}
