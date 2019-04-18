#ifndef _DISP_H_
#define _DISP_H_

#include <stdint.h>

            //enum to specifiy digit, digit0 is left most digit
            typedef enum
            {
            disp_DIGIT0, disp_DIGIT1, disp_DIGIT2
            }
disp_digitn_t;

            //setup timer1, ccp1, etc.
            void
disp_init   (void);
            //display decimal number 000-999
            void
disp_number (uint16_t);
            //display hex number 000-FFF
            void
disp_hex    (uint16_t);
            //display ascii to digit
            uint8_t
disp_ascii  (disp_digitn_t, char);
            //display raw data
            void
disp_raw    (disp_digitn_t, uint8_t);
            //clear display
            void
disp_clear  (void);
            //blink n times, ms between on/off
            void
disp_blink  (uint8_t, uint16_t);
            //set digit brightness
            void
disp_bright (disp_digitn_t, uint8_t);
            //move buffered data to digits
            //buffered so multiple displays can update at the same time
            void
disp_show   (void);
            //set decimal point
            void
disp_dp     (disp_digitn_t);


#endif // _DISP_H_
