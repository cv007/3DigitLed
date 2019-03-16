#ifndef __ADC_H__
#define __ADC_H__

#include "pins.h"
#include <stdint.h>

// channels other than pins
//=============================================================================
typedef enum {
    adc_AVSS = 0x3B,
    adc_TEMP,
    adc_DAC1,
    adc_FVR1,
    adc_FVR2
} adc_ch_t;

// simple adc read
//=============================================================================
uint16_t adc_read       (adc_ch_t); //read channel (that is not a pin)
uint16_t adc_read_pin   (pin_t*);   //read pin (coverts pin to channel)

#endif //__ADC_H__