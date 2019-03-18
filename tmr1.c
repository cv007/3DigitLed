#include <xc.h>
#include "tmr1.h"
#include "pmd.h"

// turn off timer1 module via pmd
//=============================================================================
            void
tmr1_deinit (void)
            {
            pmd_off( pmd_TMR1 );
            }

// reset all timer1 regsiters via pmd, set clock source, prescale
//=============================================================================
            void
tmr1_init   (tmr1_clksrc_t src, tmr1_pre_t pre)
            {
            pmd_reset( pmd_TMR1 );
            T1CLK = src;
            T1CONbits.CKPS = pre;
            }

// stop timer/resume timer
//=============================================================================
            void
tmr1_stop   (bool tf)
            {
            T1CONbits.ON = !tf;
            }

// also set RD16
//=============================================================================
            void
tmr1_on     (bool tf)
            {
            //if turning on, and clock source uses fosc, turn on fosc power
            if(tf){
                tmr1_clksrc_t tclk = T1CLK;
                if(tclk == tmr1_FOSC4 || tclk == tmr1_FOSC) pmd_on( pmd_FOSC );
            }
            T1CONbits.RD16 = 1;
            T1CONbits.ON = tf;
            }

//=============================================================================
            void
tmr1_set    (uint16_t v){
            TMR1 = v;
            }

//=============================================================================
            uint16_t
tmr1_get    (void)
            {
            return TMR1;
            }

//function pointer for isr
static void (*isrfp)(void) = 0;
//set here
//=============================================================================
            void
tmr1_irqon  (void(*fp)(void))
            {
            PIR4bits.TMR1IF = 0;
            PIE4bits.TMR1IE = 1;            //enable/disable irq
            isrfp = fp;
            INTCONbits.PEIE = 1;            //also need peie
            INTCONbits.GIE = 1;             //and global irq
            }

//=============================================================================
            void
tmr1_irqoff (void)
            {
            PIE4bits.TMR1IE = 0;
            }

//=============================================================================
            void
tmr1_isr    (void)
            {
            if(PIE4bits.TMR1IE && PIR4bits.TMR1IF){
                PIR4bits.TMR1IF = 0;
                if( isrfp ) isrfp();
            }
            }
