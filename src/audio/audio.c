#include "audio.h"
#include "ff.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "pico/printf.h"
#include "pico/mutex.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define AUDIO_PIN 16

#define SAMPLES_IN_BUFFER 512
#define BITS_PER_SAMPLE 10 // PWM frequency = 125 MHz / 2^10 = 122.070 kHz
#define PWM_CYCLES_PER_SAMPLE 4 // Audio sample rate = 122070 / 4 = 30.518 kHz

#define SAMPLE_RATE (125000000 / POWER_OF_2(BITS_PER_SAMPLE) / PWM_CYCLES_PER_SAMPLE)
#define AUDIO_BUFFER_SIZE (SAMPLES_IN_BUFFER * PWM_CYCLES_PER_SAMPLE)

// Convert a sample from signed 16-bit format to internal format.
#define CONVERT_SAMPLE_FROM_S16(s) (((s + POWER_OF_2(15)) >> (16 - BITS_PER_SAMPLE)) & POWER_OF_2(BITS_PER_SAMPLE) - 1)

#define POWER_OF_2(x) (1 << x)

static dma_channel_config pwm_dma_channel_config;
static uint pwm_dma_channel;
static uint audio_pin_slice;

static uint fill_write_buffer_irq;

static bool audio_initialised = false;

// Use two audio buffers. Audio plays from one while the program fills the other.
// When audio gets to the end of the playback buffer, the buffers are swaped over.
static uint16_t audio_buffer_a[AUDIO_BUFFER_SIZE];
static uint16_t audio_buffer_b[AUDIO_BUFFER_SIZE];
static uint16_t *playback_buffer = audio_buffer_a;
static uint16_t *write_buffer = audio_buffer_b;

static FATFS fat_fs;
static mutex_t file_mutex;

// These variables must be accessed through the protection of file_mutex:
static FIL audio_file;
static size_t samples_remaining_in_file;
static bool is_file_open = false;
// **********************************************************************

static void playback_buffer_finished_irh()
{
    // This interrupt is potentially shared by other DMA channels.
    // Check that our DMA channel triggered the interrupt.
    if (dma_channel_get_irq1_status(pwm_dma_channel))
    {
        // Swap playback and write buffers:
        uint16_t *temp_ptr = playback_buffer;
        playback_buffer = write_buffer;
        write_buffer = temp_ptr;

        // Start next buffer playback:
        dma_channel_set_read_addr(pwm_dma_channel, playback_buffer, true);

        // Trigger interrupt handler for filling the next buffer:
        irq_set_pending(fill_write_buffer_irq);

        // Clear interrupt request:
        dma_channel_acknowledge_irq1(pwm_dma_channel);
    }
}

static void fill_write_buffer_irh()
{
    // Fill the write buffer with samples. Start by trying to read samples from the audio file.
    // "Stretch out" these samples by duplicating them by PWM_CYCLES_PER_SAMPLE. For example if
    // PWM_CYCLES_PER_SAMPLE is 2, and the buffer had samples written like
    // [ 0, 1, 2, 3, x, x, x, x, x, x, x, x ]
    // convert the buffer to
    // [ 0, 0, 1, 1, 2, 2, 3, 3, x, x, x, x ].
    // Then if there are any remaining samples to be written (because there is no file playing
    // or the file just ended), fill them with zeros:
    // [ 0, 0, 1, 1, 2, 2, 3, 3, 0, 0, 0, 0 ].
    
    size_t sample_count = 0;

    // If file_mutex is already claimed, the program must be changing audio file. Don't bother waiting
    // to claim the mutex, since that will deadlock the program. Since a new file is about to be
    // played anyway, just fill the buffer with zeros.
    if (mutex_try_enter(&file_mutex, NULL))
    {
        if (is_file_open)
        {
            size_t bytes_read;
            size_t bytes_to_read = (samples_remaining_in_file > SAMPLES_IN_BUFFER)
                ? sizeof(write_buffer[0]) * SAMPLES_IN_BUFFER
                : sizeof(write_buffer[0]) * samples_remaining_in_file;

            f_read(&audio_file, write_buffer, bytes_to_read, &bytes_read);

            sample_count = bytes_read / sizeof(write_buffer[0]); // divide bytes_read by number of bytes per sample
            samples_remaining_in_file -= sample_count;

            // Check if we've read all the samples or if we've reached the end of the file:
            if (samples_remaining_in_file == 0 || (bytes_read < bytes_to_read))
            {
                f_close(&audio_file);
                is_file_open = false;
            }

            // Convert all samples to correct format and duplicate samples by PWM_CYCLES_PER_SAMPLE.
            // Because we'll be "stretching out" the buffer, we need to start at the end so that we don't
            // overwrite samples we haven't processed yet.
            for (
                size_t sample_index = sample_count - 1;
                sample_index != -1;
                sample_index--)
            {
                // The WAVE files we're reading store values as signed 16 bit values, but PWM will need
                // unsigned. We may also want to change the bit depth.
                uint16_t sample = CONVERT_SAMPLE_FROM_S16(write_buffer[sample_index]);
                
                // Duplicate the sample PWM_CYCLES_PER_SAMPLE times:
                for (size_t i = 0; i < PWM_CYCLES_PER_SAMPLE; i++)
                {
                    write_buffer[sample_index * PWM_CYCLES_PER_SAMPLE + i] = sample;
                }
            }
        }
        mutex_exit(&file_mutex);
    } // end of file reading

    // Fill any remaining samples with zeros:
    size_t start_index = sample_count * PWM_CYCLES_PER_SAMPLE;
    for (size_t i = start_index; i < AUDIO_BUFFER_SIZE; i++)
    {
        write_buffer[i] = CONVERT_SAMPLE_FROM_S16(0);
    }

    // Clear interrupt request:
    irq_clear(fill_write_buffer_irq);
}

