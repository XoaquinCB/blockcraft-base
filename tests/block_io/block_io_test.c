#include "block_io.h"
#include "pico/stdio_usb.h"
#include "pico/printf.h"
#include "pico/time.h"

unsigned int count = 0;

void dump_block_structure()
{
    printf("Scan number %u", count++);

    if (block_io_is_corrupted())
    {
        printf(" (corrupted)");
    }
    printf("\n");

    for (size_t grid_tile = 0; grid_tile < BLOCK_IO_TILE_COUNT; grid_tile++)
    {
        printf("Tile %2u: ", grid_tile + 1);

        size_t stack_height = block_io_get_stack_height(grid_tile);

        for (size_t height = 0; height < stack_height; height++)
        {
            uint8_t block_data = block_io_get_block(grid_tile, height);
            uint8_t block_id = block_data >> 2;
            uint8_t block_rot = block_data & 0x03;
            printf("[%02x:%1u] ", block_id, block_rot);
        }

        printf("\n");
    }
    if (block_io_is_complete())
    {
        printf("Matches target structure.\n\n");
    }
    else
    {
        printf("Doesn't match target structure.\n\n");
    }
}

int main()
{
    stdio_usb_init();
    sleep_ms(5000);
    
    printf("Starting...\n");
    
    block_io_init();

    // Set an arbitrary target structure:
    block_io_clear_target_structure();
    block_io_set_target_block(0, 0, 13);
    block_io_set_target_block(1, 0, 5);
    
    printf("Ready\n");

    while (1)
    {
        block_io_update();
        dump_block_structure();
        sleep_ms(100);
    }
}