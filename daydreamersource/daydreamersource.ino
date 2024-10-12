/*
*   Tien-Sheng Jonathan Wang
*   4/28/2024
*   
*   Source code for the DayDreamer Synth
*   
*   Programming instructions:
*   - Use Arduino 1.0.6
*   - Burn bootloader on ATMEGA328P for Arduino Uno
*   
*/
#include <StandardCplusplus.h>  // https://github.com/maniacbug/StandardCplusplus/blob/master/vector
#include <vector>

#include "Tlc5940.h"

#include "typedefs.h"
#include "midiutils.h"
#include "multiplexer.h"
#include "envelopegenerator.h"
#include "lfogenerator.h"
#include "pitchgenerator.h"

//TLC pins
#define vcoATlcPin  0
#define vcoBTlcPin  1
#define vcoCTlcPin  2
#define vcoDTlcPin  3
#define vcoETlcPin  4
#define vcoFTlcPin  5

#define vcaATlcPin  6
#define vcaBTlcPin  7
#define vcaCTlcPin  8
#define vcaDTlcPin  9
#define vcaETlcPin  10
#define vcaFTlcPin  11
#define lpfTlcPin   13
#define noiseTlcPin 12

//multiplexer pins
//for switches
#define muxA_S0 6
#define muxA_S1 7 
#define muxA_S2 8
#define muxA_Input 12
//for knobs
#define muxB_S0 2
#define muxB_S1 4
#define muxB_S2 5
#define muxB_Input A0
//for midi channel
#define muxC_S0 A1
#define muxC_S1 A2
#define muxC_S2 A3
#define muxC_Input A4

//debug led
#define debugLedPin A5

#define _NOP() do { __asm__ __volatile__ ("nop"); } while (0)

//lfogenerator object
LfoGenerator gLfoA;
bool gLfoSineOrSquare;

//mod amount
int gModWheelScaled = 0;
// pitch bend
int gPitchBendScaled = 0;

//pitchgenerator objects for vco
PitchGenerator gPitchA;
PitchGenerator gPitchB;
PitchGenerator gPitchC;
PitchGenerator gPitchD;
PitchGenerator gPitchE;
PitchGenerator gPitchF;

uint8_t gVcoAMidiValue = 0;
uint8_t gVcoBMidiValue = 0;
uint8_t gVcoCMidiValue = 0;
uint8_t gVcoDMidiValue = 0;
uint8_t gVcoEMidiValue = 0;
uint8_t gVcoFMidiValue = 0;

unsigned int gVcoAtlcValue = 0;
unsigned int gVcoBtlcValue = 0;
unsigned int gVcoCtlcValue = 0;
unsigned int gVcoDtlcValue = 0;
unsigned int gVcoEtlcValue = 0;
unsigned int gVcoFtlcValue = 0;

//envelopegenerator objects for vca
EnvelopeGenerator gEnvelopeA;
EnvelopeGenerator gEnvelopeB;
EnvelopeGenerator gEnvelopeC;
EnvelopeGenerator gEnvelopeD;
EnvelopeGenerator gEnvelopeE;
EnvelopeGenerator gEnvelopeF;

unsigned int gVcaAtlcValue = 0;
unsigned int gVcaBtlcValue = 0;
unsigned int gVcaCtlcValue = 0;
unsigned int gVcaDtlcValue = 0;
unsigned int gVcaEtlcValue = 0;
unsigned int gVcaFtlcValue = 0;

bool gMidiIsReady = false;
bool gTlcNeedsUpdate = false;

POLYPHONY gPolyphonyStatus = POLY_1;

struct PolyNoteInfo
{
    uint8_t note;
    bool doAttack;
    bool doRelease;
    uint32_t age;
}; 
std::vector<PolyNoteInfo> gOscillatorAssignmentPoly;
std::vector<uint8_t> gNotesPressed;

void addToNotesPressed(uint8_t lNote)
{
    // do nothing for out of range notes.
    if (lNote < lowestMidi || lNote > highestMidi)
    {
        return;
    }

    if(gNotesPressed.empty())
    {
        gNotesPressed.push_back(lNote);
    }
    // add note to the vector if it's not already there
    else
    {
        if(gNotesPressed.at(gNotesPressed.size()-1) != lNote)
        {
            gNotesPressed.push_back(lNote);
        }
    }
}
void removeFromNotesPressed(uint8_t lNote)
{
    for(unsigned int gNotesPressedIndex = 0; gNotesPressedIndex < gNotesPressed.size(); gNotesPressedIndex++)
    {
        if(gNotesPressed.at(gNotesPressedIndex) == lNote)
        {
            gNotesPressed.erase(gNotesPressed.begin() + gNotesPressedIndex);
        }   
    }
}

void incrementAge(std::vector<PolyNoteInfo> &lVec, uint8_t lYoungestIndex)
{
    // this takes in a vector and ages everything by +1. sets whatever our index is to age
    lVec[lYoungestIndex].age = 0;
    for(uint8_t index = 0; index < lVec.size(); index++)
    {
        lVec[index].age += 1;
    }
}

void initOscillatorAssignmentPoly()
{
    gOscillatorAssignmentPoly.clear();
    PolyNoteInfo defaultInfo = {0, false, false, 0};
    for(uint8_t i = 0; i < 6; i++)
    {
        gOscillatorAssignmentPoly.push_back(defaultInfo);
    }
}

