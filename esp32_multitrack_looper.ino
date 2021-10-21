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
 * this file should be opened with arduino, this is the main project file
 *
 * shown in: https://youtu.be/PKQmOsJ-g1I
 *
 * Author: Marcel Licence
 */

#include <Arduino.h>
#include <WiFi.h>

/* on board led */
#define LED_PIN     19


/*
 * Chip is ESP32D0WDQ5 (revision 1)
 * Features: WiFi, BT, Dual Core, 240MHz, VRef calibration in efuse, Coding Scheme None

 * ref.: https://www.makerfabs.com/desfile/files/ESP32-A1S%20Product%20Specification.pdf

 * Board: ESP32 Dev Module
 * Flash Size: 32Mbit -> 4MB
 * RAM internal: 520KB SRAM
 * PSRAM: 4M (set to enabled!!!)
 *
 */

/* our samplerate */
#define SAMPLE_RATE 44100

/* this is used to add a task to core 0 */
TaskHandle_t  Core0TaskHnd ;

/* to avoid the high click when turning on the microphone */
static float click_supp_gain = 0.0f;

/* this application starts here */
void setup()
{
    // put your setup code here, to run once:
    delay(500);

    Serial.begin(115200);

    Serial.println();

    Serial.printf("Loading data\n");

    Serial.printf("Firmware started successfully\n");

    click_supp_gain = 0.0f;

    Blink_Setup();

    Status_Setup();

#ifdef ES8388_ENABLED
    ES8388_Setup();
#else
    ac101_setup();
#endif

    setup_i2s();

    button_setup();

    /*
     * setup midi module / rx port
     */
    Midi_Setup();

#if 0
    setup_wifi();
#else
    WiFi.mode(WIFI_OFF);
#endif

    btStop();
    // esp_wifi_deinit();

    Delay_Init();
    Delay_Reset();

    Loop_init();

    Serial.printf("ESP.getFreeHeap() %d\n", ESP.getFreeHeap());
    Serial.printf("ESP.getMinFreeHeap() %d\n", ESP.getMinFreeHeap());
    Serial.printf("ESP.getHeapSize() %d\n", ESP.getHeapSize());
    Serial.printf("ESP.getMaxAllocHeap() %d\n", ESP.getMaxAllocHeap());

    Serial.printf("Total heap: %d\n", ESP.getHeapSize());
    Serial.printf("Free heap: %d\n", ESP.getFreeHeap());

    /* PSRAM will be fully used by the looper */
    Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());

    /* we need a second task for the terminal output */
    xTaskCreatePinnedToCore(CoreTask0, "terminalTask", 8000, NULL, 999, &Core0TaskHnd, 0);
}

void CoreTask0(void *parameter)
{
    while (true)
    {
        Status_Process();

        /* this seems necessary to trigger the watchdog */
        delay(1);
        yield();
    }
}

float main_gain = 1.0f;

/*
 * this is just a leftover
 */
void Synth_SetRotary(uint8_t channel, float value)
{
    switch (channel)
    {
#if 0
    case 0:
        Loop_SetChannelGainIn(0, value);
        break;
    case 1:
        Loop_SetChannelGainIn(1, value);
        break;
    case 2:
        Loop_SetChannelGainOut(0, value);
        break;
    case 3:
        Loop_SetChannelGainOut(1, value);
        break;
#endif
    default:
        break;
    }
}

/*
 * sliders are connected with the delay module
 * the rest is not used here
 */
void Synth_SetSlider(uint8_t channel, float value)
{
    switch (channel)
    {
#if 0
    case 0:
        Loop_SetChannelGainOut(0, value);
        break;
    case 1:
        Loop_SetChannelGainOut(1, value);
        break;
    case 2:
        Loop_SetChannelGainOut(2, value);
        break;
    case 3:
        Loop_SetChannelGainOut(3, value);
        break;
#endif

    case 4:
        Delay_SetInputLevel(value);
        break;
    case 5:
        Delay_SetFeedback(value);
        break;
    case 6:
        Delay_SetLevel(value);
        break;
    case 7:
        Delay_SetLength(value);
        break;
    default:
        //  Serial.printf("slider not connected!\n");
        break;
    }
}

/* little enum to make switching more clear */
enum acSource
{
    acSrcLine,
    acSrcMic
};

/* line in is used by default, so it should not be changed here */
enum acSource selSource = acSrcLine;

/* be carefull when calling this function, microphones can cause very bad feedback!!! */
void MTLooper_ToggleSource(uint8_t channel, float value)
{
    if (value > 0)
    {
        switch (selSource)
        {
        case acSrcLine:
            click_supp_gain = 0.0f;
            ac101_setSourceMic();
            selSource = acSrcMic;
            Status_TestMsg("Input: Microphone");
            break;
        case acSrcMic:
            click_supp_gain = 0.0f;
            ac101_setSourceLine();
            selSource = acSrcLine;
            Status_TestMsg("Input: LineIn");
            break;
        }
    }
}

/*
 * this should avoid having a constant offset on our signal
 * I am not sure if that is required, but in case it can avoid early clipping
 */
static float fl_offset = 0.0f;
static float fr_offset = 0.0f;

/*
 * the main audio task
 */
inline void audio_task()
{
    static float fl_sample, fr_sample;

    fl_sample = 0.0f;
    fr_sample = 0.0f;

    i2s_read_stereo_samples(&fl_sample, &fr_sample);

    fl_sample *= click_supp_gain;
    fr_sample *= click_supp_gain;

    if (click_supp_gain < 1.0f)
    {
        click_supp_gain += 0.00001f;
    }
    else
    {
        click_supp_gain = 1.0f;
    }

    fl_offset = fl_offset * 0.99 + fl_sample * 0.01;
    fr_offset = fr_offset * 0.99 + fr_sample * 0.01;

    fl_sample -= fl_offset;
    fr_sample -= fr_offset;

    /*
     * main loop core
     */
    Loop_Process(&fl_sample, &fr_sample);

    /*
     * little simple delay effect
     */
    Delay_Process(&fl_sample, &fr_sample);

    /*
     * processing of click/metronom/tempo
     */
    Click_Process(&fl_sample, &fr_sample);

    /* apply main_gain */
    fl_sample *= main_gain;
    fr_sample *= main_gain;

    /* function blocks and returns when sample is put into buffer */
    if (i2s_write_stereo_samples(&fl_sample, &fr_sample))
    {
        /* nothing for here */
    }
}

/*
 * this function will be called once a second
 * call can be delayed when one operation needs more time (> 1/44100s)
 */
void loop_1Hz(void)
{
    static uint32_t cycl = ESP.getCycleCount();
    static uint32_t lastCycl;

    lastCycl = cycl;

    button_loop();
    Blink_Process();
}

static uint32_t midi_pre_scaler = 0;

/*
 * this is the main loop
 */
void loop()
{
    audio_task(); /* audio tasks blocks for one sample -> 1/44100s */

    static uint32_t loop_cnt;

    loop_cnt ++;
    if (loop_cnt >= SAMPLE_RATE)
    {
        loop_cnt = 0;
        loop_1Hz();
    }

    midi_pre_scaler++;
    if (midi_pre_scaler > 64)
    {
        /*
         * doing midi only 64 times per sample cycle
         */
        Midi_Process();
        midi_pre_scaler = 0;
    }
}
