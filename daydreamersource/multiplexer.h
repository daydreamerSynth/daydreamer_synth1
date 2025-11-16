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
	functions and variables for handling multiplexed inputs
*/

#ifndef MULTIPLEXER_H
#define MULTIPLEXER_H

#define SW_MIDICHAN_BIT0_CHAN 0
#define SW_MIDICHAN_BIT1_CHAN 1
#define SW_MIDICHAN_BIT2_CHAN 2
#define SW_MIDICHAN_BIT3_CHAN 3	
#define SW_MIDI_MODWHEEL_ROUTE_FREQ_CHAN 5
#define SW_MIDI_MODWHEEL_ROUTE_VCF_AMT_CHAN 6
#define SW_MIDI_MODWHEEL_ROUTE_VCO_AMT_CHAN 7
//4 on muxC is unused


#define SW_MONO_POLY_CHAN 0
#define SW_1OSC_3OSC_CHAN 1
#define SW_1OSC_2OSC_CHAN 2
#define SW_1OSC_1OSC_CHAN 3
// 4 and 5 on muxA are unused
#define SW_MOD_SINE_SQUARE_CHAN 6
#define SW_LEGATOGLIDE_CHAN 7

#define KNB_MOD_VCO_AMT_CHAN 7
#define KNB_MOD_VCF_AMT_CHAN 6
#define KNB_MOD_FRQ_CHAN 5
#define KNB_ATTACK_CHAN 0
#define KNB_DECAY_CHAN 1
#define KNB_SUSTAIN_CHAN 2
#define KNB_RELEASE_CHAN 3
#define KNB_GLIDE_CHAN 4

int analogReadFromMux(uint8_t lS0Pin, uint8_t lS1Pin, uint8_t lS2Pin, uint8_t lCommonInPin, uint8_t lChannel)
{
    // shift and take the last bit
	digitalWrite(lS0Pin, lChannel & 1);
	digitalWrite(lS1Pin, (lChannel >> 1) & 1);
	digitalWrite(lS2Pin, (lChannel >> 2) & 1);
    return analogRead(lCommonInPin);
	//returns number between 0 and 1023
}

bool digitalReadFromMux(uint8_t lS0Pin, uint8_t lS1Pin, uint8_t lS2Pin, uint8_t lCommonInPin, uint8_t lChannel)
{
    // shift and take the last bit
	digitalWrite(lS0Pin, lChannel & 1);
	digitalWrite(lS1Pin, (lChannel >> 1) & 1);
	digitalWrite(lS2Pin, (lChannel >> 2) & 1);
    return digitalRead(lCommonInPin);
}

#endif