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

const PROGMEM unsigned int gTlcValues[] = {
    0, 0,
    5,      20,     36,     53,     72,     91,     111,    133,    156,    180,    206,    233,    //Octave 4  //  36 - 47   //  C2 - B2
    262,    292,    324,    359,    396,    435,    476,    521,    568,    618,    671,    728,    //Octave 5  //  48 - 59   //  C3 - B3
    789,    853,    922,    995,    1072,   1155,   1243,   1336,   1436,   1541,   1654,   1774,   //Octave 6  //  60 - 71   //  C4 - B4
    1903,   2038,   2183,   2336,   2501,   2676,   2861,   3055,   3265,   3482,   3705,   3916,   //Octave 7  //  72 - 83   //  C5 - B5
    4021,                                                                                           //Octave 8  //  84 - 95   //  C6 - B6
    4085, 4095  
};

#define noteLimitOffset 2

#define pitchBendIncrements 100
#define highestMidi 84
#define lowestMidi 36


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
