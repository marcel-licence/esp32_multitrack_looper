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

#include "AC101.h" /* only compatible with forked repo: https://github.com/marcel-licence/AC101 */

/* AC101 pins */
#define IIS_SCLK                    27
#define IIS_LCLK                    26
#define IIS_DSIN                    25
#define IIS_DSOUT                   35

#define IIC_CLK                     32
#define IIC_DATA                    33

#define GPIO_PA_EN                  GPIO_NUM_21
#define GPIO_SEL_PA_EN              GPIO_SEL_21

#define PIN_KEY_2					(13)

#define PIN_PLAY                    (23)      // KEY 4
#define PIN_VOL_UP                  (18)      // KEY 5
#define PIN_VOL_DOWN                (5)       // KEY 6

#define OUTPUT_PIN 0
#define MCLK_CH	0
#define PWM_BIT	1

static AC101 ac;

/* actually only supporting 16 bit */
#define SAMPLE_SIZE_16BIT
//#define SAMPLE_SIZE_24BIT
//#define SAMPLE_SIZE_32BIT

#define SAMPLE_RATE	44100
#define CHANNEL_COUNT	2
#define WORD_SIZE	16
#define I2S1CLK	(512*SAMPLE_RATE)
#define BCLK	(SAMPLE_RATE*CHANNEL_COUNT*WORD_SIZE)
#define LRCK	(SAMPLE_RATE*CHANNEL_COUNT)

/*
 * this function could be used to set up the masterclock
 * it is not necessary to use the ac101
 */
void ac101_mclk_setup()
{
    // Put a signal out on pin
    uint32_t freq = SAMPLE_RATE * 512; /* The maximal frequency is 80000000 / 2^bit_num */
    Serial.printf("Output frequency: %d\n", freq);
    ledcSetup(MCLK_CH, freq, PWM_BIT);
    ledcAttachPin(OUTPUT_PIN, MCLK_CH);
    ledcWrite(MCLK_CH, 1 << (PWM_BIT - 1)); /* 50% duty -> The available duty levels are (2^bit_num)-1, where bit_num can be 1-15. */
}

/*
 * complete setup of the ac101 to enable in/output
 */
void ac101_setup()
{
    Serial.printf("Connect to AC101 codec... ");
    while (not ac.begin(IIC_DATA, IIC_CLK))
    {
        Serial.printf("Failed!\n");
        delay(1000);
    }
    Serial.printf("OK\n");
#ifdef SAMPLE_SIZE_24BIT
    ac.SetI2sWordSize(AC101::WORD_SIZE_24_BITS);
#endif
#ifdef SAMPLE_SIZE_16BIT
    ac.SetI2sWordSize(AC101::WORD_SIZE_16_BITS);
#endif

#if (SAMPLE_RATE==44100)&&(defined(SAMPLE_SIZE_16BIT))
    ac.SetI2sSampleRate(AC101::SAMPLE_RATE_44100);
    /*
     * BCLK: 44100 * 2 * 16 =
     *
     * I2S1CLK/BCLK1 -> 512 * 44100 / 44100*2*16
     * BCLK1/LRCK -> 44100*2*16 / 44100 Obacht ... ein clock cycle goes high and low
     * means 32 when 32 bits are in a LR word channel * word_size
     */
    ac.SetI2sClock(AC101::BCLK_DIV_16, false, AC101::LRCK_DIV_32, false);
    ac.SetI2sMode(AC101::MODE_SLAVE);
    ac.SetI2sWordSize(AC101::WORD_SIZE_16_BITS);
    ac.SetI2sFormat(AC101::DATA_FORMAT_I2S);
#endif

    ac.SetVolumeSpeaker(3);
    ac.SetVolumeHeadphone(99);

#if 1
    ac.SetLineSource();
#else
    ac.SetMicSource(); /* handle with care: mic is very sensitive and might cause feedback using amp!!! */
#endif

#if 0
    ac.DumpRegisters();
#endif

    // Enable amplifier
    pinMode(GPIO_PA_EN, OUTPUT);
    digitalWrite(GPIO_PA_EN, HIGH);
}

/*
 * pullup required to enable reading the buttons (buttons will connect them to ground if pressed)
 */
void button_setup()
{
    // Configure keys on ESP32 Audio Kit board
    pinMode(PIN_PLAY, INPUT_PULLUP);
    pinMode(PIN_VOL_UP, INPUT_PULLUP);
    pinMode(PIN_VOL_DOWN, INPUT_PULLUP);
}

/*
 * selects the microphone as audio source
 * handle with care: mic is very sensitive and might cause feedback using amp!!!
 */
void ac101_setSourceMic(void)
{
    ac.SetMicSource();
}

/*
 * selects the line in as input
 */
void ac101_setSourceLine(void)
{
    ac.SetLineSource();
}

/*
 * very bad implementation checking the button state
 * there is some work required for a better functionality
 */
void button_loop()
{
    if (digitalRead(PIN_PLAY) == LOW)
    {
        Serial.println("PIN_PLAY pressed");
    }
    if (digitalRead(PIN_VOL_UP) == LOW)
    {
        Serial.println("PIN_VOL_UP pressed");
    }
    if (digitalRead(PIN_VOL_DOWN) == LOW)
    {
        Serial.println("PIN_VOL_DOWN pressed");
    }
}
