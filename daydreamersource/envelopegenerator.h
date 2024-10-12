/*  EnvelopeGenerator Class
 *  This class can be used for each VCA
 *  each VCA can have one of these objects. This will help manage the ADSR values
 *  
 *  update the knob values of the ADSR knobs
 *  this will give you the next value that the envelope generator DAC should generate
 */

#include "typedefs.h"

class EnvelopeGenerator
{
    public:
    EnvelopeGenerator();
    ~EnvelopeGenerator();
    //struct for each ADR 
    typedef struct LinearParameters
    {
        double mSlope;
        unsigned int mFinalAmplitude;
        unsigned long mTimeLength;
    } LinearParameters;
    LinearParameters mAttackLinearParameters;
    LinearParameters mDecayLinearParameters;
    LinearParameters mReleaseLinearParameters;

    // globals
    unsigned int mTlcScalar;        // 127 * 32 = 4064
    unsigned long mTimeScalar;    // increase this value to get longer A,D,R range
    bool mFirstReleaseIteration;
    unsigned long mTime;
    double mEnvelopeOutput;
    ADSR_STATUSES mAdsrStatus;

    // input (knob) values
    int mAttackKnobValue;
    int mDecayKnobValue;
    int mSustainKnobValue;
    int mReleaseKnobValue;
    int mVelocity;

    //functions
    unsigned int updateOutput();

    void setAdsrState(ADSR_STATUSES lNewStatus);
    void setAttackKnob(int lReading);
    void setDecayKnob(int lReading);
    void setSustainKnob(int lReading);
    void setReleaseKnob(int lReading);
    void setVelocity(int lMessageByte);
};