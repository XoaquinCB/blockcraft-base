#include "block_io.h"

#define BLOCK_IO_SPI_RX_PIN 16
#define BLOCK_IO_SPI_CS_PIN 17
#define BLOCK_IO_SPI_SCK_PIN 18
#define BLOCK_IO_SPI_TX_PIN 19
#define BLOCK_IO_SPI_SS_PIN 20

#define HEIGHT_LIMIT 7

static uint8_t target_structure[9][HEIGHT_LIMIT];
static uint8_t current_structure[9][HEIGHT_LIMIT];
static uint8_t led_override[9][HEIGHT_LIMIT];
static uint8_t grid_height[9];

static enum led_mode_t {TARGET, GREEN, RED, OFF} led_mode = TARGET;

static bool complete = false;

void block_io_init(){
    // Set baudrate
    spi_init(spi_default, 250000);

    //Set GPIO pins
    gpio_set_function(BLOCK_IO_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_init(BLOCK_IO_SPI_CS_PIN);
    gpio_set_function(BLOCK_IO_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(BLOCK_IO_SPI_TX_PIN, GPIO_FUNC_SPI);

    //Intialise the slave select register
    gpio_put(BLOCK_IO_SPI_CS_PIN, 1);
    for(int i=0;i<8;i++){
        gpio_put(BLOCK_IO_SPI_SCK_PIN, 1);
        gpio_put(BLOCK_IO_SPI_SCK_PIN, 0);
    }

} 

void block_io_set_target_block(size_t grid_tile, size_t height, uint8_t block_data){
    target_structure[grid_tile][height] = block_data;
}

size_t block_io_get_stack_height(size_t grid_tile){
    return grid_height[grid_tile];
}

uint8_t block_io_get_block(size_t grid_tile, size_t height){
    return current_structure[grid_tile][height];
}

void block_io_update(){
    //for every tile in grid
    for(int i=0;i<9;i++){
        complete = true;
        uint8_t block_counter = 0;
        uint8_t read_buffer;
        uint8_t write_buffer = 0x00; 
        uint8_t previous_rotation = 0x00;
        uint8_t current_rotation;
        uint8_t absolute_rotation;
        uint8_t rotation_mask;
        uint8_t data_mask = 0x15;
        bool correct_rotation, correct_data;
        //send initial null block to given tile stack
        spi_write_read_blocking(spi0, &write_buffer, &read_buffer, 1);
        while(read_buffer!=0x00){
            //Rotation
            current_rotation = read_buffer & 0x03;
            absolute_rotation = (current_rotation & previous_rotation) & 0x03;
            rotation_mask = (read_buffer >> 2) & 0x03;
            correct_rotation = (absolute_rotation & rotation_mask) == (target_structure[i][block_counter] & rotation_mask);
                
            //Data
            correct_data = (read_buffer >> 4) & data_mask;

            //LED data generation
            switch(led_mode){
                case 1: //TARGET
                    if (correct_data && correct_rotation){
                        write_buffer = 0x02;
                    } else {
                        write_buffer = 0x01;
                        complete = false;
                    }
                case 2: //GREEN
                    write_buffer = 0x02;
                case 3: //RED
                    write_buffer = 0x01;
                case 4: //OFF
                    write_buffer = 0x00;
            }

            block_counter++;
        }
        grid_height[i] = block_counter;
    }
}

bool block_io_complete(){
    return complete;
}
