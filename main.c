/*------------------------------------------------------------------------------
 ___  _  _  ___  ___
|_ _|| \| || __|/ _ \
 | | | .` || _|| (_) |
|___||_|\_||_|  \___/


 3 digit led driver - 0.28", CA,  22.5mm x 10mm, 12pins (no pin6)

 testing with PIC16 Curiosity Board

 change mypins.h when switching to pcb version with ML package

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

all valid incoming bytes are in the range 9,10,13,32-127 decimal
(0x09,0x0A,0x0D,0x20-0x7F)
all incoming bytes > 127 (0x80-0xFF) are ignored

<cr> = 13 decimal (0x0D) -> process previous command, or display previous data
<lf> = 10 decimal (0x0A) -> same as <cr>
<tab> = 9 decimal (0x09) -> start of command



================
=== COMMANDS ===
================

<tab> char is used to start every command (0x09, <tab>, \t)


ADDRESS

set origin address for all displays (becomes current address), 000-999
address advances by 1 for each Text char sent EXCEPT for '.',
or every second Raw byte sent,
address reset to origin at <cr>,
if address is 999 address will not increment (all digits respond to data)

for Text '.' character, the dp will be set for the previous digit and the
address will not increment

    <tab>000<cr>


TEXT - set mode to TEXT (default mode) - applies to all

    <tab>TXT<cr>


RAW - set mode to RAW - applies to all
      (2 ascii hex bytes for each digit, segment data)
      address increments every other byte

    <tab>RAW<cr>


BRIGHTNESS - set digit brightness  - 00-63 - applies to addressed digit

    <tab>B63<cr>


RESET - display hardware reset - applies to all

    <tab>RST<cr>


ID - show display ID (address of first digit of display) - applies to all

    <tab>ID?<cr>


VDD - show display voltage - applies to addressed display

    <tab>VDD<cr>


OER - show overrun error count  - applies to all

    <tab>OER<cr>


FER - show framing error count  - applies to all

    <tab>FER<cr>


ID^ - reset id (address) - applies only to addressed display

    <tab>ID^<cr>


ID! - set id (address) - applies only to addressed display
       (uses 3 digit number previously set to display)
       something like-
       <tab>858<cr> - set address of first digit of display to change
       000<cr> - this will be the new address of the display
       <tab>ID!<cr> - set address
       display will reboot
       new display address will need to be 000,003,006,...,507

    <tab>ID!<cr>


examples
===============================================================================
to display '012' starting at digit 0, and set brightness to 63 for all digits
<tab>999<cr>    //set display address to 999 (all digits respond)
<tab>B63<cr>    //set brightness to 63 (all digits)
<tab>000<cr>    //set address to 0 (any further <cr> will reset address to 0)
012<cr>         //send text '012', then latch/display (<cr>)
//address now is back to 0

to display raw data starting at digit 0, all segments of digit 0 on
<tab>RAW<cr>    //RAW mode
FF<cr>          //hex 0xFF - all segments on (8 bits, 8 segments)
//address now is back to 0 (and still in RAW mode)
<tab>TXT<cr>    //back to TEXT mode



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
#include "disp.h"
#include "commander.h"

//#pragma config IDLOC0 = 0; //set display address manually- 0,3,6,...,507

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
    pin_init_all();
    //pins now in init state- all pins setup according to what is set in
    //mypins.h, and all pins in 'off' state (whatever is defined as off)

    //setup osc
    osc_hffreq(osc_HFFREQ32);         //set HF freq
    osc_set(osc_HFINTOSC, osc_DIV1);  //set src, divider- HFINT, /1

    //init display
    disp_init();
    //display is now working

    //init commander
    commander_init();

    for(;;){
        //could also make the following function an infinite loop
        //but its difficult to debug this chip when we are using the
        //mclr pin for rx, so just continually call this function which
        //will wait for an rx char, process it, and return the char here
        //so we can take a look at it if needed
        //also can add any other processing in this loop which we don't
        //want in the commander.c file, if ever needed
        commander_go();
    }

    //testing

    #if 0
    //display all brightness levels
//     int8_t j = 1;
//     for(uint8_t i = 0; ; ){
//         disp_bright( disp_DIGIT0, i );
//         disp_bright( disp_DIGIT1, i );
//         disp_bright( disp_DIGIT2, i );
//         disp_number( i );
//         disp_show();
//         nco_waitms( 80 + 126 - i*2 );
//         i += j;
//         if( i == 64 ){ j = -1; i = 63; }
//         else if( i == 0 ){ j = 1; }
//     }

    disp_bright( disp_DIGIT0, 32 );
    disp_bright( disp_DIGIT1, 32 );
    disp_bright( disp_DIGIT2, 32 );

    //show all ascii chars
//     for(;;){
//     disp_clear();
//     disp_show();
//     for( uint8_t i = 32; i < 128; i++ ){
//         disp_ascii(disp_DIGIT0, i-2);
//         disp_ascii(disp_DIGIT1, i-1);
//         disp_ascii(disp_DIGIT2, i);
//         disp_show();
//         nco_waitms( 100 );
//     }
//     }
    //count up 0-FFF
    nco_t_t dly = nco_setus( 50000 );

    uint16_t n = 0;
    for( ; ; ){
        if( nco_expired(dly) ){
            if( ++n > 0xFFF ) n = 0;
            disp_hex( n );
            disp_show();
            nco_restart( dly );
        }
    }
    #endif


}



