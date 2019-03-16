#include <xc.h>
#include "tmr2.h"
#include "pmd.h"

// turn off timer2 module via pmd
//=============================================================================
            void
tmr2_deinit (void)
            {
            pmd_off( pmd_TMR2 );
            }

// reset all timer2 regsiters via pmd
//=============================================================================
            void
tmr2_reinit (void)
            {
            pmd_reset( pmd_TMR2 );
            }
//=============================================================================
            void
tmr2_mode   (tmr2_mode_t mode)
            {
            T2HLTbits.MODE = mode;
            }
//=============================================================================
            void
tmr2_clksrc (tmr2_clksrc_t src)
            {
            T2CLKCON = src;
            }
//=============================================================================
            void
tmr2_pre    (tmr2_pre_t pre)
            {
            T2CONbits.CKPS = pre;
            }
//=============================================================================
            void
tmr2_post   (uint8_t post)
            {
            T2CONbits.OUTPS = post;
            } //0-15 is 1:1-1:16
//=============================================================================
            void
tmr2_on     (bool tf)
            {
            //if turning on, and clock source uses fosc, turn on fosc power
            if(tf){
                tmr2_clksrc_t tclk = T2CLKCON;
                if(tclk==tmr2_FOSC4 || tclk==tmr2_FOSC) pmd_on( pmd_FOSC );
            }
            T2CONbits.ON = tf;
            }
//=============================================================================
            bool
tmr2_ison   (void)
            {
            return T2CONbits.ON;
            }
//=============================================================================
            void
tmr2_set    (uint8_t v){
            TMR2 = v;
            }
//=============================================================================
            uint8_t
tmr2_get    (void)
            {
            return TMR2;
            }
//=============================================================================
            void
tmr2_prset  (uint8_t v)
            {
            PR2 = v;
            }
//=============================================================================
            uint8_t
tmr2_prget  (void)
            {
            return PR2;
            }
