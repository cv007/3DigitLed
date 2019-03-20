#ifndef __PINS_H__
#define __PINS_H__

#include <stdbool.h>
#include <stdint.h>

            // port letter
            typedef enum
            {
            pin_A, pin_B, pin_C
            }
port_t;

            // pin number
            typedef enum
            {
            pin_0, pin_1, pin_2, pin_3,
            pin_4, pin_5, pin_6, pin_7
            }
pinnum_t;

            // pin attributes
            typedef struct
            {
            unsigned in     :1; //in=1  out=0
            unsigned ana    :1; //ana=1 dig=0
            unsigned pu     :1; //pu=1  npu=0
            unsigned odrain :1; //odrain=1 nodrain=0
            unsigned slewlo :1; //slewlo=1 slewhi=0
            unsigned st     :1; //st=1  ttl=0
            unsigned onval  :1; //lat(dir=0)/pin(dir=1) val when 'on'
            }
attrib_t;

            // pin struct - for each desired pin
            typedef struct
            {
            port_t      port;
            pinnum_t    pinnum;
            uint8_t     bitmask;
            attrib_t    attribs;
            }
pin_t;

//PPSOUT sources
//=============================================================================
            typedef enum
            {
            pps_LATOUT,
            pps_CLC1OUT,  pps_CLC2OUT,  pps_CLC3OUT,  pps_CLC4OUT,
            pps_CWG1AOUT, pps_CWG1BOUT, pps_CWG1COUT, pps_CWG1DOUT,
            pps_CCP1OUT,  pps_CCP2OUT,
            pps_PWM3OUT,  pps_PWM4OUT,  pps_PWM5OUT,  pps_PWM6OUT,
            pps_CK1OUT,   pps_TX1OUT=pps_CK1OUT,      pps_DT1OUT,
            pps_CK2OUT,   pps_TX2OUT=pps_CK2OUT,      pps_DT2OUT,
            pps_C1OUT,    pps_C2OUT,
            pps_SCL1OUT,  pps_SCK1OUT=pps_SCL1OUT,
            pps_SDA1OUT,  pps_SDO1OUT=pps_SDA1OUT,
            pps_TMR0OUT= 0x19,
            pps_NCO1OUT,
            pps_CLKROUT
            }
ppsout_t;

            //PPSIN registers, value is register offset from INTPPS (0x1E90)
            typedef enum
            {
            pps_INTIN = 0,   pps_T0CKIN, pps_T1CKIN, pps_T1GIN, pps_T2IN = 12,
            pps_CCP1IN = 17, pps_CCP2IN, pps_CWG1IN = 33,
            pps_CLCIN0 = 43, pps_CLCIN1, pps_CLCIN2, pps_CLCIN3,
            pps_ADACTIN = 51,
            pps_SCK1IN = 53, pps_SCL1IN = 53,
            pps_SDI1IN, pps_SDA1IN = 54, pps_SS1IN,
            pps_RX1IN = 59,  pps_DT1IN = 59,  pps_CK1IN,
            pps_RX2IN, pps_DT2IN = 61, pps_CK2IN
            }
ppsin_t;

            // pin functions

            //init all pins in all_pins[]
            void
pin_init_all(void);
            //init pin to off, set attributes
            void
pin_init    (pin_t*);
            //set lat to bool val
            void
pin_lat     (pin_t*, bool);
            //set lat to 'onval'
            void
pin_on      (pin_t*);
            //set lat to !'onval'
            void
pin_off     (pin_t*);
            //toggle lat
            void
pin_tog     (pin_t*);
            //get pin val (port)
            bool
pin_pin     (pin_t*);
            //onval == pin_pin
            bool
pin_ison    (pin_t*);
            //set pin to ppsout func
            void
pin_ppsout  (pin_t*, ppsout_t);
            //set pin to ppsout func (faster)
            void
pin_ppsoutF (pin_t*, ppsout_t);
            //set ppsin func to pin
            void
pin_ppsin   (pin_t*, ppsin_t);

// change mypins.h as needed
//=============================================================================
#include "mypins.h"

#endif //__PINS_H__

