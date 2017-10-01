#ifndef __BME280_H__
#define __BME280_H__

#include "Arduino.h"
#include "Wire.h"

enum {
  BME280_REGISTER_T1              = 0x88,
  BME280_REGISTER_T2              = 0x8A,
  BME280_REGISTER_T3              = 0x8C,

  BME280_REGISTER_P1              = 0x8E,
  BME280_REGISTER_P2              = 0x90,
  BME280_REGISTER_P3              = 0x92,
  BME280_REGISTER_P4              = 0x94,
  BME280_REGISTER_P5              = 0x96,
  BME280_REGISTER_P6              = 0x98,
  BME280_REGISTER_P7              = 0x9A,
  BME280_REGISTER_P8              = 0x9C,
  BME280_REGISTER_P9              = 0x9E,

  BME280_REGISTER_H1              = 0xA1,
  BME280_REGISTER_H2              = 0xE1,
  BME280_REGISTER_H3              = 0xE3,
  BME280_REGISTER_H4              = 0xE4,
  BME280_REGISTER_H5              = 0xE5,
  BME280_REGISTER_H6              = 0xE7,

  BME280_REGISTER_CHIPID          = 0xD0,
  BME280_REGISTER_VERSION         = 0xD1,
  BME280_REGISTER_SOFTRESET       = 0xE0,

  BME280_REGISTER_CAL26           = 0xE1,

  BME280_REGISTER_CONTROLHUMID    = 0xF2,
  BME280_REGISTER_STATUS          = 0xF3,
  BME280_REGISTER_CONTROL         = 0xF4,
  BME280_REGISTER_CONFIG          = 0xF5,
  BME280_REGISTER_PRESSUREDATA    = 0xF7,
  BME280_REGISTER_TEMPDATA        = 0xFA,
  BME280_REGISTER_HUMIDDATA       = 0xFD,
};

// Oversampling Humidity - written to ctrl_hum
enum {
  BME280_OSRS_Hx00 = 0,
  BME280_OSRS_Hx01 = 1,
  BME280_OSRS_Hx02 = 2,
  BME280_OSRS_Hx04 = 3,
  BME280_OSRS_Hx08 = 4,
  BME280_OSRS_Hx16 = 5,
};

// Oversampling Pressure - written to ctrl_meas (<< 2)
enum {
  BME280_OSRS_Px00  = 0,
  BME280_OSRS_Px01  = 1,
  BME280_OSRS_Px02  = 2,
  BME280_OSRS_Px04  = 3,
  BME280_OSRS_Px08  = 4,
  BME280_OSRS_Px16  = 5,
};

// Oversampling Temperature - written to ctrl_meas (<< 5)
enum {
  BME280_OSRS_Tx00  = 0,
  BME280_OSRS_Tx01  = 1,
  BME280_OSRS_Tx02  = 2,
  BME280_OSRS_Tx04  = 3,
  BME280_OSRS_Tx08  = 4,
  BME280_OSRS_Tx16  = 5,
};

// Operation mode - written to ctrl_meas
enum {
  BME280_MODE_SLEEP    = 0,
  BME280_MODE_FORCED   = 1,   // Forced mode
  BME280_MODE_NORMAL   = 3,
};

// Standby setting - written to config (<< 5)
enum {
  BME280_T_SB_00_5   = 0,
  BME280_T_SB_62_5   = 1,
  BME280_T_SB_125    = 2,
  BME280_T_SB_250    = 3,
  BME280_T_SB_500    = 4,
  BME280_T_SB_1000   = 5,
  BME280_T_SB_10     = 6,
  BME280_T_SB_20     = 7,
};

// IIR Filter settings - written to config (<< 2)
enum {
  BME280_FILTER_OFF        = 0,
  BME280_FILTER_COEF_2     = 1,
  BME280_FILTER_COEF_4     = 2,
  BME280_FILTER_COEF_8     = 3,
  BME280_FILTER_COEF_16    = 4,
};

typedef struct {
  uint16_t T1;
  int16_t  T2;
  int16_t  T3;

  uint16_t P1;
  int16_t  P2;
  int16_t  P3;
  int16_t  P4;
  int16_t  P5;
  int16_t  P6;
  int16_t  P7;
  int16_t  P8;
  int16_t  P9;

  uint8_t  H1;
  int16_t  H2;
  uint8_t  H3;
  int16_t  H4;
  int16_t  H5;
  int8_t   H6;
} bme280_compensation;

typedef struct {
  float Temperature;
  int Humidity;
  int Pressure;
} BME280Value;

class BME280 {
public:
  BME280();
  BME280Value Values;
  void SetAltitudeAboveSeaLevel(int altitude);
  bool  TryInitialize(byte address);
  bme280_compensation GetCompensationValues();
  bool IsPresent();
  void Measure();

protected:
  byte m_address;
  bool m_isPresent;

  void Write8(byte reg, byte value);
  byte Read8(byte reg);
  uint16_t Read16(byte reg);
  uint16_t Read16_LE(byte reg);
  int16_t ReadS16_LE(byte reg);

  void ReadCompensation();
  void GetTemperature();
  void GetPressure();
  void GetHumidity();

  int m_altitudeAboveSeaLevel;
  int32_t m_compensatedTemperature;
  bme280_compensation m_compensation;
  

};

#endif
