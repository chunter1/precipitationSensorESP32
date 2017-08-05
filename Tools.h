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

};


#endif