void addToAssignmentPoly(std::vector<PolyNoteInfo> &lVec, unsigned int &lNumAssigned, int lNumOsc, unsigned int lNumPressed)
{
    if(lNumAssigned < lNumOsc)
    {
        // assign to a zero in our struct
        for(uint8_t lAssignmentIndex = 0; lAssignmentIndex < lNumOsc; lAssignmentIndex++)
        {
            if(lVec[lAssignmentIndex].note == 0)
            {
                lVec[lAssignmentIndex].note = gMidiState.newNote;
                lVec[lAssignmentIndex].doAttack = true;
                lVec[lAssignmentIndex].doRelease = false;
                incrementAge(lVec, lAssignmentIndex);
                lNumAssigned++;
                break;
            }
        }
    }
    else if(lNumAssigned >= lNumOsc) // this will probably never be greater than, but equal. we do note stealing
    {
        uint8_t lStealIndex = 0;
        uint32_t lOldestAge = 0;
        if(lNumPressed-1 < lNumOsc && lNumPressed-1 > 0)    // if numpressed is less than the number osc. subtract 1 because we add 1 when note is pressed
        {
            // steal the oldest note not pressed
            for(uint8_t lAssignmentIndex = 0; lAssignmentIndex < lNumOsc; lAssignmentIndex++)
            {
                //check that the note is not pressed
                if(std::find(gNotesPressed.begin(), gNotesPressed.end(), lVec[lAssignmentIndex].note) == gNotesPressed.end())
                {
                    //check that this is the oldest note.
                    if(lVec[lAssignmentIndex].age > lOldestAge)
                    {
                        lOldestAge = lVec[lAssignmentIndex].age;
                        lStealIndex = lAssignmentIndex;
                    }
                }
            }
        }
        else    //if numpressed is more than the number osc
        {
            // steal the oldest note
            for(uint8_t lAssignmentIndex = 0; lAssignmentIndex < lNumOsc; lAssignmentIndex++)
            {
                if(lVec[lAssignmentIndex].age > lOldestAge)
                {
                    lOldestAge = lVec[lAssignmentIndex].age;
                    lStealIndex = lAssignmentIndex;
                }
            }
        }
        // Serial.print("stealing: "); Serial.println(lVec[lStealIndex].note, DEC);

        lVec[lStealIndex].note = gMidiState.newNote;
        lVec[lStealIndex].doAttack = true;
        lVec[lStealIndex].doRelease = false;
        incrementAge(lVec, lStealIndex);
    }

    //glide stuff
    //if lNumPressed =< lNumOsc, notes are not full, do not glide
    if(lNumPressed <= lNumOsc)
    {
        gPitchA.mNotesAreFull = false;
        gPitchB.mNotesAreFull = false;
        gPitchC.mNotesAreFull = false;
        gPitchD.mNotesAreFull = false;
        gPitchE.mNotesAreFull = false;
        gPitchF.mNotesAreFull = false;

    }
    else
    {
        gPitchA.mNotesAreFull = true;
        gPitchB.mNotesAreFull = true;
        gPitchC.mNotesAreFull = true;
        gPitchD.mNotesAreFull = true;
        gPitchE.mNotesAreFull = true;
        gPitchF.mNotesAreFull = true;
    }
}

bool removeFromAssignmentPoly(std::vector<PolyNoteInfo> &lVec, int lNumOsc)
{
    bool lSuccessfulRemoval = false;
    // remove from assignment array
    for(uint8_t lAssignmentIndex = 0; lAssignmentIndex < lNumOsc; lAssignmentIndex++)
    {
        if(gOscillatorAssignmentPoly[lAssignmentIndex].note == gMidiState.newNote)
        {
            gOscillatorAssignmentPoly[lAssignmentIndex].doRelease = true;
            lSuccessfulRemoval =  true;
        }
    }
    return lSuccessfulRemoval;
}

