#ifndef __PWM_H__
#define __PWM_H__

#include "pins.h"
#include <stdint.h>
#include <stdbool.h>

// returned pwm from pwm_init(), pwm_NONE=none available
//=============================================================================
typedef enum { pwm_3, pwm_4, pwm_5, pwm_6, pwm_NONE } pwmn_t;

// init with pin_t, polarity, returns pwmn_t enum
// deinit to turn off, power down module
// duty sets duty cycle 0-1023
// stop sets pin to LAT value
// resume sets pin to ppsout
//=============================================================================
pwmn_t  pwm_init        (pin_t*, bool);     //pin, polarity
void    pwm_deinit      (pwmn_t);
void    pwm_duty        (pwmn_t, uint16_t); //0-1023
void    pwm_stop        (pwmn_t);
void    pwm_resume      (pwmn_t);

#endif //__PWM__

