#ifndef _SETTINGS_h
#define _SETTINGS_h

#include "Arduino.h"
#include "nvs_flash.h"
#include "nvs.h"

#define BUFFER_SIZE 1024

typedef void CriticalActionCallbackType(bool);

class Settings {
public:
  struct ItemPosition {
    int from;
    int to;
  };

  struct BaseDataStruct {
    uint NrOfBins;
    byte NrOfBinGroups;
  } BaseData;

  Settings();

  void Begin(CriticalActionCallbackType *criticalActionCallback);
 
  void Read();
  String Write();
  
  void Add(String key, String value);
  void Add(String key, int value);
  void Add(String key, float value);

  void Remove(String key);
  bool Contains(String key);
  
  String Get(String key, String defaultValue);
  int GetInt(String key, int defaultValue);
  uint GetUInt(String key, uint defaultValue);
  bool GetBool (String key, bool defaultValue);
  byte GetByte(String key, byte defaultValue);
  float GetFloat(String key, float defaultValue);
  
  String ToString();
  void FromString(String settings);
  void Clear();


private:
  String m_data;
  static bool m_debug;
  CriticalActionCallbackType *m_criticalActionCallback;
  
  ItemPosition GetItemPosition(String key);
  String GetRaw(String key);
};


#endif

