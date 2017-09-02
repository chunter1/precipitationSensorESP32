#include "Settings.h"
// Format:
// [1]Key1[2]Value1[1]Key2[2]Value2[1]Key3[2]Value3[1]

Settings::Settings() {
  m_data.reserve(BUFFER_SIZE);
  Clear();
}

void Settings::Begin(CriticalActionCallbackType *criticalActionCallback) {
  m_criticalActionCallback = criticalActionCallback;
  nvs_flash_init();
}

void Settings::Clear() {
  m_data = String((char)1);
}

Settings::ItemPosition Settings::GetItemPosition(String key) {
  ItemPosition result;
  String searchText;
  
  searchText += (char)1;
  searchText += key;
  searchText += (char)2;

  result.from = m_data.indexOf(searchText);
  result.to = searchText.length() + result.from;

  return result;
}

String Settings::GetRaw(String key) {
  String result;
  
  ItemPosition position = GetItemPosition(key);
  result = m_data.substring(position.from, position.to);
 
  int i = position.to;
  while(m_data[i] != 1 && i < BUFFER_SIZE) {
    result += m_data[i];
    i++;
  }

  return result;
}

bool Settings::Contains(String key) {
  return GetItemPosition(key).from != -1;
}

void Settings::Remove(String key) {
  m_data.replace(GetRaw(key), "");
}

void Settings::Add(String key, String value) {
  ItemPosition position = GetItemPosition(key);
  if (position.from != -1) {
    Remove(key);
  }

  if (value.length() > 0) {
    m_data += key;
    m_data += (char)2;
    m_data += value;
    m_data += (char)1;
  }
}

void Settings::Add(String key, int value) {
  Add(key, String(value));
}

void Settings::Add(String key, float value) {
  Add(key, String(value));
}

String Settings::ToString() {
  String result;
  result += m_data;
  result.replace(String((char)1), "; ");
  result.replace(String((char)2), " ");
  result = result.substring(1);
  result.trim();
  result = "SETUP " + result;
  
  return result;
}

void Settings::FromString(String settings) {
  settings.trim();
  if (settings.startsWith("SETUP ")) {
    settings = settings.substring(6);
  }
  if (!settings.endsWith(";")) {
    settings += ";";
  }
  
  byte step = 0;
  String key = "";
  String value = "";
  for (uint i = 0; i < settings.length(); i++) {
    char cc = settings[i];
    if (step == 0) {
      if (cc == ' ' && key.length() > 0) {
        step = 1;
      }
      else {
        key += cc;
      }
    }
    else if (step == 1) {
      if (cc == ';') {
        step = 0;
        key.trim();
        value.trim();
        if (key.length() > 0 && value.length() > 0) {
          Add(key, value);
          key = "";
          value = "";
        }
      }
      else {
        value += cc;
      }
    }

  }
  
}

void Settings::Read() {
  if (m_criticalActionCallback) {
    m_criticalActionCallback(true);
  }
  nvs_handle handle;
  nvs_open("Settings", nvs_open_mode::NVS_READONLY, &handle);
  
  char *str = new char[BUFFER_SIZE];
  size_t strl = BUFFER_SIZE;
  esp_err_t err = nvs_get_str(handle, "settings", str, &strl);
  if (err == ESP_OK) {
    m_data = String(str);
  }
  else {
    m_data = String((char)1);
  }
  delete str;
  
  nvs_close(handle);

  if (m_criticalActionCallback) {
    m_criticalActionCallback(false);
  }
}

String Settings::Write() {
  String result;

  if (m_criticalActionCallback) {
    m_criticalActionCallback(true);
  }

  nvs_handle handle;
  nvs_open("Settings", nvs_open_mode::NVS_READWRITE, &handle);
  nvs_set_str(handle, "settings", (const char*)m_data.c_str());
  nvs_commit(handle);
  nvs_close(handle);

  result += m_data.length();
  result += " Byte (max. ";
  result += BUFFER_SIZE;
  result += ")";

  if (m_criticalActionCallback) {
    m_criticalActionCallback(false);
  }
  
  return result;
}

String Settings::Get(String key, String defaultValue) {
  String result = defaultValue;
  ItemPosition pos = GetItemPosition(key);

  if (pos.from != -1) {
    String raw = GetRaw(key);
    int i = raw.indexOf(2);
    if(i != -1) {
      result = raw.substring(i+1);
      result.replace(String((char)1), "");
    }
  }

  return result;
}

int Settings::GetInt(String key, int defaultValue) {
  return Get(key, String(defaultValue)).toInt();
}

uint Settings::GetUInt(String key, uint defaultValue) {
  return (uint)Get(key, String(defaultValue)).toInt();
}

bool Settings::GetBool(String key, bool defaultValue) {
  return Get(key, defaultValue ? "true" : "false").equals("true");
}

byte Settings::GetByte(String key, byte defaultValue) {
  return (byte)Get(key, (String)defaultValue).toInt();
}

float Settings::GetFloat(String key, float defaultValue) {
  return Get(key, String(defaultValue)).toFloat();
}

void Settings::SaveCalibration(SensorData *data) {
  if (m_criticalActionCallback) {
    m_criticalActionCallback(true);
  }

  nvs_handle handle;
  nvs_open("Cal", nvs_open_mode::NVS_READWRITE, &handle);
  
  for (byte binGroupNr = 0; binGroupNr < BaseData.NrOfBinGroups; binGroupNr++) {
    String key = "BGC" + String(binGroupNr);
    String value = String(data->binGroup[binGroupNr].threshold, 4);
    nvs_set_str(handle, key.c_str(), value.c_str());
    nvs_commit(handle);
  }
  
  nvs_close(handle);  

  if (m_criticalActionCallback) {
    m_criticalActionCallback(false);
  }
}

void Settings::LoadCalibration(SensorData *data) {
  if (m_criticalActionCallback) {
    m_criticalActionCallback(true);
  }
  
  nvs_handle handle;
  nvs_open("Cal", nvs_open_mode::NVS_READONLY, &handle);

  for (byte binGroupNr = 0; binGroupNr < BaseData.NrOfBinGroups; binGroupNr++) {
    char *str = new char[12];
    size_t strl = 12;
    String key = "BGC" + String(binGroupNr);
    esp_err_t error = nvs_get_str(handle, key.c_str(), str, &strl);
    float value = 1.0;
    if (error == ESP_OK) {
      value = String(str).toFloat();
    }
    delete str;
  }

  nvs_close(handle);

  if (m_criticalActionCallback) {
    m_criticalActionCallback(false);
  }
}


