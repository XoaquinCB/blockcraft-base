#include "bt_serial.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "pico/util/queue.h"

#define RX_BUFFER_SIZE 1024
#define UART_ID        uart1
#define BAUD_RATE      9600
#define DATA_BITS      8
#define STOP_BITS      1
#define PARITY         UART_PARITY_NONE
#define UART_TX_PIN    8
#define UART_RX_PIN    9
#define BT_STATUS_PIN  7

static queue_t rx_buffer;

static void on_uart_rx()
{
    while (uart_is_readable(UART_ID))
    {
        uint8_t data = uart_getc(UART_ID);
        bool success = queue_try_add(&rx_buffer, &data);
        // Currently not implementing overflow detection, so 'success' is not used.
    }
}

size_t bt_serial_available()
{
    return queue_get_level(&rx_buffer);
}

void bt_serial_init()
{
    queue_init(&rx_buffer, sizeof(uint8_t), RX_BUFFER_SIZE);

    uart_init(UART_ID, BAUD_RATE);

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    uart_set_hw_flow(UART_ID, false, false);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    uart_set_fifo_enabled(UART_ID, true);
    uart_set_translate_crlf(UART_ID, false); // don't translate CR/LF since we're working with raw bytes

    const int UART_IRQ = (UART_ID == uart0) ? UART0_IRQ : UART1_IRQ;

    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);

    gpio_init(BT_STATUS_PIN); // set status pin as a normal input pin
}

bool bt_serial_is_connected()
{
    return gpio_get(BT_STATUS_PIN);
}

uint8_t bt_serial_peek()
{
    uint8_t data;
    queue_try_peek(&rx_buffer, &data);
    return data;
}

uint8_t bt_serial_read()
{
    uint8_t data;
    queue_try_remove(&rx_buffer, &data);
    return data;
}

size_t bt_serial_read_multiple(uint8_t *buffer, size_t length)
{
    size_t available = queue_get_level(&rx_buffer);
    size_t n_read = (available > length) ? length : available;

    for (size_t i = 0; i < n_read; i++)
    {
        queue_try_remove(&rx_buffer, &buffer[i]);
    }
    return n_read;
}

void bt_serial_write(uint8_t data)
{
    uart_putc_raw(UART_ID, data);
}

void bt_serial_write_multiple(uint8_t *buffer, size_t length)
{
    uart_write_blocking(UART_ID, buffer, length);
}