/*------------------------------------------------------------------------------
    PIC16 Curiosity Board

           16F15325
         ----------
     Vdd |1     14| Vss
 E   RA5 |2     13| RA0 c1
 D   RA4 |3     12| RA1 A
MCLR/RA3 |4     11| RA2 F
 DP  RC5 |5     10| RC0 B
 C   RC4 |6      9| RC1 c2
 G   RC3 |7      8| RC2 c3
         ----------

*/


// INCLUDES
//=============================================================================
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#include "pins.h"
#include "osc.h"
#include "nco.h" //delays
#include "pmd.h" //module power
#include "pwr.h" //sleep/idle, reset flags
//#include "nvm.h" //write flash
#include "pwm.h"
#include "tmr1.h"

pwmn_t ledC1pwm;
pwmn_t ledC2pwm;
pwmn_t ledC3pwm;

typedef struct {
    uint16_t brightness; //1-1023, 0=no change
    uint8_t segdata;
    pwmn_t pwmn; //digit pwn driver (number)
} digit_t;
digit_t digits[3];

const uint8_t digit_table[] = {
    //DP A B C D E F G
    0b01111110, //0
    0b00110000, //1
    0b01101101, //2
    0b01111001, //3
    0b00110011, //4
    0b01011011, //5
    0b01011111, //6
    0b01110000, //7
    0b01111111, //8
    0b01110011, //9
    //A b C c d E F g H h i I J L l n O o P q r S U u y
};


void update_digits(void){
    //keep track of what digit to display
    static uint8_t digitn;
    //set timer1 overflow in 2ms
    tmr1_tmrset( 0 - 1000 );
    //stop previous digit driver
    pwm_stop( digits[ digitn ].pwmn );
    //next digit - 0,1,2,0,...
    if( ++digitn > 2 ) digitn = 0;
    //get segment data
    uint8_t dat = digits[ digitn ].segdata;
    //turn on/off segments
    if( dat & 0b10000000 ) pin_on( ledDP ); else pin_off( ledDP );
    if( dat & 0b01000000 ) pin_on( ledA ); else pin_off( ledA );
    if( dat & 0b00100000 ) pin_on( ledB ); else pin_off( ledB );
    if( dat & 0b00010000 ) pin_on( ledC ); else pin_off( ledC );
    if( dat & 0b00001000 ) pin_on( ledD ); else pin_off( ledD );
    if( dat & 0b00000100 ) pin_on( ledE ); else pin_off( ledE );
    if( dat & 0b00000010 ) pin_on( ledF ); else pin_off( ledF );
    if( dat & 0b00000001 ) pin_on( ledG ); else pin_off( ledG );
    //update pwm duty
    if( digits[ digitn ].brightness ){
        pwm_duty( digits[ digitn ].pwmn, digits[ digitn ].brightness );
        digits[ digitn ].brightness = 0; //mark as being set
        //so no need to set every time through
    }
    //turn on digit
    pwm_resume( digits[ digitn ].pwmn );
}


// MAIN
//=============================================================================
void main(void) {

    // init stuff

    //store reset cause in case want to check later
    //calling this function at anytime will return the latest
    //reset or wakeup cause
    pwr_cause();

    //turn off all modules to save power, enable as needed
    //current 'driver' code will tend to take care of turning on module when
    //needed, but if some module is not working, first thing to check is if the
    //module is powered on
    pmd_off( pmd_ALL );

    //pins now in reset state (all input, harmless)
    //design any connected devices so reset state leaves all in safe state
    pin_init_all();
    //pins now in init state- everything is off, but ready to go

    //setup osc
    osc_hffreq(osc_HFFREQ32);           //set HF freq
    osc_set(osc_HFINTOSC, osc_DIV1);    //set src, divider- HFINT, /1


    digits[0].pwmn = pwm_init( ledC1, false );//normal polarity (high=on)
    digits[1].pwmn = pwm_init( ledC2, false );//normal polarity (high=on)
    digits[2].pwmn = pwm_init( ledC3, false );//normal polarity (high=on)

    digits[0].brightness = 512;
    digits[1].brightness = 512;
    digits[2].brightness = 512;

    tmr1_reinit(); //also enables power to timer1
    tmr1_clksrc( tmr1_MFINTOSC_500khz );
    tmr1_pre( tmr1_PRE1 );
    tmr1_tmrset( 0 - 1000 ); //2ms
    tmr1_irqon( update_digits );
    tmr1_on( true );


    nco_t_t dly = nco_setus( 100000 );

    uint16_t n = 0;
    uint8_t n0 = 0, n1 = 0, n2 = 0;

    for( ; ; ){
        if( nco_expired(dly) ){
            if( ++n > 999 ) n = 0;
            n0 = n / 100; n1 = (n / 10) % 10; n2 = n % 10;
            nco_restart( dly );
            digits[0].segdata = digit_table[n0]|0x80;
            digits[1].segdata = digit_table[n1];
            digits[2].segdata = digit_table[n2];
        }
    }


}



