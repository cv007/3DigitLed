#ifndef __PWR_H__
#define __PWR_H__

#include <xc.h>
#include <stdint.h>

// DOZE cpu divide
//=============================================================================
typedef enum {
    pwr_DOZEDIV2, pwr_DOZEDIV4, pwr_DOZEDIV8, pwr_DOZEDIV16,
    pwr_DOZEDIV32, pwr_DOZEDIV64, pwr_DOZEDIV128, pwr_DOZEDIV256
} pwr_dozediv_t;

//=============================================================================
uint16_t    pwr_cause   (void);         //update/return latest reset flags
void        pwr_doze    (pwr_dozediv_t);//doze, set cpu div ration
void        pwr_idle    (void);         //idle, wait for irq to wake
void        pwr_sleep   (void);         //sleep, wait for irq to wake

#endif //__PWR__