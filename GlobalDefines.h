#ifndef __GLOBALDEFINES__h
#define __GLOBALDEFINES__h

#define PROGNAME                          "precipitationSensor"
#define PROGVERS                          "0.14.1"

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

// For development purpose only:
//#define DEBUG

#endif
