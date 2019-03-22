//=============================================================================
// THIS FILE IS INCLUDED in pins.h and pins.c
// (so can set all pins in one file- this file)
// setup pins as you want- port/letter/attributes
// ALSO create pointers to the array members
// (declared for pins.h, defined for pins.c)
// make changes between //..... , all else do not change
//=============================================================================

#ifdef __PINS_C__   //define _allpins[] for pins.c

// make _all_pins[] a little nicer to define
// PIN(A,5) instead of pin_A, pin_5, 1<<pin_5, ... (see pin_t definition)
#define PIN(pt,pn) pin_##pt, pin_##pn, 1<<pin_##pn

pin_t _allpins[] = {
//              |    a     o  s     o |- initial
//    port/pin  | i  n  p  d  l  s  n |  values
//    bitmask   | n  a  u  n  w  t  v |
//.............................................................SETUP PINS......
// common anode driven by npn, so common anode pins are high=on
// segments are cathode, so segment pins are low=on
// slew rate turned on for all outputs (slw), 25ns vs normal 5ns
// mclr pin used as rx pin, need to set fuses- MCLRE=OFF, LVF=OFF
// (high voltgae programming only)
    { PIN(A,0), { 0, 0, 0, 0, 1, 0, 1 } }, //[0] ledC1
    { PIN(A,1), { 0, 0, 0, 0, 1, 0, 0 } }, //[1] ledA
    { PIN(A,2), { 0, 0, 0, 0, 1, 0, 0 } }, //[2] ledF
    { PIN(C,0), { 0, 0, 0, 0, 1, 0, 0 } }, //[3] ledB
    { PIN(C,1), { 0, 0, 0, 0, 1, 0, 1 } }, //[4] ledC2
    { PIN(C,2), { 0, 0, 0, 0, 1, 0, 1 } }, //[5] ledC3

    { PIN(A,5), { 0, 0, 0, 0, 1, 0, 0 } }, //[6] ledE
    { PIN(A,4), { 0, 0, 0, 0, 1, 0, 0 } }, //[7] ledD
    { PIN(C,5), { 0, 0, 0, 0, 1, 0, 0 } }, //[8] ledDP
    { PIN(C,4), { 0, 0, 0, 0, 1, 0, 0 } }, //[9] ledC
    { PIN(C,3), { 0, 0, 0, 0, 1, 0, 0 } }, //[10] ledG

    { PIN(A,3), { 1, 0, 1, 0, 0, 0, 0 } }  //[11] pinRX //MCLR as input
/*
            16F15325
            ----------
        Vdd |1     14| Vss
    E   RA5 |2     13| RA0 c1
    D   RA4 |3     12| RA1 A
 rx MCLR/RA3 |4     11| RA2 F
    DP  RC5 |5     10| RC0 B
    C   RC4 |6      9| RC1 c2
    G   RC3 |7      8| RC2 c3
            ----------
*/

//.............................................................SETUP DONE......
};

#endif //__PINS_C__

//=============================================================================
// set up pointers using user specified names to array members above
// the declarations need to match the definitions
//=============================================================================

#ifndef __PINS_C__ //if not pins.c, just declare

//.............................................................DECLARE.........
pin_t* ledC1;
pin_t* ledC2;
pin_t* ledC3;
pin_t* ledA;
pin_t* ledB;
pin_t* ledC;
pin_t* ledD;
pin_t* ledE;
pin_t* ledF;
pin_t* ledG;
pin_t* ledDP;
pin_t* pinRX;

//.............................................................................

#else //define for pins.c

// match declarations above, point to desired _all_pins[] array member
//.............................................................DEFINE...........
pin_t* ledC1 = &_allpins[0];
pin_t* ledA = &_allpins[1];
pin_t* ledF = &_allpins[2];
pin_t* ledB = &_allpins[3];
pin_t* ledC2 = &_allpins[4];
pin_t* ledC3 = &_allpins[5];

pin_t* ledE = &_allpins[6];
pin_t* ledD = &_allpins[7];
pin_t* ledDP = &_allpins[8];
pin_t* ledC = &_allpins[9];
pin_t* ledG = &_allpins[10];

pin_t* pinRX = &_allpins[11];
//.............................................................................
#endif


/*
           16F15325
         ----------
     Vdd |1     14| Vss
     RA5 |2     13| RA0
     RA4 |3     12| RA1
MCLR/RA3 |4     11| RA2
     RC5 |5     10| RC0
     RC4 |6      9| RC1
     RC3 |7      8| RC2
         ----------

           16F15325
         ----------
     Vdd |1     14| Vss
 E   RA5 |2     13| RA0 c1
 D   RA4 |3     12| RA1 A
MCLR/RA3 |4     11| RA2 F
 DP  RC5 |5     10| RC0 B
 C   RC4 |6      9| RC1 c2
 G   RC3 |7      8| RC2 c3
         ----------

*/
