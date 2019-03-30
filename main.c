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

all valid incoming bytes are in the range 9,10,13,32-127 decimal
(0x09,0x0A,0x0D,0x20-0x7F)
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

plan B
\t char is start of command mode (0x09, <tab>)

<tab>000<cr> - set address 000-999 - applies to all
<tab>TXT<cr> - text mode (ascii)  - applies to all
<tab>RAW<cr> - raw mode (segment data)  - applies to all
<tab>B63<cr> - brightness 00-63  - applies to address
<tab>RST<cr> - reset all displays (hardware reset)  - applies to all
<tab>ID?<cr> - show display id (address)  - applies to all
<tab>VDD<cr> - show voltage  - applies to address

examples from above-
<tab>999<cr>
<tab>B63<cr>
<tab>000<cr>
012<cr>
<tab>RAW<cr>
FF<cr>

typedef enum {
    CMD0, CMD1, CMD2, CMD3,
    DATA, DATA2,
    BAD
} state_t;
state_t state;

enum { LF = 10, CR = 13, TAB = 9 };
typedef enum { TEXT, RAW } mode_t;

uint8_t cmd[3];
mode_t mode = TEXT;
uint8_t rawdat;

for(;;){
    while( rxinfo.head == rx.info.tail );
    char c = rxinfo.buf[ ++rxinfo.tail ];
    if( rxinfo.tail >= sizeof( rxinfo.buf ) ) rxinfo.tail = 0;

    if( c == CR || c == LF ){
        if(state == CMD4){
            process command;
        }
        else if( state == DATA ) update display
        address.current = address.origin
        state = DATA;
        continue;
    }
    else if( c == TAB && state != BAD ){
        state = CMD0;
        continue;
    };
    else if( c < ' ' || c > 127 ){
        continue;
    }

    switch( state ){

    case CMD0: cmd[0] = c; state = CMD1; break;
    case CMD1: cmd[1] = c; state = CMD2; break;
    case CMD2: cmd[2] = c; state = CMD3; break;
    case CMD3:
        //should not get here
        state = BAD;
        break;
    case DATA:
        if( address.current >= address.my && address.current <= address.my+2 ||
            address.current == 999 ){
            if( mode == TEXT ){
                disp_ascii( disp_DIGIT0, c );
            } else {
                if( c >= '0' && c <= '9' ) c -= '0';
                else if( c >= 'A' && c <= 'F' ) c -= 'A' + 10;
                if( c > 15 ) state = BAD;
                else {
                    raw = c<<8; state = DATA2;
                }
            }
        }
        address.current++;
        break;
    case DATA2:
        disp_raw( which digit, raw );
        state = DATA;
        break;
    case TAB:
        state = CMD1;
        break;
    case BAD: //need CR LF to exit this state
        break;
    }

}
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

    //testing


    //display all brightness levels
    for(;;){
        int8_t j = 1;
        for(uint8_t i = 0; ; ){
            disp_bright( disp_DIGIT0, i );
            disp_bright( disp_DIGIT1, i );
            disp_bright( disp_DIGIT2, i );
            disp_number( i );
            disp_show();
            nco_waitms( 20 + 126 - i*2 );
            i += j;
            if( i == 64 ){ j = -1; i = 63; }
            else if( i == 0 ){ j = 1; }
        }
    }


    //show all ascii chars
    for(;;){
    disp_clear();
    disp_show();
    for( uint8_t i = 32; i < 128; i++ ){
        disp_ascii(disp_DIGIT0, i-2);
        disp_ascii(disp_DIGIT1, i-1);
        disp_ascii(disp_DIGIT2, i);
        disp_show();
        nco_waitms( 500 );
    }
    }
    //count up 0-FFF
    nco_t_t dly = nco_setus( 100000 );

    uint16_t n = 0;
    for( ; ; ){
        if( nco_expired(dly) ){
            if( ++n > 0xFFF ) n = 0;
            disp_number( n );
            disp_show();
            nco_restart( dly );
        }
    }


}



