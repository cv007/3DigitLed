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

//digit data
typedef struct {
    uint16_t brightness; //1-1023, 0=no change
    uint8_t segdata;
    pwmn_t pwmn; //digit pwn driver (number)
} digit_t;
digit_t digits[3];

const uint8_t digit_table_hex[] = {
    //DP A B C D E F G
    // 0-9
    0b01111110, //0
    0b00110000, //1
    0b01101101, //2
    0b01111001, //3
    0b00110011, //4
    0b01011011, //5
    0b01011111, //6
    0b01110000, //7
    0b01111111, //8
    0b01111011, //9
    //hex A - F
    0b01110111, //A
    0b00011111, //b
    0b01001110, //C
    0b00111101, //d
    0b01001111, //E
    0b01000111  //F
    //A b C c d E F g H h i I J L l n O o P q r S U u y
};
const uint8_t digit_table_ascii[] = {
    //DP A B C D E F G
    // 32-127
    0b00000000, //32 space
    0b0,        //33 !
    0b00100010, //34 "
    0b0,        //35 #
    0b0,        //36 $
    0b0,        //37 %
    0b0,        //38 &
    0b00000010, //39 '
    0b0,        //40 (
    0b0,        //41 )
    0b0,        //42 *
    0b0,        //43 +
    0b00000100, //44 ,
    0b00000001, //45 -
    0b10000000, //46 . (dp)
    0b0,        //47 /
    0b01111110, //48 0
    0b00110000, //49 1
    0b01101101, //50 2
    0b01111001, //51 3
    0b00110011, //52 4
    0b01011011, //53 5
    0b01011111, //54 6
    0b01110000, //55 7
    0b01111111, //56 8
    0b01111011, //57 9
    0b0,        //58 :
    0b0,        //59 ;
    0b0,        //60 <
    0b00001001, //61 =
    0b0,        //62 >
    0b0,        //63 ?
    0b0,        //64 @
    0b01110111, //65 A
    0b00011111, //66 B (b)
    0b01001110, //67 C
    0b00111101, //68 D (d)
    0b01001111, //69 E
    0b01000111, //70 F
    0b01111011, //71 G (9)
    0b00110111, //72 H
    0b00110000, //73 I (1)
    0b00111100, //74 J
    0b0,        //75 K
    0b00001110, //76 L
    0b0,        //77 M
    0b0,        //78 N
    0b01111110, //79 O (0)
    0b01100111, //80 P
    0b0,        //81 Q
    0b0,        //82 R
    0b01011011, //83 S (5)
    0b0,        //84 T
    0b00111110, //85 U
    0b0,        //86 V
    0b0,        //87 W
    0b0,        //88 X
    0b00100111, //89 Y
    0b0,        //90 Z
    0b01001110, //91 [ (C)
    0b0,        //92 slash
    0b01111000, //93 ]
    0b01000000, //94 ^
    0b0,        //95 _
    0b00000010, //96 `
    0b01110111, //97 a (A)
    0b00011111, //98 b
    0b00001101, //99 c
    0b00111101, //100 d
    0b01001111, //101 e (E)
    0b01000111, //102 f (F)
    0b01111011, //103 g (9)
    0b00010111, //104 h
    0b00010000, //105 i
    0b00111100, //106 j (J)
    0b0,        //107 k
    0b00001110, //108 l (L)
    0b0,        //109 m
    0b00010101, //110 n
    0b00011101, //111 o
    0b01100111, //112 p (P)
    0b01110011, //113 q
    0b00000101, //114 r
    0b01011011, //115 s *5)
    0b0,        //116 t
    0b00011100, //117 u
    0b0,        //118 v
    0b0,        //119 w
    0b0,        //120 x
    0b00100111, //121 y (Y)
    0b0,        //122 z
    0b0,        //123 {
    0b0,        //124 |
    0b0,        //125 }
    0b0,        //126 ~
    0b0         //127
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
    osc_hffreq(osc_HFFREQ32);           //set HF freq
    osc_set(osc_HFINTOSC, osc_DIV1);  //set src, divider- HFINT, /1

    //get pwm for led common drivers
    digits[0].pwmn = pwm_init( ledC1, false );//normal polarity (high=on)
    digits[1].pwmn = pwm_init( ledC2, false );//normal polarity (high=on)
    digits[2].pwmn = pwm_init( ledC3, false );//normal polarity (high=on)

    //set initial brightness
    digits[0].brightness = 256;
    digits[1].brightness = 256;
    digits[2].brightness = 256;

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


    //show all ascii chars
    show_clr();
    for( uint8_t i = 32, j = 0; i < 128; i++ ){
        digits[j++].segdata = digit_table_ascii[i-32];
        nco_waitms( 200 );
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



