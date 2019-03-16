#ifndef __TMR2_H__
#define __TMR2_H__

#include <stdint.h>
#include <stdbool.h>

//only one mode for now
//=============================================================================
typedef enum { tmr2_FREERUN_0 } tmr2_mode_t;

//not all listed here yet (pwm requires fosc/4)
//=============================================================================
typedef enum { tmr2_FOSC4 = 1, tmr2_FOSC } tmr2_clksrc_t;

// prescale
//=============================================================================
typedef enum {
    tmr2_PRE1,  tmr2_PRE2,  tmr2_PRE4,  tmr2_PRE8,
    tmr2_PRE16, tmr2_PRE32, tmr2_PRE64, tmr2_PRE128
} tmr2_pre_t;

//=============================================================================
void    tmr2_deinit     (void);
void    tmr2_reinit     (void);
void    tmr2_mode       (tmr2_mode_t);
void    tmr2_clksrc     (tmr2_clksrc_t);
void    tmr2_pre        (tmr2_pre_t);
void    tmr2_post       (uint8_t); //0-15 is 1:1-1:16
void    tmr2_on         (bool);
bool    tmr2_ison       (void);
void    tmr2_tmrset     (uint8_t);
uint8_t tmr2_tmrget     (void);
void    tmr2_prset      (uint8_t);
uint8_t tmr2_prget      (void);

#endif //__TMR2_H__