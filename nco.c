#include <xc.h>
#include "nco.h"
#include "pmd.h"
#include <stdint.h>

//use NCO as delay timer
//=============================================================================
//use mfintosc@500khz - so doesn't matter what fosc is
//resolution is 2us per nco tick, inc set to 2, so can use as a normal counter
//code execute time not accounted for
//=============================================================================

// private
//.............................................................................
static uint8_t m_count; //overflow count, upper 12bits (isr incs by 16)
                        //nco acc is lowest 20bits, this fills in upper 12bits

typedef struct {
    uint32_t start;     //start time
    uint32_t count;     //us count
    bool running;       //0=expired, 1=running
} m_timers_t;

// timers available for use (curently 8+1)
static m_timers_t m_timers[nco_TNONE+1]; //last used for blocking wait timer

static bool m_started;         //keep track if nco is init

// private
// init nco timer - done automatically if nco needed
//.............................................................................
            static
            void
m_init      ()
            {
            m_count = 0;                    //reset count
            pmd_reset( pmd_NCO1 );          //on with all default values
            NCO1CLKbits.N1CKS = 0b0011;     //MFINTOSC 500khz (2us per tick)
            NCO1INC = 2;                    //add 2 to acc per tick
            PIR7bits.NCO1IF = 0;            //clear irq flag
                                            //(flag is not part of module)
            NCO1CONbits.N1EN = 1;           //enable nco1
            PIE7bits.NCO1IE = 1;            //enable irq
            INTCONbits.PEIE = 1;            //also need peie
            INTCONbits.GIE = 1;             //and global irq
            m_started = true;
            }

// deinit if no nco longer needed
//=============================================================================
            void
nco_deinit  ()
            {
            PIE7bits.NCO1IE = 0;            //disable irq
            pmd_off( pmd_NCO1 );            //disable module via pmd
            m_started = false;              //so next use will init
            for(uint8_t i = 0; i < nco_TNONE; i++){
                nco_release( (nco_t_t)i );
            }
            }

// called from isr to update overflow counter
//=============================================================================
            void
nco_isr     ()
            {
            if(PIE7bits.NCO1IE && PIR7bits.NCO1IF){
                PIR7bits.NCO1IF = 0;
                m_count += 16; //leave lower 4bits = 0
            }
            }

// get current count (only used here, but others can use if wanted)
//=============================================================================
            uint32_t
nco_count   ()
            {
            __uint24 acc;
            uint16_t c;
            //acc keeps incrementing (3 bytes), so just check if
            //lowest byte overflowed while reading, if so read again
            //if did not rollover, no interrupt so m_count is good also
            do{
                acc = NCO1ACC;
                c = m_count;
            } while ((uint8_t)acc > NCO1ACCL); //check if low byte rollover
            return (uint32_t)acc | ((uint32_t)c<<16); //combine
            }

// check if a timer is expired
//=============================================================================
            bool
nco_expired (nco_t_t t)
            {
            if( m_timers[t].running == false) return true;
            if( (nco_count() - m_timers[t].start) > m_timers[t].count){
                m_timers[t].running = false;
                return true;
            }
            return false;
            }

// get a timer if available, set count to us (max 3600000000)
//=============================================================================
            nco_t_t
nco_setus   (uint32_t us)
            {
            if(m_started == false) m_init();
            if( ((uint8_t*)us)[3] > 0xF0 ) us = 0xF0FFFFFF;
            for(uint8_t i = 0; i < (uint8_t)nco_TNONE; i++){
                if(m_timers[i].count) continue;
                m_timers[i].start = nco_count();
                m_timers[i].count = us;
                m_timers[i].running = true;
                return (nco_t_t)i;
            }
            return nco_TNONE;
            }

// get a timer if available, set count to ms (max 3600000)
//=============================================================================
            nco_t_t
nco_setms   (__uint24 ms)
            {
            return nco_setus( (uint32_t)ms * 1000 );
            }

// get a timer if available, set count to s (max 3600)
//=============================================================================
            nco_t_t
nco_sets    (uint16_t s)
            {
            return nco_setus( (uint32_t)s * 1000 * 1000 );
            }

// restart a timer, using previously set count value
//=============================================================================
            void
nco_restart (nco_t_t t)
            {
            m_timers[t].start = nco_count();
            m_timers[t].running = true;
            }

// release a timer so can be used by others
// (or if wanting a different count, need to release and get another with
//  the count that is wanted)
//=============================================================================
            void
nco_release (nco_t_t t)
            {
            m_timers[t].start = 0;
            m_timers[t].count = 0;
            m_timers[t].running = false;
            }

// blocking wait for us (uses last timer struct- nco_TNONE)
//=============================================================================
            void
nco_waitus  (uint32_t us)
            {
            bool b = m_started; //if we started it, we will deinit
            if(b == false) m_init();
            if( us >= 0xF1000000 ) us = 0xF1000000; //limit
            m_timers[nco_TNONE].start = nco_count();
            m_timers[nco_TNONE].count = us;
            m_timers[nco_TNONE].running = true;
            while( nco_expired( nco_TNONE ) == false);
            if(b == false) nco_deinit(); //deinit if we did the init
            }

// blocking wait for ms
//=============================================================================
            void
nco_waitms  (uint32_t ms)
            {
            if( ms >= 0x3D0000 ) ms = 0x3D0000;
            nco_waitus(  (uint32_t)ms * 1000ul );
            }

// blocking wait for s
//=============================================================================
            void
nco_waits   (uint16_t s)
            {
            if(s > 4043) s = 4043;
            nco_waitms( (uint32_t)s * 1000ul);
            }