void audio_init()
{
    // Prevent re-initialisation:
    if (audio_initialised)
    {
        return;
    }

    // Mount the SD card:
    FRESULT result = f_mount(&fat_fs, "0:", 1);
    if (result != FR_OK)
    {
        // Can't play audio since files cannot be loaded.
        printf("audio: Error: Failed to mount SD card. Continuing without audio.\n");
        return;
    }
    mutex_init(&file_mutex);

    // Claim DMA channel:
    pwm_dma_channel = dma_claim_unused_channel(false); // don't panic
    if (pwm_dma_channel == -1)
    {
        // Can't play audio since we need a DMA channel.
        printf("audio: Error: Failed to claim DMA channel. Continuing without audio.\n");
        return;
    }

    // Claim user IRQ:
    fill_write_buffer_irq = user_irq_claim_unused(false); // don't panic
    if (fill_write_buffer_irq == -1)
    {
        // Can't play audio since we need a user interrupt for filling the audio buffer.
        printf("audio: Error: Failed to claim user interrupt. Continuing without audio.\n");
        return;
    }

    // Set up PWM:
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);
    audio_pin_slice = pwm_gpio_to_slice_num(AUDIO_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1); // fastest PWM speed
    pwm_config_set_wrap(&config, POWER_OF_2(BITS_PER_SAMPLE) - 1);
    pwm_init(audio_pin_slice, &config, true); // start PWM now

    // Set up DMA:
    pwm_dma_channel_config = dma_channel_get_default_config(pwm_dma_channel);
    channel_config_set_transfer_data_size(&pwm_dma_channel_config, DMA_SIZE_16);
    channel_config_set_read_increment(&pwm_dma_channel_config, true);
    channel_config_set_write_increment(&pwm_dma_channel_config, false);
    channel_config_set_dreq(&pwm_dma_channel_config, DREQ_PWM_WRAP0 + audio_pin_slice); // transfer each time PWM overflows
    dma_channel_configure(
        pwm_dma_channel,
        &pwm_dma_channel_config,
        &pwm_hw->slice[audio_pin_slice].cc, // write to PWM counter-compare
        playback_buffer, // read from playback_buffer
        AUDIO_BUFFER_SIZE,
        false // don't start yet
    );

    // Set up interrupt handler for DMA finish:
    dma_channel_set_irq1_enabled(pwm_dma_channel, true);
    irq_set_priority(DMA_IRQ_1, PICO_HIGHEST_IRQ_PRIORITY); // needs to be serviced quickly to avoid glitches in the audio.
    irq_add_shared_handler(DMA_IRQ_1, playback_buffer_finished_irh, PICO_SHARED_IRQ_HANDLER_HIGHEST_ORDER_PRIORITY);
    irq_set_enabled(DMA_IRQ_1, true);

    // Set up interrupt handler for filling the write_buffer:
    irq_set_priority(fill_write_buffer_irq, PICO_LOWEST_IRQ_PRIORITY); // Allow other interrupts to execute over this one.
                                                                       // Important since file reading is done in this interrupt, and it relies on other interrupts.
                                                                       // Also this interrupt is quite long.
    irq_set_exclusive_handler(fill_write_buffer_irq, fill_write_buffer_irh);
    irq_set_enabled(fill_write_buffer_irq, true);

    // If we made it through initialisation, set flag:
    audio_initialised = true;

    printf("audio: Initialisation successful.\n");

    // Start the first DMA transfer. Subsequent transfers will be triggered
    // automatically whenever the whole buffer has been played.
    dma_channel_start(pwm_dma_channel);
}

