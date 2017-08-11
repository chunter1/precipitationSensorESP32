#include "Publisher.h"

void Publisher::Begin(Settings *settings) {
  m_settings = settings;
  m_dummyPrefix = m_settings->Get("DPR", "PRECIPITATION_SENSOR");
}

void Publisher::Transmit() {
  if (m_nbrOfReadings) {
    unsigned long startTime = millis();
    WiFiClient client;

    String ip = m_settings->Get("fhemIP", "192.168.1.100");
    uint port = m_settings->GetUInt("fhemPort", 8083);

    String payload = String("/fhem?XHR=1&cmd=") + m_readings;

    if (client.connect(ip.c_str(), port)) {
      client.setNoDelay(true);

      String post = String("POST ") + payload + F(" HTTP/1.1\r\nHost: ") + ip + F("\r\nConnection: close\r\n\r\n");

      client.print(post);
      client.stop();
   
      ////Serial.print("Transmitted ");
      ////Serial.print(payload.length());
      ////Serial.print(" byte for ");
      ////Serial.print(m_nbrOfReadings);
      ////Serial.print(" readings in ");
      ////Serial.print(millis() - startTime);
      ////Serial.println(" ms");

      m_readings = "";
      m_nbrOfReadings = 0;
    }

  }
}

void Publisher::Publish(SensorData *sensorData) {
  m_sensorData = sensorData;
  m_readings = "";
  m_nbrOfReadings = 0;

  if (m_settings->GetBool("PubCompact", true)) {
    m_dummySuffix = "";
    AddCommonReadings();
    AddCompactReadings();
  }
  if (m_settings->GetBool("PubBC", false)) {
    m_dummySuffix = "_BINS_COUNT";
    AddCommonReadings();
    AddBinsCountReadings();
  }
  if (m_settings->GetBool("PubBM", false)) {
    m_dummySuffix = "_BINS_MAG";
    AddCommonReadings();
    AddBinsMagReadings();
  }
  if (m_settings->GetBool("PubBG", false)) {
    m_dummySuffix = "_BIN_GROUPS";
    AddCommonReadings();
    AddBinGroupsReadings();
  }

  Transmit();
}

void Publisher::AddReading(String name, String value) {
  m_readings += "setreading%20" + m_dummyPrefix + m_dummySuffix + "%20" + name + "%20" + value + "%3B";
  m_nbrOfReadings++;

  // If we reach the limit for one transmission
  if (m_nbrOfReadings >= 100 || m_readings.length() > 8192) {
    Transmit();
  }
}

void Publisher::AddReading(String name, uint32_t value) {
  AddReading(name, String(value));
}

void Publisher::AddReading(String name, int32_t value) {
  AddReading(name, String(value));
}

void Publisher::AddReading(String name, float value) {
  AddReading(name, String(value, 4));
}

void Publisher::AddCommonReadings() {
  AddReading("snapshots", m_sensorData->snapshotCtr);
  AddReading("ADCclipping", m_sensorData->clippingCtr);
  AddReading("ADCpeak", (100 * (m_sensorData->ADCpeakSample > 2048 ? 2048 : m_sensorData->ADCpeakSample)) / 2048);
  AddReading("detections", m_sensorData->totalDetectionsCtr);
  AddReading("ADCoffset", m_sensorData->ADCoffset);
  AddReading("RBoverflows", m_sensorData->RBoverflowCtr);
  AddReading("MagPeak", m_sensorData->magPeak);
  AddReading("MagAVG", m_sensorData->magAVG);
  AddReading("MagAVGkorr", m_sensorData->magAVGkorr);
  AddReading("state", m_sensorData->totalDetectionsCtr);
}

