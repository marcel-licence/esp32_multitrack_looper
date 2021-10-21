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
 * this file contains the code of the multitrack looper
 *
 * Author: Marcel Licence
 */


/* 
 * MAX_LOOP can only increased by decreasing TRACK_CNT (track count) 
 * 
 * for example using only 2 tracks will allow MAX_LOOP to be set to 1026492
 * all PSRAM memory will be used
 * if values are set to big the startup of the firmware will fail
 */
#define MAX_LOOP	513246
#define TRACK_CNT	4

int16_t *loopLine[TRACK_CNT];
float loopGainOut[TRACK_CNT];
float loopGainIn;
float loopPanLCH[TRACK_CNT];
float loopPanRCH[TRACK_CNT];
bool loopActive[TRACK_CNT];
bool loopContainsData[TRACK_CNT];
float loopMeter[TRACK_CNT + 2];
uint32_t loopErase[TRACK_CNT] ;

uint8_t loopInCh = 0xFF;
uint32_t loopMax = MAX_LOOP;
uint32_t loopMaxCalc = MAX_LOOP;

/*
 * init of the multitrack looper
 */
void Loop_init(void)
{
    psramInit();
    Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());

    for (int n = 0; n < TRACK_CNT; n++)
    {
        loopLine[n] = (int16_t *)ps_malloc(sizeof(int16_t) * MAX_LOOP);

        if (loopLine[n] == NULL)
        {
			/* when you see the message the count of TRACK_CNT is too high or MAX_LOOP is too big */
            Serial.printf("not enough memory!\n");
            return;
        }
        else
        {
            for (int i = 0; i < MAX_LOOP; i++)
            {
                loopLine[n][i] = 0.0f;
            }
            Serial.printf("done!..\n");
        }
        loopGainOut[n] = 1.0f;
        loopPanLCH[n] = 1.0f;
        loopPanRCH[n] = 1.0f;

        Serial.printf("- Free PSRAM: %d\n", ESP.getFreePsram());
    }

    Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());

    loopGainIn = 0.0f;
    loopInCh = 0;

    Serial.printf("Loop Max Length: %0.0f seconds\n", ((float)MAX_LOOP) / 44100.0f);

    Loop_CompleteReset();
}

uint32_t loopLen = (MAX_LOOP - 2);
uint32_t loopIn = 0;
float loop_f = 0;
float loopSpeed = 1.0f;

uint32_t loopSubCnt = 0;

#define absf(a)	(a>0)?a:(-a)

/*
 * this function processes the input audio and mixes also the audio output to the signal
 */
void Loop_Process(float *signal_l, float *signal_r)
{
    float tempL = 0.0f, tempR = 0.0f;

    loopMeter[TRACK_CNT] = max(absf(*signal_l), loopMeter[TRACK_CNT] );
    loopMeter[TRACK_CNT] = max(absf(*signal_r), loopMeter[TRACK_CNT] );

    loopSubCnt++;

    if (Loop_AnyTrackActive() == false)
    {
        loopLen = (MAX_LOOP - 2);
        loopIn = 0;
        loop_f = 0.0f;
    }

    /*
     * audio through
     */
    *signal_l *= loopGainIn;
    *signal_r *= loopGainIn;

    tempL = *signal_l;
    tempR = *signal_r;

    for (int n = 0; n < TRACK_CNT; n++)
    {
        /*
         * do erasing first if active
         * to avoid getting any signal out of the track
         */
        if (loopErase[n] != 0xFFFFFFFFUL)
        {
            loopLine[n][loopIn] = 0;
        }

        if (n == loopInCh)
        {
            loopContainsData[loopInCh] = true;
            loopLine[loopInCh][loopIn] += (((float)0x8000) * *signal_l * loopPanLCH[loopInCh]);
            loopLine[loopInCh][loopIn] += (((float)0x8000) * *signal_r * loopPanRCH[loopInCh]);
        }

        /*
         * calc float value for later use
         */
        float loopOut = ((float)loopLine[n][loopIn]) / ((float)0x8000);

        /*
         * update the vu meter value
         */
        loopMeter[n] = max(absf(loopOut), loopMeter[n]);

        /*
         * playback only if active
         */
        if (loopActive[n])
        {
            tempL += loopGainOut[n] * loopOut * loopPanLCH[n];
            tempR += loopGainOut[n] * loopOut * loopPanRCH[n];
        }

#if 0 /* I don't remember the use of this */
        delayLine_l[delayIn] += delayLine_l[loopOut] * delayFeedback;
        delayLine_r[delayIn] += delayLine_r[loopOut] * delayFeedback;
#endif
    }

    loop_f += loopSpeed;

    if (loop_f >= loopMax)
    {
        loop_f -= loopMax;
    }

    loopIn = loop_f;

    for (int n = 0; n < TRACK_CNT; n++)
    {
		/* stop erasing when final position has been reached */
        if (loopErase[n] == loopIn)
        {
            loopErase[n] = 0xFFFFFFFFUL;
        }
    }

    *signal_l = tempL;
    *signal_r = tempR;

    /*
     * get max value of left and right channel
     */
    loopMeter[TRACK_CNT + 1] = max(absf(*signal_l), loopMeter[TRACK_CNT + 1] );
    loopMeter[TRACK_CNT + 1] = max(absf(*signal_r), loopMeter[TRACK_CNT + 1] );

    Loop_ProcessButton();
}

