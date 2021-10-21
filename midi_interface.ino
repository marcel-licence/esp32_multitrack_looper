/*
 * The GNU GENERAL PUBLIC LICENSE (GNU GPLv3)
 *
 * Copyright (c) 2021 Marcel Licence
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Dieses Programm ist Freie Software: Sie können es unter den Bedingungen
 * der GNU General Public License, wie von der Free Software Foundation,
 * Version 3 der Lizenz oder (nach Ihrer Wahl) jeder neueren
 * veröffentlichten Version, weiter verteilen und/oder modifizieren.
 *
 * Dieses Programm wird in der Hoffnung bereitgestellt, dass es nützlich sein wird, jedoch
 * OHNE JEDE GEWÄHR,; sogar ohne die implizite
 * Gewähr der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
 * Siehe die GNU General Public License für weitere Einzelheiten.
 *
 * Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
 * Programm erhalten haben. Wenn nicht, siehe <https://www.gnu.org/licenses/>.
 */

/*
 * a simple implementation to use midi
 *
 * Author: Marcel Licence
 */


/*
 * look for midi interface using 1N136
 * to convert the MIDI din signal to
 * a uart compatible signal
 */
#if 0
#define RXD2 16 /* U2RRXD */
#define TXD2 17
#else
#define RXD2 22
#define TXD2 21
#endif


/*
 * structure is used to build the mapping table
 */
struct midiControllerMapping
{
    uint8_t channel;
    uint8_t data1;
    const char *desc;
    void(*callback_mid)(uint8_t ch, uint8_t data1, uint8_t data2);
    void(*callback_val)(uint8_t userdata, float value);
    uint8_t user_data;
};

/*
 * this mapping is used for the edirol pcr-800
 * this should be changed when using another controller
 */
struct midiControllerMapping edirolMapping[] =
{
    { 0x8, 0x52, "back", NULL, Loop_ResetToStart, 0},
    { 0x8, 0x52, "back", NULL, Click_Reset, 0},
    { 0xD, 0x52, "stop", NULL, Loop_Stop, 0},
    { 0xe, 0x52, "start", NULL, Loop_PlayNormal, 0},
    { 0xe, 0x52, "start", NULL, Loop_StartAll, 0},
    { 0xa, 0x52, "rec", NULL, Loop_SetLength, 0},

    /* upper row of buttons */
    { 0x0, 0x50, "A1", NULL, Loop_SelectTrack, 0},
    { 0x1, 0x50, "A2", NULL, Loop_SelectTrack, 1},
    { 0x2, 0x50, "A3", NULL, Loop_SelectTrack, 2},
    { 0x3, 0x50, "A4", NULL, Loop_SelectTrack, 3},

    { 0x4, 0x50, "A5", NULL, Click_ToggleOnOff, 0},
    { 0x5, 0x50, "A6", NULL, Loop_JumpPosQuarter, 1},
    { 0x6, 0x50, "A7", NULL, Loop_JumpPosQuarter, 2},
    { 0x7, 0x50, "A8", NULL, Loop_JumpPosQuarter, 3},
    { 0x5, 0x50, "A6", NULL, Click_JumpPosQuarter, 1},
    { 0x6, 0x50, "A7", NULL, Click_JumpPosQuarter, 2},
    { 0x7, 0x50, "A8", NULL, Click_JumpPosQuarter, 3},

    { 0x0, 0x53, "A9", NULL, Click_OnOff, 0},

    /* lower row of buttons */
    { 0x0, 0x51, "B1", NULL, Loop_StopChannel, 0},
    { 0x1, 0x51, "B2", NULL, Loop_StopChannel, 1},
    { 0x2, 0x51, "B3", NULL, Loop_StopChannel, 2},
    { 0x3, 0x51, "B4", NULL, Loop_StopChannel, 3},

    { 0x4, 0x51, "B5", NULL, Loop_EraseTrack, 0},
    { 0x5, 0x51, "B6", NULL, Loop_EraseTrack, 1},
    { 0x6, 0x51, "B7", NULL, Loop_EraseTrack, 2},
    { 0x7, 0x51, "B8", NULL, Loop_EraseTrack, 3},

    { 0x1, 0x53, "B9", NULL, MTLooper_ToggleSource, 0},

    /* pedal */
    { 0x0, 0x0b, "VolumePedal", NULL, NULL, 0},

    { 0x0, 0x11, "S1", NULL, Loop_SetChannelGainOut, 0},
    { 0x1, 0x11, "S2", NULL, Loop_SetChannelGainOut, 1},
    { 0x2, 0x11, "S3", NULL, Loop_SetChannelGainOut, 2},
    { 0x3, 0x11, "S4", NULL, Loop_SetChannelGainOut, 3},

    { 0x4, 0x11, "S5", NULL, NULL, 0},
    { 0x5, 0x11, "S6", NULL, NULL, 0},
    { 0x6, 0x11, "S7", NULL, NULL, 0},
    { 0x7, 0x11, "S8", NULL, NULL, 0},