void Publisher::AddCompactReadings() {
  String groupDetections;
  String groupMagPeak;
  String groupMagAVG;
  String groupMagAVGkorr;
  for (byte i = 0; i < m_settings->BaseData.NrOfBinGroups; i++) {
    groupDetections += String(m_sensorData->binGroup[i].detections) + "%20";
    groupMagPeak += String(m_sensorData->binGroup[i].magPeak) + "%20";
    groupMagAVG += String(m_sensorData->binGroup[i].magAVG, 4) + "%20";
    groupMagAVGkorr += String(m_sensorData->binGroup[i].magAVGkorr, 4) + "%20";
  }
  AddReading("GroupDetections", groupDetections);
  AddReading("GroupMagPeak", groupMagPeak);
  AddReading("GroupMagAVG", groupMagAVG);
  AddReading("GroupMagAVGkorr", groupMagAVGkorr);

}

void Publisher::AddBinsCountReadings() {
  char name[10];
  
  // find peak value for scaling
  uint16_t countMax = 0;
  for (uint16_t binNr = 1; binNr < m_settings->BaseData.NrOfBins; binNr++) {
    if (m_sensorData->bin[binNr].detections > countMax) {
      countMax = m_sensorData->bin[binNr].detections;
    }
  }
  
  // transfer occurrence counter and maximum value of each FFT-bin (except DC)
  for (uint16_t binNr = 1; binNr < m_settings->BaseData.NrOfBins; binNr++) {
    sprintf(name, "%03d", binNr);
  
    // The value of the reading
    String value;
    for (uint8_t x = 0; x < NR_OF_BARS; x++) {
      if (countMax > 0) {
        if (x < ((m_settings->BaseData.NrOfBins * m_sensorData->bin[binNr].detections) / countMax))
          value += "|";
        else
          value += ".";
      } else {
        value += "X";
      }
    }
  
    value += "%20" + String(m_sensorData->bin[binNr].detections);
        
    AddReading(name, value);
  }
}

void Publisher::AddBinsMagReadings() {
  const uint8_t NR_OF_TRANSFERS = 16;
  uint16_t magPeak;
  char name[10];
  
  // find peak value for scaling
  magPeak = 0;
  for (uint16_t binNr = 1; binNr < m_settings->BaseData.NrOfBins; binNr++) {
    if (m_sensorData->bin[binNr].magPeak > magPeak) {
      magPeak = m_sensorData->bin[binNr].magPeak;
    }
  }
  
  // transfer occurrence counter and maximum value of each FFT-bin (except DC)
  for (uint16_t binNr = 1; binNr < m_settings->BaseData.NrOfBins; binNr++) {
    sprintf(name, "%03d", binNr);
  
    // The value of the reading
    String value;
    for (uint8_t x = 0; x < NR_OF_BARS; x++) {
      if (magPeak > 0) {
        if (x < ((NR_OF_BARS * m_sensorData->bin[binNr].magPeak) / magPeak))
          value += "|";
        else
          value += ".";
      } else {
        value += "X";
      }
    }
  
    value += "%20" + String(m_sensorData->bin[binNr].magPeak);
    value += "%20" + String(m_sensorData->bin[binNr].detections);
  
    AddReading(name, value);
  }
  
}

void Publisher::AddBinGroupsReadings() {
  uint16_t magPeak;
  char name[12];
    
  // find peak value for scaling
  magPeak = 0;
  for (byte binGroupNr = 0; binGroupNr < m_settings->BaseData.NrOfBinGroups; binGroupNr++) {
    if (m_sensorData->binGroup[binGroupNr].magPeak > magPeak) {
      magPeak = m_sensorData->binGroup[binGroupNr].magPeak;
    }
  }
  
  // transfer occurrence counter and maximum value of each FFT-bin (except DC)
  for (byte binGroupNr = 1; binGroupNr <  m_settings->BaseData.NrOfBinGroups; binGroupNr++) {
    sprintf(name, "%03d", binGroupNr);
    
    String value;
    for (uint8_t x = 0; x < NR_OF_BARS; x++) {
      if ((x < ((NR_OF_BARS * m_sensorData->binGroup[binGroupNr].magPeak) / magPeak)) && (magPeak > 0))
        value += "|";
      else
        value += ".";
    }
          
    value += "%20" + String(m_sensorData->binGroup[binGroupNr].magPeak);
    value += "%20" + String(m_sensorData->binGroup[binGroupNr].detections);
    
    AddReading(name, value);
  }
     
}


