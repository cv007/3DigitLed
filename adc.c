#include <xc.h>
#include "adc.h"
#include "pmd.h"
#include "nco.h"


// simple read, turn on adc module power, setup, wait for acquisition,
// start conversion, get result, turn off adc module, return result
//=============================================================================
            uint16_t
adc_read    (adc_ch_t ch)
            {
            pmd_reset( pmd_ADC );           //->off->on
            ADCON0bits.ADON = 1;
            ADCON1bits.ADFM = 1;            //right justified
            ADCON1bits.ADCS = 0b111;        //ADCRC clock
            ADCON0bits.CHS = (uint8_t)ch;   //set channel
            nco_waitus( 5 );                //acquisition time

            ADCON0bits.GOnDONE = 1;         //start
            while(ADCON0bits.GOnDONE);      //wait
            uint16_t res = ADRES;
            pmd_off( pmd_ADC );              //adc module off
            return res;
            }

// pin version, will convert pin to channel
//=============================================================================
            uint16_t
adc_read_pin(pin_t* p)
            {
            return adc_read( (adc_ch_t)p->port<<3|p->pinnum );
            }
