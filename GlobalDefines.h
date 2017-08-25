#ifndef __GLOBALDEFINES__h
#define __GLOBALDEFINES__h

#define PROGNAME                         "precipitationSensor"
#define PROGVERS                         "0.10.0"

#define DEFAULT_SSID                      "SSID"
#define DEFAULT_PASSWORD                  "PASSWORD"
#define DEFAULT_PUBLISH_INTERVAL          60
#define DEFAULT_DETECTION_TRESHOLD        2.0
                         
#define ADC_PIN_NR                        34
#define DEBUG_GPIO_ISR                    5
#define DEBUG_GPIO_MAIN                   23

#define NR_OF_FFT_SAMPLES_bit             9                                 // DO NOT MODIFY - Number of samples in [bits] e.g. 9 bits = 512 samples
#define NR_OF_FFT_SAMPLES                 (1 << NR_OF_FFT_SAMPLES_bit)
#define NR_OF_BINS                        (NR_OF_FFT_SAMPLES >> 1)
#define NR_OF_BIN_GROUPS                  32

#define RINGBUFFER_SIZE                   (NR_OF_FFT_SAMPLES << 2)

#endif
