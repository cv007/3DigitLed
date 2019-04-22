#include "commander.h"
#include <stdint.h>
#include <xc.h>
#include "uart1.h"
#include "nvm.h"
#include "disp.h"
#include "string.h"
#include "fvr.h"
#include "adc.h"

            typedef enum
            {
            CMD0, CMD1, CMD2, CMD3,
            TXT, RAW1, RAW2,
            BAD
            }
state_t;
state_t state = TXT;

            typedef enum
            {
            TEXT, RAW
            }
mode_t;
mode_t mode = TEXT;

            typedef struct
            {
            uint16_t my;
            uint16_t origin;
            uint16_t current;
            uint16_t uid;
            uint8_t new[3]; //store ascii here, for setting new address
            }
address_t;
address_t address;

            typedef struct
            {
            uint8_t buf[16];
            uint8_t head;
            uint8_t tail;
            uint8_t ferr_count;
            uint8_t oerr_count;
            }
rx_t;
volatile rx_t rxinfo;

            enum
            {
            TAB = 9, LF = 10, CR = 13
            };

uint8_t cmd[3];


//.............................................................................
            void
uart1rx     (void)
            {
            RC1STAbits_t err = RC1STAbits;  //get errors first
            uint8_t c = RC1REG;             //then rx byte
            if( err.FERR ){                 //framing error
                rxinfo.ferr_count++;
                return;
            }
            if( err.OERR ){                 //rx overrun error
                RC1STAbits.CREN = 0;        //reset usart rx
                RC1STAbits.CREN = 1;
                rxinfo.oerr_count++;
                return;
            }
            rxinfo.head++;
            if( rxinfo.head >= sizeof(rxinfo.buf) ) rxinfo.head = 0;
            if( rxinfo.head == rxinfo.tail ){
                rxinfo.oerr_count++; //even though not hardware error, add
                rxinfo.head--;
                if( rxinfo.head >= sizeof(rxinfo.buf) ){
                    rxinfo.head = sizeof(rxinfo.buf)-1;
                }
                return;
            }
            rxinfo.buf[rxinfo.head] = c;       //save rx byte
            }

// 3 char ascii to int '000'-'999' -> 0-999
// return -1 if any char not ascii number
//.............................................................................
            int16_t
atoi3       (uint8_t* arr)
            {
            int16_t ret = 0;
            uint8_t i = 0;
            for( uint8_t mul = 100; ; i++ ){
                uint8_t v = arr[i] - '0';   //v is unsigned
                if( v > 9 ) return -1;      //so only '0'-'9' will pass
                ret += v * mul;
                if(mul == 1) break;
                mul = mul == 10 ? 1 : 10;
            }
            return ret;
            }

//.............................................................................
            void
addr_inc    (void)
            {
            //999 does not inc, and don't let address get to 999
            if( address.current < 998 ) address.current++;
            }

//.............................................................................
            bool
addr_match  (uint16_t a)
            {
            return address.current == a || address.current == 999;
            }

//.............................................................................
            void
proc_cmd    (void)
            {
            /*
            <tab>000<cr> - set address 000-999 - applies to all
            <tab>TXT<cr> - text mode (ascii)  - applies to all
            <tab>RAW<cr> - raw mode (segment data)  - applies to all
            <tab>B63<cr> - brightness 00-63  - applies only to addressed digit
            <tab>RST<cr> - reset all displays (hardware reset)  - applies to all
            <tab>ID?<cr> - show display id (address)  - applies to all
            <tab>VDD<cr> - show voltage  - applies to all
            <tab>OER<cr> - show overrun error count  - applies to all
            <tab>FER<cr> - show framing error count  - applies to all

            <tab>ID^<cr> - reset id (address) - applies only to addressed display
            */

            if( cmd[0] >= '0' && cmd[0] <= '9' ){
                int16_t v = atoi3(cmd);
                if( v >= 0 ) address.origin = v;
                return;
            }
            if( 0 == strncmp ( (const char*)cmd, "TXT", 3 ) ){
                mode = TEXT;
                return;
            }
            if( 0 == strncmp ( (const char*)cmd, "RAW", 3 ) ){
                mode = RAW;
                return;
            }
            if( 0 == strncmp ( (const char*)cmd, "RST", 3 ) ){
                RESET();
            }
            if( 0 == strncmp ( (const char*)cmd, "ID?", 3 ) ){
                disp_number( address.my );
                disp_show();
                return;
            }
            if( 0 == strncmp ( (const char*)cmd, "VDD", 3 ) ){
                disp_clear();
                disp_show(); //do adc with led's off
                fvr_adc( fvr_ADC2X );
                uint16_t adcv = adc_read( adc_FVR1 ); //am assuming adc fvr is fvr1
                fvr_adc( fvr_ADCOFF );
                //Vdd = fvr / adcv * 1023
                //Vdd*100 =  2.048*100*1000 / adcv * 1023 / 1000
                //Vdd*100 =  2.048*100*1023 / adcv
                //Vdd  = 209510/512 = 409 (4.09v)
                adcv = 209510UL / adcv;
                disp_number( adcv );
                disp_dp( disp_DIGIT0 );
                disp_show();
                return;
            }
            if( 0 == strncmp ( (const char*)cmd, "OER", 3 ) ){
                disp_number( rxinfo.oerr_count );
                disp_show();
                return;
            }
            if( 0 == strncmp ( (const char*)cmd, "FER", 3 ) ){
                disp_number( rxinfo.ferr_count );
                disp_show();
                return;
            }
            //brightness applies only to current address
            if( cmd[0] == 'B' ){
                cmd[0] = '0'; //change 'B' to '0', B63 -> 063
                int16_t v = atoi3(cmd);
                if( v < 0 ) return; //bad number
                if( v > 63 ) v = 63; //if over max, set to max
                if( addr_match(address.my) ) disp_bright( disp_DIGIT0, v );
                if( addr_match(address.my+1) ) disp_bright( disp_DIGIT1, v );
                if( addr_match(address.my+2) ) disp_bright( disp_DIGIT2, v );
                disp_show();
                return;
            }
            if( 0 == strncmp ( (const char*)cmd, "ID^", 3 ) ){
                //has to match digit0 address
                if( ! addr_match(address.my) ) return;
                nvm_writeW( nvm_ID0, 0x3FFF ); //erase
                RESET(); //and reset (which will flash address)
            }
            if( 0 == strncmp ( (const char*)cmd, "ID!", 3 ) ){
                //has to match digit0 address
                if( ! addr_match(address.my) ) return;
                int16_t a = atoi3( address.new ); //get number previously displayed
                if( a >= 0 && a <= 507 && (a % 3 == 0) && address.current != 999 ){
                    nvm_writeW( nvm_ID0, a );
                }
                RESET();
            }
            }

