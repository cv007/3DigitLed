#include <xc.h>
#include "adc.h"
#include "pmd.h"
#include "nco.h"

//positive reference (private var)
//.............................................................................
static adc_pref_t m_pref = adc_PREF_VDD;

//can set positive reference before calling adc read (if not default Vdd)
//adc_read will reset m_pref back to Vdd when done (0)
//=============================================================================
            void
adc_pref    (adc_pref_t ref)
            {
            m_pref = ref;
            }


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
            ADCON1bits.ADPREF = m_pref;     //positive reference
            ADCON0bits.CHS = (uint8_t)ch;   //set channel
            nco_waitus( 5 );                //acquisition time

            ADCON0bits.GOnDONE = 1;         //start
            while(ADCON0bits.GOnDONE);      //wait
            uint16_t res = ADRES;
            pmd_off( pmd_ADC );             //adc module off
            m_pref = adc_PREF_VDD;          //reset positive reference
            return res;
            }

// pin version, will convert pin to channel
//=============================================================================
            uint16_t
adc_read_pin(pin_t* p)
            {
            return adc_read( (adc_ch_t)p->port<<3|p->pinnum );
            }
