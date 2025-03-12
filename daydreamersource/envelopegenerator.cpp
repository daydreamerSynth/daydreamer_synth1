/*
*   This file is part of daydreamer_synth1.
*
*   daydreamer_synth1 is free software: you can redistribute it and/or modify it 
*   under the terms of the GNU General Public License as published by the Free Software Foundation, 
*   either version 3 of the License, or (at your option) any later version.
*
*   daydreamer_synth1 is distributed in the hope that it will be useful, 
*   but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
*   FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License along with 
*   daydreamer_synth1. If not, see <https://www.gnu.org/licenses/>
*/

#include "envelopegenerator.h"
#include <HardwareSerial.h>


EnvelopeGenerator::EnvelopeGenerator()
{
    mAttackLinearParameters.mSlope = 0;
    mAttackLinearParameters.mFinalAmplitude = 0;
    mAttackLinearParameters.mTimeLength = 0;
    mDecayLinearParameters.mSlope = 0;
    mDecayLinearParameters.mFinalAmplitude = 0;
    mDecayLinearParameters.mTimeLength = 0;
    mReleaseLinearParameters.mSlope = 0;
    mReleaseLinearParameters.mFinalAmplitude = 0;
    mReleaseLinearParameters.mTimeLength = 0;

    mTlcScalar = 32;
    mTimeScalar = 100;
    mFirstReleaseIteration = false;
    mTime = 0;
    mEnvelopeOutput = 0;
    mAdsrStatus = OFF_STATE;
    mAttackKnobValue = 0;
    mDecayKnobValue = 0;
    mSustainKnobValue = 0;
    mReleaseKnobValue = 0;
    mVelocity = 0;
}
EnvelopeGenerator::~EnvelopeGenerator(){}

void EnvelopeGenerator::setVelocity(int lMessageByte)
{
    mVelocity = lMessageByte;
}
void EnvelopeGenerator::setAdsrState(ADSR_STATUSES lNewStatus)
{
    mAdsrStatus = lNewStatus;
}
void EnvelopeGenerator::setAttackKnob(int lReading)
{
    mAttackKnobValue = lReading;
}
void EnvelopeGenerator::setDecayKnob(int lReading)
{
    mDecayKnobValue = lReading;
}
void EnvelopeGenerator::setSustainKnob(int lReading)
{
    mSustainKnobValue = lReading;
}
void EnvelopeGenerator::setReleaseKnob(int lReading)
{
    mReleaseKnobValue = lReading;
}


unsigned int EnvelopeGenerator::updateOutput()
{
    // this portion of code is critical path. adding Serial.print or anything time consuming will fuck things up.
    switch(mAdsrStatus)
    {
        case ATTACK_STATE:
            // Serial.println(millis(), DEC);
            // Serial.println("attack state");
            mFirstReleaseIteration = true;
            // consider using knobValue to affect mSlope directly, and not use it for time to calculate mSlope from.
            (mTime == 0) ? mEnvelopeOutput = 300 : mEnvelopeOutput; //300 is practically 0. start at 300 to save start time from silence
            mAttackLinearParameters.mTimeLength = mAttackKnobValue * mTimeScalar + 1;
            mAttackLinearParameters.mFinalAmplitude = mVelocity * mTlcScalar;    
            
            if(mTime < mAttackLinearParameters.mTimeLength && mAttackLinearParameters.mFinalAmplitude - mEnvelopeOutput > 1) // > 1 because it takes to long to get to 0
            {
                mTime++;
                mAttackLinearParameters.mSlope = (mAttackLinearParameters.mFinalAmplitude - mEnvelopeOutput) / (mAttackLinearParameters.mTimeLength - mTime);
                mEnvelopeOutput += mAttackLinearParameters.mSlope * (mTime);
            }
            else
            {
                mEnvelopeOutput = mAttackLinearParameters.mFinalAmplitude;
                mTime = 0;
                mAdsrStatus = DECAY_STATE;
            }

            // Serial.print("t = ");   Serial.print(t, DEC);
            // Serial.print("      mTimeLength = ");   Serial.print(mAttackLinearParameters.mTimeLength, DEC);
            // Serial.print("      mVelocity = ");   Serial.print(mVelocity, DEC);
            // Serial.print("      mSlope = ");   Serial.print(mAttackLinearParameters.mSlope, DEC);
            // Serial.print("      mEnvelopeOutput = ");   Serial.print(mEnvelopeOutput, DEC);
            // Serial.print("      mFinalAmplitude = ");   Serial.print(mAttackLinearParameters.mFinalAmplitude, DEC);
            // Serial.println();

            break;

        case DECAY_STATE:
            // Serial.println("decay state");
            mDecayLinearParameters.mTimeLength = mDecayKnobValue * mTimeScalar + 1;
            mDecayLinearParameters.mFinalAmplitude = mAttackLinearParameters.mFinalAmplitude * (double)mSustainKnobValue/1023;  //1023 is max 10 bit ADC value  

            if(mTime < mDecayLinearParameters.mTimeLength && mEnvelopeOutput - mDecayLinearParameters.mFinalAmplitude > 1)   // > 1 because it takes to long to get to 0
            {
                mTime++;
                mDecayLinearParameters.mSlope = (mDecayLinearParameters.mFinalAmplitude - mEnvelopeOutput) / (mDecayLinearParameters.mTimeLength - mTime);
                mEnvelopeOutput += mDecayLinearParameters.mSlope * (mTime);
            }
            else
            {
                mEnvelopeOutput = mDecayLinearParameters.mFinalAmplitude;
                mTime = 0;
                mAdsrStatus = SUSTAIN_STATE;
            }
            break;

        case SUSTAIN_STATE:
            // Serial.println("sustain state");
            // this does nothing
            break;

        case RELEASE_STATE:
            // Serial.println("release state");
            // reset t when first in release
            if(mFirstReleaseIteration)
            {
                mFirstReleaseIteration = false;
                mTime = 0;
            }
            mReleaseLinearParameters.mTimeLength = mReleaseKnobValue * mTimeScalar + 1;
            mReleaseLinearParameters.mFinalAmplitude = 0;

            if(mTime < mReleaseLinearParameters.mTimeLength && mEnvelopeOutput > 200)  // > 200 because it takes too long to get to 0 and 200 is practically 0
            {
                mTime++;
                mReleaseLinearParameters.mSlope = (mReleaseLinearParameters.mFinalAmplitude - mEnvelopeOutput) / (mReleaseLinearParameters.mTimeLength - mTime);
                mEnvelopeOutput += mReleaseLinearParameters.mSlope * (mTime);
            }
            else
            {
                mEnvelopeOutput = mReleaseLinearParameters.mFinalAmplitude;
                mTime = 0;
                mAdsrStatus = OFF_STATE;
            }
            break;
        case OFF_STATE:
        default:
            break;
    }
    // Serial.println(mTime, DEC);
    return mEnvelopeOutput;
}