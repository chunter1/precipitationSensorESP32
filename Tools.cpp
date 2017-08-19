#include "Tools.h"

String Tools::GetChipId() {
  uint64_t chipmacid;
  esp_efuse_mac_get_default((uint8_t*)(&chipmacid));
  String r1((uint16_t)(chipmacid >> 32), HEX);
  String r2((uint32_t)(chipmacid), HEX);
  r1.toUpperCase();
  r2.toUpperCase();
  return r1 + r2;
}

String Tools::GetChipRevision() {
  String result;

  byte revision = (REG_READ(EFUSE_BLK0_RDATA3_REG) >> EFUSE_RD_CHIP_VER_RESERVE_S) && EFUSE_RD_CHIP_VER_RESERVE_V;
  switch (revision) {
  case 0:
    result = "0 (2016.09)";
    break;
  case 1:
    result = "1 (2017.02)";
    break;
  default:
    result = String(revision) + "(unknown)";
    break;
  }

  return result;
}

IPAddress Tools::IPAddressFromString(String ipString) {
  byte o1p = ipString.indexOf(".", 0);
  byte o2p = ipString.indexOf(".", o1p + 1);
  byte o3p = ipString.indexOf(".", o2p + 1);
  byte o4p = ipString.indexOf(".", o3p + 1);

  return IPAddress(strtol(ipString.substring(0, o1p).c_str(), NULL, 10),
                   strtol(ipString.substring(o1p + 1, o2p).c_str(), NULL, 10),
                   strtol(ipString.substring(o2p + 1, o3p).c_str(), NULL, 10),
                   strtol(ipString.substring(o3p + 1, o4p).c_str(), NULL, 10));
}

byte Tools::UTF8ToASCII(byte utf8Char) {
  byte result = 0;
  static byte lastChar;
  if (utf8Char < 128) {
    lastChar = 0;
    result = utf8Char;
  }
  else {
    switch (lastChar) {
    case 0xC2:
      result = utf8Char;
      break;
    case 0xC3:
      result = utf8Char | 0xC0;
      break;
    case 0x82:
      if (utf8Char == 0xAC) {
        result = 0x80;
      }
      else {
        result = 0;
      }
      break;
    }

    lastChar = utf8Char;
  }

  return result;
}
String Tools::UTF8ToASCII(String utf8) {
  String result = "";
  for (uint i = 0; i < utf8.length(); i++) {
    char c = UTF8ToASCII(utf8.charAt(i));
    if (c != 0) {
      result += c;
    }
  }
  return result;
}