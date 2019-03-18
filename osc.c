#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#include "osc.h"

//private vars
//if ext clock or sosc clock in use, set crystal frequency here
//=============================================================================
const uint32_t m_extfreq;
const uint32_t m_soscfreq;


// private funcs
// get system clock by reading clock registers
//.............................................................................
            void
m_osc_sysclk()
            {
            osc_osc_t src = (osc_osc_t)OSCCON2 & 0x70;
            uint8_t div = OSCCON2 & 0x0F;

            osc_sysclk = 0;
            if(src == osc_EXTOSC)         osc_sysclk = m_extfreq>>div;
            else if(src == osc_SOSC)      osc_sysclk = m_soscfreq>>div;
            else if(src == osc_EXTOSC4X)  osc_sysclk = (m_extfreq<<2)>>div;
            else if(src == osc_LFOSC)     osc_sysclk = 31000>>div;

            //we have to run the following even if sysclk already found
            //to get hf int osc freq
            uint8_t i = OSCFRQbits.HFFRQ;
            uint32_t hff = 1000000;
            if(i == osc_HFFREQ12){
                hff = 12000000;
            } else {
                if(i > osc_HFFREQ12) i--;
                for( ; i; i--, hff<<=1);
            }
            osc_hfclk = hff; //store hf clk freq
            if(src == osc_HFINTOSC2X) hff<<=1; //2x

            //now store to osc_sysclk if not already set
            if( osc_sysclk == 0 ) osc_sysclk = hff>>div;
            }

// set oscillator selection with divider value
// should never end up with an invalid clock as we check if the switch to the
// new clock is ready before we allow the switch- if times out, don't use
// (assuming the fuses are set to use internal frc)
//=============================================================================
            bool
osc_set     (osc_osc_t nosc, osc_div_t div)
            {
            OSCCON3bits.CSWHOLD = 1;    //allows us to abandon if timeout
            OSCCON1 = nosc | div;       //set new src and div at same time

            //a loop 'timeout' in case does not switch
            for( volatile uint16_t i = 8192; i; i-- ){
                if(OSCCON3bits.NOSCR == 0) continue; //not ready
                OSCCON3bits.CSWHOLD = 0;//ok, now let it switch
                m_osc_sysclk();         //compute new value
                return true;
            }
            //failed to switch in time, set back to current osc settings
            //(since using cswhold, a switch cannot occur in this little
            // bit of time after the loop)
            OSCCON1 = OSCCON2;
            return false;
            }

//set HF internal oscillator frequency
//=============================================================================
            void
osc_hffreq  (osc_hffreq_t freq)
            {
            OSCFRQ = freq;
            m_osc_sysclk();             //compute new value
            }