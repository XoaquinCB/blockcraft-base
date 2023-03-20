#include "audio.h"
#include "block_io.h"
#include "bt_commands.h"
#include "bt_serial.h"
#include "pico/stdio_usb.h"
#include "pico/time.h"

int main()
{
    stdio_usb_init();

    audio_init();
    block_io_init();
    bt_serial_init();
    
    while (1)
    {
        bt_commands_update_rx();
        block_io_update();
        bt_commands_send_current_structure();

        // Update roughly 20 times per second:
        sleep_ms(50);
    }
}