/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * UART interface
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdlib.h>

#define UART_TX_BUF_LEN 1024

enum class uart_state
{
    ready,
    transmitting,
    receiving
};


/**
 * @brief UART implementation
 */
class uart_impl
{
public:
    /// Initialize UART
    void init();

    void print(const char *str);
    void printf(const char *fmt, ...);
    void print_hex(const uint8_t *data, size_t len, bool crlf);


    /**
     * @brief Submits the specified data for transmission.
     * 
     * The specified data is added to the transmission
     * buffer and transmitted asynchronously.
     * 
     * @param data pointer to byte array
     * @param len length of byte array
     */
    void transmit(const uint8_t *data, size_t len);

    /**
     * @brief Returns the available space in the transmit buffer
     * 
     * @return space, in number of bytes
     */
    size_t tx_data_avail();

    /// Called when a chunk of data has been transmitted
    void on_tx_complete();

    /**
     * @brief Sets the baud rate.
     * 
     * @param baudrate baud rate, in bps
     */ 
    void set_baudrate(int baudrate);

    /**
     * @brief Gets the baud rate.
     * 
     * @return baud rate, in bps
     */
    int baudrate() { return _baudrate; }

private:
    void start_transmit();

    // Buffer for data to be transmitted via UART
    //  *  0 <= head < buf_len
    //  *  0 <= tail < buf_len
    //  *  head == tail => empty
    //  *  head + 1 == tail => full (modulo UART_TX_BUF_LEN)
    // `tx_buf_head` points to the positions where the next character
    // should be inserted. `tx_buf_tail` points to the character after
    // the last character that has been transmitted.
    uint8_t tx_buf[UART_TX_BUF_LEN];
    volatile int tx_buf_head;
    volatile int tx_buf_tail;

    // The number of bytes currently being transmitted
    volatile int tx_size;

    int _baudrate;

    volatile uart_state tx_state;
};

/// Global UART instance
extern uart_impl uart;

#endif
