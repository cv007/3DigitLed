#ifndef __FVR_H__
#define __FVR_H__

#include <stdbool.h>
#include <stdint.h>

// fvr selection - adc or compdac, off 1x 2x 4x
//=============================================================================
typedef enum {
    fvr_ADCOFF, fvr_ADC1X, fvr_ADC2X, fvr_ADC4x
} fvr_adc_t;

typedef enum {
    fvr_COMPOFF, fvr_COMP1X, fvr_COMP2X, fvr_COMP4x,
    fvr_DACOFF = 0, fvr_DAC1X, fvr_DAC2X, fvr_DAC4X
} fvr_compdac_t;

// use the fvr_...OFF enums to disable adc or compdac fvr
//=============================================================================
void fvr_adc    (fvr_adc_t);
void fvr_dac    (fvr_compdac_t);




#endif //__FVR_H__