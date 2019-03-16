#include "pwr.h"
#include <xc.h>

// private
// H[PCON1]:L[PCON0], also merge STATUS<4:3> into [PCON1]
//.............................................................................
static uint16_t     m_pwr_bootflags;

// private
//.............................................................................
enum {
    PCON0_DEFAULT = 0b00111111,
    PCON1_DEFAULT = 0b00000010,
    STATUS_DEFAULT = 0b00011000,
    ALL_DEFAULT = (STATUS_DEFAULT<<8)|(PCON1_DEFAULT<<8)|PCON0_DEFAULT
};

// return reset flags, if not default state- update stored value and
// set flags back to default
// caller can figure out what to do with flags
// call this after power up to store cause of reset (for later use if wanted)
// the latest reset cause or wakeup cause will be recorded here
//=============================================================================
            uint16_t
pwr_cause   (void)
            {
            //get current flags, including status /to /pd
            uint16_t tmp = (STATUS & STATUS_DEFAULT)<<8;
            tmp |= PCON1<<8;
            tmp |= PCON0;
            //if changed, save and reset to defaults
            if(tmp != ALL_DEFAULT){
                m_pwr_bootflags = tmp;
                PCON0 = PCON0_DEFAULT;
                PCON1 = PCON1_DEFAULT;
                STATUS |= STATUS_DEFAULT;
            }
            return m_pwr_bootflags;
            }

// after any sleep, update the reset cause so will always have the
// latest wakup cause

// cpu, memory run at slowed ratio until irq, then back to full speed
//=============================================================================
            void
pwr_doze    (pwr_dozediv_t div)
            {
            CPUDOZE = 0x60|div; //DOZEN=1,ROI=1, DOZE=div
            SLEEP();            //to doze
            //wait here for enabled irq- if GIE=1, will go to isr here
            pwr_cause();        //update reset flags
            }

// cpu, memory halted until irq
//=============================================================================
            void
pwr_idle    (void)
            {
            CPUDOZE = 0x80;     //IDLEN=1, all DOZE bits 0
            SLEEP();            //to idle
            //wait here for enabled irq- if GIE=1, will go to isr here
            pwr_cause();        //update reset flags
            }

// cpu, memory halted until irq
//=============================================================================
            void
pwr_sleep   (void)
            {
            CPUDOZE = 0;        //IDLEN=0, all DOZE bits 0
            SLEEP();            //to sleep
            //wait here for enabled irq- if GIE=1, will go to isr here
            pwr_cause();        //update reset flags
            }