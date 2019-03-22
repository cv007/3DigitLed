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

// enable tx or rx, set pps pin
//=============================================================================
            void
uart1_trxon (uart1_txrx_t trx, pin_t* p)
            {
            if( trx == uart1_RX ){
                RC1STAbits.CREN = 1;
                pin_ppsin( p, pps_RX1IN );
            } else {
                TX1STAbits.TXEN = 1;
                pin_ppsout( p, pps_TX1OUT );
            }
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
//set here
//=============================================================================
            void
uart1_irqon (uart1_txrx_t trx, void(*fp)(void))
            {
            if( trx == uart1_RX ){
                PIR3bits.RC1IF = 0;
                PIE3bits.RC1IE = 1;             //enable irq
                isrfprx = fp;
            } else {
                PIR3bits.TX1IF = 0;
                PIE3bits.TX1IE = 1;             //enable irq
                isrfptx = fp;
            }
            INTCONbits.PEIE = 1;                //also need peie
            INTCONbits.GIE = 1;                 //and global irq
            }

//=============================================================================
            void
uart1_irqoff(uart1_txrx_t trx)
            {
            if( trx == uart1_RX ) PIE3bits.RC1IE = 0;
            else PIE3bits.TX1IE = 0;
            }

//=============================================================================
            void
uart1_isr   (void)
            {
            if(PIE3bits.RC1IE && PIR3bits.RC1IF){
                PIR3bits.RC1IF = 0;
                if( isrfprx ) isrfprx();
            }
            if(PIE3bits.TX1IE && PIR3bits.TX1IF){
                PIR3bits.TX1IF = 0;
                if( isrfptx ) isrfptx();
            }
            }
