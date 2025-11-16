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
    // Analog read max is 1023. lReading is between 0 and 1023,, so that manes this  can be length 8 (knob up) to 1031 (knob down)
    mLfoRecordLength = ((1023 - lReading) + 8);
}

void LfoGenerator::calculateModulation(bool lSineOrSquare)
{
    static double tNum = 0;
    if(tNum < mLfoRecordLength)
    // if time is less than the set record, increment
    {
        tNum++;
    }
    else
    {
    //reset time to 0
        tNum = 0;
    }
    double t = tNum/mLfoRecordLength;
    //turn the time into a percentage
    
    // t goes between 0 and 1.0
    // so basically the LFO speed knob turns up the speed of time passing through a periodic function

    if(lSineOrSquare)
    {
        mLfoVcfScalarOutput = mLfoVcfScalar * calculateSine(t);
        mLfoVcoScalarOutput = mLfoVcoScalar * calculateSine(t);
    }
    else
    {
        mLfoVcfScalarOutput = mLfoVcfScalar * calculateSquare(t);
        mLfoVcoScalarOutput = mLfoVcoScalar * calculateSquare(t);
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

int calculateLogFromLinear(int lLinearValue)
{
    // Requires that lLinearValue be between 0 and 1023

    // Add 1 to the value to prevent log(0) and cast to a float
    double lLinearFloat = (double)lLinearValue + 1.0;
    // 3. Calculate the logarithmic value and normalize it
    double lLogValue = log(lLinearFloat);
    // 4. Normalize and scale the output to the desired range (0-1023)
    // Max log value is log(1024)
    double lLogCurveValue = (lLogValue / log(1024.0)) * 1023.0;
    // Convert the result back to an integer for use
    return (int)lLogCurveValue;
}