void audio_play_sound(size_t sound_number)
{
    // If audio wasn't initialised, we can't play audio:
    if (!audio_initialised)
    {
        printf("audio: Error: Can't play audio file. Audio not initialised.\n");
        return;
    }

    mutex_enter_blocking(&file_mutex);

    // If a file is open, close it:
    if (is_file_open)
    {
        f_close(&audio_file);
        is_file_open = false;
    }

    // Audio files have numeric names (in lowercase hex) like "5b.wav", given by the sound_number.
    const size_t filename_length =
        sizeof(sound_number) * 2 // enough space for two hex digits per byte
        + 3  // ".wav"
        + 1; // null terminator
    char filename[filename_length];
    snprintf(filename, filename_length, "%x.wav", sound_number);

    // Open file:
    FRESULT result = f_open(&audio_file, filename, FA_READ);
    if (result == FR_OK)
    {
        is_file_open = true;
        samples_remaining_in_file = 0; // default value in case we can't process the file

        size_t bytes_read;
        uint8_t buffer[4];
        
        // Read RIFF header "ChunkID":
        f_lseek(&audio_file, 0); // make sure we're at the beginning of the file
        f_read(&audio_file, buffer, 4, &bytes_read);
        if (memcmp(buffer, "RIFF", 4) != 0)
        {
            printf("audio: Error: Audio file \"%s\" is not a valid RIFF (WAVE) file.", filename);
            goto invalid_file;
        }
        
        // Read format (should be WAVE):
        f_lseek(&audio_file, 8); // seek to format
        f_read(&audio_file, buffer, 4, &bytes_read);
        if (memcmp(buffer, "WAVE", 4) != 0)
        {
            printf("audio: Error: Audio file \"%s\" is not a valid WAVE file.", filename);
            goto invalid_file;
        }
        
        // Read audio format (should be PCM):
        f_lseek(&audio_file, 20);
        f_read(&audio_file, buffer, 2, &bytes_read);
        uint16_t audio_format = (buffer[1] << 8) | (buffer[0]);
        if (audio_format != 1) // PCM format
        {
            printf("audio: Error: Audio file \"%s\" does not use PCM format. Only PCM is supported.", filename);
            goto invalid_file;
        }
        
        // Read number of channels (should be 1):
        f_lseek(&audio_file, 22);
        f_read(&audio_file, buffer, 2, &bytes_read);
        uint16_t num_channels = (buffer[1] << 8) | (buffer[0]);
        if (num_channels != 1)
        {
            printf("audio: Error: Audio file \"%s\" has %u channels. Only one is supported (mono).", filename, num_channels);
            goto invalid_file;
        }
        
        // Read bits per sample (should be 16):
        f_lseek(&audio_file, 34);
        f_read(&audio_file, buffer, 2, &bytes_read);
        uint16_t bits_per_sample = (buffer[1] << 8) | (buffer[0]);
        if (bits_per_sample != 16)
        {
            printf("audio: Error: Audio file \"%s\" uses %u bits per sample. Only 16 is supported.", filename, bits_per_sample);
            goto invalid_file;
        }

        printf("audio: Playing audio file \"%s\".\n", filename);
        
        // Read sample rate:
        f_lseek(&audio_file, 24);
        f_read(&audio_file, buffer, 4, &bytes_read);
        uint32_t file_sample_rate = (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | (buffer[0]);
        if (file_sample_rate != SAMPLE_RATE)
        {
            printf("audio: Warning: Audio file \"%s\" has a sample rate of %u Hz but will be played at %u Hz.\n",
                filename,
                file_sample_rate,
                SAMPLE_RATE
                );
        }
        
        // Read number of samples:
        f_lseek(&audio_file, 40);
        f_read(&audio_file, buffer, 4, &bytes_read);
        uint32_t data_size = (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | (buffer[0]);
        samples_remaining_in_file = data_size / 2;
        
        // Place file pointer at the start of the samples:
        f_lseek(&audio_file, 44);
    }
    else
    {
        printf("audio: Error: Failed to open audio file \"%s\".\n", filename);
    }
    
    mutex_exit(&file_mutex);
    return;

invalid_file:
    // If the file couldn't be processed, close it:
    f_close(&audio_file);
    is_file_open = false;
    mutex_exit(&file_mutex);
}