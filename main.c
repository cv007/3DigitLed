/*------------------------------------------------------------------------------
 ___  _  _  ___  ___
|_ _|| \| || __|/ _ \
 | | | .` || _|| (_) |
|___||_|\_||_|  \___/


 3 digit led driver - 0.28", CA,  22.5mm x 10mm, 12pins (no pin6)

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

 using timer1 to refresh led display
 using ccp1 to control brightness via compare match

 sys clock set to 32MHz
 timer1 set to prescale of 2
 refresh rate per digit = 32MHz/2/65536 = 244 = 81Hz/digit
 ccp1 setup to compare match in led display refresh isr (timer1 isr)
 ccp1 period set to brightness value, clear on match
 current digit common pin set to use cc1out pps
 common pin will be on until period match, for 0-65535 levels of brightness
 (works better than pwm for lower brightness levels)
 brightness set from lookup table(CIE 1931), 64 values from 0-63 (to 0-65535)


 each digit has an address
 left digit has lowest address, then middle, then right
 each 3digit display has 3 addresses
 valid (user set) addresses are from 0-507 in steps of 3
 default (unset) addresses are from 512-998 generated from factory muid
*/


// INCLUDES
//=============================================================================
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#include "pins.h"
#include "osc.h"
#include "nco.h"    //delays
#include "pmd.h"    //module power
#include "pwr.h"    //sleep/idle, reset flags
#include "nvm.h"    //read/write flash
#include "tmr1.h"   //display refresh
#include "ccp1.h"   //display brightness
#include "tables.h" //lookup tables

//digit data
typedef struct {
    uint16_t brightness; //brightness value from table
    uint8_t segdata;    //segment data, used in isr
    pin_t* drvpin;      //common driver pin for digit
} digit_t;
digit_t digits[3];

//called from timer1 isr
void update_digits(void){
    //keep track of what digit to display, display in sequence
    static uint8_t n;

    //CCP (65535 levels of brightness- better at low brightness than pwm)
    //common pin back to LAT (off)
    pin_ppsoutF( digits[n].drvpin, pps_LATOUT );
    //stop timer1 and reset to 0 (so low ccp pr match values are not missed)
    tmr1_stop( true );
    tmr1_set( 0 );
    //need to reset ccp out so can match again (default is 0/low)
    ccp1_mode( ccp1_OFF );
    //and set mode again (ccp out is now opposite match state, high)
    //but no ppsout set yet, so no high output on pin
    ccp1_mode( ccp1_COMP_CLRMATCH );


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

    //PWM
    //turn on current digit (pin set to pwm output)
    //pwm_resume( digits[ n ].pwmn );

    //CCP
    pin_ppsoutF( digits[n].drvpin, pps_CCP1OUT );
    ccp1_prset( digits[n].brightness );
    tmr1_stop( false );
}

//to 3 digit decimal, 0-999
void show0_999(uint16_t n){
    if( n > 999 ) return;
    digits[0].segdata = digit_table_hex[ n / 100 ];
    digits[1].segdata = digit_table_hex[ (n / 10) % 10 ];
    digits[2].segdata = digit_table_hex[ n % 10 ];
}
//to 3 digit hex, 0-FFF
void show0_FFF(uint16_t n){
    if( n > 0xFFF ) return;
    digits[0].segdata = digit_table_hex[ (n>>8) & 0xF ];
    digits[1].segdata = digit_table_hex[ (n>>4) & 0xF ];
    digits[2].segdata = digit_table_hex[ n & 0xF ];
}
//clear display
void show_clr(void){
    digits[0].segdata = 0;
    digits[1].segdata = 0;
    digits[2].segdata = 0;
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
    osc_hffreq(osc_HFFREQ32);         //set HF freq
    osc_set(osc_HFINTOSC, osc_DIV1);  //set src, divider- HFINT, /1

    //CCP for brightness control
    ccp1_init( ccp1_OFF );

    //set initial brightness
    digits[0].brightness = brightness_table[32];
    digits[1].brightness = brightness_table[32];
    digits[2].brightness = brightness_table[32];

    //set common drive pins
    digits[0].drvpin = ledC1;
    digits[1].drvpin = ledC2;
    digits[2].drvpin = ledC3;

    //setup timer1 to update digits via irq
    tmr1_init( tmr1_FOSC, tmr1_PRE2 ); //32MHz/2/65536 = 244 = 81Hz/digit
    tmr1_irqon( update_digits ); //set isr function, enable irq
    tmr1_on( true );
    //display refresh now on

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

    //display all brightness levels
    for(;;){
    for(uint8_t i = 0; i < 64; i++ ){
        digits[0].brightness = brightness_table[i];
        digits[1].brightness = brightness_table[i];
        digits[2].brightness = brightness_table[i];
        show0_999( i );
        nco_waitms( 20 + 63 - i );
    }
    for(uint8_t i = 63; i--; ){
        digits[0].brightness = brightness_table[i];
        digits[1].brightness = brightness_table[i];
        digits[2].brightness = brightness_table[i];
        show0_999( i );
        nco_waitms( 20 + 63 - i );
    }
    }


    //show all ascii chars
    show_clr();
    for( uint8_t i = 32, j = 0; i < 128; i++ ){
        if( ! digit_table_ascii[i-32] ) continue;
        digits[j++].segdata = digit_table_ascii[i-32];
        nco_waitms( 100 );
        if( j>2 ) j=0;
    }

    //count up 0-FFF
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



