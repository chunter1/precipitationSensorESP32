#ifndef _SETTINGS_h
#define _SETTINGS_h

#include "Arduino.h"
#include "nvs_flash.h"
#include "nvs.h"

#define BUFFER_SIZE 512

class Settings {
public:
  struct ItemPosition {
    int from;
    int to;
  };

  Settings();
 
  void Read();
  String Write();
  
  void Add(String key, String value);
  void Add(String key, int value);
  void Remove(String key);
  bool Contains(String key);
  
  String Get(String key, String defaultValue);
  int GetInt(String key, int defaultValue);
  uint GetUInt(String key, uint defaultValue);
  bool GetBool (String key, bool defaultValue);
  byte GetByte(String key, byte defaultValue);
  
  String ToString();
  void FromString(String settings);
  void Clear();

  void SetTimer(hw_timer_t *timer);

private:
  String m_data;
  static bool m_debug;
  hw_timer_t *m_timer;
  
  ItemPosition GetItemPosition(String key);
  String GetRaw(String key);
};


#endif

