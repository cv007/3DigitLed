#include "util.h"
#include <stdint.h>


// 3 char ascii to int '000'-'999' -> 0-999
// return -1 if any char not ascii number
//=============================================================================
            int16_t
atoi3       (char* str)
            {
            //'000' - '999'
            int16_t ret = 0;
            for( uint8_t mul = 100; ; ){
                uint8_t n = *str++ - '0';
                if( n > 9 ) return -1;
                ret += n * mul;
                if(mul == 1) break;
                mul = mul == 10 ? 1 : 10;
            }
            return ret;
            }
