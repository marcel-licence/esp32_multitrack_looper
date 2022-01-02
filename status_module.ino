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
 * this file contains the implementation of the terminal output
 * the output is vt100 compatible and you should use a terminal like teraTerm
 *
 * source: https://ttssh2.osdn.jp/index.html.en​
 *
 * Author: Marcel Licence
 */


char statusMsg[128] = ""; /*!< buffer for top line message */
uint32_t status_cnt = 0; /*!< counter for timeout to reset top line */
uint32_t loopMeterCount = 0; /*!< prescaler to reduce the serial output */

float *meter; /* pointer to vu meter information */

#define VU_MAX	24 /*!< character length of vu meter */

float statusVuLookup[VU_MAX]; /*!< precalculated lookup */

/*
 * prepares the module
 */
void Status_Setup(void)
{
    /*
     * prepare lookup for log vu meters
     */
    float vuValue = 1.0f;
    for (int i = VU_MAX; i > 0; i--)
    {
        int n = i - 1;

        statusVuLookup[n] = vuValue;

        vuValue *= 1.0f / sqrt(2.0f); /* div by 1/sqrt(2) means 3db */
    }
}

/*
 * function which prints the vu meter "line"
 */
void Status_PrintVu(float *value)
{
    /*
     * first pick a color
     */
    if (*value >= 0.7071f) /* -3dB */
    {
        Serial.printf("\033[0;31m"); /* red */
    }
    else if (*value >= 0.5) /* -6dB */
    {
        Serial.printf("\033[0;33m"); /* yellow */
    }
    else
    {
        Serial.printf("\033[0;32m"); /* green */
    }

    for (int i = 0; i < VU_MAX; i++)
    {

        if (statusVuLookup[i] <= *value)
        {
            Serial.printf("#");
        }
        else
        {
            Serial.printf(" ");
        }
    }

    Serial.printf("\033[0m"); /* switch back to default */

    *value *= 0.5; /* slowly lower the value */
}

/*
 * refresh complete output
 * 32 character width
 * 14 character height
 */
void Status_PrintAll(void)
{
    char emptyLine[] = "                                ";
    meter = Loop_GetMeterValues();
    Serial.printf("\033[?25l");
    Serial.printf("\033[%d;%dH", 0, 0);
    Serial.printf("--------------------------------\n");
    Serial.printf("%s", statusMsg);
    Serial.printf("%s\n", &emptyLine[strlen(statusMsg)]);
    Serial.printf("--------------------------------\n");

    Serial.printf("Input   ");
    Status_PrintVu(&meter[4]);
    Serial.println();

    Serial.printf("--------------------------------\n");
    /*
     * display track info
     */
    for (int n = 0; n < 4; n++)
    {
        if (Loop_GetRecTrack() == n)
        {
            Serial.printf("\033[0;31m");
        }
        else if (Loop_IsTrackActive(n))
        {
            Serial.printf("\033[0;32m");
        }
        else if (Loop_TrackContainsData(n))
        {
            Serial.printf("\033[0;34m");
        }
        else
        {
            Serial.printf("\033[0m");
        }

        Serial.printf("Track%d%c ", n + 1, Loop_IsUnderErase(n) ? '*' : ' ');

        Serial.printf("\033[0m");

        Status_PrintVu(&meter[n]);

        Serial.println();
    }
    Serial.printf("--------------------------------\n");

    Serial.printf("Output  ");
    Status_PrintVu(&meter[5]);

    Serial.println();

    Serial.printf("--------------------------------\n");
    for (int i = 0; i < 32; i++)
    {
        if ( i == (int)(Loop_GetRelPos() * 32.0f))
        {
            Serial.printf("O");
        }
        else
        {
            if (i == (int)(Loop_GetRelLen() * 32.0f))
            {
                Serial.printf("<");
            }
            else
            {
                Serial.printf("=");
            }
        }
    }
    Serial.println();
    Serial.printf("Click: <");
    for (int i = 0; i < 16; i++)
    {
        if ( i == (int)(Click_GetRelPos() * 32.0f))
        {
            Serial.printf("%d", (i % 4) + 1);
        }
        else
        {
            Serial.printf("%c", (i % 4) == 0 ? ':' : '-');
        }
    }
    Serial.println(">       ");

}

void Status_Process(void)
{
    status_cnt ++;
    if (status_cnt > 1000 * 3)
    {
        statusMsg[0] = 0;
        Status_PrintAll();
        status_cnt = 0;
    }

    loopMeterCount++;
    if (loopMeterCount > 10)
    {
        loopMeterCount = 0;
        Status_PrintAll();
    }
}

/*
 * update top line message including a float value
 */
void Status_ValueChangedFloat(const char *descr, float value)
{
    status_cnt = 0;
    sprintf(statusMsg, "%s: %0.2f", descr, value);
}

/*
 * update top line message including an integer value
 */
void Status_ValueChangedInt(const char *descr, int value)
{
    status_cnt = 0;
    sprintf(statusMsg, "%s: %d", descr, value);
}

/*
 * update top line message
 */
void Status_TestMsg(const char *text)
{
    status_cnt = 0;
    sprintf(statusMsg, "%s", text);
}
