/*
https://www.songstuff.com/recording/article/midi_message_format/
https://github.com/TSJWang/Attinyx5_midi_cv_converter/blob/main/attiny85_MIDI_CV_converter/attiny85_MIDI_CV_converter.ino
https://www.instructables.com/Send-and-Receive-MIDI-with-Arduino/

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
#define STATUS_NOTE_ON  0x90
#define STATUS_NOTE_OFF 0x80
#define STATUS_PITCH    0xE0
#define STATUS_CONTROL  0xB0
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

/********************************************************************************************************
checkMidi
reads the serial port for new midi messages
puts them into gMidiBuffer
********************************************************************************************************/
void checkMidi()
{
    do
    {
        if(Serial.available())
        {
            gMidiBuffer.push(Serial.read());
        }
    }
    while (Serial.available() > 1); //when at least 3 bytes available (one message)   
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
    if(!gMidiBuffer.isEmpty())
    {
        uint8_t lMidibyte = gMidiBuffer.pop();
        // Serial.println(lMidibyte, HEX);
        
        switch(gMidiState.parseStatus)
        {
            case STATUS:
                //check that we are on the right midi channel
                if(static_cast<uint8_t>(lMidibyte & 15) != gMidiChannelNumber)
                {
                    return;
                }

                // get status
                switch(lMidibyte>>4)
                {
                    case (STATUS_NOTE_ON>>4):
                        gMidiState.status = NOTE_ON;
                        gMidiState.parseStatus = DATA1;
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
                    default:
                        gMidiState.status = UNDEFINED_STATUS;
                        gMidiState.parseStatus = STATUS;
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
                    case NOTE_OFF:
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