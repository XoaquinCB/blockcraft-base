#ifndef BLOCK_IO
#define BLOCK_IO

#include <stdint.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/dma.h"

/**
 * Initialise spi and gpio pins
 */
void block_io_init();
/**
 * Reads entire grid and sends appropriate LED data
 */
void block_io_update();
/**
 * Set a single block in the target structure 
 */
void block_io_set_target_block(size_t grid_tile, size_t height, uint8_t block_data);
/**
 * Return the height a stack on a given tile of the real structure, not the target 
 */
size_t block_io_get_stack_height(size_t grid_tile);
/**
 * Returns the data of a block at a given location of actual structure 
 */
uint8_t block_io_get_block(size_t grid_tile, size_t height);
/**
 * Returns a boolean whether tracking whether the actual structure matches the target 
 */
bool block_io_complete();

#endif//BLOCK_IO