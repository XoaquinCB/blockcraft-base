#include <stdbool.h>
#include "bt_serial.h"
#include "pico/stdio_usb.h"
#include "pico/printf.h"
#include "pico/time.h"

int main()
{
    bt_serial_init();
    stdio_usb_init();
    
    sleep_ms(5000); // give user some time to open a serial console
    
    while (true)
    {
        printf("Waiting for bluetooth device...\n");
        while (!bt_serial_is_connected());
        printf("Device connected\n");
        
        char str[] = "Hello world!\n\r";
        bt_serial_write_multiple(str, sizeof(str));
        
        while (bt_serial_is_connected())
        {
            while (bt_serial_available())
            {
                uint8_t data = bt_serial_read();
                bt_serial_write(data);
                printf("%c", data);
            }
        }
        
        printf("\nDevice disconnected\n");
    }
}