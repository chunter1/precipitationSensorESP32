#include "Publisher.h"

void Publisher::Begin(Settings *settings, DataPort *dataPort, BME280 *bme280) {
  m_settings = settings;
  m_dataPort = dataPort;
  m_bme280 = bme280;
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
  payload += "snapshots=" + String(m_sensorData->snapshotValidCtr) + ",";
  payload += "ADCclipping=" + String(m_sensorData->clippingCtr) + ",";
  payload += "ADCpeak=" + String((100 * (m_sensorData->ADCpeakSample > 2048 ? 2048 : m_sensorData->ADCpeakSample)) / 2048) + ",";
  payload += "ADCoffset=" + String(m_sensorData->ADCoffset) + ",";
  payload += "RBoverflows=" + String(m_sensorData->RbOvCtr) + ",";
  payload += "MagMax=" + String(m_sensorData->magMax) + ",";
  payload += "MagAVG=" + String(m_sensorData->magAVG, 8) + ",";
  payload += "MagAVGkorr=" + String(m_sensorData->magAVGkorr, 8) + ",";
  payload += "DomGroupMagAVGkorr=" + String(m_sensorData->DomGroupMagAVGkorr) + ",";
  payload += "DomGroupMagAboveThreshCnt=" + String(m_sensorData->DomGroupMagAboveThreshCnt) + ",";
  payload += "PreciAmount=" + String(m_sensorData->preciAmount, 8) + ",";
  payload += "PreciAmountAcc=" + String(m_sensorData->preciAmountAcc, 8) + ",";

  String groupMagMax;
  String groupMagAVG;
  String groupMagAVGkorr;
  String groupMagAVGkorrGated;
  String groupMagAVGkorrDom;
  String groupMagAVGkorrDom2;
  String groupMagThresh;
  String groupMagAboveThreshCnt;
  String groupMagAboveThreshCntDom;
  String Debug0;
  
  for (byte i = 0; i < m_settings->BaseData.NrOfBinGroups; i++) {
    groupMagMax += String(m_sensorData->binGroup[i].magMax) + " ";
    groupMagAVG += String(m_sensorData->binGroup[i].magAVG, 4) + " ";
    groupMagAVGkorr += String(m_sensorData->binGroup[i].magAVGkorr
    , 4) + " ";
    groupMagAVGkorrGated += String(m_sensorData->binGroup[i].magAVGkorrGated, 4) + " ";
    groupMagAVGkorrDom += String(m_sensorData->binGroup[i].magAVGkorrDom, 4) + " ";
    groupMagAVGkorrDom2 += String(m_sensorData->binGroup[i].magAVGkorrDom2, 4) + " ";
    groupMagThresh += String(m_sensorData->binGroup[i].magThresh) + " ";
    groupMagAboveThreshCnt += String(m_sensorData->binGroup[i].magAboveThreshCnt, 4) + " ";
    groupMagAboveThreshCntDom += String(m_sensorData->binGroup[i].magAboveThreshCntDom, 4) + " ";
  }

  // output first 32 bins for 50Hz noise analyzation
  for (byte i = 0; i < 32; i++) {
    Debug0 += String(m_sensorData->bin[i].magMax) + " ";
  }

  payload += "GroupMagMax=" + groupMagMax + ",";
  payload += "GroupMagAVG=" + groupMagAVG + ",";
  payload += "GroupMagAVGkorr=" + groupMagAVGkorr + ",";
  payload += "GroupMagAVGkorrGated=" + groupMagAVGkorrGated + ",";
  payload += "GroupMagAVGkorrDom=" + groupMagAVGkorrDom + ",";
  payload += "GroupMagAVGkorrDom2=" + groupMagAVGkorrDom2 + ",";
  payload += "GroupMagThresh=" + groupMagThresh + ",";
  payload += "GroupMagAboveThreshCnt=" + groupMagAboveThreshCnt + ",";
  payload += "GroupMagAboveThreshCntDom=" + groupMagAboveThreshCntDom + ",";
  payload += "Debug0=" + Debug0 + ",";
  
  if (m_bme280->IsPresent()) {
    m_bme280->Measure();
    payload += "Temperature=" + String(m_bme280->Values.Temperature, 1) + ",";
    payload += "Humidity=" + String(m_bme280->Values.Humidity) + ",";
    payload += "Pressure=" + String(m_bme280->Values.Pressure) + ",";
  }

  m_dataPort->AddPayload(payload);
}

