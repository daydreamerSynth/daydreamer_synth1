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
https://www.songstuff.com/recording/article/midi_message_format/
https://github.com/TSJWang/Attinyx5_midi_cv_converter/blob/main/attiny85_MIDI_CV_converter/attiny85_MIDI_CV_converter.ino
https://www.instructables.com/Send-and-Receive-MIDI-with-Arduino/
https://www.somascape.org/midi/tech/spec.html

The structure of a midi message:
0   1   2   3   4   5   6   7       8   9   10  11  12  13  14  15      16  17  18  19  20  21  22  23
Status          | MIDI Channel      Data1 (newNote number)                 Data2 (Velocity    (1 - 127(7F)

Status:
    Note on:    0x9n
    Note off:   0x8n
    pitch bend: 0xEn
    control message: 0xBn

    channel number (n) (0-F (15)

Data1:
    pitch bend MSB (0 - 7F)
    
    control message:
        modulation (01)
        sustain pedal (40)

    Note Numbers:
        0 (C)
        78 (highest C)
        3C (middle C)

Data2:
    pitch bend LSB (0 - 7F)
    control message:
        modulation (0 - 7F)
        sustain pedal (0 or 7F)
    Velocity:
        1 - 7F for newNote on
        40 for newNote off
*/

#include <stdint.h>
#include <HardwareSerial.h>
#include "queue.h"
#include "typedefs.h"
#include "multiplexer.h"

#ifndef MIDIUTILS_H
#define MIDIUTILS_H

//LSB of status is the channel
#define STATUS_NOTE_OFF         0x80
#define STATUS_NOTE_ON          0x90
#define STATUS_POLYAFTERTOUCH   0xA0
#define STATUS_CONTROL          0xB0
#define STATUS_PROGCHANGE       0xC0
#define STATUS_AFTERTOUCH       0xD0
#define STATUS_PITCH            0xE0
//control messages
#define CONTROL_MOD     0x01
#define CONTROL_SUS     0x40

Queue gMidiBuffer(60);   //bigger is not better! 3 bytes per message. this handles 20 messages

uint8_t gMidiChannelNumber = 0;

struct
{
    PARSE_STATUSES parseStatus;
    STATUSES status;
    //data1
    uint8_t newNote;
    uint8_t pitchBendMSB;
    CONTROL_STATUSES controlStatus;

    //data2
    uint8_t velocity;
    uint8_t pitchBendLSB;
    uint8_t modulation;
    bool sustainIsOn;
} gMidiState = {STATUS, UNDEFINED_STATUS, 0x3C, 0x00, UNDEFINED_CONTROL, 1, 0x00, 0, false};

// Helper functions to handle "Note On with Velocity 0"
bool isNoteOn()
{
    return (gMidiState.status == NOTE_ON && gMidiState.velocity > 0);
}
bool isNoteOff()
{
    return (gMidiState.status == NOTE_OFF || (gMidiState.status == NOTE_ON && gMidiState.velocity == 0));
}

/********************************************************************************************************
checkMidi
reads the serial port for new midi messages
puts them into gMidiBuffer
********************************************************************************************************/
void checkMidi()
{
    while(Serial.available())
    {
        gMidiBuffer.push(Serial.read());
    }
}

/********************************************************************************************************
getMidiStates()
reads gMidiBuffer
turns them into information stored in gMidiState.
********************************************************************************************************/
void getMidiStates()
{
    // Serial.println(gMidiBuffer.size(), DEC);  // good for seeing parsing latency
    // delay(300);
    // Optimization: Process all available bytes until a message is complete or buffer is empty
    while(!gMidiBuffer.isEmpty() && gMidiState.parseStatus != DONE)
    {
        uint8_t lMidibyte = gMidiBuffer.pop();
        // Serial.println(lMidibyte, HEX);
        
        // Ignore RealTime messages (0xF8-0xFF) to prevent them from being interpreted as data
        if (lMidibyte >= 0xF8) 
        {
            return;
        }

        // If a Status byte arrives while expecting Data, reset to STATUS state 
        if ((lMidibyte & 0x80) &&                // if the message is a status byte (0b1000 0000 is 0x80)
            gMidiState.parseStatus != STATUS)    //and we arent already in status state
        {
            gMidiState.parseStatus = STATUS;
        }

        switch(gMidiState.parseStatus)
        {
            case STATUS:
                //check that we are on the right midi channel
                //System messages (0xF0-0xFF) do not have a channel, so we only check for < 0xF0
                //Only check channel if it is a status byte (>= 0x80). Data bytes must pass through.
                if(lMidibyte >= 0x80 && lMidibyte < 0xF0 && 
                    static_cast<uint8_t>(lMidibyte & 0x0F) != gMidiChannelNumber)
                {
                    return;
                }

                // get status
                switch(lMidibyte>>4)
                {
                    case (STATUS_NOTE_ON>>4):
                        gMidiState.status = NOTE_ON;
                        gMidiState.parseStatus = DATA1; // move onto the next parsing state
                        break;
                    case (STATUS_NOTE_OFF>>4):
                        gMidiState.status = NOTE_OFF;
                        gMidiState.parseStatus = DATA1;
                        break;
                    case (STATUS_PITCH>>4):
                        gMidiState.status = PITCH_BEND;
                        gMidiState.parseStatus = DATA1;
                        break;
                    case (STATUS_CONTROL>>4):
                        gMidiState.status = CONTROL;
                        gMidiState.parseStatus = DATA1;
                        break;
                    case (STATUS_POLYAFTERTOUCH>>4):
                    case (STATUS_AFTERTOUCH>>4):
                    case (STATUS_PROGCHANGE>>4):
                        gMidiState.status = UNDEFINED_STATUS;
                        gMidiState.parseStatus = DATA1;
                        break;
                    
                    default:
                        // check to see if we recieved a non status byte
                        if (lMidibyte>>7 == 0)
                        {
                            // if so, the status stays the same. 
                            gMidiState.parseStatus = DATA1;
                            // Running Status: This byte is Data1. Process it immediately.
                            switch(gMidiState.status)
                            {
                                case NOTE_ON:
                                case NOTE_OFF:
                                    gMidiState.newNote = lMidibyte;
                                    break;
                                case PITCH_BEND:
                                    gMidiState.pitchBendLSB = lMidibyte;
                                    break;
                                case CONTROL:
                                    switch(lMidibyte)
                                    {
                                        case CONTROL_MOD:
                                            gMidiState.controlStatus = MODULATION;
                                            break;
                                        case CONTROL_SUS:
                                            gMidiState.controlStatus = SUSTAIN_PEDAL;
                                            break;
                                        default:
                                            gMidiState.controlStatus = UNDEFINED_CONTROL;
                                            break;
                                    }
                                    break;
                                default:
                                    break;
                            }
                            gMidiState.parseStatus = DATA2;
                        }
                        else
                        {
                            gMidiState.status = UNDEFINED_STATUS;
                            gMidiState.parseStatus = STATUS;    // do not move onto next parsing state, parse first byte again
                        }
                        break;
                }
                break;
            
            // get data1
            case DATA1:
                switch(gMidiState.status)
                {
                    case NOTE_ON:
                    case NOTE_OFF:
                        gMidiState.newNote = lMidibyte;
                        break;
                    case PITCH_BEND:
                        gMidiState.pitchBendLSB = lMidibyte;
                        break;
                    case CONTROL:
                        switch(lMidibyte)
                        {
                            case CONTROL_MOD:
                                gMidiState.controlStatus = MODULATION;
                                break;
                            case CONTROL_SUS:
                                gMidiState.controlStatus = SUSTAIN_PEDAL;
                                break;
                            default:
                                gMidiState.controlStatus = UNDEFINED_CONTROL;
                                break;
                        }
                        break;
                    default:
                        break;
                }
                gMidiState.parseStatus = DATA2;
                break;

            //get data2
            case DATA2:
                switch(gMidiState.status)
                {
                    case NOTE_ON:
                        gMidiState.velocity = lMidibyte;
                        break;
                    case PITCH_BEND:
                        gMidiState.pitchBendMSB = lMidibyte;
                        break;
                    case CONTROL:
                        switch(gMidiState.controlStatus)
                        {
                            case SUSTAIN_PEDAL:
                                gMidiState.sustainIsOn = (lMidibyte > 0x3F);
                                break;
                            case MODULATION:
                                gMidiState.modulation = lMidibyte;
                                break;
                            default:
                                break;
                        }
                        break;
                    case NOTE_OFF:
                        // no need to record velocity for a note off.
                    default:
                        break;
                }
                gMidiState.parseStatus = DONE;
                break;

            default:
                break;
        }
    }
}

#endif