bool reset_active = false;
uint32_t reset_active_time = 0;

/*
 * reset the looper to initial state
 */
void Loop_CompleteReset(void)
{
    loopMax = MAX_LOOP;
    loopIn = 0;
    loop_f = 0;
    loopSpeed = 1.0f;
    loopInCh = 0xFF;

    for (int channel = 0; channel < TRACK_CNT; channel ++)
    {
        Status_ValueChangedInt("LoopTrackErase", channel);
        for (int n = 0; n < MAX_LOOP; n++)
        {
            loopLine[channel][n] = 0.0f;
        }
        loopActive[channel] = false;
        loopContainsData[channel] = false;
    }

    memset(loopErase, 0xFF, sizeof(loopErase));
}

void Loop_ProcessButton(void)
{
    if (reset_active)
    {
        reset_active_time++;
        if (reset_active_time > 44100 * 3)
        {
            Loop_CompleteReset();
            reset_active = 0;
        }
    }
    else
    {
        reset_active_time = 0;
    }
}

void Loop_ResetToStart(uint8_t channel, float value)
{
    if (value > 0)
    {
        reset_active = true;
        loop_f = 0;
    }
    else
    {
        reset_active = 0;
    }
}

void Loop_PlayNormal(uint8_t channel, float value)
{
    if (value > 0)
    {
        loop_f = loopIn;
        loopSpeed = 1.0f;
    }
}

float getLoopSpeed(void)
{
    return loopSpeed;
}

void Loop_SetLength(uint8_t channel, float value)
{
    if (value > 0)
    {
        loopMax = loop_f;
    }
}

void Loop_SetEndByTempo(void)
{
    loopMaxCalc = loop_f;
    loop_f = 0;
}

void Loop_SetSpeed(uint8_t channel, float value)
{
    value = pow(2.0f, 4.0f * (value - 0.5));
    loopSpeed = value;
    Status_ValueChangedFloat("LoopSpeed", loopSpeed);
}

void Loop_StartAll(uint8_t channel, float value)
{
    if (value > 0)
    {
        for (int n = 0; n < TRACK_CNT; n++)
        {
            if (loopContainsData[n])
            {
                loopActive[n] = true;
            }
        }
    }
}

/*
 * this function will activate recording
 * or playback
 * or toggle between both modes
 */
void Loop_SelectTrack(uint8_t channel, float value)
{
    if (value > 0)
    {

        if (channel < TRACK_CNT)
        {
            /*
             * count in if all empty
             */
            if (Loop_AnyTrackActive() == false)
            {
                Click_StartFirst();
            }

            if (loopContainsData[channel] == false)
            {
                /*
                 * in case of selecting an empty channel we switch always to record mode
                 */
                loopActive[channel] = true;
                loopInCh = channel;
            }
            else
            {
                if (loopActive[channel] == false)
                {
                    /*
                     * if we have data in it but track is not active we go into playback
                     */
                    loopInCh = 0xFF;
                    loopActive[channel] = true;
                }
                else
                {
                    /*
                     * if still active we switch between playback and overdub/record
                     */
                    if (loopInCh == channel)
                    {
                        /*
                         * still selected going to playback
                         */
                        loopInCh = 0xFF;
                    }
                    else
                    {
                        loopInCh = channel;
                        Status_ValueChangedInt("LoopRecTrack", loopInCh);
                    }
                }
            }


        }
    }
}

