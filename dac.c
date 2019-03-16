#include <xc.h>
#include "dac.h"
#include "pmd.h"
#include "fvr.h"

#include <stdint.h>
#include <stdbool.h>

// init dac to use specified positive reference, set value
// also use to set dac value (pmd_on and setting con0 again harmless)
// not real fast, but fast not needed
//=============================================================================
            void 
dac_init    (dac_pss_t pss, uint8_t v)
            {
            pmd_on( pmd_DAC1 );
            if(pss >= dac_FVR1V){
                fvr_dac( (fvr_compdac_t)(pss & 3) ); //bits0,1 hold fvr lvl
                pss = 8;//bit3=fvr
            }
            DAC1CON1 = v;
            DAC1CON0 = pss | 0xA0; //pss|DAC1EN|DAC1OE1
            }

//=============================================================================
            void 
dac_deinit  (void)
            {
            DAC1CON0 = 0;
            pmd_off( pmd_DAC1 );
            fvr_dac( fvr_DACOFF );
            }
