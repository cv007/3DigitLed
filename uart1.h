#ifndef _UART1_H_
#define _UART1_H_

#include <stdint.h>
#include "pins.h"

            typedef enum
            {
            uart1_TX, uart1_RX
            }
uart1_txrx_t;

            //power off
            void
uart1_deinit (void);

            //power on, set baud
            void
uart1_init  (uint32_t);

            //enable tx or rx, set pps pin
            void
uart1_trxon(uart1_txrx_t, pin_t*);

            //set baud
            void
uart1_baud (uint32_t);

            //turn on irq (tx ot rx), set isr function
            void
uart1_irqon (uart1_txrx_t, void(*)(void));

            //turn off irq (tx ot rx)
            void
uart1_irqoff(uart1_txrx_t);


#endif // _UART1_H_