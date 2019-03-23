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
#include "disp.h"
#include "uart1.h"

typedef struct {
    uint8_t buf[32];
    uint8_t head;
    uint8_t tail;
    uint8_t ferr_count;
    uint8_t oerr_count;
} rx_t;
volatile rx_t rxinfo;

void uart1rx(void)
{
    RC1STAbits_t err = RC1STAbits;//get errors first
    uint8_t c = RC1REG;          //then rx byte
    if( err.FERR ){             //framing error
        rxinfo.ferr_count++;
        return;
    }
    if( err.OERR ){             //rx overrun error
        RC1STAbits.CREN = 0;    //reset usart rx
        RC1STAbits.CREN = 1;
        rxinfo.oerr_count++;
        return;
    }
    rxinfo.head++;
    if( rxinfo.head >= 32 ) rxinfo.head = 0;
    if( rxinfo.head == rxinfo.tail ){
        rxinfo.oerr_count++; //even though not hardware error, add
        return;
    }
    rxinfo.buf[rxinfo.head++] = c;       //save rx byte
    if( rxinfo.head >= 32 )     //keep inside buffer
        rxinfo.head = 0;
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
    pin_init_all();
    //pins now in init state- all pins setup according to what is set in
    //mypins.h, and all pins in 'off' state (whatever is defined as off)

    //setup osc
    osc_hffreq(osc_HFFREQ32);         //set HF freq
    osc_set(osc_HFINTOSC, osc_DIV1);  //set src, divider- HFINT, /1

    //init display
    disp_init();
    //display is now working

    //setup uart1
    uart1_init( 19200 );
    uart1_trxon( uart1_RX, pinRX );
    uart1_irqon( uart1_RX, uart1rx );

    //testing- check what brg set to
    disp_ascii( disp_DIGIT0, 'B' );
    disp_ascii( disp_DIGIT1, 'a' );
    disp_ascii( disp_DIGIT2, 'u' );
    disp_show();
    nco_waitms( 3000 );
    disp_number( SP1BRG );
    disp_show();
    nco_waitms( 3000 );

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


    //display my address if not a user set address
    if( myaddress > 512 ){
        disp_number( myaddress );
        disp_show();
        disp_blink( 9, 333 );
    }

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
    disp_clear();
    disp_show();
    for( uint8_t i = 32, j = 0; i < 128; i++ ){
        if( ! disp_ascii((disp_digitn_t)j, i) ) continue;
        disp_show();
        nco_waitms( 100 );
        if( j>2 ) j=0;
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



