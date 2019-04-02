#include <xc.h>
#include "disp.h"
#include <stdbool.h>
#include <stdint.h>
#include "pins.h"
#include "ccp1.h"
#include "tables.h"
#include "tmr1.h"
#include "nco.h"

            //digit struct
            typedef struct
            {
            uint16_t bright;        //brightness value from table
            uint16_t bright_buf;    //buffered, so can update all at once
            uint8_t segdata;        //segment data, used in isr
            uint8_t segdata_buf;    //buffered, so can update all at once
            pin_t* drvpin;          //common driver pin for digit
            }
digit_t;

// digit data
//....................................................................private..
            static digit_t
m_digits[3];


// display update, called from timer1 isr
//....................................................................private..
            static void
m_update    (void)
            {
            //keep track of what digit to display, display in sequence
            static uint8_t n;

            //CCP (65535 levels of brightness- better at low brightness than pwm)
            //common pin back to LAT value (off)
            pin_ppsoutF( m_digits[n].drvpin, pps_LATOUT );
            //stop timer1 and reset to 0 (so low ccp pr match values are not missed)
            tmr1_stop( true );
            tmr1_set( 0 );
            //need to reset ccp out so can match again (reset sets output to 0/low)
            ccp1_mode( ccp1_OFF );
            //and set mode again (ccp out is now opposite match state, high)
            //but no ppsout set yet, so no high output on pin
            ccp1_mode( ccp1_COMP_CLRMATCH );

            //advance to next digit - 0,1,2,0,1,2,...
            if( ++n >= sizeof(m_digits)/sizeof(m_digits[0]) ) n = 0;

            //get segment data for current digit
            uint8_t dat = m_digits[ n ].segdata;

            //turn on/off segments
            if( dat & 0b10000000 ) pin_on( ledDP ); else pin_off( ledDP );
            if( dat & 0b01000000 ) pin_on( ledA ); else pin_off( ledA );
            if( dat & 0b00100000 ) pin_on( ledB ); else pin_off( ledB );
            if( dat & 0b00010000 ) pin_on( ledC ); else pin_off( ledC );
            if( dat & 0b00001000 ) pin_on( ledD ); else pin_off( ledD );
            if( dat & 0b00000100 ) pin_on( ledE ); else pin_off( ledE );
            if( dat & 0b00000010 ) pin_on( ledF ); else pin_off( ledF );
            if( dat & 0b00000001 ) pin_on( ledG ); else pin_off( ledG );

            //set ccp pr match for brightness (when to turn off)
            ccp1_prset( m_digits[n].bright );
            //set current drive pin to ccp output (pin now driven on)
            pin_ppsoutF( m_digits[n].drvpin, pps_CCP1OUT );
            //and start timer1
            tmr1_stop( false );
            }

// init display
//=============================================================================
            void
disp_init   (void)
            {
            //CCP for brightness control
            ccp1_init( ccp1_OFF );

            //set initial brightness
            disp_bright( disp_DIGIT0, 32 );
            disp_bright( disp_DIGIT1, 32 );
            disp_bright( disp_DIGIT2, 32 );

            //set common drive pins
            m_digits[0].drvpin = ledC1;
            m_digits[1].drvpin = ledC2;
            m_digits[2].drvpin = ledC3;

            //setup timer1 to update digits via irq
            //32MHz/2/65536 = 244 = 81Hz/digit
            tmr1_init( tmr1_FOSC, tmr1_PRE2 );
            tmr1_irqon( m_update ); //set isr function, enable irq
            tmr1_on( true );

            //display is now working
            }

//to 3 digit decimal, 0-999
//=============================================================================
            void
disp_number (uint16_t n)
            {
            if( n > 999 ) return;
            m_digits[0].segdata_buf = tables_segment_hex[ n / 100 ];
            m_digits[1].segdata_buf = tables_segment_hex[ (n / 10) % 10 ];
            m_digits[2].segdata_buf = tables_segment_hex[ n % 10 ];
            }

//to 3 digit hex, 0-FFF
//=============================================================================
            void
disp_hex    (uint16_t n)
            {
            if( n > 0xFFF ) return;
            m_digits[0].segdata_buf = tables_segment_hex[ (n>>8) & 0xF ];
            m_digits[1].segdata_buf = tables_segment_hex[ (n>>4) & 0xF ];
            m_digits[2].segdata_buf = tables_segment_hex[ n & 0xF ];
            }

//clear display
//=============================================================================
            void
disp_clear  (void)
            {
            m_digits[0].segdata_buf = 0;
            m_digits[1].segdata_buf = 0;
            m_digits[2].segdata_buf = 0;
            }

// flash display n times, at ms milliseconds between on/off
//(off ms, on ms) * n
// leave display on when returning
//=============================================================================
            void
disp_blink  (uint8_t n, uint16_t ms)
            {
            uint16_t save0 = m_digits[0].segdata;
            uint16_t save1 = m_digits[1].segdata;
            uint16_t save2 = m_digits[2].segdata;
            for( ; n; n-- ){
                disp_clear();
                disp_show();
                nco_waitms( ms );
                m_digits[0].segdata_buf = save0;
                m_digits[1].segdata_buf = save1;
                m_digits[2].segdata_buf = save2;
                disp_show();
                nco_waitms( ms );
            }
            }

// set digit brightness (each digit has brightness level)
//=============================================================================
            void
disp_bright (disp_digitn_t n, uint8_t v)
            {
            m_digits[n].bright_buf = tables_bright[ v & 63 ];
            }

// display ascii char to digit, if not a char that can be displayed
// return value will be 0
//=============================================================================
            uint8_t
disp_ascii  (disp_digitn_t n, char c)
            {
            if(c > 127 || c < 32) return 0;
            uint8_t ret = tables_segment_ascii[c-32];
            m_digits[n].segdata_buf = ret;
            return ret;
            }

// display raw data to digit
//=============================================================================
            void
disp_raw    (disp_digitn_t n, uint8_t v)
            {
            m_digits[n].segdata_buf = v;
            }

//transfer buffered data to segment data
// isr uses segdata, so need to call this function to update buffer to segdata
//=============================================================================
            void
disp_show   (void)
            {
            di(); //irq's off while data changes
            m_digits[0].segdata = m_digits[0].segdata_buf;
            m_digits[1].segdata = m_digits[1].segdata_buf;
            m_digits[2].segdata = m_digits[2].segdata_buf;
            m_digits[0].bright = m_digits[0].bright_buf;
            m_digits[1].bright = m_digits[1].bright_buf;
            m_digits[2].bright = m_digits[2].bright_buf;
            ei();
            }


//set decimal point
//=============================================================================
            void
disp_dp     (disp_digitn_t n)
            {
            m_digits[n].segdata_buf |= 0x80;
            }
