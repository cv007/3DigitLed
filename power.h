#ifndef __POWER_H__
#define __POWER_H__

#include "pins.h"
#include <stdbool.h>

/*

this is for SN@Pmate- to control the power enable pins

call power_init with the pins used to control power-
    P_enreg, P_en3, P_en5

the pins 'onval' will be used via the pin_on/off functions-
    P_enreg 'onval' will be 1, and will be a normal output
    P_enX 'onval' will be 0 with odrain enabled, so 'on' will enable the
        output latch low driver and enable the p-ch mosfet, when not enabled
        the pin output floats but the pc-ch mosfet is disabled via the
        external pullup resistor

after calling power_init, you can then call power_reg() to enable/disable
    the 3v3 switching regulator, and power_out() to enable either 3v3 or 5v
    to the power output on the programmer connector

*/

//power output select, regulator select
//=============================================================================
typedef enum { power_OUT_OFF, power_OUT_3V3, power_OUT_5V } power_out_t;
typedef enum { power_REG_OFF, power_REG_ON } power_reg_t;

//functions
//=============================================================================
void        power_init      (pin_t*, pin_t*, pin_t*);
void        power_out       (power_out_t sel);
void        power_reg       (power_reg_t sel);
void        power_regadj    (uint16_t); //set millivolts out

//vars
//=============================================================================
power_reg_t power_reg_state;
power_out_t power_out_state;


#endif //__POWER_H__
