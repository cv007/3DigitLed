#include "pmd.h"
#include <xc.h>
#include <stdint.h>

//enable module
//associated sfr registers now in POR reset state (if was previously off)
//=============================================================================
            void
pmd_on      (pmdctrl_t m)
            {
            if( m == pmd_ALL ) PMD0 = PMD1 = PMD2 = PMD3 = PMD4 = PMD5 = 0;
            else *(&PMD0 + (m >> 3)) &= ~(1<<(m&7));
            }

//disable module - all module sfr's read as 0, write does nothing
//associated PPS also disabled
//=============================================================================
            void
pmd_off     (pmdctrl_t m)
            {
            if( m == pmd_ALL ) PMD0 = PMD1 = PMD2 = PMD3 = PMD4 = PMD5 = 0xFF;
            else *(&PMD0 + (m >> 3)) |= (1<<(m&7));
            }

//reset module ( ?->off->on )
//associated sfr registers now in POR reset state
//=============================================================================
            void
pmd_reset   (pmdctrl_t m)
            {
            pmd_off( m );
            pmd_on( m );
            }

//get module status, 0=off, 1=on
//=============================================================================
            bool
pmd_ison    (pmdctrl_t m)
            {
            if( m == pmd_ALL ) return false;
            return *(&PMD0 + (m >> 3)) & (1<<(m&7));
            }