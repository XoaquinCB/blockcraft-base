#include "audio.h"
#include "block_io.h"
#include "bt_commands.h"
#include "bt_serial.h"
#include "pico/time.h"

int main()
{
    audio_init();
    block_io_init();
    bt_serial_init();
    
    while (1)
    {
        bt_commands_update_rx();
        block_io_update();
        bt_commands_send_current_structure();

        // Update roughly 50 times per second:
        sleep_ms(20);
    }
}