    /* rotary */
    { 0x0, 0x10, "R1", NULL, Loop_SetChannelPan, 0},
    { 0x1, 0x10, "R2", NULL, Loop_SetChannelPan, 1},
    { 0x2, 0x10, "R3", NULL, Loop_SetChannelPan, 2},
    { 0x3, 0x10, "R4", NULL, Loop_SetChannelPan, 3},

    { 0x4, 0x10, "R5", NULL, NULL, 0},
    { 0x5, 0x10, "R6", NULL, NULL, 0},
    { 0x6, 0x10, "R7", NULL, Click_SetTempo, 0},
    { 0x7, 0x10, "R8", NULL, Loop_SetSpeed, 0},

    { 0x0, 0x12, "R9", NULL, Loop_SetChannelGainIn, 0},
};





/* use define to dump midi data */
//#define DUMP_SERIAL2_TO_SERIAL

/* constant to normalize midi value to 0.0 - 1.0f */
#define NORM127MUL	0.007874f

inline void Midi_NoteOn(uint8_t ch, uint8_t note)
{
    Loop_NoteOn(ch, note);
    //  Synth_NoteOn(note);
}

inline void Midi_NoteOff(uint8_t ch, uint8_t note)
{
    //  Synth_NoteOff(note);
}

/*
 * this function will be called when a control change message has been received
 */
inline void Midi_ControlChange(uint8_t channel, uint8_t data1, uint8_t data2)
{
    for (int i = 0; i < (sizeof(edirolMapping) / sizeof(edirolMapping[0])); i++)
    {
        if ((edirolMapping[i].channel == channel) && (edirolMapping[i].data1 == data1))
        {
            if (edirolMapping[i].callback_mid != NULL)
            {
                edirolMapping[i].callback_mid(channel, data1, data2);
            }
            if (edirolMapping[i].callback_val != NULL)
            {
                edirolMapping[i].callback_val(edirolMapping[i].user_data, (float)data2 * NORM127MUL);
            }
        }
    }

    if (data1 == 17)
    {
        if (channel < 10)
        {
            Synth_SetSlider(channel,  data2 * NORM127MUL);
        }
    }
#if 0
    if (data1 == 17)
    {
        if (channel < 10)
        {
            Synth_SetSlider(channel,  data2 * NORM127MUL);
        }
    }
    if ((data1 == 18) && (channel == 1))
    {
        Synth_SetSlider(8,  data2 * NORM127MUL);
    }
#endif
    if ((data1 == 16) && (channel < 9))
    {
        Synth_SetRotary(channel, data2 * NORM127MUL);

    }
    if ((data1 == 18) && (channel == 0))
    {
        Synth_SetRotary(8,  data2 * NORM127MUL);
    }
}

/*
 * function will be called when a short message has been received over midi
 */
inline void HandleShortMsg(uint8_t *data)
{
    uint8_t ch = data[0] & 0x0F;

    switch (data[0] & 0xF0)
    {
    /* note on */
    case 0x90:
        if (data[2] > 0)
        {
            Midi_NoteOn(ch, data[1]);
        }
        else
        {
            Midi_NoteOff(ch, data[1]);
        }
        break;
    /* note off */
    case 0x80:
        Midi_NoteOff(ch, data[1]);
        break;
    case 0xb0:
        Midi_ControlChange(ch, data[1], data[2]);
        break;
    }
}

void Midi_Setup()
{
    Serial2.begin(31250, SERIAL_8N1, RXD2, TXD2);
    pinMode(RXD2, INPUT_PULLUP);  /* 25: GPIO 16, u2_RXD */
}

/*
 * this function should be called continuously to ensure that incoming messages can be processed
 */
void Midi_Process()
{
    /*
     * watchdog to avoid getting stuck by receiving incomplete or wrong data
     */
    static uint32_t inMsgWd = 0;
    static uint8_t inMsg[3];
    static uint8_t inMsgIndex = 0;

    //Choose Serial1 or Serial2 as required

    if (Serial2.available())
    {
        uint8_t incomingByte = Serial2.read();

#ifdef DUMP_SERIAL2_TO_SERIAL
        Serial.printf("%02x", incomingByte);
#endif
        /* ignore live messages */
        if ((incomingByte & 0xF0) == 0xF0)
        {
            return;
        }

        if (inMsgIndex == 0)
        {
            if ((incomingByte & 0x80) != 0x80)
            {
                inMsgIndex = 1;
            }
        }

        inMsg[inMsgIndex] = incomingByte;
        inMsgIndex += 1;

        if (inMsgIndex >= 3)
        {
#ifdef DUMP_SERIAL2_TO_SERIAL
            Serial.printf(">%02x %02x %02x\n", inMsg[0], inMsg[1], inMsg[2]);
#endif
            HandleShortMsg(inMsg);
            inMsgIndex = 0;
        }

        /*
         * reset watchdog to allow new bytes to be received
         */
        inMsgWd = 0;
    }
    else
    {
        if (inMsgIndex > 0)
        {
            inMsgWd++;
            if (inMsgWd == 0xFFF)
            {
                inMsgIndex = 0;
            }
        }
    }
}

