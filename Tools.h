#ifndef _TOOLS_h
#define _TOOLS_h

#include "Arduino.h"
#include "soc/efuse_reg.h"
#include "IPAddress.h"

class Tools {
public:
  static String GetChipId();
  static String GetChipRevision();
  static IPAddress IPAddressFromString(String ipString);
  static byte UTF8ToASCII(byte ascii);
  static String UTF8ToASCII(String utf8);

};


#endif

