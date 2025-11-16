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

/*
 *  LfoGenerator class
 *  This class can be used for each LFO
 *  For this project I will only have one LfoGenerator Object.
 *  
 *  update the knob values of the gLfoAmtPot and lfoFreqPot
 *  this will give you the next scalar (-1 to 1) of LFO
 *  Calculation should happen within a timer interrupt.
 *  getting potentiometer values should happen outside of timer interrupts
 *  
 *  I this project, I am using a ~60Hz timer on TIMER0. I will change recordlength accordingly to this samplerate
*/

class LfoGenerator
{
    public:
    
    LfoGenerator();
    ~LfoGenerator();

    double mLfoVcfScalar;
    double mLfoVcoScalar;
    double mLfoRecordLength;

    double mLfoVcfScalarOutput; //-1 to 1 value output which is calculated
    double mLfoVcoScalarOutput;

    void calculateModulation(bool lSineOrSquare);
    double calculateSine(double lt_sine);
    double calculateSquare(double lt_square);
    void setLfoRecordLength(int lReading);
    void setLfoVcfScalar(int lReading);
    void setLfoVcoScalar(int lReading);
};

int calculateLogFromLinear(int lLinearValue);