#include "audio.h"
#include "pico/printf.h"
#include "pico/stdio_usb.h"
#include "pico/time.h"

int main()
{
    stdio_usb_init();

    sleep_ms(5000); // give user some time to open a serial console

    printf("Starting...\n");

    audio_init();

    while (1)
    {
        audio_play_sound(0);
        sleep_ms(5000);
        audio_play_sound(1);
        sleep_ms(5000);
    }
}