#include <xc.h>
#include "uart1.h"
#include "pmd.h"
#include "osc.h"


// turn off uart1 module via pmd
//=============================================================================
            void
uart1_deinit(void)
            {
            pmd_off( pmd_UART1 );
            }

// reset all uart1 regsiters via pmd, set baud
//=============================================================================
            void
uart1_init  (uint32_t baud)
            {
            pmd_reset( pmd_UART1 );
            uart1_baud( baud );
            }

// enable/disable tx, set pps pin
//=============================================================================
            void
uart1_txon  (bool tf, pin_t* p)
            {
            TX1STAbits.TXEN = tf;
            pin_ppsout( p, tf ? pps_TX1OUT : pps_LATOUT );
            RC1STAbits.SPEN = 1;
            }

// enable/disable rx, set pps pin
//=============================================================================
            void
uart1_rxon  (bool tf, pin_t* p)
            {
            RC1STAbits.CREN = tf;
            pin_ppsin( p, pps_RX1IN );
            RC1STAbits.SPEN = 1;
            }

// set baud, minimum 600 baud, maximum ?
// (could do 921600 w/8-32MHz fosc, would get 1MBaud actual if clocks
//  are accurate)
//=============================================================================
            void
uart1_baud  (uint32_t baud)
            {
            TX1STAbits.BRGH = 1;    //fosc/4
            BAUD1CONbits.BRG16 = 1; //
            SP1BRG = (osc_sysclk / baud / 4) - 1;;
            }


//function pointers for isr
static void (*isrfprx)(void) = 0;
static void (*isrfptx)(void) = 0;

//=============================================================================
            void
uart1_txirqon(bool tf, void(*fp)(void))
            {
            PIE3bits.TX1IE = tf;                //enable/disable irq
            isrfptx = fp;
            if( tf ){
                INTCONbits.PEIE = 1;            //also need peie
                INTCONbits.GIE = 1;             //and global irq
            }
            }

//=============================================================================
            void
uart1_rxirqon(bool tf, void(*fp)(void))
            {
            PIR3bits.RC1IF = 0;
            PIE3bits.RC1IE = tf;                //enable/disable irq
            PIR3bits.RC1IF = 0;
            isrfprx = fp;
            if( tf ){
                INTCONbits.PEIE = 1;            //also need peie
                INTCONbits.GIE = 1;             //and global irq
            }
            }

//=============================================================================
            void
uart1_isr   (void)
            {
            if(PIE3bits.RC1IE && PIR3bits.RC1IF){
                PIR3bits.RC1IF = 0;
                if( isrfprx ) isrfprx();
            }
            //TX unused
            //if(PIE3bits.TX1IE && PIR3bits.TX1IF){
            //    PIR3bits.TX1IF = 0;
            //    if( isrfptx ) isrfptx();
            //}
            }
