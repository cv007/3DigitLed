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
#include "nvm.h" //read/write flash
#include "pwm.h"
#include "tmr1.h"


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
    0b01110111, //A
    0b00011111, //b
    0b01001110, //C
    0b00111101, //d
    0b01001111, //E
    0b01000111 //F
    //A b C c d E F g H h i I J L l n O o P q r S U u y
};

//called from timer1 isr
void update_digits(void){
    //keep track of what digit to display, display in sequence
    static uint8_t n;

    //set timer1 overflow in 4ms (83Hz per digit)
    tmr1_set( 0 - 2000 );

    //stop previous digit driver (pin back to LAT value, which is off)
    pwm_stop( digits[ n ].pwmn );

    //advance to next digit - 0,1,2,0,1,2,...
    if( ++n >= sizeof(digits)/sizeof(digits[0]) ) n = 0;

    //get segment data for current digit
    uint8_t dat = digits[ n ].segdata;

    //turn on/off segments
    if( dat & 0b10000000 ) pin_on( ledDP ); else pin_off( ledDP );
    if( dat & 0b01000000 ) pin_on( ledA ); else pin_off( ledA );
    if( dat & 0b00100000 ) pin_on( ledB ); else pin_off( ledB );
    if( dat & 0b00010000 ) pin_on( ledC ); else pin_off( ledC );
    if( dat & 0b00001000 ) pin_on( ledD ); else pin_off( ledD );
    if( dat & 0b00000100 ) pin_on( ledE ); else pin_off( ledE );
    if( dat & 0b00000010 ) pin_on( ledF ); else pin_off( ledF );
    if( dat & 0b00000001 ) pin_on( ledG ); else pin_off( ledG );

    //update pwm duty if brightness value changed (changed if not 0)
    if( digits[ n ].brightness ){
        pwm_duty( digits[ n ].pwmn, digits[ n ].brightness );
        digits[ n ].brightness = 0; //mark as being set
        //so no need to set every time through
    }

    //turn on current digit (pin set to pwm output)
    pwm_resume( digits[ n ].pwmn );
}

//to 3 digit decimal, 0-999
void show0_999(uint16_t n){
    if( n > 999 ) return;
    digits[0].segdata = digit_table[ n / 100 ];
    digits[1].segdata = digit_table[ (n / 10) % 10 ];
    digits[2].segdata = digit_table[ n % 10 ];
}
//to 3 digit hex, 0-FFF
void show0_FFF(uint16_t n){
    if( n > 0xFFF ) return;
    digits[0].segdata = digit_table[ (n>>8) & 0xF ];
    digits[1].segdata = digit_table[ (n>>4) & 0xF ];
    digits[2].segdata = digit_table[ n & 0xF ];
}

// MAIN
//=============================================================================
void main(void) {

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
    //pins now in init state- all pins setup according to what is set in
    //mypins.h, and all pins in 'off' state

    //setup osc
    osc_hffreq(osc_HFFREQ32);           //set HF freq
    osc_set(osc_HFINTOSC, osc_DIV1);  //set src, divider- HFINT, /1

    //get pwm for led common drivers
    digits[0].pwmn = pwm_init( ledC1, false );//normal polarity (high=on)
    digits[1].pwmn = pwm_init( ledC2, false );//normal polarity (high=on)
    digits[2].pwmn = pwm_init( ledC3, false );//normal polarity (high=on)

    //set initial brightness
    digits[0].brightness = 1023;
    digits[1].brightness = 1023;
    digits[2].brightness = 1023;

    //setup timer1 to update digits via irq
    tmr1_init( tmr1_MFINTOSC_500khz, tmr1_PRE1 );
    tmr1_irqon( update_digits ); //set isr function, enable irq
    tmr1_on( true );

    //get display address (0, 3, 6, 9, 12, ..., 507)
    uint16_t myaddress = nvm_read( nvm_ID0 );
    if( myaddress == 0x3FFF ){
        uint32_t r = nvm_mui();
        //turn 32bit number into 512-998
        //0-871 -> 512-998
        r = r % 487; //0-486
        r += 512; //512-998
        myaddress = r;
    }

    //testing


    //display my address briefly
    show0_999( myaddress );
    nco_waits( 3 );



    //count up 0-999
    nco_t_t dly = nco_setus( 100000 );

    uint16_t n = 0;
    for( ; ; ){
        if( nco_expired(dly) ){
            if( ++n > 0xFFF ) n = 0;
            show0_FFF( n );
            digits[1].segdata |= 0x80; //add DP
            nco_restart( dly );
        }
    }


}



