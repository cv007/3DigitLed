#ifndef __ADC_H__
#define __ADC_H__

#include "pins.h"
#include <stdint.h>

            // channels other than pins
            typedef enum
            {
            adc_AVSS = 0x3B,
            adc_TEMP,
            adc_DAC1,
            adc_FVR1,
            adc_FVR2
            }
adc_ch_t;

            typedef enum
            {
            adc_PREF_VDD,
            adc_PREF_REFPIN = 2,
            adc_PREF_FVR,
            }
adc_pref_t;

            //set positive reference (if not using default of Vdd)
            void
adc_pref    (adc_pref_t);
            //read channel (that is not a pin)
            uint16_t
adc_read    (adc_ch_t);
            //read pin (coverts pin to channel)
            uint16_t
adc_read_pin(pin_t*);

#endif //__ADC_H__