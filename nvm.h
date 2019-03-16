#ifndef __NVM_H__
#define __NVM_H__

#include <stdint.h>
#include <stdbool.h>


//=============================================================================
enum {
    nvm_ROWERASE =  1u<<15, //unused bits in wdata, to signify row erase
    nvm_ROWUPDATE = 1u<<14, //unused bits in wdata, to signify row write
    nvm_ID0 = 0x8000,       //these are the only words in config space
    nvm_ID1 = 0x8001,       //  that can be written (in a 16F15325)
    nvm_ID2 = 0x8002,
    nvm_ID3 = 0x8003
};

// all functions will take care of module power, and will leave the
// module powered off when done
// nvm_writeW is a basic way to write a word to any writable flash word
//   it will take care of preserving all other bytes in the flash row
// nvm_writeNW is used to write N words to any writable flash address
//   it will take care of preserving all other bytes in the flash row
// nvm_pgm can be used when anything else is needed- you are responsible
//   for erasing the row, preserving any data, etc.
//=============================================================================
uint16_t    nvm_read    (uint16_t);
bool        nvm_writeW  (uint16_t, uint16_t); //write single word
bool        nvm_writeNW (uint16_t, uint16_t*, uint8_t); //write n words
bool        nvm_pgm     (uint16_t, uint16_t); //write latch, erase, row pgm
uint32_t    nvm_mui     (void); //come up with a unique id from MUID bytes

//=============================================================================
/*
examples-

    read User ID0-
        uint16_t w = nvm_read( nvm_ID0 );
    read Dev ID-
        uint16_t w = nvm_read( 0x8006 );
    read word at irq vector address
        uint16_t w = nvm_read( 4 );

    write 0x25 to User ID0-
        nvm_writeW( nvm_ID0, 0x25 );

    write 4 words to address 0 :)
        uint16_t x[4] = { 1,2,3,4 };
        nvm_writeNW( 0, x, 4 );

    writes will return true if no errors

*/

#endif //__NVM_H__
