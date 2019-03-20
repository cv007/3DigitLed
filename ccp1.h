#ifndef __CCP1_H__
#define __CCP1_H__

#include <stdint.h>
#include <stdbool.h>

//compare mode only

            //ccp modes, not all listed here
            typedef enum
            {
            ccp1_OFF = 0,
            ccp1_COMP_SETMATCH = 8,
            ccp1_COMP_CLRMATCH = 9
            }
ccp1_mode_t;

            //power off ccp1
            void
ccp1_deinit (void);
            //power on ccp1, set mode
            void
ccp1_init   (ccp1_mode_t);
            //set ccp1 mode
            void
ccp1_mode   (ccp1_mode_t);
            //turn on ccp1
            void
ccp1_on     (bool);
            //set compare match
            void
ccp1_prset  (uint16_t);

#endif //__CCP1_H__

