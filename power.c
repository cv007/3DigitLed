#include "power.h"
#include "pins.h"
#include "dac.h"

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

// private
//.............................................................................
pin_t*      m_reg_pin;
pin_t*      m_out3_pin;
pin_t*      m_out5_pin;

// register pins to use, turn off out/reg (should already be off)
//=============================================================================
            void
power_init  (pin_t* preg, pin_t* pout3, pin_t* pout5)
            {
            m_reg_pin = preg;
            m_out3_pin = pout3;
            m_out5_pin = pout5;
            power_out( power_OUT_OFF );
            power_reg( power_REG_OFF );
            }

// power output selection
//=============================================================================
            void
power_out   (power_out_t sel)
            {
            if(m_out3_pin == 0 || m_out5_pin == 0) return; //no init, no power
            power_out_state = sel;
            //all off

            pin_off( m_out3_pin );
            pin_off( m_out5_pin );
            if(sel == power_OUT_OFF) return;

            //TODO
            //check for voltage on output pin- P_mout
            //if voltage, do not enable, blink lights to signal error
            if(sel == power_OUT_3V3) pin_on( m_out3_pin );
            else pin_on( m_out5_pin );
            }

// switching regulator enable/disable
//=============================================================================
            void
power_reg   (power_reg_t sel)
            {
            if(m_reg_pin == 0) return; //no init, no power
            power_reg_state = sel;
            //TODO
            //check if Vdd is close to 5v, if under a certain value
            //do not turn on, blink error lights
            if(sel) pin_on( m_reg_pin );
            else {
                pin_off( m_reg_pin );
                dac_deinit();
            }
            }

// switching regulator adjust via dac
// provide millivolts, will take care of which fvr to select
//=============================================================================
            void
power_regadj(uint16_t mv)
            {
            if(power_reg_state == power_REG_OFF) return;
            dac_pss_t fvrn = dac_FVR1V; // 1.024v
            mv >>= 5; // /32
            if(mv > 31){ mv >>= 1; fvrn++; } // /64, 2.048v
            if(mv > 31){ mv >>= 1; fvrn++; } // /128, 4.096v
            if(mv > 31) mv = 31; //mv too large, set to max
            dac_init( fvrn, mv );
            }
            /*
            Fvr1 = 1.024v /32 = 0.032 = 32mv *31 = 0.992v
            Fvr1 = 2.048v /32 = 0.064 = 64mv *31 = 1.984v
            Fvr1 = 4.096v /32 = 0.128 = 128mv *31= 3.968v
            */