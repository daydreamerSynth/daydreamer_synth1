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
    t = 0;
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
            (t == 0) ? mEnvelopeOutput = 300 : mEnvelopeOutput; //300 is practically 0. start at 300 to save start time from silence
            mAttackLinearParameters.mTimeLength = mAttackKnobValue * mTimeScalar + 1;
            mAttackLinearParameters.mFinalAmplitude = mVelocity * mTlcScalar;    
            
            if(t < mAttackLinearParameters.mTimeLength && mAttackLinearParameters.mFinalAmplitude - mEnvelopeOutput > 1) // > 1 because it takes to long to get to 0
            {
                t++;
                mAttackLinearParameters.mSlope = (mAttackLinearParameters.mFinalAmplitude - mEnvelopeOutput) / (mAttackLinearParameters.mTimeLength - t);
                mEnvelopeOutput += mAttackLinearParameters.mSlope * (t);
            }
            else
            {
                mEnvelopeOutput = mAttackLinearParameters.mFinalAmplitude;
                t = 0;
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

            if(t < mDecayLinearParameters.mTimeLength && mEnvelopeOutput - mDecayLinearParameters.mFinalAmplitude > 1)   // > 1 because it takes to long to get to 0
            {
                t++;
                mDecayLinearParameters.mSlope = (mDecayLinearParameters.mFinalAmplitude - mEnvelopeOutput) / (mDecayLinearParameters.mTimeLength - t);
                mEnvelopeOutput += mDecayLinearParameters.mSlope * (t);
            }
            else
            {
                mEnvelopeOutput = mDecayLinearParameters.mFinalAmplitude;
                t = 0;
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
                t = 0;
            }
            mReleaseLinearParameters.mTimeLength = mReleaseKnobValue * mTimeScalar + 1;
            mReleaseLinearParameters.mFinalAmplitude = 0;

            if(t < mReleaseLinearParameters.mTimeLength && mEnvelopeOutput > 200)  // > 200 because it takes too long to get to 0 and 200 is practically 0
            {
                t++;
                mReleaseLinearParameters.mSlope = (mReleaseLinearParameters.mFinalAmplitude - mEnvelopeOutput) / (mReleaseLinearParameters.mTimeLength - t);
                mEnvelopeOutput += mReleaseLinearParameters.mSlope * (t);
            }
            else
            {
                mEnvelopeOutput = mReleaseLinearParameters.mFinalAmplitude;
                t = 0;
                mAdsrStatus = OFF_STATE;
            }
            break;
        case OFF_STATE:
        default:
            break;
    }
    // Serial.println(t, DEC);
    return mEnvelopeOutput;
}