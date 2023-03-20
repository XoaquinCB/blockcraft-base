#include "hw_config.h"
#include "sd_card.h"
#include "spi.h"
#include <stdbool.h>
#include <stdint.h>

// Hardware Configuration of SPI "objects"
// Note: multiple SD cards can be driven by one SPI if they use different slave
// selects.
static spi_t spis[] = {  // One for each SPI.
    {
        .hw_inst = spi1,  // SPI component
        .miso_gpio = 12, // GPIO number (not pin number)
        .mosi_gpio = 11,
        .sck_gpio = 10,
        .baud_rate = 12500 * 1000, // 12.5 MHz
    }
};

// Hardware Configuration of the SD Card "objects"
static sd_card_t sd_cards[] = {  // One for each SD card
    {
        .pcName = "0:",   // Name used to mount device
        .spi = &spis[0],  // Pointer to the SPI driving this card
        .ss_gpio = 13,    // The SPI slave select GPIO for this SD card
        .use_card_detect = false,
    }
};

size_t sd_get_num()
{
    return count_of(sd_cards);
}

sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sd_get_num())
    {
        return &sd_cards[num];
    }
    else
    {
        return NULL;
    }
}

size_t spi_get_num()
{
    return count_of(spis);
}

spi_t *spi_get_by_num(size_t num) {
    if (num <= sd_get_num())
    {
        return &spis[num];
    }
    else
    {
        return NULL;
    }
}