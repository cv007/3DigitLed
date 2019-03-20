#ifndef __PMD_H__
#define __PMD_H__

#include <stdbool.h>

            //specific modules to disable/enable, or all at once
            //pmd_FOSC is system clock routed to any peripheral,
            //if need Fosc for any module, you need to enable pmd_FOSC also
            typedef enum
            {
            pmd_IOC, pmd_CLKR, pmd_NVM, pmd_FVR = 6, pmd_FOSC, //original name SYSC
            pmd_TMR0, pmd_TMR1, pmd_TMR2, pmd_NCO1 = 15,
            pmd_ZCD, pmd_CMP1, pmd_CMP2, pmd_ADC = 21, pmd_DAC1,
            pmd_CCP1 = 24, pmd_CCP2, pmd_PWM3, pmd_PWM4, pmd_PWM5, pmd_PWM6,
            pmd_CWG1 = 32, pmd_MSSP1 = 36, pmd_UART1 = 38, pmd_UART2,
            pmd_CLC1 = 41, pmd_CLC2, pmd_CLC3, pmd_CLC4,
            pmd_ALL = 255
            }
pmdctrl_t;

            // turn module on
            void
pmd_on      (pmdctrl_t md);
            // turn module off
            void
pmd_off     (pmdctrl_t md);
            // reset module (?->off->on- all regs reset)
            void
pmd_reset   (pmdctrl_t md);
            // module is on? (M_ALL returns false)
            bool
pmd_ison    (pmdctrl_t md);


#endif //__PMD_H__
