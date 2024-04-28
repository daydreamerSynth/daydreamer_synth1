#include "pitchgenerator.h"
#include <HardwareSerial.h>

/* Glide seems to work. 
one bug with this implementation is that we cannot pitch bend while doing a glide. 
I think this should be ok*/

PitchGenerator::PitchGenerator()
{
    //inputs
    mTargetMidiByte = 0;
    mGlideLength = 0;   //0 TO 1023
    mLegatoOnlyGlide = false;
    mPitchAndLfoBend = 0;
    //outputs
    mOutPitch = calculatePitchBendTlc(60); //c3;

    //calculations
    mDoNewGlide = true;
    mNotesAreFull = false;
    mt_glide = 0;
    mStartPitch = mOutPitch;
    mNextPitch = mOutPitch;
    mVcoMidiValueIsChanged = true;
    mCurrEnvStatus = OFF_STATE;
    mPrevEnvStatus = OFF_STATE;
    mGlideSlope = 0;
    mTimeScalar = 100;
}

PitchGenerator::~PitchGenerator(){}

void PitchGenerator::setLegatoOnlyGlide(bool lConstantOrLegato)
{
    mLegatoOnlyGlide = !lConstantOrLegato;
}

void PitchGenerator::setGlideLength(int lReading)
{
    mGlideLength = lReading;
}

// finds the output of midi note to TLC based on LFO and pitch bend input
unsigned int PitchGenerator::calculatePitchBendTlc(unsigned int lMidiByteIn)
{
    /*
    The memory is like this 
    .. _ _ [] _ _ 
       ^lowest pitch bend, -100
            ^ the real note
                ^highest pitch bend, +100
    ^ -200
                        +200
    */
    unsigned int pureTlc = pgm_read_word_near(gTlcValues + (lMidiByteIn - lowestMidi + noteLimitOffset));   //offset in memory for the pureTlc tone
    static unsigned int pitchBentTlc;
    unsigned int tlcCompare;
    // mPitchAndLfoBend or gPitchBendScaled
    if(mPitchAndLfoBend == 0)
    {
        pitchBentTlc = pureTlc;
    }
    else if (mPitchAndLfoBend > 0)
    {
        tlcCompare = pgm_read_word_near(gTlcValues + (lMidiByteIn - lowestMidi + 2*noteLimitOffset));   //next full step note TLC value
        pitchBentTlc = pureTlc + ((double)(tlcCompare - pureTlc)/pitchBendIncrements) * mPitchAndLfoBend;
        pitchBentTlc = pitchBentTlc > 4095 ? 4095 : pitchBentTlc;
    }
    else
    {
        tlcCompare = pgm_read_word_near(gTlcValues + (lMidiByteIn - lowestMidi));   //previous full step note TLC value
        pitchBentTlc = pureTlc + ((double)(pureTlc - tlcCompare)/pitchBendIncrements) * mPitchAndLfoBend;
        pitchBentTlc = (int)pitchBentTlc < 0 ? 0 : pitchBentTlc;
    }
    
    return pitchBentTlc;
}


// does gliding accounting for pitchBentTlc
unsigned int PitchGenerator::calculateOutPitch(unsigned int lMidiByteIn, ADSR_STATUSES lAdsrStatus)
{
    mPrevEnvStatus = mCurrEnvStatus;
    mCurrEnvStatus = lAdsrStatus;
    // same midi note message
    if(lMidiByteIn == mTargetMidiByte)
    {
        mVcoMidiValueIsChanged = false;
    }
    // a different midi note message
    else
    {
        mt_glide = 0;
        mVcoMidiValueIsChanged = true;   //this is so we know we can get the new information
        mDoNewGlide = true;      // this is a hacky solution to my LFO changing the output constantly. Only doGlide when there is a new midi note.
    }

    mTargetMidiByte = lMidiByteIn;

    if(mVcoMidiValueIsChanged)
    {
        // Serial.print("prev: "); Serial.print(mPrevEnvStatus, DEC); Serial.print("     curr: "); Serial.println(mCurrEnvStatus, DEC);
        // if the note is not holding go to the note
        // if(mLegatoOnlyGlide && (mPrevEnvStatus != mCurrEnvStatus))
        if(mLegatoOnlyGlide && !mNotesAreFull)
        {
            mOutPitch = calculatePitchBendTlc(mTargetMidiByte);
            mt_glide = mGlideLength;
            mGlideSlope = 0;
            mStartPitch = mOutPitch;
        }
        // if the note is hold, glide to the note
        else
        {
            mStartPitch = mOutPitch;
            mNextPitch = calculatePitchBendTlc(mTargetMidiByte);
        }
    }

    // do the gliding
    if(mt_glide < mGlideLength && mDoNewGlide)
    {
        mt_glide++;
        mNextPitch = calculatePitchBendTlc(mTargetMidiByte);
        mGlideSlope = (static_cast<double>(mNextPitch) - mStartPitch) / (mGlideLength);
        mOutPitch = mStartPitch + mGlideSlope * static_cast<double>(mt_glide);
    }
    else
    {
        mOutPitch = calculatePitchBendTlc(mTargetMidiByte);
        mDoNewGlide = false;
    }
    return mOutPitch;
}