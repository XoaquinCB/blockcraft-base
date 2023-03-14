#ifndef BLOCK_IO_H
#define BLOCK_IO_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define BLOCK_IO_TILE_COUNT 9

typedef enum { TARGET, GREEN, RED, OFF } led_mode_t;

/**
 * Removes all the blocks from the target structure. Call this before setting a new target
 * structure with block_io_set_target_block().
 */
void block_io_clear_target_structure();

/**
 * Returns the data of a block at a given location of actual structure.
 */
uint8_t block_io_get_block(size_t grid_tile, size_t height);

/**
 * Return the height of a stack on a given tile of the actual structure, not the target.
 */
size_t block_io_get_stack_height(size_t grid_tile);

/**
 * Initialise block I/O module.
 */
void block_io_init();

/**
 * Returns whether the current structure matches the target.
 */
bool block_io_is_complete();

/**
 * Returns whether the current structure is corrupted. If it is, then some of the data given
 * by block_io_get_block(), block_io_get_stack_height() and block_io_is_complete() may be wrong.
 * Another call to block_io_update will read the structure again and should fix the corruption.
 */
bool block_io_is_corrupted();

/**
 * Sets the block LED mode.
 */
void block_io_set_led_mode(led_mode_t mode);

/**
 * Sets a single block in the target structure. Use block_io_clear_target_structure() first when
 * starting a new target structure.
 */
void block_io_set_target_block(size_t grid_tile, size_t height, uint8_t block_data);

/**
 * Reads entire grid and sends appropriate LED data.
 */
void block_io_update();

#endif /* BLOCK_IO_H */