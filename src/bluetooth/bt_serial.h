#ifndef BT_SERIAL_H
#define BT_SERIAL_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * Returns the number of bytes available for reading from the bluetooth serial port.
 */
size_t bt_serial_available();

/**
 * Initialises the bluetooth serial port.
 */
void bt_serial_init();

/**
 * Returns whether a bluetooth device is connected to the serial port.
 */
bool bt_serial_is_connected();

/**
 * Returns the next byte from the bluetooth serial port without removing it from the internal buffer. If
 * no bytes are available, an undefined value is returned. Successive calls to this function will return
 * the same value (if there is data available), as will the next call to 'bt_serial_read()'.
 */
uint8_t bt_serial_peek();

/**
 * Returns the next byte from the bluetooth serial port. If no bytes are available, an undefined value is
 * returned.
 */
uint8_t bt_serial_read();

/**
 * Reads multiple bytes from the bluetooth serial port into a buffer. The function stops once it has
 * read the given number of bytes in 'length' or if there are no more bytes available for reading.
 * Returns the number of bytes actually read.
 */
size_t bt_serial_read_multiple(uint8_t *buffer, size_t length);

/**
 * Writes a single byte to the bluetooth serial port.
 */
void bt_serial_write(uint8_t data);

/**
 * Writes multiple bytes to the bluetooth serial port.
 */
void bt_serial_write_multiple(uint8_t *buffer, size_t length);

#endif /* BT_SERIAL_H */