void doMidiStates()
{
    if(gMidiState.parseStatus == DONE)
    {
        /****************************************Handle MIDI notes*****************************************/
        if(gMidiState.status == NOTE_ON)
        {
            addToNotesPressed(gMidiState.newNote);
        }
        else if(gMidiState.status == NOTE_OFF)
        {
            removeFromNotesPressed(gMidiState.newNote);
        }
        switch(gPolyphonyStatus)
        {
            /*************************************************Polyphonic Section************************************************************/
            case POLY_3:
            {
                static unsigned int lNumAssigned = 0;
                static unsigned int lNumPressed = 0;
                const unsigned int lNumOsc = 2;

                lNumPressed = gNotesPressed.size();
                if(gMidiState.status == NOTE_ON && (gMidiState.newNote >= lowestMidi && gMidiState.newNote <= highestMidi))
                {
                    addToAssignmentPoly(gOscillatorAssignmentPoly, lNumAssigned, lNumOsc, lNumPressed);
                }
                else if(gMidiState.status == NOTE_OFF)
                {
                    removeFromAssignmentPoly(gOscillatorAssignmentPoly, lNumOsc);
                }

                // assign the oscillators
                for(int lAssignmentIndex = 0; lAssignmentIndex < lNumOsc; lAssignmentIndex++)
                {
                    //update the sustaining State
                    if(!gMidiState.sustainIsOn && gOscillatorAssignmentPoly[lAssignmentIndex].doRelease)
                    {
                        gOscillatorAssignmentPoly[lAssignmentIndex].note = 0;
                    }

                    if(gOscillatorAssignmentPoly[lAssignmentIndex].note != 0)
                    {
                        //turn the note on
                        uint8_t vcoMidiToSet = gOscillatorAssignmentPoly[lAssignmentIndex].note;
                        if(gOscillatorAssignmentPoly[lAssignmentIndex].doAttack)
                        {
                            switch(lAssignmentIndex)
                            {
                                case 0:
                                    gVcoAMidiValue = vcoMidiToSet;
                                    gEnvelopeA.setVelocity(gMidiState.velocity);
                                    gEnvelopeA.setAdsrState(ATTACK_STATE);

                                    gVcoBMidiValue = vcoMidiToSet;
                                    gEnvelopeB.setVelocity(gMidiState.velocity);
                                    gEnvelopeB.setAdsrState(ATTACK_STATE);
                                    
                                    gVcoCMidiValue = vcoMidiToSet;
                                    gEnvelopeC.setVelocity(gMidiState.velocity);
                                    gEnvelopeC.setAdsrState(ATTACK_STATE);
                                    break;

                                case 1:
                                    gVcoEMidiValue = vcoMidiToSet;
                                    gEnvelopeE.setVelocity(gMidiState.velocity);
                                    gEnvelopeE.setAdsrState(ATTACK_STATE);

                                    gVcoFMidiValue = vcoMidiToSet;
                                    gEnvelopeF.setVelocity(gMidiState.velocity);
                                    gEnvelopeF.setAdsrState(ATTACK_STATE);
                                    
                                    gVcoDMidiValue = vcoMidiToSet;
                                    gEnvelopeD.setVelocity(gMidiState.velocity);
                                    gEnvelopeD.setAdsrState(ATTACK_STATE);
                                    break;
                            }
                            gOscillatorAssignmentPoly[lAssignmentIndex].doAttack = false;
                        }
                    }
                    else
                    {
                        // turn the note off
                        if(gOscillatorAssignmentPoly[lAssignmentIndex].doRelease)
                        {
                            switch(lAssignmentIndex)
                            {
                                case 0:
                                    gEnvelopeA.setAdsrState(RELEASE_STATE);
                                    gEnvelopeB.setAdsrState(RELEASE_STATE);
                                    gEnvelopeC.setAdsrState(RELEASE_STATE);
                                    break;
                                case 1:
                                    gEnvelopeD.setAdsrState(RELEASE_STATE);
                                    gEnvelopeE.setAdsrState(RELEASE_STATE);
                                    gEnvelopeF.setAdsrState(RELEASE_STATE);
                                    break;
                            }
                            gOscillatorAssignmentPoly[lAssignmentIndex].doRelease = false;
                            gOscillatorAssignmentPoly[lAssignmentIndex].age = 0;
                            lNumAssigned--;
                        }
                    }
                }
                break;
            }
            case POLY_2:
            {
                static unsigned int lNumAssigned = 0;
                static unsigned int lNumPressed = 0;
                const unsigned int lNumOsc = 3;

                lNumPressed = gNotesPressed.size();
                if(gMidiState.status == NOTE_ON && (gMidiState.newNote >= lowestMidi && gMidiState.newNote <= highestMidi))
                {
                    addToAssignmentPoly(gOscillatorAssignmentPoly, lNumAssigned, lNumOsc, lNumPressed);
                }
                else if(gMidiState.status == NOTE_OFF)
                { 
                    removeFromAssignmentPoly(gOscillatorAssignmentPoly, lNumOsc);
                }

                // assign the oscillators
                for(int lAssignmentIndex = 0; lAssignmentIndex < lNumOsc; lAssignmentIndex++)
                {
                    //update the sustaining State
                    if(!gMidiState.sustainIsOn && gOscillatorAssignmentPoly[lAssignmentIndex].doRelease)
                    {
                        gOscillatorAssignmentPoly[lAssignmentIndex].note = 0;
                    }

                    if(gOscillatorAssignmentPoly[lAssignmentIndex].note != 0)
                    {
                        //turn the note on
                        uint8_t vcoMidiToSet = gOscillatorAssignmentPoly[lAssignmentIndex].note;
                        if(gOscillatorAssignmentPoly[lAssignmentIndex].doAttack)
                        {
                            switch(lAssignmentIndex)
                            {
                                case 0:
                                    gVcoAMidiValue = vcoMidiToSet;
                                    gEnvelopeA.setVelocity(gMidiState.velocity);
                                    gEnvelopeA.setAdsrState(ATTACK_STATE);

                                    gVcoBMidiValue = vcoMidiToSet;
                                    gEnvelopeB.setVelocity(gMidiState.velocity);
                                    gEnvelopeB.setAdsrState(ATTACK_STATE);
                                    break;

                                case 1:
                                    gVcoCMidiValue = vcoMidiToSet;
                                    gEnvelopeC.setVelocity(gMidiState.velocity);
                                    gEnvelopeC.setAdsrState(ATTACK_STATE);

                                    gVcoDMidiValue = vcoMidiToSet;
                                    gEnvelopeD.setVelocity(gMidiState.velocity);
                                    gEnvelopeD.setAdsrState(ATTACK_STATE);
                                    break;

                                case 2:
                                    gVcoEMidiValue = vcoMidiToSet;
                                    gEnvelopeE.setVelocity(gMidiState.velocity);
                                    gEnvelopeE.setAdsrState(ATTACK_STATE);

                                    gVcoFMidiValue = vcoMidiToSet;
                                    gEnvelopeF.setVelocity(gMidiState.velocity);
                                    gEnvelopeF.setAdsrState(ATTACK_STATE);
                                    break;
                            }
                            gOscillatorAssignmentPoly[lAssignmentIndex].doAttack = false;
                        }
                    }
                    else
                    {
                        // turn the note off
                        if(gOscillatorAssignmentPoly[lAssignmentIndex].doRelease)
                        {
                            switch(lAssignmentIndex)
                            {
                                case 0:
                                    gEnvelopeA.setAdsrState(RELEASE_STATE);
                                    gEnvelopeB.setAdsrState(RELEASE_STATE);
                                    break;
                                case 1:
                                    gEnvelopeC.setAdsrState(RELEASE_STATE);
                                    gEnvelopeD.setAdsrState(RELEASE_STATE);
                                    break;
                                case 2:
                                    gEnvelopeE.setAdsrState(RELEASE_STATE);
                                    gEnvelopeF.setAdsrState(RELEASE_STATE);
                                    break;
                            }
                            gOscillatorAssignmentPoly[lAssignmentIndex].doRelease = false;
                            gOscillatorAssignmentPoly[lAssignmentIndex].age = 0;
                            lNumAssigned--;
                        }
                    }
                }

                // check our note pressed vector
                // Serial.println(gNotesPressed.size(), DEC);
                // check our assignment array
                // for(int lAssignmentIndex = 0; lAssignmentIndex < lNumOsc; lAssignmentIndex++)
                // {
                //     Serial.print("note: "); Serial.print(gOscillatorAssignmentPoly[lAssignmentIndex].note, DEC);
                //     Serial.print("      doAttack: "); Serial.print(gOscillatorAssignmentPoly[lAssignmentIndex].doAttack, DEC);
                //     Serial.print("      doRelease: "); Serial.print(gOscillatorAssignmentPoly[lAssignmentIndex].doRelease, DEC);
                //     Serial.print("      age: "); Serial.print(gOscillatorAssignmentPoly[lAssignmentIndex].age, DEC);
                //     Serial.println();
                // }

                break;
            }
            case POLY_1:
            {
                static unsigned int lNumAssigned = 0;
                static unsigned int lNumPressed = 0;
                const unsigned int lNumOsc = 6;

                lNumPressed = gNotesPressed.size();

                if(gMidiState.status == NOTE_ON && (gMidiState.newNote >= lowestMidi && gMidiState.newNote <= highestMidi))
                {
                    addToAssignmentPoly(gOscillatorAssignmentPoly, lNumAssigned, lNumOsc, lNumPressed);
                }
                else if(gMidiState.status == NOTE_OFF)
                {
                    removeFromAssignmentPoly(gOscillatorAssignmentPoly, lNumOsc);
                }
                // assign the oscillators
                for(int lAssignmentIndex = 0; lAssignmentIndex < lNumOsc; lAssignmentIndex++)
                {
                    //update the sustaining State
                    if(!gMidiState.sustainIsOn && gOscillatorAssignmentPoly[lAssignmentIndex].doRelease)
                    {
                        gOscillatorAssignmentPoly[lAssignmentIndex].note = 0;
                    }

                    if(gOscillatorAssignmentPoly[lAssignmentIndex].note != 0)
                    {
                        //turn the note on
                        uint8_t vcoMidiToSet = gOscillatorAssignmentPoly[lAssignmentIndex].note;
                        if(gOscillatorAssignmentPoly[lAssignmentIndex].doAttack)
                        {
                            switch(lAssignmentIndex)
                            {
                                case 0:
                                    gVcoAMidiValue = vcoMidiToSet;
                                    gEnvelopeA.setVelocity(gMidiState.velocity);
                                    gEnvelopeA.setAdsrState(ATTACK_STATE);
                                    break;

                                case 1:
                                    gVcoBMidiValue = vcoMidiToSet;
                                    gEnvelopeB.setVelocity(gMidiState.velocity);
                                    gEnvelopeB.setAdsrState(ATTACK_STATE);
                                    break;

                                case 2:
                                    gVcoCMidiValue = vcoMidiToSet;
                                    gEnvelopeC.setVelocity(gMidiState.velocity);
                                    gEnvelopeC.setAdsrState(ATTACK_STATE);
                                    break;
                                case 3:
                                    gVcoDMidiValue = vcoMidiToSet;
                                    gEnvelopeD.setVelocity(gMidiState.velocity);
                                    gEnvelopeD.setAdsrState(ATTACK_STATE);
                                    break;

                                case 4:
                                    gVcoEMidiValue = vcoMidiToSet;
                                    gEnvelopeE.setVelocity(gMidiState.velocity);
                                    gEnvelopeE.setAdsrState(ATTACK_STATE);
                                    break;

                                case 5:
                                    gVcoFMidiValue = vcoMidiToSet;
                                    gEnvelopeF.setVelocity(gMidiState.velocity);
                                    gEnvelopeF.setAdsrState(ATTACK_STATE);
                                    break;
                            }
                            gOscillatorAssignmentPoly[lAssignmentIndex].doAttack = false;
                        }
                    }
                    else
                    {
                        // turn the note off
                        if(gOscillatorAssignmentPoly[lAssignmentIndex].doRelease)
                        {
                            switch(lAssignmentIndex)
                            {
                                case 0:
                                    gEnvelopeA.setAdsrState(RELEASE_STATE);
                                    break;
                                case 1:
                                    gEnvelopeB.setAdsrState(RELEASE_STATE);
                                    break;
                                case 2:
                                    gEnvelopeC.setAdsrState(RELEASE_STATE);
                                    break;
                                case 3:
                                    gEnvelopeD.setAdsrState(RELEASE_STATE);
                                    break;
                                case 4:
                                    gEnvelopeE.setAdsrState(RELEASE_STATE);
                                    break;
                                case 5:
                                    gEnvelopeF.setAdsrState(RELEASE_STATE);
                                    break;
                            }
                            gOscillatorAssignmentPoly[lAssignmentIndex].doRelease = false;
                            gOscillatorAssignmentPoly[lAssignmentIndex].age = 0;
                            lNumAssigned--;
                        }
                    }
                }

                // check our note pressed vector
                // Serial.println(gNotesPressed.size(), DEC);
                // check our assignment array
                // for(int lAssignmentIndex = 0; lAssignmentIndex < lNumOsc; lAssignmentIndex++)
                // {
                //     Serial.print("note: "); Serial.print(gOscillatorAssignmentPoly[lAssignmentIndex].note, DEC);
                //     Serial.print("      doAttack: "); Serial.print(gOscillatorAssignmentPoly[lAssignmentIndex].doAttack, DEC);
                //     Serial.print("      doRelease: "); Serial.print(gOscillatorAssignmentPoly[lAssignmentIndex].doRelease, DEC);
                //     Serial.print("      age: "); Serial.print(gOscillatorAssignmentPoly[lAssignmentIndex].age, DEC);
                //     Serial.println();
                // }

                break;
            }
            
            /*************************************************Monophonic Section************************************************************/
            case MONO_6:
            {
                static bool doRelease = false;       
                // if there are notes in our vector, play the one at the top.
                if(gNotesPressed.size() == 1 && (gEnvelopeA.mAdsrStatus == OFF_STATE || gEnvelopeA.mAdsrStatus == RELEASE_STATE))
                {
                    doRelease = false;
                    gEnvelopeA.setAdsrState(ATTACK_STATE);
                    gEnvelopeA.setVelocity(gMidiState.velocity);
                    gEnvelopeB.setAdsrState(ATTACK_STATE);
                    gEnvelopeB.setVelocity(gMidiState.velocity);
                    gEnvelopeC.setAdsrState(ATTACK_STATE);
                    gEnvelopeC.setVelocity(gMidiState.velocity);
                    gEnvelopeD.setAdsrState(ATTACK_STATE);
                    gEnvelopeD.setVelocity(gMidiState.velocity);
                    gEnvelopeE.setAdsrState(ATTACK_STATE);
                    gEnvelopeE.setVelocity(gMidiState.velocity);
                    gEnvelopeF.setAdsrState(ATTACK_STATE);
                    gEnvelopeF.setVelocity(gMidiState.velocity);
                }
                if (!gNotesPressed.empty())
                {
                    gVcoAMidiValue = gNotesPressed.at(gNotesPressed.size()-1);
                    gVcoBMidiValue = gVcoAMidiValue;
                    gVcoCMidiValue = gVcoAMidiValue;
                    gVcoDMidiValue = gVcoAMidiValue;
                    gVcoEMidiValue = gVcoAMidiValue;
                    gVcoFMidiValue = gVcoAMidiValue;
                }
                // otherwise, release our VCA
                else
                {
                    gMidiState.sustainIsOn ? doRelease = false : doRelease = true;
                    // glide stuff
                    gPitchA.mNotesAreFull = false;
                    gPitchB.mNotesAreFull = false;
                    gPitchC.mNotesAreFull = false;
                    gPitchD.mNotesAreFull = false;
                    gPitchE.mNotesAreFull = false;
                    gPitchF.mNotesAreFull = false;
                }
                if(doRelease)
                {
                    gEnvelopeA.setAdsrState(RELEASE_STATE);
                    gEnvelopeB.setAdsrState(RELEASE_STATE);
                    gEnvelopeC.setAdsrState(RELEASE_STATE);
                    gEnvelopeD.setAdsrState(RELEASE_STATE);
                    gEnvelopeE.setAdsrState(RELEASE_STATE);
                    gEnvelopeF.setAdsrState(RELEASE_STATE); 
                }

                // glide stuff
                if(gNotesPressed.size() > 1)
                {
                    gPitchA.mNotesAreFull = true;
                    gPitchB.mNotesAreFull = true;
                    gPitchC.mNotesAreFull = true;
                    gPitchD.mNotesAreFull = true;
                    gPitchE.mNotesAreFull = true;
                    gPitchF.mNotesAreFull = true;
                }
                break;
            }
            case MONO_3:
            {
                static bool doRelease = false;       
                // if there are notes in our vector, play the one at the top.
                if(gNotesPressed.size() == 1 && (gEnvelopeA.mAdsrStatus == OFF_STATE || gEnvelopeA.mAdsrStatus == RELEASE_STATE))
                {
                    doRelease = false;
                    gEnvelopeA.setAdsrState(ATTACK_STATE);
                    gEnvelopeA.setVelocity(gMidiState.velocity);
                    gEnvelopeB.setAdsrState(ATTACK_STATE);
                    gEnvelopeB.setVelocity(gMidiState.velocity);
                    gEnvelopeC.setAdsrState(ATTACK_STATE);
                    gEnvelopeC.setVelocity(gMidiState.velocity);
                }
                if (!gNotesPressed.empty())
                {
                    gVcoAMidiValue = gNotesPressed.at(gNotesPressed.size()-1);
                    gVcoBMidiValue = gVcoAMidiValue;
                    gVcoCMidiValue = gVcoAMidiValue;
                }
                // otherwise, release our VCA
                else
                {
                    gMidiState.sustainIsOn ? doRelease = false : doRelease = true;
                    // glide stuff
                    gPitchA.mNotesAreFull = false;
                    gPitchB.mNotesAreFull = false;
                    gPitchC.mNotesAreFull = false;
                }
                if(doRelease)
                {
                    gEnvelopeA.setAdsrState(RELEASE_STATE);
                    gEnvelopeB.setAdsrState(RELEASE_STATE);  
                    gEnvelopeC.setAdsrState(RELEASE_STATE);  
                }

                // glide stuff
                if(gNotesPressed.size() > 1)
                {
                    gPitchA.mNotesAreFull = true;
                    gPitchB.mNotesAreFull = true;
                    gPitchC.mNotesAreFull = true;
                }
                break;
            }
            case MONO_2:
            {
                static bool doRelease = false;       
                // if there are notes in our vector, play the one at the top.
                if(gNotesPressed.size() == 1 && (gEnvelopeA.mAdsrStatus == OFF_STATE || gEnvelopeA.mAdsrStatus == RELEASE_STATE))
                {
                    doRelease = false;
                    gEnvelopeA.setAdsrState(ATTACK_STATE);
                    gEnvelopeA.setVelocity(gMidiState.velocity);
                    gEnvelopeB.setAdsrState(ATTACK_STATE);
                    gEnvelopeB.setVelocity(gMidiState.velocity);
                }
                if (!gNotesPressed.empty())
                {
                    gVcoAMidiValue = gNotesPressed.at(gNotesPressed.size()-1);
                    gVcoBMidiValue = gVcoAMidiValue;
                }
                // otherwise, release our VCA
                else
                {
                    gMidiState.sustainIsOn ? doRelease = false : doRelease = true;
                    // glide stuff
                    gPitchA.mNotesAreFull = false;
                    gPitchB.mNotesAreFull = false;                 
                }
                if(doRelease)
                {
                    gEnvelopeA.setAdsrState(RELEASE_STATE);
                    gEnvelopeB.setAdsrState(RELEASE_STATE);  
                }

                // glide stuff
                if(gNotesPressed.size() > 1)
                {
                    gPitchA.mNotesAreFull = true;
                    gPitchB.mNotesAreFull = true;
                }
                break;
            }
            case MONO_1:
            {
                static bool doRelease = false;
                // static bool doRelease = false;
                if(gNotesPressed.size() == 1 && (gEnvelopeA.mAdsrStatus == OFF_STATE || gEnvelopeA.mAdsrStatus == RELEASE_STATE))
                {
                    doRelease = false;
                    gEnvelopeA.setAdsrState(ATTACK_STATE);
                    gEnvelopeA.setVelocity(gMidiState.velocity);
                }

                if (!gNotesPressed.empty())
                {
                    gVcoAMidiValue = gNotesPressed.at(gNotesPressed.size()-1);
                }
                // otherwise, release our VCA
                else
                {
                    gMidiState.sustainIsOn ? doRelease = false : doRelease = true;
                    // glide stuff
                    gPitchA.mNotesAreFull = false;                    
                }
                if(doRelease)
                {
                    gEnvelopeA.setAdsrState(RELEASE_STATE);
                }
                // Serial.println(gNotesPressed.size(), DEC);

                //glide stuff
                if(gNotesPressed.size() > 1)
                {
                    gPitchA.mNotesAreFull = true;
                }
                break;
            }
        }

        /**************************************Handle Pitch Bend*******************************************/
        if(gMidiState.status == PITCH_BEND)
        {
            long int rawSigned = gMidiState.pitchBendLSB | (gMidiState.pitchBendMSB << 7);    //0 - 8192 - 16383
            gPitchBendScaled = ((rawSigned * pitchBendIncrements) / 8191) - pitchBendIncrements; //-8192 to  8191 ) to -100 +100, whatever pitchBendIncrements is
        }

        /**************************************Handle Mod Wheel*******************************************/
        if(gMidiState.status == CONTROL && gMidiState.controlStatus == MODULATION)
        {
            // store this as in a global
            gModWheelScaled = gMidiState.modulation << 3; //127 * 8 = 1016
            // Serial.println(gModWheelScaled, DEC);
        }

        // this is where we process the midi mesages to see what to do with them in the instrument
        // this would be a good place to handle all the midi statuses
        // pitch bend - LSB - MSB               updates synthstate
        // control  -   mod or sus - amount     updates synthsatte 
        gMidiState.parseStatus = STATUS; 
    }
}


