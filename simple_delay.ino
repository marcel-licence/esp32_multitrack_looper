/*
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
 * This is a simple implementation of a delay line
 * - level adjustable
 * - feedback
 * - length adjustable
 *
 * here is some magic the module also uses some PSRAM for the second channel!!!
 *
 * Author: Marcel Licence
 */


/* max delay can be changed but changes also the memory consumption */
#define MAX_DELAY   22050*2

/*
 * module variables
 */
int16_t *delayLine_l;
int16_t *delayLine_r;
float delayToMix = 0;
float delayInLvl = 1.0f;
float delayFeedback = 0;
uint32_t delayLen = 11098;
uint32_t delayIn = 0;
uint32_t delayOut = 0;

void Delay_Init(void)
{
    psramInit();
    Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());

    delayLine_l = (int16_t *)malloc(sizeof(int16_t) * MAX_DELAY);
    if (delayLine_l == NULL)
    {
        Serial.printf("No more heap memory!\n");
    }
    delayLine_r = (int16_t *)ps_malloc(sizeof(int16_t) * MAX_DELAY);
    if (delayLine_r == NULL)
    {
        Serial.printf("No more heap memory!\n");
    }
    Delay_Reset();
}

void Delay_Reset(void)
{
    for (int i = 0; i < MAX_DELAY; i++)
    {
        delayLine_l[i] = 0;
        delayLine_r[i] = 0;
    }
}

void Delay_Process(float *signal_l, float *signal_r)
{
#if 0
    *signal_l *= (1.0f - delayFeedback);
    *signal_r *= (1.0f - delayFeedback);
#endif

    delayLine_l[delayIn] = (((float)0x8000) * *signal_l * delayInLvl);
    delayLine_r[delayIn] = (((float)0x8000) * *signal_r * delayInLvl);

    delayOut = delayIn + (1 + MAX_DELAY - delayLen);

    if (delayOut >= MAX_DELAY)
    {
        delayOut -= MAX_DELAY;
    }

    *signal_l += ((float)delayLine_l[delayOut]) * delayToMix / ((float)0x8000);
    *signal_r += ((float)delayLine_r[delayOut]) * delayToMix / ((float)0x8000);

    delayLine_l[delayIn] += (((float)delayLine_l[delayOut]) * delayFeedback);
    delayLine_r[delayIn] += (((float)delayLine_r[delayOut]) * delayFeedback);

    delayIn ++;

    if (delayIn >= MAX_DELAY)
    {
        delayIn = 0;
    }
}

void Delay_SetInputLevel(float value)
{
    delayInLvl = value;
    Status_ValueChangedFloat("DelayInputLevel", value);
}


void Delay_SetFeedback(float value)
{
    delayFeedback = value;
    Status_ValueChangedFloat("DelayFeedback", value);
}

void Delay_SetLevel(float value)
{
    delayToMix = value;
    Status_ValueChangedFloat("DelayOutputLevel", value);
}

void Delay_SetLength(float value)
{
    delayLen = (uint32_t)(((float)MAX_DELAY - 1.0f) * value);
    Status_ValueChangedFloat("DelayLenMs", delayLen * (1000.0f / 44100.0f));
}