void Publisher::Publish(SensorData *sensorData) {
  m_sensorData = sensorData;
  m_readings = "";
  m_nbrOfReadings = 0;

  if (m_dataPort->IsEnabled()) {
    SendToDataPort();
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
  if (m_settings->GetBool("PubGCAL", false)) {
    m_dummySuffix = "";
    AddGroupMagCalReading();
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
  AddReading("snapshots", m_sensorData->snapshotValidCtr);
  AddReading("ADCclipping", m_sensorData->clippingCtr);
  AddReading("ADCpeak", (100 * (m_sensorData->ADCpeakSample > 2048 ? 2048 : m_sensorData->ADCpeakSample)) / 2048);
  AddReading("ADCoffset", m_sensorData->ADCoffset);
  AddReading("RBoverflows", m_sensorData->RbOvCtr);
  AddReading("MagMax", m_sensorData->magMax);
  AddReading("MagAVG", m_sensorData->magAVG);
  AddReading("MagAVGkorr", m_sensorData->magAVGkorr);
}

void Publisher::AddBinsMagReadings() {
  const uint8_t NR_OF_TRANSFERS = 16;
  uint16_t magMax;
  char name[10];
  
  // find peak value for scaling
  magMax = 0;
  for (uint16_t binNr = 1; binNr < m_settings->BaseData.NrOfBins; binNr++) {
    if (m_sensorData->bin[binNr].magMax > magMax) {
      magMax = m_sensorData->bin[binNr].magMax;
    }
  }
  
  // transfer maximum value of each FFT-bin (except DC)
  for (uint16_t binNr = 1; binNr < m_settings->BaseData.NrOfBins; binNr++) {
    sprintf(name, "%03d", binNr);
  
    // The value of the reading
    String value;
    for (uint8_t x = 0; x < NR_OF_BARS; x++) {
      if (magMax > 0) {
        if (x < ((NR_OF_BARS * m_sensorData->bin[binNr].magMax) / magMax))
          value += "|";
        else
          value += ".";
      } else {
        value += "X";
      }
    }
  
    value += "%20" + String(m_sensorData->bin[binNr].magMax);
  
    AddReading(name, value);
  }
  
}

void Publisher::AddBinGroupsReadings() {
  uint16_t magMax;
  char name[12];
    
  // find peak value for scaling
  magMax = 0;
  for (byte binGroupNr = 0; binGroupNr < m_settings->BaseData.NrOfBinGroups; binGroupNr++) {
    if (m_sensorData->binGroup[binGroupNr].magMax > magMax) {
      magMax = m_sensorData->binGroup[binGroupNr].magMax;
    }
  }
  
  // transfer maximum value of each FFT-bin (except DC)
  for (byte binGroupNr = 0; binGroupNr <  m_settings->BaseData.NrOfBinGroups; binGroupNr++) {
    sprintf(name, "%03d", binGroupNr);
    
    String value;
    for (uint8_t x = 0; x < NR_OF_BARS; x++) {
      if ((x < ((NR_OF_BARS * m_sensorData->binGroup[binGroupNr].magMax) / magMax)) && (magMax > 0))
        value += "|";
      else
        value += ".";
    }
          
    value += "%20" + String(m_sensorData->binGroup[binGroupNr].magMax);
    
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

void Publisher::AddGroupMagCalReading() {
  String groupsMagThresh;
  for (byte binGroupNr = 0; binGroupNr <  m_settings->BaseData.NrOfBinGroups; binGroupNr++) {
    groupsMagThresh += String(m_sensorData->binGroup[binGroupNr].magThresh) + "%20";
  }
  AddReading("groupsMagThresh", groupsMagThresh);
}
