#ifndef __TMR1_H__
#define __TMR1_H__

#include <stdint.h>
#include <stdbool.h>

//simple counter mode only

            //clock sources, not all listed here
            typedef enum {
                tmr1_FOSC4 = 1,
                tmr1_FOSC = 2,
                tmr1_HFINTOSC = 3,
                tmr1_MFINTOSC_500khz = 5
            }
tmr1_clksrc_t;

            // prescale
            typedef enum
            {
            tmr1_PRE1,  tmr1_PRE2,  tmr1_PRE4,  tmr1_PRE8
            }
tmr1_pre_t;


            //power off timer1
            void
tmr1_deinit (void);
            //power on, init timer1 to clock source, prescale
            void
tmr1_init   (tmr1_clksrc_t, tmr1_pre_t);
            //stop timer1
            void
tmr1_stop   (bool);
            //turn on timer1
            void
tmr1_on     (bool);
            //set timer1 value
            void
tmr1_set    (uint16_t);
            //get timer1 value
            uint16_t
tmr1_get    (void);
            //turn on timer1 irq, set isr function
            void
tmr1_irqon  (void(*)(void));
            //turn off irq
            void
tmr1_irqoff (void);

#endif //__TMR1_H__
