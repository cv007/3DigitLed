#include "nvm.h"
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "pmd.h"

//DCI area in flash
//=============================================================================
enum { 
    ERSIZ = 0x8200,     //Erase Row Size
    WLSIZ,              //Number of write latches
    URSIZ,              //Number of User Rows
    EESIZ,              //EE Data memory size
    PCNT                //Pin Count
};

//get from DCI
//=============================================================================
static uint8_t  m_row_size;     //ERSIZ
static uint16_t m_flash_size;   //ERSIZ*URSIZ= user flash words available
static bool     m_ison;         //keep track of module on status

// private
// init nvm- turn on power, get some flash info if needed (from DCI area)
// if already on, nothing to do
//=============================================================================
            static
            void
m_init      (void)
            {
            if( m_ison ) return;
            m_ison = true; //set before nvm_read, so it does not turn off module
            pmd_on( pmd_NVM );
            if( m_row_size == 0 ) m_row_size = nvm_read( ERSIZ );
            if( m_flash_size == 0 ) m_flash_size = m_row_size*nvm_read( URSIZ );
            }

// read byte from flash or config areas (address >= 0x8000)
// if module was powered off when called, will also power off when done
// if module already powered on, then another function is in use so do not
// power off when done (or the nvm info will be lost, like latch data)
//=============================================================================
            uint16_t 
nvm_read    (uint16_t waddr)
            {
            if( m_ison == false ) pmd_on( pmd_NVM ); 
            NVMADR = waddr;
            NVMCON1 = (waddr & 0x8000) ? 0x41 : 1;  //nvmregs bit6
            uint16_t dat = NVMDAT;                  //get before power off
            if( m_ison == false ) pmd_off( pmd_NVM );
            return dat;
            }

// write a single word to flash
//=============================================================================
            bool
nvm_writeW  (uint16_t waddr, uint16_t wdata)
            {
            return nvm_writeNW( waddr, &wdata, 1 );
            }

// write N words to flash (n = 1 to 255)
//=============================================================================
            bool
nvm_writeNW (uint16_t waddr, uint16_t* pwdata, uint8_t n)
            {
            m_init(); //to make sure _row_size set before we use it
            uint16_t row = waddr & ~(m_row_size-1);
            uint8_t  idx = waddr - row;
            //program a row until n==0
            for(; n;){
                m_init(); //so nvm_read does not turn off nvm 
                //save first word in row, so we can program it again
                //when done writing the latches- just to cover the case
                //when writing to id0-3- which will fail if the update
                //write is not a writable address
                uint16_t dat0; 
                //fill latches with old data or new data
                for( uint8_t i = 0; i < m_row_size; i++ ){
                    uint16_t ndat;
                    if( (i<idx) || n == 0 ) ndat = nvm_read( row+i );
                    else { ndat = *pwdata++; n--; }
                    if( i == 0 ) dat0 = ndat; //save for later
                    if( 0 == nvm_pgm( i, ndat ) ) return false;
                }
                //erase and write to first latch again with rowupdate
                if( 0 == (nvm_pgm( row, nvm_ROWERASE ) && 
                          nvm_pgm( row, dat0|nvm_ROWUPDATE )) ) return false;
                row += m_row_size;
                idx = 0;               
            }
            return true;
}

// write flash row , write latches, or erase row
// waddr >= 0x8000 -> is config address
// wdata - bit15 set = erase row, bit14 set = write row
//         bit15:14 clear = write to latch
// (only bit13:0 used in this pic as it has a 14bit word size)

// to write to latch                waddr, wdata
// to erase row                     waddr, nvm_ROWERASE
// to update flash from latches     waddr, wdata|nvm_ROWUPDATE
//=============================================================================
            bool
nvm_pgm     (uint16_t waddr, uint16_t wdata)
            {
            m_init();               //if not already powered on, do so
            NVMADR = waddr;
            NVMDAT = wdata;
            NVMCON1= waddr & 0x8000 ? 0x40 : 0; //nvmregs bit6            
            if(wdata & nvm_ROWERASE) NVMCON1bits.FREE = 1;
            else if( 0 == (wdata & nvm_ROWUPDATE) ) NVMCON1bits.LWLO = 1;
            uint8_t irqs = INTCON;  //save irq status
            INTCON = 0;             //irq's now off

            //---------------------------------------------------irq off-------
            NVMCON1bits.WREN = 1;
            NVMCON2 = 0x55;
            NVMCON2 = 0xAA;
            NVMCON1bits.WR = 1;             //program/erase takes ~2-2.5ms
            bool err = NVMCON1bits.WRERR;   //get error before we clear nvmcon1
            NVMCON1 = 0;                    //to clear wrerr, wren
            INTCON = irqs;                  //restore irq's
            //---------------------------------------------------irq restored--

            // if was a rowupdate, we are done with current state of nvm,
            // so can turn nvm module off now to save power
            if( wdata & nvm_ROWUPDATE ){
                pmd_off( pmd_NVM );
                m_ison = false;
            }                 
            return (err == 0);
            }