//.............................................................................
            void
proc_txt    (char c)
            {
            //take care of DP (show on previous digit)
            if( c == '.' ){
                //is dp, address already one ahead, so look for match+1
                //and set dp of previous digit, do not inc address
                if( addr_match(address.my+1) ) disp_dp( disp_DIGIT0 );
                if( addr_match(address.my+2) ) disp_dp( disp_DIGIT1 );
                if( addr_match(address.my+3) ) disp_dp( disp_DIGIT2 );
                //do not inc addr
            } else {
                if( addr_match(address.my  ) ){
                    disp_ascii( disp_DIGIT0, c );
                    address.new[0] = c; //also store to set new address
                }
                if( addr_match(address.my+1) ){
                    disp_ascii( disp_DIGIT1, c );
                    address.new[1] = c;
                }
                if( addr_match(address.my+2) ){
                    disp_ascii( disp_DIGIT2, c );
                    address.new[2] = c;
                }
                addr_inc();
            }
            }

//.............................................................................
            void
proc_raw    (char c)
            {
            static uint8_t dat;

            if( c >= '0' && c <= '9' ){ c -= '0'; }
            else if( c >= 'A' && c <= 'F' ){ c = c - 'A' + 10; }
            else if( c >= 'a' && c <= 'f' ){ c = c - 'a' + 10; }
            else { state = BAD; return; }

            if( state == RAW1 ){
                dat = c<<4;
                state = RAW2;
                return;
            }
            dat |= c;
            if( addr_match( address.my ) ) disp_raw( disp_DIGIT0, dat );
            if( addr_match( address.my+1 ) ) disp_raw( disp_DIGIT1, dat );
            if( addr_match( address.my+2 ) ) disp_raw( disp_DIGIT2, dat );
            state = RAW1;
            addr_inc();
            }


//=============================================================================
            void
commander_init()
            {
            //get stored display address (0, 3, 6, 9, 12, ..., 507)
            address.my = nvm_read( nvm_ID0 );

            //make unique id from muid (if no id set in ID0)
            uint32_t r = nvm_mui(); //get factory muid
            //turn 32bit number into 513-996
            r = r % 484; //0-483
            r += 513; //513-996
            r = r - (r % 3); //and make divisable by 3
            address.uid = r;

            //if no address stored, use uid address
            //and show in on power up as reminder to set address
            if( address.my == 0x3FFF ){
                address.my = address.uid;
                disp_number( address.my );
                disp_show();
                disp_blink( 5, 333 );
            }

            //setup uart1
            uart1_init( 19200 );
            uart1_rxon( true, pinRX );
            uart1_rxirqon( true, uart1rx );

            }

// continually call from main
//=============================================================================
            char //void
commander_go()
            {
            //wait for rx
            while( rxinfo.head == rxinfo.tail );

            //process rx byte from buffer
            if( ++rxinfo.tail >= sizeof( rxinfo.buf ) ) rxinfo.tail = 0;
            char c = rxinfo.buf[ rxinfo.tail ];

            //cr/lf- process command or display text/raw data
            //reset state to text or raw, reset address
            if( c == CR || c == LF ){
                if( state == CMD3 ){
                    proc_cmd();
                }
                else if( state == TXT || state == RAW1 ){
                    disp_show();
                }
                state = mode == TEXT ? TXT : RAW1;
                address.current = address.origin;
                return c;
            }

            //start of command is tab char, only if not in bad state
            if( c == TAB && state != BAD ){
                state = CMD0;
                return c;
            };

            //ignore non ascii chars (could also make to cause bad state)
            if( c < ' ' || c > 127 ) return c;

            //rx char into command array, or process text/raw char
            switch( state ){
                case CMD0: cmd[0] = c; state = CMD1; break;
                case CMD1: cmd[1] = c; state = CMD2; break;
                case CMD2: cmd[2] = c; state = CMD3; break;
                case CMD3: state = BAD; break; //should not get here
                case TXT: proc_txt(c); break;
                case RAW1: proc_raw(c); break;
                case RAW2: proc_raw(c); break;
                case BAD: break; //need CR LF to exit this state
            }
            return c;
            }

