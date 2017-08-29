#include "Publisher.h"

void Publisher::Begin(Settings *settings, DataPort *dataPort) {
  m_settings = settings;
  m_dataPort = dataPort;
  m_dummyPrefix = m_settings->Get("DPR", "PRECIPITATION_SENSOR");
}

void Publisher::Transmit() {
  if (m_nbrOfReadings && WiFi.status() == WL_CONNECTED) {
    WiFiClient client;

    String ip = m_settings->Get("fhemIP", "192.168.1.100");
    uint port = m_settings->GetUInt("fhemPort", 8083);

    String payload = String("/fhem?XHR=1&cmd=") + m_readings;
    String post = String("POST ") + payload + F(" HTTP/1.1\r\nHost: ") + ip + F("\r\nConnection: close\r\n\r\n");

    if (client.connect(ip.c_str(), port)) {
      client.setNoDelay(true);
      client.print(post);
      client.stop();

      m_readings = "";
      m_nbrOfReadings = 0;
    }

  }
}

void Publisher::SendToDataPort() {
  String payload = "data=";
  payload += "snapshots=" + String(m_sensorData->snapshotCtr) + ",";
  payload += "ADCclipping=" + String(m_sensorData->clippingCtr) + ",";
  payload += "ADCpeak=" + String((100 * (m_sensorData->ADCpeakSample > 2048 ? 2048 : m_sensorData->ADCpeakSample)) / 2048) + ",";
  payload += "ADCoffset=" + String(m_sensorData->ADCoffset) + ",";
  payload += "RBoverflows=" + String(m_sensorData->RbOvCtr) + ",";
  payload += "MagPeak=" + String(m_sensorData->magPeak) + ",";
  payload += "MagAVG=" + String(m_sensorData->magAVG) + ",";
  payload += "MagAVGkorr=" + String(m_sensorData->magAVGkorr) + ",";
  payload += "MagAVGkorrThresh=" + String(m_sensorData->magAVGkorrThresh) + ",";

  String groupDetections;
  String groupMagPeak;
  String groupMagAVG;
  String groupMagAVGkorr;
  String groupMagAVGkorrThresh;

  for (byte i = 0; i < m_settings->BaseData.NrOfBinGroups; i++) {
    groupMagPeak += String(m_sensorData->binGroup[i].magPeak) + " ";
    groupMagAVG += String(m_sensorData->binGroup[i].magAVG, 4) + " ";
    groupMagAVGkorr += String(m_sensorData->binGroup[i].magAVGkorr, 4) + " ";
    groupMagAVGkorrThresh += String(m_sensorData->binGroup[i].magAVGkorrThresh, 8) + " ";
  }

  payload += "GroupDetections=" + groupDetections + ",";
  payload += "GroupMagPeak=" + groupMagPeak + ",";
  payload += "GroupMagAVG=" + groupMagAVG + ",";
  payload += "GroupMagAVGkorr=" + groupMagAVGkorr + ",";
  payload += "GroupMagAVGkorrThresh=" + groupMagAVGkorrThresh + ",";

  m_dataPort->AddPayload(payload);
}

void Publisher::Publish(SensorData *sensorData) {
  m_sensorData = sensorData;
  m_readings = "";
  m_nbrOfReadings = 0;

  if (m_dataPort->IsEnabled()) {
    SendToDataPort();
  }

  if (m_settings->GetBool("PubCompact", false)) {
    m_dummySuffix = "";
    AddCommonReadings();
    AddCompactReadings();    
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
  if (m_settings->GetBool("PubBMA", false)) {
    m_dummySuffix = "";
    AddBinMagAVGReading();
  }
  if (m_settings->GetBool("PubBMAK", false)) {
    m_dummySuffix = "";
    AddBinMagAVGkorrReading();
  }
  if (m_settings->GetBool("PubBMAKT", false)) {
    m_dummySuffix = "";
    AddBinMagAVGkorrThreshReading();
  }
  if (m_settings->GetBool("PubBTHRESHS", false)) {
    m_dummySuffix = "";
    AddBinMagAVGthresholdsReading();
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
  AddReading("ADCoffset", m_sensorData->ADCoffset);
  AddReading("RBoverflows", m_sensorData->RbOvCtr);
  AddReading("MagPeak", m_sensorData->magPeak);
  AddReading("MagAVG", m_sensorData->magAVG);
  AddReading("MagAVGkorr", m_sensorData->magAVGkorr);
  AddReading("MagAVGkorrThresh", m_sensorData->magAVGkorrThresh);
}

void Publisher::AddCompactReadings() {
  String groupDetections;
  String groupMagPeak;
  String groupMagAVG;
  String groupMagAVGkorr;
  String groupMagAVGkorrThresh;
  for (byte i = 0; i < m_settings->BaseData.NrOfBinGroups; i++) {
    groupMagPeak += String(m_sensorData->binGroup[i].magPeak) + "%20";
    groupMagAVG += String(m_sensorData->binGroup[i].magAVG, 4) + "%20";
    groupMagAVGkorr += String(m_sensorData->binGroup[i].magAVGkorr, 4) + "%20";
    groupMagAVGkorrThresh += String(m_sensorData->binGroup[i].magAVGkorrThresh, 8) + "%20";
  }
  AddReading("GroupDetections", groupDetections);
  AddReading("GroupMagPeak", groupMagPeak);
  AddReading("GroupMagAVG", groupMagAVG);
  AddReading("GroupMagAVGkorr", groupMagAVGkorr);
  AddReading("GroupMagAVGkorrThresh", groupMagAVGkorrThresh);
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
  
  // transfer maximum value of each FFT-bin (except DC)
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
  
  // transfer maximum value of each FFT-bin (except DC)
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
    
    AddReading(name, value);
  }

}

void Publisher::AddBinMagAVGReading() {
  String binsMagAVG;
  for (uint16_t binNr = 0; binNr < m_settings->BaseData.NrOfBins; binNr++) {
    binsMagAVG += String(m_sensorData->bin[binNr].magAVG, 4) + "%20";
  }
  AddReading("BinMagAVG", binsMagAVG);
}

void Publisher::AddBinMagAVGkorrReading() {
  String binsMagAVGkorr;
  for (uint16_t binNr = 0; binNr < m_settings->BaseData.NrOfBins; binNr++) {
    binsMagAVGkorr += String(m_sensorData->bin[binNr].magAVGkorr, 4) + "%20";
  }
  AddReading("BinMagAVGkorr", binsMagAVGkorr);
}

void Publisher::AddBinMagAVGkorrThreshReading() {
  String binsMagAVGkorrThresh;
  for (uint16_t binNr = 0; binNr < m_settings->BaseData.NrOfBins; binNr++) {
    binsMagAVGkorrThresh += String(m_sensorData->bin[binNr].magAVGkorrThresh, 4) + "%20";
  }
  AddReading("BinMagAVGkorrThresh", binsMagAVGkorrThresh);
}

void Publisher::AddBinMagAVGthresholdsReading() {
  String binsThreshold;
  for (uint16_t binNr = 0; binNr < m_settings->BaseData.NrOfBins; binNr++) {
    binsThreshold += String(m_sensorData->bin[binNr].threshold, 4) + "%20";
  }
  AddReading("BinThreshold", binsThreshold);
}