// bool WRITETODEBUG = true;
ISR(TIMER0_COMPA_vect){
    // ISR notes https://electronoobs.com/eng_arduino_tut140.php
    cli();

    // if (!WRITETODEBUG)
    // {
    //     WRITETODEBUG = true;
    // }
    // else{
    //     WRITETODEBUG = false;
    // }
    // digitalWrite(debugLedPin, WRITETODEBUG);
    
    gLfoA.calculateModulation(gLfoSineOrSquare);

    sei();
}
// ISR(BADISR_vect)
// {
//     digitalWrite(debugLedPin, HIGH);
// }

/************************************************************************************************************************************/
void setup()
{
    //Init PolyNoteInfo for polyphonic oscillator assignment struct
    initOscillatorAssignmentPoly();
    
    //switch mux
    pinMode(muxA_S0, OUTPUT);
    pinMode(muxA_S1, OUTPUT);
    pinMode(muxA_S2, OUTPUT);
    pinMode(muxA_Input, INPUT_PULLUP);
    //knob mux
    pinMode(muxB_S0, OUTPUT);
    pinMode(muxB_S1, OUTPUT);
    pinMode(muxB_S2, OUTPUT);
    pinMode(muxB_Input, INPUT);
    //midi chan switch mux
    pinMode(muxC_S0, OUTPUT);
    pinMode(muxC_S1, OUTPUT);
    pinMode(muxC_S2, OUTPUT);
    pinMode(muxC_Input, INPUT_PULLUP);

    pinMode(debugLedPin, OUTPUT);

    //http://www.8bit-era.cz/arduino-timer-interrupts-calculator.html note that the CTC mode register in this code is using the wrong one. its A not B
    //this timer is set up for 60 Hz, 4 ms
    cli();                     //stop interrupts for till we make the settings
    TCCR0A = 0;                // Reset entire TCCR2A to 0 
    TCCR0B = 0;                // Reset entire TCCR2B to 0
    TCNT0 = 0;
    TCCR0A |= (1 << WGM01);    // turn on CTC mode
    OCR0A = 255;                // set compare register A to this value . must be <256
    TCCR0B |= (1 << CS02);  //256 prescalar
    TIMSK0 |= (1 << OCIE0A);   //Set OCIE2A to 1 so we enable compare match A 
    sei();                     //Enable back the interrupts

    Tlc.init();

    //turn off oscillators.. does this prevent boot up scream from happening?
    // set VCOs
    Tlc.set(vcoATlcPin, 0);
    Tlc.set(vcoBTlcPin, 0);
    Tlc.set(vcoCTlcPin, 0);
    Tlc.set(vcoDTlcPin, 0);
    Tlc.set(vcoETlcPin, 0);
    Tlc.set(vcoFTlcPin, 0);
    // set VCAs
    Tlc.set(vcaATlcPin, 0);
    Tlc.set(vcaBTlcPin, 0);
    Tlc.set(vcaCTlcPin, 0);
    Tlc.set(vcaDTlcPin, 0);
    Tlc.set(vcaETlcPin, 0);
    Tlc.set(vcaFTlcPin, 0);

    Serial.begin(31250);
}

