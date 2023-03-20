#include "audio.h"
#include "block_io.h"
#include "bt_commands.h"
#include "bt_serial.h"
#include "pico/printf.h"
#include "pico/time.h"
#include <stdbool.h>
#include <stdint.h>

#define BT_COMMAND_NONE 0x00
#define BT_COMMAND_CURRENT_STRUCTURE 0x10
#define BT_COMMAND_TARGET_STRUCTURE 0x10
#define BT_COMMAND_SET_LEDS 0x20
#define BT_COMMAND_PLAY_AUDIO 0x30
#define BT_COMMAND_USER_SIGNAL_COMPLETION 0x40
#define BT_COMMAND_CONFIRM_COMPLETION 0x50

static uint8_t current_command = BT_COMMAND_NONE;
static size_t blocks_remaining = 0;

static repeating_timer_t led_timer;
static bool led_on;
static uint8_t led_timer_count;
static bool is_structure_correct;

static bool device_connected_previous = false;

static bool led_timer_callback(repeating_timer_t *rt)
{
    // Flash LEDs:
    led_on = !led_on;
    if (led_on)
    {
        // Flash red or green depending on completion:
        if (is_structure_correct)
        {
            block_io_set_led_mode(GREEN);
        }
        else
        {
            block_io_set_led_mode(RED);
        }
    }
    else
    {
        block_io_set_led_mode(OFF);
    }
    
    // After 8 flashes, stop:
    if (led_timer_count++ == 15)
    {
        block_io_set_led_mode(TARGET);
        cancel_repeating_timer(&led_timer);
    }
}

void bt_commands_update_rx()
{
    // Play sound on bluetooth connect/disconnect:
    bool device_connected = bt_serial_is_connected();
    if (!device_connected_previous && device_connected)
    {
        printf("bt_commands: device connected\n");
        audio_play_sound(AUDIO_SOUND_BT_CONNECTED);
    }
    else if (device_connected_previous && !device_connected)
    {
        printf("bt_commands: device disconnected\n");
        audio_play_sound(AUDIO_SOUND_BT_DISCONNECTED);
    }
    device_connected_previous = device_connected;
    
    // Read next command:
    if (current_command == BT_COMMAND_NONE)
    {
        if (bt_serial_available())
        {
            current_command = bt_serial_read();
            printf("bt_commands: commmand received: ");
        }
    }
    
    // Handle command: 
    switch (current_command & 0xF0)
    {
        case BT_COMMAND_SET_LEDS:
            printf("set LEDs - ");
            uint8_t led_mode = current_command & 0x03;
            switch (led_mode)
            {
                case 0:
                    printf("off\n");
                    block_io_set_led_mode(OFF);
                    break;
                case 1:
                    printf("red\n");
                    block_io_set_led_mode(RED);
                    break;
                case 2:
                    printf("green\n");
                    block_io_set_led_mode(GREEN);
                    break;
                case 3:
                    printf("target\n");
                    block_io_set_led_mode(TARGET);
                    break;
            }

            current_command = BT_COMMAND_NONE;
            break;
        case BT_COMMAND_PLAY_AUDIO:
            uint8_t audio_number = current_command & 0x0F;
            printf("play audio %u\n", audio_number);
            audio_play_sound(audio_number);

            current_command = BT_COMMAND_NONE;
            break;
        case BT_COMMAND_USER_SIGNAL_COMPLETION:
            printf("signal completion\n");
            // Set up timer to flash LEDs:
            led_timer_count = 0;
            led_on = false;
            cancel_repeating_timer(&led_timer); // cancel any ongoing timer
            add_repeating_timer_ms(-200, led_timer_callback, NULL, &led_timer);
            
            // Send response saying whether the structure is complete:
            is_structure_correct = block_io_is_complete();
            bt_serial_write(BT_COMMAND_CONFIRM_COMPLETION | is_structure_correct);

            current_command = BT_COMMAND_NONE;
            break;
        case BT_COMMAND_TARGET_STRUCTURE:
            // If this is the first byte after the command:
            if (blocks_remaining == 0)
            {
                if (bt_serial_available())
                {
                    // Next byte after the command is the number of blocks in
                    // the target structure:
                    blocks_remaining = bt_serial_read();

                    printf("target structure with %u blocks\n", blocks_remaining);
                    
                    // Clear target structure to prepare for writing new structure:
                    block_io_clear_target_structure();
                }
            }
            
            // Handle the remaining bytes:
            if (blocks_remaining > 0)
            {
                // Each block takes up two bytes - wait for both to arrive:
                if (bt_serial_available() >= 2)
                {
                    // First byte contains positional data:
                    uint8_t position = bt_serial_read();
                    uint8_t grid_tile = (position >> 4) && 0x0F;
                    uint8_t height = position & 0x0F;

                    // Second byte contains the block data:
                    uint8_t block_data = bt_serial_read();
                    
                    printf("bt_commands:   block[%u][%u] = 0x%02x\n", grid_tile, height, block_data);

                    block_io_set_target_block(grid_tile, height, block_data);
                    blocks_remaining--;

                    // Once all blocks have been read, the command is finished:
                    if (blocks_remaining == 0)
                    {
                        current_command = BT_COMMAND_NONE;
                    }
                }
            }
            
            break;
        default:
            // Unrecognised command
            current_command = BT_COMMAND_NONE;
            break;
    }
}

void bt_commands_send_current_structure()
{
    if (!bt_serial_is_connected() || block_io_is_corrupted())
    {
        return;
    }

    size_t total_blocks = 0;
    for (size_t grid_tile = 0; grid_tile < BLOCK_IO_TILE_COUNT; grid_tile++)
    {
        size_t stack_height = block_io_get_stack_height(grid_tile);
        if (stack_height > 16)
        {
            // Stack too high to send over bluetooth
            printf("bt_commands: structure is too tall to send over bluetooth.");
            return;
        }
        total_blocks += stack_height;
    }

    if (total_blocks > 255)
    {
        // Too many blocks to send over bluetooth
        printf("bt_commands: structure has too many blocks to send over bluetooth.");
        return;
    }

    printf("bt_commands: sending structure containing %u blocks\n", total_blocks);
    // Send structure over bluetooth:
    bt_serial_write(BT_COMMAND_CURRENT_STRUCTURE);
    bt_serial_write(total_blocks);
    for (size_t grid_tile = 0; grid_tile < BLOCK_IO_TILE_COUNT; grid_tile++)
    {
        size_t stack_height = block_io_get_stack_height(grid_tile);
        for (size_t y = 0; y < stack_height; y++)
        {
            uint8_t location_data = (grid_tile << 4) | (y & 0x0F);
            uint8_t block_data = block_io_get_block(grid_tile, y);
            printf("bt_commands:   block[%u][%u] = 0x%02x\n", grid_tile, y, block_data);
            bt_serial_write(location_data);
            bt_serial_write(block_data);
        }
    }
}