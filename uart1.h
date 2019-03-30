#ifndef _UART1_H_
#define _UART1_H_

#include <stdint.h>
#include "pins.h"

            //power off
            void
uart1_deinit (void);

            //power on, set baud
            void
uart1_init  (uint32_t);

            //enable/disable tx, set pps pin
            void
uart1_txon (bool, pin_t*);

            //enable/disable rx, set pps pin
            void
uart1_rxon (bool, pin_t*);

            //set baud
            void
uart1_baud (uint32_t);

            //enable/disable tx irq, set isr function
            void
uart1_txirqon(bool, void(*)(void));

            //enable/disable rx irq, set isr function
            void
uart1_rxirqon(bool, void(*)(void));



#endif // _UART1_H_