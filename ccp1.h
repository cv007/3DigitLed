#ifndef __CCP1_H__
#define __CCP1_H__

#include <stdint.h>
#include <stdbool.h>

//compare mode only

//modes, not all listed here
//=============================================================================
typedef enum {
    ccp1_OFF = 0,
    ccp1_COMP_SETMATCH = 8,
    ccp1_COMP_CLRMATCH = 9
} ccp1_mode_t;

//=============================================================================
void    ccp1_deinit     (void);
void    ccp1_init       (ccp1_mode_t);
void    ccp1_mode       (ccp1_mode_t);
void    ccp1_on         (bool);
void    ccp1_prset      (uint16_t);

#endif //__CCP1_H__

