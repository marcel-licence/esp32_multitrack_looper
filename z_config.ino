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
 * Dieses Programm ist Freie Software: Sie k�nnen es unter den Bedingungen
 * der GNU General Public License, wie von der Free Software Foundation,
 * Version 3 der Lizenz oder (nach Ihrer Wahl) jeder neueren
 * ver�ffentlichten Version, weiter verteilen und/oder modifizieren.
 *
 * Dieses Programm wird in der Hoffnung bereitgestellt, dass es n�tzlich sein wird, jedoch
 * OHNE JEDE GEW�HR,; sogar ohne die implizite
 * Gew�hr der MARKTF�HIGKEIT oder EIGNUNG F�R EINEN BESTIMMTEN ZWECK.
 * Siehe die GNU General Public License f�r weitere Einzelheiten.
 *
 * Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
 * Programm erhalten haben. Wenn nicht, siehe <https://www.gnu.org/licenses/>.
 */

/*
 * z_config.ino
 *
 * Put all your project configuration here (no defines etc)
 * This file will be included at the and can access all
 * declarations and type definitions
 *
 *  Created on: 02.01.2022
 *      Author: Marcel Licence
 */

/*
 * this mapping is used for the edirol pcr-800
 * this should be changed when using another controller
 */
struct midiControllerMapping edirolMapping[] =
{
    /* transport buttons */
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

    /* Central slider */
#ifndef AS5600_ENABLED
    // { 0x0, 0x13, "H1", NULL, App_SetBrightness, 0},
#else
    { 0x0, 0x13, "H1", NULL, Sampler_ScratchFader, 0},
#endif
};


struct midiMapping_s midiMapping =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    edirolMapping,
    sizeof(edirolMapping) / sizeof(edirolMapping[0]),
};

