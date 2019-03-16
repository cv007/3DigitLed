#include <xc.h>
#include "pwm.h"
#include "pmd.h"
#include "tmr2.h"
#include <stdint.h>
#include "pins.h"

//private
//.............................................................................
//keep track of pwm's in use (store pin_t* so can do pps)
static pin_t* m_pwm_inuse[4] = {0};
//byte offsets to pwm3,4,5,6 registers from pwm3dcl
static uint8_t m_pwm_regs[4] = { 0, 4, 8, 0x78 };


//init pwm, provide pin info ptr, find available pwm channel
//init timer2 if not already on, set pps out for pin
//return pwm_n if success, pwm_NONE if no pwm available
//=============================================================================
            pwmn_t
pwm_init    (pin_t* p, bool inv)
            {
            uint8_t i = 0;
            for(; i < 4 && m_pwm_inuse[i]; i++);
            if(i >= 4) return pwm_NONE;  //no pwm available
            m_pwm_inuse[i] = p;          //ok, save pin

            pmd_reset( pmd_PWM3 + i );  //reset/enable pwm module
            if(tmr2_ison() == 0){       //if timer2 not on, setup
                tmr2_reinit();          //resets, and powers on module
                tmr2_mode( tmr2_FREERUN_0 );
                tmr2_clksrc( tmr2_FOSC4 ); //only clock supported for pwm
                tmr2_pre( tmr2_PRE16 );
                tmr2_prset( 255 );
                tmr2_on( true );
            }
            pwm_duty( i, 0 );           //init duty cycle
            pwm_resume( p );            //setup pps out
            *(&PWM3CON + m_pwm_regs[i]) = inv ? 0x90 : 0x80; //on, polarity
            return (pwmn_t)i;
            }

//turn off pwm module, mark as available, set pps back to lat
//=============================================================================
            void
pwm_deinit  (pwmn_t n)
            {
            pwm_stop( n );              //pps out back to lat
            m_pwm_inuse[n] = 0;         //clear inuse for this pwm
            pmd_off( pmd_PWM3 + n );    //pwm module power off
            }

//set duty cycle
//=============================================================================
            void
pwm_duty    (pwmn_t n, uint16_t dc) //0-1023 -> H<7:0>L<7:6> (<<6)
            {
            *((volatile uint16_t*)(&PWM3DCL + m_pwm_regs[n])) = dc<<6;
            }

//stop pin output
//=============================================================================
            void
pwm_stop    ( pwmn_t n )
            {
            pin_ppsout( m_pwm_inuse[n], pps_LATOUT );
            }

//resume pin output
//=============================================================================
            void
pwm_resume  ( pwmn_t n )
            {
            pin_ppsout( m_pwm_inuse[n], pps_PWM3OUT+n );
            }

