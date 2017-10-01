#include "Arduino.h"
#include "Wire.h"
#include "BME280.h"

BME280::BME280() { 
  m_isPresent = false;
}

void BME280::SetAltitudeAboveSeaLevel(int altitude) {
  m_altitudeAboveSeaLevel = altitude;
}

bool BME280::TryInitialize(byte address) {
  m_isPresent = false;
  m_address = address;

  Values.Temperature = 1;
  Values.Humidity = 2;
  Values.Pressure = 3;

  #define OSRS_H BME280_OSRS_Hx01         
  #define OSRS_P BME280_OSRS_Px01         
  #define OSRS_T BME280_OSRS_Tx01         
  #define T_SB   BME280_T_SB_1000         
  #define FILTER_COEF BME280_FILTER_COEF_4

  if (Read8(BME280_REGISTER_CHIPID) == 0x60) {
    ReadCompensation();
    Write8(BME280_REGISTER_CONTROL, BME280_MODE_SLEEP);
    Write8(BME280_REGISTER_CONFIG, ( (T_SB << 5) | (FILTER_COEF << 2) ));
    Write8(BME280_REGISTER_CONTROLHUMID, OSRS_H);
    
    Measure();
    if (Values.Pressure > 0) {
      m_isPresent = true;
    }
  } 

  return m_isPresent;
}

bool BME280::IsPresent() {
  return m_isPresent;
}

void BME280::ReadCompensation() {
  m_compensation.T1 = Read16_LE(BME280_REGISTER_T1);
  m_compensation.T2 = ReadS16_LE(BME280_REGISTER_T2);
  m_compensation.T3 = ReadS16_LE(BME280_REGISTER_T3);

  m_compensation.P1 = Read16_LE(BME280_REGISTER_P1);
  m_compensation.P2 = ReadS16_LE(BME280_REGISTER_P2);
  m_compensation.P3 = ReadS16_LE(BME280_REGISTER_P3);
  m_compensation.P4 = ReadS16_LE(BME280_REGISTER_P4);
  m_compensation.P5 = ReadS16_LE(BME280_REGISTER_P5);
  m_compensation.P6 = ReadS16_LE(BME280_REGISTER_P6);
  m_compensation.P7 = ReadS16_LE(BME280_REGISTER_P7);
  m_compensation.P8 = ReadS16_LE(BME280_REGISTER_P8);
  m_compensation.P9 = ReadS16_LE(BME280_REGISTER_P9);

  m_compensation.H1 = Read8(BME280_REGISTER_H1);
  m_compensation.H2 = ReadS16_LE(BME280_REGISTER_H2);
  m_compensation.H3 = Read8(BME280_REGISTER_H3);
  m_compensation.H4 = (Read8(BME280_REGISTER_H4) << 4) | (Read8(BME280_REGISTER_H4 + 1) & 0xF);
  m_compensation.H5 = (Read8(BME280_REGISTER_H5 + 1) << 4) | (Read8(BME280_REGISTER_H5) >> 4);
  m_compensation.H6 = (int8_t)Read8(BME280_REGISTER_H6);
}

bme280_compensation BME280::GetCompensationValues() {
  return m_compensation;
}

void BME280::GetTemperature() {
  int32_t var1, var2;

// measurement time
#define T_MEASURE ( 1 + (2 * (OSRS_T)) + (2 * (OSRS_P)) + (2 * (OSRS_H)))

    Write8(BME280_REGISTER_CONTROL, ( (OSRS_T << 5) | (OSRS_P << 2) | BME280_MODE_FORCED ));
    do {
      delay(1);     // 1ms delay
    } while ((1 << 3) & Read8(BME280_REGISTER_STATUS));     // until measurement finished
    
  int32_t adc_T = Read16(BME280_REGISTER_TEMPDATA);
  adc_T <<= 8;
  adc_T |= Read8(BME280_REGISTER_TEMPDATA + 2);
  adc_T >>= 4;
 
  var1 = ((((adc_T >> 3) - ((int32_t)m_compensation.T1 << 1))) * ((int32_t)m_compensation.T2)) >> 11;
  var2 = (((((adc_T >> 4) - ((int32_t)m_compensation.T1)) * ((adc_T >> 4) - ((int32_t)m_compensation.T1))) >> 12) * ((int32_t)m_compensation.T3)) >> 14;

  m_compensatedTemperature = var1 + var2;

  float T = (m_compensatedTemperature * 5 + 128) >> 8;
  
  Values.Temperature = T / 100.0;    // 2 digits precision

  T = round(T / 10) / 10.0;
}

void BME280::GetPressure() {
  int64_t var1, var2, p;

  int32_t adc_P = Read16(BME280_REGISTER_PRESSUREDATA);
  adc_P <<= 8;
  adc_P |= Read8(BME280_REGISTER_PRESSUREDATA+2);
  adc_P >>= 4;
 
  var1 = ((int64_t)m_compensatedTemperature) - 128000;
  var2 = var1 * var1 * (int64_t)m_compensation.P6;
  var2 = var2 + ((var1*(int64_t)m_compensation.P5) << 17);
  var2 = var2 + (((int64_t)m_compensation.P4) << 35);
  var1 = ((var1 * var1 * (int64_t)m_compensation.P3) >> 8) + ((var1 * (int64_t)m_compensation.P2) << 12);
  var1 = (((((int64_t)1) << 47) + var1))*((int64_t)m_compensation.P1) >> 33;

  if (var1 == 0) {
    Values.Pressure = 0;
    return;  
  }
  p = 1048576 - adc_P;
  p = (((p<<31) - var2)*3125) / var1;
  var1 = (((int64_t)m_compensation.P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)m_compensation.P8) * p) >> 19;

  p = ((p + var1 + var2) >> 8) + (((int64_t)m_compensation.P7) << 4);
  p = (float)p / 256;
  p /= pow(((float) 1.0 - ((float)m_altitudeAboveSeaLevel / 44330.0)), (float) 5.255);
  p /= 100.0;
  Values.Pressure = (int)p;
}


void BME280::GetHumidity() {
  int32_t adc_H = Read16(BME280_REGISTER_HUMIDDATA);
  int32_t v_x1_u32r;

  v_x1_u32r = (m_compensatedTemperature - ((int32_t)76800));
  v_x1_u32r = (((((adc_H << 14) - (((int32_t)m_compensation.H4) << 20) - (((int32_t)m_compensation.H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)m_compensation.H6)) >> 10) * (((v_x1_u32r * ((int32_t)m_compensation.H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)m_compensation.H2) + 8192) >> 14));
  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)m_compensation.H1)) >> 4));
  v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
  v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
  float h = (v_x1_u32r>>12);

  Values.Humidity = h / 1024;
}

void BME280::Measure() {
  GetTemperature();
  GetHumidity();
  GetPressure();
}

void BME280::Write8(byte reg, byte value) {
  Wire.beginTransmission(m_address);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

byte BME280::Read8(byte reg) {
  uint8_t value;

  Wire.beginTransmission(m_address);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(m_address, (byte)1);
  value = Wire.read();
  Wire.endTransmission();

  return value;
}

uint16_t BME280::Read16(byte reg) {
  uint16_t value;

  Wire.beginTransmission(m_address);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(m_address, (byte)2);
  value = (Wire.read() << 8) | Wire.read();
  Wire.endTransmission();

  return value;
}

uint16_t BME280::Read16_LE(byte reg) {
  uint16_t temp = Read16(reg);
  return (temp >> 8) | (temp << 8);
}

int16_t BME280::ReadS16_LE(byte reg){
  return (int16_t)Read16_LE(reg);
}





