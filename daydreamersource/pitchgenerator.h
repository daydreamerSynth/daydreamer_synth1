/* PitchGenerator Class
 * 
 * This class will handle generating the right CV voltage for each voice of the oscillator
 * The inputs for the calculation:
 * - midiByte
 * - pitch bend message
 * - glide/glide time 
 * - glide or glide setting
 * 
 * The outputs:
 * - pitch in uint16_t for TLC
*/

#include "typedefs.h"
#include <avr/pgmspace.h>

#ifndef PITCHGENERATOR_H
#define PITCHGENERATOR_H

// useful for below: https://newt.phys.unsw.edu.au/jw/notes.html
//highest and lowest is C2 - C6.
//but with pitch bendable its really C#2 - Bb6. take away 4 notes of range

// FOR REV 0.0
// const PROGMEM unsigned int gTlcValues[] = {
//     0, 0,
//     5,      20,     36,     53,     72,     91,     111,    133,    156,    180,    206,    233,    //Octave 4  //  36 - 47   //  C2 - B2
//     262,    292,    324,    359,    396,    435,    476,    521,    568,    618,    671,    728,    //Octave 5  //  48 - 59   //  C3 - B3
//     789,    853,    922,    995,    1072,   1155,   1243,   1336,   1436,   1541,   1654,   1774,   //Octave 6  //  60 - 71   //  C4 - B4
//     1903,   2038,   2183,   2336,   2501,   2676,   2861,   3055,   3265,   3482,   3705,   3916,   //Octave 7  //  72 - 83   //  C5 - B5
//     4021,                                                                                           //Octave 8  //  84 - 95   //  C6 - B6
//     4085, 4095  
// };
// #define highestMidi 84
// #define lowestMidi 36

// FOR REV 1.0
// const PROGMEM unsigned int gTlcValues[] = {
//     0, 0,
//     3,      18,     33,     48,     65,     83,     102,    122,    143,    165,    189,    214,    //Octave 4  //  36 - 47   //  C2 - B2
//     241,    269,    300,    332,   366,    402,     441,    483,    526,    573,    623,    676,    //Octave 5  //  48 - 59   //  C3 - B3
//     733,    793,    857,    925,   998,   1075,    1157,   1244,   1337,   1437,   1542,   1653,   //Octave 6  //  60 - 71   //  C4 - B4
//     1773,   1900,   2034,   2176,  2328,   2489,   2659,   2840,   3030,   3230,   3440,   3654,   //Octave 7  //  72 - 83   //  C5 - B5
//     3860,                                                                                          //Octave 8  //  84 - 95   //  C6 - B6
//     3970, 4032, 4090  
// };
// #define highestMidi 85
// #define lowestMidi 36

// FOR REV 1.1
// const PROGMEM unsigned int gTlcValues[] = {
//     0, 0,
//     13,     26,     40,     55,     71,     88,     106,    126,    146,    168,    191,    215,    //Octave 4  //  36 - 47   //  C2 - B2
//     242,    270,    300,    331,   365,    400,     439,    479,    522,    568,    617,    669,    //Octave 5  //  48 - 59   //  C3 - B3
//     724,    784,    846,    913,   984,   1060,    1140,   1226,   1317,   1414,   1516,   1626,   //Octave 6  //  60 - 71   //  C4 - B4
//     1743,   1866,   1998,   2137,  2285,   2442,   2608,   2784,   2970,   3166,   3371,   3583,   //Octave 7  //  72 - 83   //  C5 - B5
//     3795,                                                                                          //Octave 8  //  84 - 95   //  C6 - B6
//     3948, 4014, 4073  
// };
// FOR REV 1.2
const PROGMEM unsigned int gTlcValues[] = {
    0, 0,
    14,     27,     41,     55,     71,     88,     106,    126,    146,    168,    191,    215,    //Octave 4  //  36 - 47   //  C2 - B2
    242,    270,    300,    331,   365,    400,     439,    479,    522,    568,    617,    669,    //Octave 5  //  48 - 59   //  C3 - B3
    724,    784,    846,    913,   984,   1060,    1140,   1226,   1320,   1419,   1522,   1633,   //Octave 6  //  60 - 71   //  C4 - B4
    1751,   1876,   2010,   2151,  2302,   2461,   2630,   2809,   3000,   3199,   3408,   3622,   //Octave 7  //  72 - 83   //  C5 - B5
    3831,                                                                                          //Octave 8  //  84 - 95   //  C6 - B6
    3959, 4022, 4080  
};
#define highestMidi 85
#define lowestMidi 36

#define noteLimitOffset 2

#define pitchBendIncrements 100



class PitchGenerator
{
    public:
    PitchGenerator();
    ~PitchGenerator();

    //inputs
    unsigned int mTargetMidiByte;
    int mGlideLength;
    bool mLegatoOnlyGlide;
    int mPitchAndLfoBend;
    //outputs
    unsigned int mOutPitch;

    //calculations
    bool mDoNewGlide;
    bool mNotesAreFull;
    int mt_glide;
    unsigned int mStartPitch;
    unsigned int mNextPitch;
    bool mVcoMidiValueIsChanged;
    ADSR_STATUSES mCurrEnvStatus;
    ADSR_STATUSES mPrevEnvStatus;
    double mGlideSlope;
    int mTimeScalar;

    void setGlideLength(int lReading);
    unsigned int calculatePitchBendTlc(unsigned int lMidiByteIn);
    unsigned int calculateOutPitch(unsigned int lMidiByteIn, ADSR_STATUSES lAdsrStatus);
    void setLegatoOnlyGlide(bool lConstantOrLegato);
};

#endif PITCHGENERATOR_H
