#include "commander.h"
#include <stdint.h>
#include <xc.h>
#include "uart1.h"
#include "nvm.h"
#include "disp.h"
#include "string.h"

            typedef enum
            {
            CMD0, CMD1, CMD2, CMD3,
            TXT, RAW1, RAW2,
            BAD
            }
state_t;
state_t state;

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
atoi3       (void)
            {
            int16_t ret = 0;
            uint8_t i = 0;
            for( uint8_t mul = 100; ; i++ ){
                uint8_t n = cmd[i] - '0';
                if( n > 9 ) return -1;
                ret += n * mul;
                if(mul == 1) break;
                mul = mul == 10 ? 1 : 10;
            }
            return ret;
            }

//.............................................................................
            void
addr_inc    (void)
            {
            if( address.current < 510 && address.current != 999 ){
                address.current++;
            }
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
            <tab>B63<cr> - brightness 00-63  - applies to address
            <tab>RST<cr> - reset all displays (hardware reset)  - applies to all
            <tab>ID?<cr> - show display id (address)  - applies to all
            <tab>VDD<cr> - show voltage  - applies to all
            */

            if( cmd[0] >= '0' && cmd[0] <= '9' ){
                int16_t v = atoi3();
                if( v >= 0 ) address.origin = v;
                return;
            }
            if( 0 == strncmp ( (const char*)cmd, "TXT", 3 ) ){
                mode = TEXT;
                state = TXT;
                return;
            }
            if( 0 == strncmp ( (const char*)cmd, "RAW", 3 ) ){
                mode = RAW;
                state = RAW1;
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
                //TODO
            }
            //brightness applies only to current address
            if( cmd[0] == 'B' ){
                cmd[0] = '0'; //B63 -> 063
                int16_t v = atoi3();
                if( v < 0 || v > 63 ) return;
                if( addr_match(address.my) ) disp_bright( disp_DIGIT0, v );
                if( addr_match(address.my+1) ) disp_bright( disp_DIGIT1, v );
                if( addr_match(address.my+2) ) disp_bright( disp_DIGIT2, v );
                disp_show();
            }
            }

//.............................................................................
            void
proc_txt    (char c)
            {
            //TODO
            //take care of DP (show on previous digit)
            if( addr_match(address.my) ) disp_ascii( disp_DIGIT0, c );
            if( addr_match(address.my+1) ) disp_ascii( disp_DIGIT1, c );
            if( addr_match(address.my+2) ) disp_ascii( disp_DIGIT2, c );
            addr_inc();
            }

//.............................................................................
            void
proc_raw    (char c)
            {
            static uint8_t dat;

            if( c >= '0' && c <= '9' ) c -= '0';
            else if( c >= 'A' && c <= 'F' ) c -= 'A' + 10;
            if( c > 15 ){ state = BAD; return; }

            if( state == RAW1 ){
                dat = c<<8;
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

//.............................................................................
            void
crlf        (void)
            {
            if( state == CMD3 ) proc_cmd();
            else if( state == TXT || state == RAW1 ) disp_show();
            address.current = address.origin;
            state = mode == TEXT ? TXT : RAW1;
            }


//=============================================================================
            void
commander_init()
            {
            //get display address (0, 3, 6, 9, 12, ..., 507)
            address.my = nvm_read( nvm_ID0 );
            if( address.my == 0x3FFF ){
                uint32_t r = nvm_mui();
                //turn 32bit number into 513-996
                r = r % 484; //0-483
                r += 513; //513-996
                r = r - (r % 3); //and make divisable by 3
                address.my = r;
            }

            //display my address if not a user set address
            if( address.my > 512 ){
                disp_number( address.my );
                disp_show();
                disp_blink( 9, 333 );
            }

            //setup uart1
            uart1_init( 19200 );
            uart1_rxon( true, pinRX );
            uart1_rxirqon( true, uart1rx );

            }

// continually call from main
//=============================================================================
            void
commander_go()
            {
            while( rxinfo.head == rxinfo.tail );
            char c = rxinfo.buf[ ++rxinfo.tail ];
            if( rxinfo.tail >= sizeof( rxinfo.buf ) ) rxinfo.tail = 0;

            if( c == CR || c == LF ){ crlf(); return; }
            if( c == TAB && state != BAD ){ state = CMD0; return; };
            if( c < ' ' || c > 127 ) return;

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
            }
