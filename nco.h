#ifndef __NCO_H__
#define __NCO_H__

#include <stdint.h>
#include <stdbool.h>

            //available timers
            typedef enum
            {
            nco_T0, nco_T1, nco_T2, nco_T3,
            nco_T4, nco_T5, nco_T6, nco_T7,
            nco_TNONE
            }
nco_t_t;

            //init is automatic, deinit if no longer want nco running
            void
nco_deinit  (void);

            //us limited to 0xF1000000 4,043,309,056
            //ms limited to 0x3D0000 3,997,696
            //s limited to 4043

            //non-blocking
            bool
nco_expired (nco_t_t);
            nco_t_t
nco_setus   (uint32_t);
            nco_t_t
nco_setms   (__uint24);
            nco_t_t
nco_sets    (uint16_t);
            void
nco_restart (nco_t_t);
            void
nco_release (nco_t_t);

            //blocking
            void
nco_waitus  (uint32_t);
            void
nco_waitms  (uint32_t);
            void
nco_waits   (uint16_t);

/*

examples-

    simple blocking wait
    nco_waitus( 500000 ); // 0.500000 s
    nco_waitms( 5000 );   // 5.000000 s
    nco_waits( 60 );      // 60.000000 s

    non-blocking
    nco_t_t dly = nco_setus( 500000 ); //get a timer, set to 0.500000 s
    //optionally check if actually got an available timer
    //or add to nco_t_t enum to make more available
    if( dly == nco_TNONE ){ none available, what to do ?  }

    //do something, check if timer expired
    if( nco_expired(dly) ){
        do something;
        nco_restart( dly ); //restart using same count
    }

    //if want to change time, release and get another timer
    nco_release( dly );
    dly = nco_setms( 1000 ); // 1.000000 s


*/


#endif //__NCO_H__

