//#include <xc.h>

//add as needed, just using extern so function is not exposed in a header
extern void nco_isr(void);
extern void tmr1_isr(void);
extern void uart1_isr(void);

// INTERRUPT
//=============================================================================
            void __interrupt()
ISR         ()
            {
            //nco timer
            nco_isr();
            //tmr1
            tmr1_isr();
            //uart1
            uart1_isr();
            }

