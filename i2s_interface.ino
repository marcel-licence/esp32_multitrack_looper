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
 * this file includes all required function to setup and drive the i2s interface
 *
 * Author: Marcel Licence
 */

#include <driver/i2s.h>



/*
 * no dac not tested within this code
 * - it has the purpose to generate a quasy analog signal without a DAC
 */
//#define I2S_NODAC

/*
 * Define and connect your PINS to DAC here
 */
#if 0
#define I2S_BCLK_PIN	25
#define I2S_WCLK_PIN	27

#define I2S_DOUT_PIN	26

#else

#endif

const i2s_port_t i2s_num = I2S_NUM_0; // i2s port number



bool i2s_write_sample_32ch2(uint8_t *sample);

bool i2s_write_sample_32ch2(uint8_t *sample)
{
    static size_t bytes_written = 0;
    static size_t bytes_read = 0;
    i2s_read(i2s_num, (char *)sample, 8, &bytes_read, portMAX_DELAY);

    i2s_write(i2s_num, (const char *)sample, 8, &bytes_written, portMAX_DELAY);

    if (bytes_written > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

#ifdef SAMPLE_SIZE_24BIT

bool i2s_write_sample_24ch2(uint8_t *sample);

bool i2s_write_sample_24ch2(uint8_t *sample)
{
    static size_t bytes_written1 = 0;
    static size_t bytes_written2 = 0;
    i2s_write((i2s_port_t)i2s_num, (const char *)&sample[1], 3, &bytes_written1, portMAX_DELAY);
    i2s_write((i2s_port_t)i2s_num, (const char *)&sample[5], 3, &bytes_written2, portMAX_DELAY);

    if ((bytes_written1 + bytes_written2) > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

#endif

bool i2s_write_stereo_samples(float *fl_sample, float *fr_sample)
{

#ifdef SAMPLE_SIZE_24BIT
#if 0
    static union sampleTUNT
    {
        uint8_t sample[8];
        int32_t ch[2];
    } sampleDataU;
#else
    static union sampleTUNT
    {
        int32_t ch[2];
        uint8_t bytes[8];
    } sampleDataU;
#endif
#endif
#ifdef SAMPLE_SIZE_16BIT
    static union sampleTUNT
    {
        uint32_t sample;
        int16_t ch[2];
    } sampleDataU;
#endif

    sampleDataU.ch[0] = int16_t(*fl_sample * 16383.0f);
    sampleDataU.ch[1] = int16_t(*fr_sample * 16383.0f);

    static size_t bytes_written = 0;

    i2s_write(i2s_num, (const char *)&sampleDataU.sample, 4, &bytes_written, portMAX_DELAY);

    if (bytes_written > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void i2s_read_stereo_samples(float *fl_sample, float *fr_sample)
{
    static size_t bytes_read = 0;

    static union
    {
        uint32_t sample;
        int16_t ch[2];
    } sampleData;


    i2s_read(i2s_num, (char *)&sampleData.sample, 4, &bytes_read, portMAX_DELAY);

    //sampleData.ch[0] &= 0xFFFE;
    //sampleData.ch[1] &= 0;

    *fl_sample = ((float)sampleData.ch[0] * (5.5f / 65535.0f));
    *fr_sample = ((float)sampleData.ch[1] * (5.5f / 65535.0f));
}

/*
 * i2s configuration
 */

#if 1
i2s_config_t i2s_config =
{
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX), // | I2S_MODE_DAC_BUILT_IN
    .sample_rate = SAMPLE_RATE,
#ifdef SAMPLE_SIZE_24BIT
    .bits_per_sample = I2S_BITS_PER_SAMPLE_24BIT, /* the DAC module will only take the 8bits from MSB */
#endif
#ifdef SAMPLE_SIZE_16BIT
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, /* the DAC module will only take the 8bits from MSB */
#endif
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = (i2s_comm_format_t)( I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = 0, // default interrupt priority
#if 0
    .dma_buf_count = 2,
    .dma_buf_len = 1024,
#else
    .dma_buf_count = 8,
    .dma_buf_len = 64,
#endif
    .use_apll = 0
};
#else
i2s_config_t i2s_config =
{
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX ),
    .sample_rate = SAMPLE_RATE,
#ifdef I2S_NODAC
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_I2S_MSB,
#else
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
#endif
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = 0
};
#endif


i2s_pin_config_t pins =
{
    .bck_io_num = IIS_SCLK,
    .ws_io_num =  IIS_LCLK,
    .data_out_num = IIS_DSIN,
    .data_in_num = IIS_DSOUT
};



void setup_i2s()
{
    i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pins);
    i2s_set_sample_rates(i2s_num, SAMPLE_RATE);
    i2s_start(i2s_num);
}
