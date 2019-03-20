#ifndef __OSC_H__
#define __OSC_H__

            //COSC, NOSC
            typedef enum
            {
            //0 reserved, same as HFINT
            osc_HFINTOSC2X =  1<<4,
            osc_EXTOSC4X =    2<<4,
            //3 reserved, same as HFINT
            osc_SOSC =        4<<4,
            osc_LFOSC =       5<<4,
            osc_HFINTOSC =    6<<4,
            osc_EXTOSC =      7<<4
            }
osc_osc_t;

            //NDIV, CDIV
            typedef enum
            {
            osc_DIV1,  osc_DIV2,   osc_DIV4,   osc_DIV8,  osc_DIV16, osc_DIV32,
            osc_DIV64, osc_DIV128, osc_DIV256, osc_DIV512
            }
osc_div_t;

            //HFFRQ
            typedef enum
            {
            osc_HFFREQ1,  osc_HFFREQ2,  osc_HFFREQ4, osc_HFFREQ8,
            osc_HFFREQ12, osc_HFFREQ16, osc_HFFREQ32
            }
osc_hffreq_t;


            uint32_t
osc_sysclk; //store current freq
            uint32_t
osc_hfclk;  //store current hf int osc freq

// NOTE -
// set _extfreq, _soscfreq in osc.c if these sosc or ext are used


            bool
osc_set     (osc_osc_t, osc_div_t);
            void
osc_hffreq  (osc_hffreq_t);

#endif //__OSC_H__

