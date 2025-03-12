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

#ifndef TYPEDEFS_H
#define TYPEDEFS_H
//1 oscillator mono, 2 oscillator mono, 3 oscillator mono, 6 oscillator mono, 1 oscillator poly (6 note), 2 oscillator poly (3 note), 3 oscillator poly (2 note)
typedef enum {MONO_1, MONO_2, MONO_3, MONO_6, POLY_1, POLY_2, POLY_3} POLYPHONY;  

typedef enum {STATUS, DATA1, DATA2, DONE} PARSE_STATUSES;
typedef enum {NOTE_ON, NOTE_OFF, PITCH_BEND, CONTROL, UNDEFINED_STATUS} STATUSES;
typedef enum {MODULATION, SUSTAIN_PEDAL, UNDEFINED_CONTROL} CONTROL_STATUSES;

typedef enum {ATTACK_STATE, DECAY_STATE, SUSTAIN_STATE, RELEASE_STATE, OFF_STATE} ADSR_STATUSES;

#endif // TYPEDEFS_H