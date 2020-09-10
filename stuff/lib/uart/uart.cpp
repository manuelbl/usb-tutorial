/*
 * USB Serial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * UART implementation
 */

#include "uart.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <string.h>
#include <algorithm>
#include <cstdarg>
#include <stdio.h>

uart_impl uart;

static char format_buf[128];

static const char *HEX_DIGITS = "0123456789ABCDEF";

void uart_impl::print(const char *str)
{
    transmit((const uint8_t *)str, strlen(str));
}

void uart_impl::printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(format_buf, sizeof(format_buf), fmt, args);
    va_end(args);
    print(format_buf);
}

void uart_impl::print_hex(const uint8_t *data, size_t len, _Bool crlf)
{
    while (len > 0)
    {
        char *p = format_buf;
        char *end = format_buf + sizeof(format_buf);

        while (len > 0 && p + 4 <= end)
        {
            uint8_t byte = *data++;
            *p++ = HEX_DIGITS[byte >> 4U];
            *p++ = HEX_DIGITS[byte & 0xfU];
            *p++ = ' ';
            len--;
        }

        if (len == 0 && crlf)
        {
            p[-1] = '\r';
            *p++ = '\n';
        }

        transmit((const uint8_t *)format_buf, p - format_buf);
    }
}

void uart_impl::init()
{
    tx_state = uart_state::ready;
    tx_buf_head = tx_buf_tail = 0;
    tx_size = 0;

    // Enable USART2 interface clock
    rcc_periph_clock_enable(RCC_USART2);

    // Enable TX, RX pin clock
    rcc_periph_clock_enable(RCC_GPIOA);

    // Configure RX/TXpins
    gpio_set(GPIOA, GPIO2);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO2);

    // configure TX DMA
    rcc_periph_clock_enable(RCC_DMA1);
    dma_channel_reset(DMA1, 7);
    dma_set_peripheral_address(DMA1, 7, (uint32_t)&USART2_DR);
    dma_set_read_from_memory(DMA1, 7);
    dma_enable_memory_increment_mode(DMA1, 7);
    dma_set_memory_size(DMA1, 7, DMA_CCR_MSIZE_8BIT);
    dma_set_peripheral_size(DMA1, 7, DMA_CCR_MSIZE_8BIT);
    dma_set_priority(DMA1, 7, DMA_CCR_PL_MEDIUM);
    dma_enable_transfer_complete_interrupt(DMA1, 7);

    // enable DMA interrupt (notifying about a completed transmission)
    nvic_set_priority(NVIC_DMA1_CHANNEL7_IRQ, 2 << 6);
    nvic_enable_irq(NVIC_DMA1_CHANNEL7_IRQ);

    // configure baud rate etc.
    set_baudrate(9600);
    usart_set_mode(USART2, USART_MODE_TX);
    usart_enable(USART2);
}

void uart_impl::transmit(const uint8_t *data, size_t len)
{
    size_t size;

    {
        //irq_guard guard;

        int buf_tail = tx_buf_tail;
        int buf_head = tx_buf_head;

        size_t avail_chunk_size;
        if (buf_head < buf_tail)
            avail_chunk_size = buf_tail - buf_head - 1;
        else if (buf_tail != 0)
            avail_chunk_size = UART_TX_BUF_LEN - buf_head;
        else
            avail_chunk_size = UART_TX_BUF_LEN - 1 - buf_head;

        if (avail_chunk_size == 0)
            return; // buffer full - discard data

        // Copy data to transmit buffer
        size = std::min(len, avail_chunk_size);
        memcpy(tx_buf + buf_head, data, size);
        buf_head += size;
        if (buf_head >= UART_TX_BUF_LEN)
            buf_head = 0;
        tx_buf_head = buf_head;
    }

    // start transmission
    start_transmit();

    // Use second transmit in case of remaining data
    // (usually because of wrap around at the end of the buffer)
    if (size < len)
        transmit(data + size, len - size);
}

void uart_impl::start_transmit()
{
    int start_pos;

    {
        //irq_guard guard;

        if (tx_state != uart_state::ready || tx_buf_head == tx_buf_tail)
            return; // UART busy or queue empty

        // Determine TX chunk size
        start_pos = tx_buf_tail;
        int end_pos = tx_buf_head;
        if (end_pos <= start_pos)
            end_pos = UART_TX_BUF_LEN;
        tx_size = end_pos - start_pos;
        if (tx_size > 32)
            tx_size = 32; // no more than 32 bytes to free up space soon
        tx_state = uart_state::transmitting;

        // set transmit chunk
        dma_set_memory_address(DMA1, 7, (uint32_t)(tx_buf + start_pos));
        dma_set_number_of_data(DMA1, 7, tx_size);
    }

    // start transmission
    dma_enable_channel(DMA1, 7);
    usart_enable_tx_dma(USART2);
}

void uart_impl::on_tx_complete()
{
    if (!dma_get_interrupt_flag(DMA1, 7, DMA_TCIF))
        return;

    {
        //irq_guard guard;

        // Update TX buffer
        int buf_tail = tx_buf_tail + tx_size;
        if (buf_tail >= UART_TX_BUF_LEN)
            buf_tail = 0;
        tx_buf_tail = buf_tail;
        tx_size = 0;
        tx_state = uart_state::ready;
    }

    // Disable DMA
    dma_disable_channel(DMA1, 7);
    dma_clear_interrupt_flags(DMA1, 7, DMA_TCIF);

    // Check for next transmission
    start_transmit();
}

size_t uart_impl::tx_data_avail()
{
    int head = tx_buf_head;
    int tail = tx_buf_tail;
    if (head >= tail)
        return UART_TX_BUF_LEN - (head - tail) - 1;
    else
        return tail - head - 1;
}

void uart_impl::set_baudrate(int baudrate)
{
    _baudrate = baudrate;
    usart_disable(USART2);
    usart_set_baudrate(USART2, _baudrate);
    usart_set_databits(USART2, 8);
    usart_set_stopbits(USART2, USART_STOPBITS_1);
    usart_set_parity(USART2, USART_PARITY_NONE);
    usart_enable(USART2);
}

extern "C" void dma1_channel7_isr()
{
    if (dma_get_interrupt_flag(DMA1, 7, DMA_TCIF)) {
        uart.on_tx_complete();
    }
}
