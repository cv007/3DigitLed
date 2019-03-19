/*------------------------------------------------------------------------------
 ___  _  _  ___  ___
|_ _|| \| || __|/ _ \
 | | | .` || _|| (_) |
|___||_|\_||_|  \___/


 3 digit led driver - 0.28", CA,  22.5mm x 10mm, 12pins (no pin6)

 testing with PIC16 Curiosity Board

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

 _  _   ___  _____  ___  ___
| \| | / _ \|_   _|| __|/ __|
| .` || (_) | | |  | _| \__ \
|_|\_| \___/  |_|  |___||___/

using timer1 to refresh led display
using ccp1 to control brightness via compare match

===============================================================================

sys clock set to 32MHz
timer1 set to prescale of 2
refresh rate per digit = 32MHz/2/65536 = 244 = 81Hz/digit
ccp1 setup to compare match in led display refresh isr (timer1 isr)
ccp1 period set to brightness value, clear on match
current digit common pin set to use cc1out pps
common pin will be on until period match, for 0-65535 levels of brightness
(works better than pwm for lower brightness levels)
brightness set from lookup table(CIE 1931), 64 values from 0-63 (to 0-65535)

===============================================================================

displays are 3 digits, each digit with a fixed base address
left digit = base address, middle = address+1, right digit = address+2
valid addresses for displays are from 0-507 step 3 (0,3,6,...507)
default (unset) display addresses are from 512-998 generated from factory muid
address 999 is reserved- all digits will respond to address 999

all valid incoming bytes are in the range 10,13,32-127 decimal
(0x0A,0x0D,0x20-0x7F)
all incoming bytes > 127 (0x80-0xFF) are ignored

<cr> = 13 decimal (0x0D) = latch command OR
<lf> = 10 decimal (0x0A) = latch command
latch command executes previous valid command, or displays previous sent
display data
a latch command sent when no valid previous command will do nothing, and
will reset state to start of command state

first char after <cr> must be a command byte
all valid commands except T,R are 4 bytes followed by <cr>
T,R commands are both single byte, followed by data, then <cr>

commands
===============================================================================
A000<cr>..A999<cr>  A = address set         000 - 999 = address in ascii decimal
    set current address for all displays
    address advances by 1 for each Text char sent EXCEPT for '.', or every
    second Raw byte sent, resets to set address at <cr>, if address is 999,
    address will not increment

    for Text '.' character, the dp will be set for the previous digit and the
    address will not increment

Tdisplaystring...<cr>   T = text                any number of ascii characters
Rxx...<cr>              R = raw data            2 ascii hex bytes per digit
B000 - B063<cr>         B = brightness          000 - 063 = brightness level
#xxx<cr>                # = other               xxx = other commands

A000<cr>                // address set
Ttest<cr>               // send text <cr>=latch
RFF0042<cr>             // raw (2 ascii hex bytes per digit, '0'-'9','A'-'F')
B000<cr>                // brightness 000-063

#000<cr>                // display power source voltage
#001<cr>                // display id# (digit 0 of display)
#002<cr>                // show overrun error count
#003<cr>                // show framing error count
#999<cr>                // reset all displays (micro reset)

examples
===============================================================================
to display '012' starting at digit 0, and set brightness to 63 for all digits
A999<cr>    //set display address to 999 (all digits respond)
B063<cr>    //set brightness to 63 (all digits), will remain 63 until changed
A000<cr>    //set address to 0 (any further <cr> will reset address to 0)
T012<cr>    //send text '012', and latch/display (<cr>)
//address now is back to 0

to display raw data starting at digit 0, all segments of digit 0 on
A000<cr>        //set address to 0
RFF<cr>         //hex 0xFF - all segments on (8 bits, 8 segments)
//address now is back to 0



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
    //common pin back to LAT value (off)
    pin_ppsoutF( digits[n].drvpin, pps_LATOUT );
    //stop timer1 and reset to 0 (so low ccp pr match values are not missed)
    tmr1_stop( true );
    tmr1_set( 0 );
    //need to reset ccp out so can match again (reset sets output to 0/low)
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

    //set ccp pr match for brightness (when to turn off)
    ccp1_prset( digits[n].brightness );
    //set current drive pin to ccp output (pin now driven on)
    pin_ppsoutF( digits[n].drvpin, pps_CCP1OUT );
    //and start timer1
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
/*------------------------------------------------------------------------------
 - flash display n times, at ms milliseconds between on/off
 - (off ms, on ms) * n
 - leave display on when returning
------------------------------------------------------------------------------*/
void display_flash(uint8_t n, uint16_t ms)
{
    uint16_t save0 = digits[0].segdata;
    uint16_t save1 = digits[1].segdata;
    uint16_t save2 = digits[2].segdata;
    for( ; n; n-- ){
        show_clr();
        nco_waitms( ms );
        digits[0].segdata = save0;
        digits[1].segdata = save1;
        digits[2].segdata = save2;
        nco_waitms( ms );
    }
}

/*------------------------------------------------------------------------------
 - 3 char ascii to int '000'-'999' -> 0-999
 - return -1 if any char not ascii number
------------------------------------------------------------------------------*/
int16_t ascii3int( char* str )
{
    //'000' - '127'
    int16_t ret = 0;
    //100= 0b01100100
    // 10= 0b00001010
    //  1= 0b00000001
    for( uint8_t d = 100; ; str++ ){
        if( *str < '0' || *str > '9' ) return -1;
        for( ; *str > '0'; ret += d, (*str)-- );
        if( d == 1 ) break;
        d = d == 10 ? 1 : 10; //d=100->10->1
    }
    return ret;
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
        //turn 32bit number into 513-996
        r = r % 484; //0-483
        r += 513; //513-996
        r = r - (r % 3); //and make divisable by 3
        myaddress = r;
    }

    //testing


    //display my address briefly
    show0_999( myaddress );
    if( myaddress < 512 ) nco_waits( 3 );
    else display_flash( 9, 333 );

    //display all brightness levels
    for(;;){
    for(uint8_t i = 0; i < 64; i++ ){
        digits[0].brightness = brightness_table[i];
        digits[1].brightness = brightness_table[i];
        digits[2].brightness = brightness_table[i];
        show0_999( i );
        nco_waitms( 20 + 126 - i*2 );
    }
    for(uint8_t i = 63; i--; ){
        digits[0].brightness = brightness_table[i];
        digits[1].brightness = brightness_table[i];
        digits[2].brightness = brightness_table[i];
        show0_999( i );
        nco_waitms( 20 + 126 - i*2 );
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



