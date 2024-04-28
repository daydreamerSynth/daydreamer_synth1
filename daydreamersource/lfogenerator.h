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