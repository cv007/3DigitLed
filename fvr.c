#include <xc.h>
#include "fvr.h"
#include "pmd.h"

#include <stdint.h>
#include <stdbool.h>

// enable adc fvr at 1x,2x, or 4x
// enable module if 1,2,4x, disable module if 0x and comp/dac not in use
//=============================================================================
            void 
fvr_adc     (fvr_adc_t e)
            {
            uint8_t fvr = FVRCON;   //get (will be 0 if module off)
            fvr &= ~3;              //clear adc bits0-1
            fvr |= e | 0x80;        //add new value (w/enable)
            if(fvr & 0x0F){         //any adc or dac/comp bits set? (0-3)
                pmd_on( pmd_FVR );  //yes, turn on module (may already be on)
                FVRCON |= fvr;      //set register 
            } else pmd_off( pmd_FVR ); //not in use, turn off module
            }

// enable comp/dac fvr at 1x,2x, or 4x
// enable module if 1,2,4x, disable module if 0x and adc not in use
//=============================================================================
            void 
fvr_dac     (fvr_compdac_t e)
            {
            uint8_t fvr = FVRCON;   //get (will be 0 if module off)
            fvr &= ~0xC;            //clear adc bits2-3
            fvr |= (e<<2) | 0x80;   //add new value (w/enable)
            if(fvr & 0x0F){         //any adc or dac/comp bits set? (0-3)
                pmd_on( pmd_FVR );  //yes, turn on module (may already be on)
                FVRCON |= fvr;      //set register 
            } else pmd_off( pmd_FVR ); //not in use, turn off module                
            }

