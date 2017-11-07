#ifndef __GLOBALDEFINES__h
#define __GLOBALDEFINES__h

#define PROGNAME                          "precipitationSensor"
#define PROGVERS                          "0.16.0"

#define CPU_CLOCK                         80000000

#define DEFAULT_SSID                      "SSID"
#define DEFAULT_PASSWORD                  "PASSWORD"
#define DEFAULT_PUBLISH_INTERVAL          60
#define DEFAULT_THRESHOLD_FACTOR          2.0
                         
#define DEBUG_GPIO_ISR                    5
#define DEBUG_GPIO_MAIN                   23

#define SAMPLE_RATE                       40960                              // DO NOT MODIFY
#define NR_OF_FFT_SAMPLES_bit             10                                 // DO NOT MODIFY
#define NR_OF_FFT_SAMPLES                 (1 << NR_OF_FFT_SAMPLES_bit)
#define NR_OF_BINS                        (NR_OF_FFT_SAMPLES >> 1)
#define NR_OF_BIN_GROUPS                  32

#define RINGBUFFER_SIZE                   (NR_OF_FFT_SAMPLES << 2)

// Hydrometeor classification (EXPERIMENTAL!)
#define DOM_GROUP_RAIN_FIRST              7                                  // First binGroup number classified as rain (everything below as snow)
#define DOM_GROUP_RAIN_LAST               23                                 // Last binGroup number classified as rain (everything above as hail)

const uint16_t defaultBinGroupBoundary[NR_OF_BIN_GROUPS] = {
  6, 9, 12, 15, 18, 21, 24, 27,
  30, 33, 36, 39, 42, 45, 48, 51,
  54, 57, 60, 63, 66, 69, 72, 75,
  78, 103, 128, 153, 178, 203, 228, 255
};

// For development purpose only:
//#define DEBUG

#endif