void Loop_StopChannel(uint8_t channel, float value)
{
    if (channel < TRACK_CNT)
    {
        loopActive[channel] = false;

        /* stop recording if rec is active */
        if (channel == loopInCh)
        {
            loopInCh = 0xFF;
        }
    }
}

void Loop_Stop(uint8_t channel, float value)
{
    if (value > 0)
    {
        for (int n = 0; n < TRACK_CNT; n ++)
        {
            loopActive[n] = false;
        }
    }
}

/*
 * erase track will be activated
 * a complete clear in this call was too slow
 */
void Loop_EraseTrack(uint8_t channel, float value)
{
    if (channel < TRACK_CNT)
    {
        if (value > 0)
        {
            Status_ValueChangedInt("LoopEraseTrack", channel);

            loopErase[channel] = loop_f;
            loopContainsData[channel] = false;
            if (channel != loopInCh)
            {
                loopActive[channel] = false;
            }
        }
    }
}

void Loop_SetChannelGainIn(uint8_t channel, float value)
{
    loopGainIn = value;
    Status_ValueChangedFloat("LoopGainIn", loopGainIn);
}

void Loop_SetChannelGainOut(uint8_t channel, float value)
{
    if (channel < TRACK_CNT)
    {
        loopGainOut[channel] = value;
        //Serial.printf("Ch[%d].outLevel: %0.2f\n", channel, value);
        Status_ValueChangedFloat("LoopTrackLevel", value); // todo track number is missing
    }
}

void Loop_SetChannelPan(uint8_t channel, float value)
{
    if (channel < TRACK_CNT)
    {
        loopPanLCH[channel] = value < 0.5 ? 1 : 2 * (0.5 - (value - 0.5));
        loopPanRCH[channel] = value > 0.5 ? 1 : 2 * (value);
        //Serial.printf("Ch[%d].Pan: %0.2f:%0.2f\n", channel, loopPanLCH[channel], loopPanRCH[channel]);
        Status_ValueChangedInt("LoopTrackPan", channel); // todo track number is missing
    }
}

void Loop_JumpPosQuarter(uint8_t quarter, float value)
{
    if (value > 0)
    {
        if (loopMax < loopMaxCalc)
        {
            loop_f = ((float)loopMax) * ((float)quarter) * (0.25f);
        }
        else
        {
            loop_f = ((float)loopMaxCalc) * ((float)quarter) * (0.25f);
        }
    }
}

void Loop_NoteOn(uint8_t ch, uint8_t note)
{
    if (ch == 15)
    {
        loop_f = 0;
        loopSpeed = pow(2.0f, 1.0f / 12.0f * (note - 69)); /* this would be the a as middle */
    }
}

float *Loop_GetMeterValues(void)
{
    return loopMeter;
}

float Loop_GetRelPos(void)
{
    return loop_f / ((float)MAX_LOOP);
}

float Loop_GetRelLen(void)
{
    return loopMax / ((float)MAX_LOOP);
}

float Loop_GetMaxRecLengthSeconds(void)
{
    return ((float)MAX_LOOP) / 44100.0f;
}

uint8_t Loop_GetRecTrack(void)
{
    return loopInCh;
}

bool Loop_IsTrackActive(uint8_t channel)
{
    if (channel < TRACK_CNT)
    {
        return loopActive[channel];
    }
    else
    {
        return false;
    }
}

bool Loop_IsUnderErase(uint8_t channel)
{
    if (channel < TRACK_CNT)
    {
        return loopErase[channel] != 0xFFFFFFFFUL;
    }
    else
    {
        return false;
    }
}

bool Loop_TrackContainsData(uint8_t channel)
{
    if (channel < TRACK_CNT)
    {
        return loopContainsData[channel];
    }
    else
    {
        return false;
    }
}

bool Loop_AnyTrackActive(void)
{
    for (int n = 0; n < TRACK_CNT; n++)
    {
        if (loopActive[n])
        {
            return true;
        }
        if (loopErase[n] != 0xFFFFFFFFUL)
        {
            return true;
        }
    }
    return false;
}