void loop()
{
    // midi channel selection
    // do NOT change midi channel while holding down a note! 
    // your note will keep playing because the NOTE_OFF message is on a different channel
    gMidiChannelNumber = \
        !digitalReadFromMux(muxC_S0, muxC_S1, muxC_S2, muxC_Input, SW_MIDICHAN_BIT3_CHAN) + \
        !digitalReadFromMux(muxC_S0, muxC_S1, muxC_S2, muxC_Input, SW_MIDICHAN_BIT2_CHAN) * 2 + \
        !digitalReadFromMux(muxC_S0, muxC_S1, muxC_S2, muxC_Input, SW_MIDICHAN_BIT1_CHAN) * 4 + \
        !digitalReadFromMux(muxC_S0, muxC_S1, muxC_S2, muxC_Input, SW_MIDICHAN_BIT0_CHAN) * 8;

    // do this only if no notes are pressed.
    if((gEnvelopeA.mAdsrStatus == RELEASE_STATE || gEnvelopeA.mAdsrStatus == OFF_STATE) &&
        (gEnvelopeB.mAdsrStatus == RELEASE_STATE || gEnvelopeB.mAdsrStatus == OFF_STATE) &&
        (gEnvelopeC.mAdsrStatus == RELEASE_STATE || gEnvelopeC.mAdsrStatus == OFF_STATE) &&
        (gEnvelopeD.mAdsrStatus == RELEASE_STATE || gEnvelopeD.mAdsrStatus == OFF_STATE) &&
        (gEnvelopeE.mAdsrStatus == RELEASE_STATE || gEnvelopeE.mAdsrStatus == OFF_STATE) &&
        (gEnvelopeF.mAdsrStatus == RELEASE_STATE || gEnvelopeF.mAdsrStatus == OFF_STATE))
    {
        if(!digitalReadFromMux(muxA_S0, muxA_S1, muxA_S2, muxA_Input, SW_MONO_POLY_CHAN))
        {
            // if digitalRead is 0, we are in poly
            if(!digitalReadFromMux(muxA_S0, muxA_S1, muxA_S2, muxA_Input, SW_1OSC_1OSC_CHAN))
            {
                // if the 1 osc switch
                gPolyphonyStatus = POLY_1;
            }
            else if(!digitalReadFromMux(muxA_S0, muxA_S1, muxA_S2, muxA_Input, SW_1OSC_3OSC_CHAN))
            {
                // if the 3 osc switch
                gPolyphonyStatus = POLY_3;
            }
            else if(!digitalReadFromMux(muxA_S0, muxA_S1, muxA_S2, muxA_Input, SW_1OSC_2OSC_CHAN))
            {
                // if the 2 osc switch
                gPolyphonyStatus = POLY_2;
            }
            else
            {
                // all switches are off.
                gPolyphonyStatus = MONO_6;
            }
        }
        else
        {
            //digitalread is 1, we are mono
            if(!digitalReadFromMux(muxA_S0, muxA_S1, muxA_S2, muxA_Input, SW_1OSC_1OSC_CHAN))
            {
                // if the 1 osc switch
                gPolyphonyStatus = MONO_1;
            }
            else if(!digitalReadFromMux(muxA_S0, muxA_S1, muxA_S2, muxA_Input, SW_1OSC_3OSC_CHAN))
            {
                // if the 3 osc switch
                gPolyphonyStatus = MONO_3;
            }
            else if(!digitalReadFromMux(muxA_S0, muxA_S1, muxA_S2, muxA_Input, SW_1OSC_2OSC_CHAN))
            {
                // if the 2 osc switch
                gPolyphonyStatus = MONO_2;
            }
            else
            {
                // all switches are off.
                gPolyphonyStatus = MONO_6;
            }

        }
    }

    checkMidi();
    getMidiStates();
    doMidiStates();

    // get LFO, knob values. I don't think I need to disable interrupts; these values are just being read from by one consumer
    // cli();
    int gKnobLfoFrequency = analogReadFromMux(muxB_S0, muxB_S1, muxB_S2, muxB_Input, KNB_MOD_FRQ_CHAN); 
    int gKnobLfoVcfAmount = analogReadFromMux(muxB_S0, muxB_S1, muxB_S2, muxB_Input, KNB_MOD_VCF_AMT_CHAN);
    int gKnobLfoVcoAmount = analogReadFromMux(muxB_S0, muxB_S1, muxB_S2, muxB_Input, KNB_MOD_VCO_AMT_CHAN);
    int gLfoVcfAmplitudeReading = (!digitalReadFromMux(muxC_S0, muxC_S1, muxC_S2, muxC_Input, SW_MIDI_MODWHEEL_ROUTE_VCF_AMT_CHAN)) ?  max(gKnobLfoVcfAmount, gModWheelScaled): gKnobLfoVcfAmount;
    int gLfoVcoAmplitudeReading = (!digitalReadFromMux(muxC_S0, muxC_S1, muxC_S2, muxC_Input, SW_MIDI_MODWHEEL_ROUTE_VCO_AMT_CHAN)) ?  max(gKnobLfoVcoAmount, gModWheelScaled): gKnobLfoVcoAmount;
    int gLfoRecordLengthReading = (!digitalReadFromMux(muxC_S0, muxC_S1, muxC_S2, muxC_Input, SW_MIDI_MODWHEEL_ROUTE_FREQ_CHAN)) ?  max(gKnobLfoFrequency, gModWheelScaled): gKnobLfoFrequency;
    
    
    gLfoA.setLfoRecordLength(gLfoRecordLengthReading);
    gLfoA.setLfoVcfScalar(gLfoVcfAmplitudeReading);
    gLfoA.setLfoVcoScalar(gLfoVcoAmplitudeReading);
    gLfoSineOrSquare = digitalReadFromMux(muxA_S0, muxA_S1, muxA_S2, muxA_Input, SW_MOD_SINE_SQUARE_CHAN);
    // sei();

    // MOD to Oscillator switch
    int gPitchToSet = gPitchBendScaled + static_cast<int>(gLfoA.mLfoVcoScalarOutput * 100);
    gPitchA.mPitchAndLfoBend = gPitchToSet;
    gPitchB.mPitchAndLfoBend = gPitchToSet;
    gPitchC.mPitchAndLfoBend = gPitchToSet;
    gPitchD.mPitchAndLfoBend = gPitchToSet;
    gPitchE.mPitchAndLfoBend = gPitchToSet;
    gPitchF.mPitchAndLfoBend = gPitchToSet;

    // pitch glide settings
    int mGlideLengthReading = analogReadFromMux(muxB_S0, muxB_S1, muxB_S2, muxB_Input, KNB_GLIDE_CHAN);
    gPitchA.setGlideLength(mGlideLengthReading);
    gPitchB.setGlideLength(mGlideLengthReading);
    gPitchC.setGlideLength(mGlideLengthReading);
    gPitchD.setGlideLength(mGlideLengthReading);
    gPitchE.setGlideLength(mGlideLengthReading);
    gPitchF.setGlideLength(mGlideLengthReading);
    bool lConstantOrLegato = digitalReadFromMux(muxA_S0, muxA_S1, muxA_S2, muxA_Input, SW_LEGATOGLIDE_CHAN);
    gPitchA.setLegatoOnlyGlide(lConstantOrLegato);
    gPitchB.setLegatoOnlyGlide(lConstantOrLegato);
    gPitchC.setLegatoOnlyGlide(lConstantOrLegato);
    gPitchD.setLegatoOnlyGlide(lConstantOrLegato);
    gPitchE.setLegatoOnlyGlide(lConstantOrLegato);
    gPitchF.setLegatoOnlyGlide(lConstantOrLegato);

    int gAttackPotReading = analogReadFromMux(muxB_S0, muxB_S1, muxB_S2, muxB_Input, KNB_ATTACK_CHAN);
    int gDecayPotReading = analogReadFromMux(muxB_S0, muxB_S1, muxB_S2, muxB_Input, KNB_DECAY_CHAN);
    int gSustainPotReading = analogReadFromMux(muxB_S0, muxB_S1, muxB_S2, muxB_Input, KNB_SUSTAIN_CHAN);
    int gReleasePotReading = analogReadFromMux(muxB_S0, muxB_S1, muxB_S2, muxB_Input, KNB_RELEASE_CHAN);
    if(gEnvelopeA.mAdsrStatus != OFF_STATE)
    {
        // lastADSRUpdateTime = currentMillisTime;
        // Serial.println(millis(), DEC);

        gEnvelopeA.setAttackKnob(gAttackPotReading);
        gEnvelopeA.setDecayKnob(gDecayPotReading);
        gEnvelopeA.setSustainKnob(gSustainPotReading);
        gEnvelopeA.setReleaseKnob(gReleasePotReading);
        gVcaAtlcValue = gEnvelopeA.updateOutput();
        gVcoAtlcValue = gPitchA.calculateOutPitch(gVcoAMidiValue, gEnvelopeA.mAdsrStatus);
        gTlcNeedsUpdate = true;
    }
    if(gEnvelopeB.mAdsrStatus != OFF_STATE)
    {
        gEnvelopeB.setAttackKnob(gAttackPotReading);
        gEnvelopeB.setDecayKnob(gDecayPotReading);
        gEnvelopeB.setSustainKnob(gSustainPotReading);
        gEnvelopeB.setReleaseKnob(gReleasePotReading);
        gVcaBtlcValue = gEnvelopeB.updateOutput();
        gVcoBtlcValue = gPitchB.calculateOutPitch(gVcoBMidiValue, gEnvelopeB.mAdsrStatus);
        gTlcNeedsUpdate = true;
    }
    if(gEnvelopeC.mAdsrStatus != OFF_STATE)
    {
        gEnvelopeC.setAttackKnob(gAttackPotReading);
        gEnvelopeC.setDecayKnob(gDecayPotReading);
        gEnvelopeC.setSustainKnob(gSustainPotReading);
        gEnvelopeC.setReleaseKnob(gReleasePotReading);
        gVcaCtlcValue = gEnvelopeC.updateOutput();
        gVcoCtlcValue = gPitchC.calculateOutPitch(gVcoCMidiValue, gEnvelopeC.mAdsrStatus);
        gTlcNeedsUpdate = true;
    }
    if(gEnvelopeD.mAdsrStatus != OFF_STATE)
    {
        gEnvelopeD.setAttackKnob(gAttackPotReading);
        gEnvelopeD.setDecayKnob(gDecayPotReading);
        gEnvelopeD.setSustainKnob(gSustainPotReading);
        gEnvelopeD.setReleaseKnob(gReleasePotReading);
        gVcaDtlcValue = gEnvelopeD.updateOutput();
        gVcoDtlcValue = gPitchD.calculateOutPitch(gVcoDMidiValue, gEnvelopeD.mAdsrStatus);
        gTlcNeedsUpdate = true;
    }
    if(gEnvelopeE.mAdsrStatus != OFF_STATE)
    {
        gEnvelopeE.setAttackKnob(gAttackPotReading);
        gEnvelopeE.setDecayKnob(gDecayPotReading);
        gEnvelopeE.setSustainKnob(gSustainPotReading);
        gEnvelopeE.setReleaseKnob(gReleasePotReading);
        gVcaEtlcValue = gEnvelopeE.updateOutput();
        gVcoEtlcValue = gPitchE.calculateOutPitch(gVcoEMidiValue, gEnvelopeE.mAdsrStatus);
        gTlcNeedsUpdate = true;
    }
    if(gEnvelopeF.mAdsrStatus != OFF_STATE)
    {
        gEnvelopeF.setAttackKnob(gAttackPotReading);
        gEnvelopeF.setDecayKnob(gDecayPotReading);
        gEnvelopeF.setSustainKnob(gSustainPotReading);
        gEnvelopeF.setReleaseKnob(gReleasePotReading);
        gVcaFtlcValue = gEnvelopeF.updateOutput();
        gVcoFtlcValue = gPitchF.calculateOutPitch(gVcoFMidiValue, gEnvelopeF.mAdsrStatus);
        gTlcNeedsUpdate = true;
    }
    digitalWrite(debugLedPin, LOW);
    if(gTlcNeedsUpdate)
    {
        // set VCOs
        Tlc.set(vcoATlcPin, gVcoAtlcValue);
        Tlc.set(vcoBTlcPin, gVcoBtlcValue);
        Tlc.set(vcoCTlcPin, gVcoCtlcValue);
        Tlc.set(vcoDTlcPin, gVcoDtlcValue);
        Tlc.set(vcoETlcPin, gVcoEtlcValue);
        Tlc.set(vcoFTlcPin, gVcoFtlcValue);

        // set VCAs
        Tlc.set(vcaATlcPin, gVcaAtlcValue);
        Tlc.set(vcaBTlcPin, gVcaBtlcValue);
        Tlc.set(vcaCTlcPin, gVcaCtlcValue);
        Tlc.set(vcaDTlcPin, gVcaDtlcValue);
        Tlc.set(vcaETlcPin, gVcaEtlcValue);
        Tlc.set(vcaFTlcPin, gVcaFtlcValue);

        uint16_t maxVcaValues = max(gVcaAtlcValue, max(gVcaBtlcValue,max(gVcaCtlcValue,max(gVcaDtlcValue,max(gVcaEtlcValue,gVcaFtlcValue)))));
        // set white noise VCA
        Tlc.set(noiseTlcPin, maxVcaValues);
        
        // set VCF
        uint16_t vcfValueToSet = (maxVcaValues - maxVcaValues * (gLfoA.mLfoVcfScalarOutput + gLfoA.mLfoVcfScalar) / 2);
        Tlc.set(lpfTlcPin, vcfValueToSet);

        Tlc.update();
        digitalWrite(debugLedPin, HIGH);
        gTlcNeedsUpdate = false;
    }
}
