#ifndef __SIGPROC__h
#define __SIGPROC__h

#include "Arduino.h"
#include "GlobalDefines.h"
#include "Settings.h"
#include "SensorData.h"
#include "Statistics.h"
#include "Publisher.h"

class SigProc {
public:
  void Begin(Settings *settings, SensorData *sensorData, Statistics *statistics, Publisher *publisher);
  void Handle();
  void StartCapture();
  void StopCapture();
  void Calc();
  bool IsCapturing();

private:
  Settings *m_settings;
  SensorData *m_sensorData;
  Statistics *m_statistics;  
  Publisher *m_publisher;
  hw_timer_t *timer = NULL;
  static volatile byte m_adcPin;
  static volatile int16_t sampleRb[RINGBUFFER_SIZE];
  static volatile uint32_t samplePtrIn;
  static volatile uint32_t samplePtrOut;
  static volatile uint32_t RbOvFlag;
  uint publishInterval;
  bool m_isCapturing;

  static void IRAM_ATTR onTimer();
  uint8_t SnapPending();
  inline int16_t MAS(int16_t a, int16_t b);
  void HannWindow(int16_t *re, uint8_t m);
  void FFT(int16_t *fr, int16_t *fi, uint16_t m);

  void DebugConsoleOutBins(uint16_t startIdx, uint16_t stopIdx);
  void DebugConsoleOutSamples(uint16_t startIdx, uint16_t stopIdx);
};


#endif
