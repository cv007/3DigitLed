#include <xc.h>
#include "ccp1.h"
#include "pmd.h"

// turn off ccp1 module via pmd
//=============================================================================
            void
ccp1_deinit (void)
            {
            pmd_off( pmd_CCP1 );
            }

// reset all ccp1 registers via pmd, set mode, turn on
//=============================================================================
            void
ccp1_init   (ccp1_mode_t mode)
            {
            pmd_reset( pmd_CCP1 );
            CCP1CONbits.MODE = mode;
            CCP1CONbits.EN = 1;
            }

// set mode (set to 0 to stop/reset)
//=============================================================================
            void
ccp1_mode   (ccp1_mode_t mode)
            {
            CCP1CONbits.MODE = mode;
            }

//=============================================================================
            void
ccp1_on     (bool tf)
            {
            CCP1CONbits.EN = tf;
            }

//=============================================================================
            void
ccp1_prset  (uint16_t v){
            CCPR1 = v;
            }



