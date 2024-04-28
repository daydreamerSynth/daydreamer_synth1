#include "lfogenerator.h"
#include <HardwareSerial.h>
#include <math.h>

LfoGenerator::LfoGenerator()
{
    mLfoVcoScalar = 0;
    mLfoVcfScalar = 0;
    mLfoRecordLength = 0;

    mLfoVcfScalarOutput = 0;
    mLfoVcoScalarOutput = 0;
}

LfoGenerator::~LfoGenerator(){}

void LfoGenerator::setLfoVcfScalar(int lReading)
{
    mLfoVcfScalar = static_cast<double>(lReading) / 1023;
}

void LfoGenerator::setLfoVcoScalar(int lReading)
{
    mLfoVcoScalar = static_cast<double>(lReading) / 1023;
}

void LfoGenerator::setLfoRecordLength(int lReading)
{
    mLfoRecordLength = ((1023 - lReading) + 8);
}

void LfoGenerator::calculateModulation(bool lSineOrSquare)
{
    static double tNum = 0;
    if(tNum < mLfoRecordLength)
    {
        tNum++;
    }
    else
    {
        tNum = 0;
    }
    double t = tNum/mLfoRecordLength;
    
    // t goes between 0 and 1.0

    if(lSineOrSquare)
    {
        mLfoVcfScalarOutput = mLfoVcfScalar * calculateSine(t);
    }
    else
    {
        mLfoVcfScalarOutput = mLfoVcfScalar * calculateSquare(t);
    }
}

double LfoGenerator::calculateSine(double lt_sine)
{
    return sin(2 * 3.14 * lt_sine);
}
double LfoGenerator::calculateSquare(double lt_square)
{
    if (lt_square <= 0.5)
    {
        return 1.0;
    }
    else
    {
        return -1.0;
    }
}