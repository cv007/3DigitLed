#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "pins.h"

#define __PINS_C__      //so we get the definitions from mypins.txt
#include "mypins.h"     //change mypins.h as needed

// private
// set pin attributes
//.............................................................................
            static
            void
m_pin_attribs(pin_t* p)
            {
            //for registers spaced 11bytes apart (instead of doing pt*11)
            static uint8_t _offset11[] = { 0, 11, 22 };
            uint8_t bm = p->bitmask;
            uint8_t pt = p->port;
            uint8_t o11 = _offset11[pt];
            attrib_t a = p->attribs;
            if(a.pu)     *(&WPUA + o11)     |= bm;  //wpu = pu
                else     *(&WPUA + o11)     &= ~bm;
            if(a.odrain) *(&ODCONA + o11)   |= bm;  //odcon = odrain
                else     *(&ODCONA + o11)   &= ~bm;
            if(a.slewlo) *(&SLRCONA + o11)  |= bm;  //slrcon = slewlo
                else     *(&SLRCONA + o11)  &= ~bm;
            if(a.st)     *(&INLVLA + o11)   |= bm;  //inlvl = st
                else     *(&INLVLA + o11)   &= ~bm;
            if(a.ana)    *(&ANSELA + o11)   |= bm;  //ansel = ana
                else     *(&ANSELA + o11)   &= ~bm;
            if(a.in)     *(&TRISA + pt)     |= bm;  //tris = in
                else     *(&TRISA + pt)     &= ~bm;
            }


// init all pins using its current attributes
//=============================================================================
            void
pin_init_all(void)
            {
            //pps registers remain unchanged after any reset other than POR
            //so set pps to lat, to prevent mysterious problems
            //(like using a pin before its used with a pps function- which will
            // not work after a reset other than POR since pps still set)
            for(uint8_t i = 0; i < 24; *(&RA0PPS + i) = 0, i++);
            for(uint8_t i = 0; i < sizeof(_allpins)/sizeof(_allpins[0]); i++){
                pin_init(&_allpins[i]);
            }
            }

// init pin using its current attributes
//=============================================================================
            void
pin_init    (pin_t* p)
            {
            pin_off(p);         //make sure lat is in state that will be 'off'
            m_pin_attribs(p);   // like if using odrain- lat will be set to
            }                   // high (off) before tris/odrain setup

// set lat value of pin
//=============================================================================
            void
pin_lat     (pin_t* p, bool b)
            {
            if(b) *(&LATA + (p->port)) |= p->bitmask;
            else *(&LATA + (p->port)) &= ~p->bitmask;
            }

// all following functions take a pin_t pointer
// if you want to change an attribute (like dir), change the attribute in the
// pin struct, then call pinit_init() to make the change (all attributes will
// be set at once)


// set lat value to its 'on' state
//=============================================================================
            void
pin_on      (pin_t* p)
            {
            if(p->attribs.onval) *(&LATA + (p->port)) |= p->bitmask;
            else *(&LATA + (p->port)) &= ~p->bitmask;
            }

// set lat value to its 'off' state
//=============================================================================
            void
pin_off     (pin_t* p)
            {
            if(p->attribs.onval) *(&LATA + (p->port)) &= ~p->bitmask;
            else *(&LATA + (p->port)) |= p->bitmask;
            }

// toggle lat value of pin
//=============================================================================
            void
pin_tog     (pin_t* p)
            {
            *(&LATA + (p->port)) ^= p->bitmask;
            }

// get port (pin) value of pin
//=============================================================================
            bool
pin_pin     (pin_t* p)
            {
            return *(&PORTA + (p->port)) & p->bitmask;
            }

// get port (pin) value of pin
//=============================================================================
            bool
pin_ison    (pin_t* p)
            {
            bool b = *(&PORTA + (p->port)) & p->bitmask;
            return ( b == p->attribs.onval );
            }


// set pin pps out value to a function
// also sets to digital output if not already
//=============================================================================
            void
pin_ppsout  (pin_t* p, ppsout_t e)
            {
            p->attribs.in = 0;
            p->attribs.ana = 0;
            m_pin_attribs(p);  
            *(&RA0PPS + (p->port << 3) + p->pinnum) = e;
            }

// set pin pps out value to a function
// Fast version
//=============================================================================
            void
pin_ppsoutF (pin_t* p, ppsout_t e)
            {
            *(&RA0PPS + (p->port << 3) + p->pinnum) = e;
            }

// set pin pps in function to a pin value
// also sets to digital input if not already
//=============================================================================
            void
pin_ppsin   (pin_t* p, ppsin_t e)
            {
            p->attribs.in = 1;  //optional
            p->attribs.ana = 0; //can be removed
            m_pin_attribs(p);   //to speed up
            *(&INTPPS + e) = (p->port << 3) | p->pinnum;
            }

