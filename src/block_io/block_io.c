#include <string.h>
#include "block_io.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/time.h"

#define SS_DATA_PIN 0
#define SS_CLK_PIN 1
#define SPI_SCK_PIN 2
#define SPI_TX_PIN 3
#define SPI_RX_PIN 4

#define HEIGHT_LIMIT 16

static uint8_t target_structure[BLOCK_IO_TILE_COUNT][HEIGHT_LIMIT];
static uint8_t current_structure[BLOCK_IO_TILE_COUNT][HEIGHT_LIMIT];
static uint8_t grid_height[BLOCK_IO_TILE_COUNT];
static bool is_complete = false;
static bool is_corrupted = false;
static led_mode_t led_mode = TARGET;

void block_io_clear_target_structure()
{
    memset(target_structure, 0, sizeof(target_structure));
}

uint8_t block_io_get_block(size_t grid_tile, size_t height)
{
    if (grid_tile < BLOCK_IO_TILE_COUNT && height < HEIGHT_LIMIT)
    {
        return current_structure[grid_tile][height];
    }
    else
    {
        return 0;
    }
}

size_t block_io_get_stack_height(size_t grid_tile)
{
    if (grid_tile < BLOCK_IO_TILE_COUNT)
    {
        return grid_height[grid_tile];
    }
    else
    {
        return 0;
    }
}

void block_io_init()
{
    // Set baudrate:
    spi_init(spi0, 250000);

    // Set GPIO pins:
    gpio_set_function(SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_init(SS_DATA_PIN);
    gpio_init(SS_CLK_PIN);
    gpio_set_dir(SS_DATA_PIN, GPIO_OUT);
    gpio_set_dir(SS_CLK_PIN, GPIO_OUT);

    // Intialise the slave select shift register by filling it with ones:
    gpio_put(SS_DATA_PIN, 1);
    for(int i=0;i<BLOCK_IO_TILE_COUNT;i++)
    {
        gpio_put(SS_CLK_PIN, 0);
        sleep_us(1);
        gpio_put(SS_CLK_PIN, 1);
        sleep_us(1);
    }
}

bool block_io_is_complete()
{
    return is_complete;
}

bool block_io_is_corrupted()
{
    return is_corrupted;
}

void block_io_set_led_mode(led_mode_t mode)
{
    led_mode = mode;
}

void block_io_set_target_block(size_t grid_tile, size_t height, uint8_t block_data)
{
    if (grid_tile < BLOCK_IO_TILE_COUNT && height < HEIGHT_LIMIT)
    {
        target_structure[grid_tile][height] = block_data;
    }
}

void block_io_update()
{
    // Shift in a single '0' into the SS shift register:
    gpio_put(SS_DATA_PIN, 0);
    gpio_put(SS_CLK_PIN, 0);
    sleep_us(1);
    gpio_put(SS_CLK_PIN, 1);
    sleep_us(1);
    gpio_put(SS_DATA_PIN, 1);
    
    is_complete = true;
    is_corrupted = false;

    // For every tile in grid:
    for(int grid_tile = 0; grid_tile < BLOCK_IO_TILE_COUNT; grid_tile++)
    {
        uint8_t read_buffer;
        uint8_t write_buffer = 0x00; // first byte will be the zero byte
        uint8_t previous_rotation = 0x00; // base tile has a rotation of 0
        uint8_t height;

        // Shift SS to next slave:
        gpio_put(SS_CLK_PIN, 0);
        sleep_us(1);
        gpio_put(SS_CLK_PIN, 1);
        sleep_us(1);

        // Process and shift blocks:
        for (height = 0; height < HEIGHT_LIMIT; height++)
        {
            spi_write_read_blocking(spi0, &write_buffer, &read_buffer, 1);

            // Once we've read all the blocks and gotten to the top of the stack, the next
            // thing we'll read is the null block that we sent at the beginning. Exit the loop.
            if (read_buffer == 0x00)
            {
                grid_height[grid_tile] = height;

                // Check whether this is the top block in the target_structure too:
                if (target_structure[grid_tile][height] != 0x00)
                {
                    is_complete = false;
                }

                break;
            }

            // Decode block data:
            uint8_t block_id = read_buffer & 0xFC;
            uint8_t relative_rotation = read_buffer & 0x03;
            uint8_t absolute_rotation = (relative_rotation + previous_rotation) & 0x03;
            uint8_t rotation_mask = ((read_buffer >> 2) & 0x03);
            uint8_t block_mask = 0xFC | rotation_mask;
            uint8_t absolute_block = block_id | absolute_rotation;
            uint8_t target_block = target_structure[grid_tile][height];
            bool is_correct = (absolute_block & block_mask) == (target_block & block_mask);
            previous_rotation = absolute_rotation;

            if (!is_correct)
            {
                is_complete = false;
            }

            // Save block to memory:
            current_structure[grid_tile][height] = absolute_block;

            // LED data generation:
            switch(led_mode)
            {
                case TARGET:
                    if (is_correct)
                    {
                        write_buffer = 0x02; // green LED
                    }
                    else
                    {
                        write_buffer = 0x01; // red LED
                    }
                    break;
                case GREEN:
                    write_buffer = 0x02; // green LED
                    break;
                case RED:
                    write_buffer = 0x01; // red LED
                    break;
                case OFF: // fallthrough
                default:
                    write_buffer = 0x00; // no LED
                    break;
            }
        } // end of block stack for-loop

        // If we reached the height limit without reading a zero byte, read one more
        // byte to see if we get the zero byte. If not, then the stack is either higher
        // than the limit, or the data has been corrupted and we missed the zero byte.
        // We will assume that the data was corrupted.
        if (height == HEIGHT_LIMIT)
        {
            spi_write_read_blocking(spi0, &write_buffer, &read_buffer, 1);
            
            if (read_buffer != 0x00)
            {
                grid_height[grid_tile] = 0;
                is_complete = false;
                is_corrupted = true;
            }
        }
    } // end of grid tile for-loop
}