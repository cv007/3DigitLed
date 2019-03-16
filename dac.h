#ifndef __DAC_H__
#define __DAC_H__

#include <stdbool.h>
#include <stdint.h>

// positive source select
//=============================================================================
typedef enum {
    dac_VDD, dac_VREF = 4, 
    dac_FVR1V = 8|1, dac_FVR2V, dac_FVR4V
} dac_pss_t;

//only RA0/DAC1OUT1 output available on 16F15325

// use dac_init() to also set the dac level
// dac_deinit() powers off dac and fvr if used
//=============================================================================
void dac_init   (dac_pss_t, uint8_t);   //pos src, dac level
void dac_deinit (void);


#endif //__DAC_H__