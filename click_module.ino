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
 * this file contains the module controlling the tempo of the looper
 * it counts always 4 bars
 * min tempo will be calculated depending on the max loop length
 *
 * Author: Marcel Licence
 */

#define SEQ_TRACK_CNT	12
#define SEQ_STEPS		16

/*
 * 1: only 4th notes
 * 2: for 8th notes
 */
#define SEQ_SUBSTEP_MUL	2

#define DEFAULT_BPM		140

uint32_t seq_prescaler = 60.0f * 44100.0f / (2.0f * SEQ_SUBSTEP_MUL * (float)DEFAULT_BPM);
uint32_t seq_prescaler_next = 0;
uint32_t seq_pos = 0;
uint32_t seq_bar = 0;
bool seq_click = true; /*!< generate audible click */
bool seq_tempo = true; /*!< control looper with set tempo */
uint32_t seq_counter = 0;

bool seq_active = true;

/*
 * This function integrates the metronome sound
 * In addition to that it controls the looper if activated
 */
void Click_Process(float *left, float *right)
{
    if (seq_active)
    {
        if (seq_click)
        {
            if (seq_pos == 0)
            {
                if (seq_bar == 0)
                {
                    /* create highest pitch sound for the first beat of the first bar */
                    *left += ((float)((seq_counter & 16) / 16)) * 0.1f;
                    *right += ((float)((seq_counter & 16) / 16)) * 0.1f;
                }
                else
                {
                    /* generate middle pitched sound for the first beat of all other bars */
                    *left += ((float)((seq_counter & 32) / 32)) * 0.1f;
                    *right += ((float)((seq_counter & 32) / 32)) * 0.1f;
                }
            }
            else if ((seq_pos % 4) == 0)
            {
                /* generate sound with lowest pitch for 2, 3, 4 */
                *left += ((float)((seq_counter & 64) / 64)) * 0.1f;
                *right += ((float)((seq_counter & 64) / 64)) * 0.1f;
            }
        }

        /* only if module is active the loop should be reset when jumping to first beat in first bar */
        if (seq_tempo)
        {
            if ((seq_pos == 0) && (seq_counter == 0) && (seq_bar == 0))
            {
                //Loop_ResetToStart(0, 1);
                Loop_SetEndByTempo();
            }

            if ((seq_pos == 0) && (seq_counter == 1) && (seq_bar == 0))
            {
                //Loop_ResetToStart(0, 0);
            }
        }

        seq_counter ++;

        if (seq_counter > (seq_prescaler / getLoopSpeed()))
        {
            seq_counter = 0;
            seq_pos++;
            if (seq_pos >= 16)
            {
                seq_pos = 0;
                seq_bar = (seq_bar + 1) % 4;
            }

            if (Loop_AnyTrackActive() == false)
            {
                seq_pos = seq_pos % 4;
                seq_bar = 0;
            }
        }
    }
}

/*
 * count in two bars
 */
void Click_StartFirst(void)
{
    if (seq_active)
    {
        seq_counter = 0;
        seq_bar = 2;
        seq_pos = 0;
    }
    else
    {
        Loop_ResetToStart(0, 1);
        Loop_ResetToStart(0, 0);
    }
}

/*
 * this function return the relative beat position as float 0..1
 * it can be used to display the beat position
 */
float Click_GetRelPos(void)
{
    return ((float)seq_bar * 16.0f + (float)seq_pos) / (16.0f * 8.0f);
}

/*
 * reset the beat, bar to the beginning
 */
void Click_Reset(uint8_t channel, float value)
{
    if (value > 0)
    {
        seq_pos = 0;
        seq_counter = 0;
        seq_bar = 0;
    }
}

/*
 * jump beat to beginning of given bar (quarter)
 * todo: there is an issue with this function
 */
void Click_JumpPosQuarter(uint8_t quarter, float value)
{
    if (value > 0)
    {
        seq_pos = 0;
        seq_counter = 0;
        seq_bar = quarter;
    }
}

/*
 * toggle the sound of the metronome on/off
 */
void Click_OnOff(uint8_t channel, float value)
{
    if (value > 0)
    {
        if (seq_click)
        {
            seq_click = false;
            Status_TestMsg("Click off");
        }
        else
        {
            seq_click = true;
            Status_TestMsg("Click on");
        }
    }
}

/*
 * turn the complete module on/off
 * when turned off the looper will not be reset by this module
 */
void Click_ToggleOnOff(uint8_t channel, float value)
{
    if (value > 0)
    {
        if (seq_active)
        {
            seq_active = false;
            Status_TestMsg("Metronome off");
        }
        else
        {
            seq_active = true;
            Status_TestMsg("Metronome on");
        }
    }
}

/*
 * set the tempo
 * prescaler will be calculated by samplerate
 */
void Click_SetTempo(uint8_t channel, float value)
{
    /*
     * time = 4 * 4 / (BPM / 60)
     * BPM = ((4 * 4) / time) * 60
     */

    float min_val = ((4.0f * 4.0f) / Loop_GetMaxRecLengthSeconds()) * 60;
    float max_val = 240;

    value = min_val + value * (max_val - min_val);

    seq_prescaler = 60.0f * 44100.0f / (2.0f * SEQ_SUBSTEP_MUL * value);

    Status_ValueChangedFloat("ClickSetTempo", value);
}
