/*
 * USB Tutorial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * Main program
 */

#include "common.h"
#include "tft.h"
#include "usb_descriptor.h"
#include "wcid.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/usbd.h>
#include <algorithm>
#include <string.h>

static void usb_set_config(usbd_device *usbd_dev, uint16_t wValue);
void usb_data_received(usbd_device *usbd_dev, uint8_t ep);
bool circ_buf_has_data(int len);
bool circ_buf_has_avail(int len);
void usb_copy_received_packet();

// USB device instance
usbd_device *usb_device;

// buffer for control requests
uint8_t usbd_control_buffer[256];

// Circular buffer for data:
#define ROW_LEN 256 // 128 pixels x 2 byte = 256 bytes
#define NUM_ROWS 4
#define BUF_SIZE (ROW_LEN * NUM_ROWS)

uint8_t pixel_buf[BUF_SIZE];

// 0 <= head < BUF_SIZE
// 0 <= tail < BUF_SIZE
// head == tail: buffer is empty
// Therefore, the buffer must never be filled completely.
volatile int buf_head = 0; // updated by USB callbacks
volatile int buf_tail = 0; // updated by TFT display functions

// indicates if the endpoint is forced to NAK to prevent receiving further data
volatile bool forced_nak = false;

void init()
{
    // Enable required clocks
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_AFIO);
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_USB);

    // Initialize systick services
    systick_init();
}

void usb_init()
{
    // reset USB peripheral
    rcc_periph_reset_pulse(RST_USB);

    // Pull USB D+ (A12) low for 80ms to trigger device reenumeration
    // (hack for boards without proper USB pull up control)
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);
    gpio_clear(GPIOA, GPIO12);
    delay(80);

    // create USB device
    usb_device = usbd_init(&st_usbfs_v1_usb_driver, &usb_device_desc, usb_config_descs,
                           usb_desc_strings, sizeof(usb_desc_strings) / sizeof(usb_desc_strings[0]),
                           usbd_control_buffer, sizeof(usbd_control_buffer));

    // Set callback for config calls
    usbd_register_set_config_callback(usb_device, usb_set_config);
    register_wcid_desc(usb_device);

    // Enable interrupt
    nvic_set_priority(NVIC_USB_LP_CAN_RX0_IRQ, 2 << 6);
    nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
}

// Called when the host connects to the device and selects a configuration
void usb_set_config(usbd_device *usbd_dev, __attribute__((unused)) uint16_t wValue)
{
    register_wcid_desc(usbd_dev);

    usbd_ep_setup(usbd_dev, EP_DATA_OUT, USB_ENDPOINT_ATTR_BULK, BULK_MAX_PACKET_SIZE, usb_data_received);
    register_wcid_desc(usb_device);
}

// Called when data has been received
void usb_data_received(__attribute__((unused)) usbd_device *usbd_dev, __attribute__((unused)) uint8_t ep)
{
    // copy data into circular buffer
    usb_copy_received_packet();

    if (!forced_nak && !circ_buf_has_avail(2 * BULK_MAX_PACKET_SIZE))
    {
        usbd_ep_nak_set(usbd_dev, EP_DATA_OUT, 1);
        forced_nak = true;
    }
}

void usb_update_nak()
{
    if (forced_nak && circ_buf_has_avail(2 * BULK_MAX_PACKET_SIZE))
    {
        usbd_ep_nak_set(usb_device, EP_DATA_OUT, 0);
        forced_nak = false;
    }
}

// Copy packet received via USB into circular buffer
void usb_copy_received_packet()
{
    // Retrieve USB data
    uint8_t packet[BULK_MAX_PACKET_SIZE] __attribute__((aligned(4)));
    int len = usbd_ep_read_packet(usb_device, EP_DATA_OUT, packet, sizeof(packet));

    int head = buf_head;

    // copy first part (from head to end of circular buffer)
    int n = std::min(len, BUF_SIZE - head);
    memcpy(pixel_buf + head, packet, n);

    // copy second part if needed (to start of circular buffer)
    if (n < len)
        memcpy(pixel_buf, packet + n, len - n);

    // update head
    head += len;
    if (head >= BUF_SIZE)
        head -= BUF_SIZE;
    buf_head = head;
}

// Returns if there are at least `len` bytes of data in buffer
bool circ_buf_has_data(int len)
{
    int head = buf_head;
    int tail = buf_tail;

    if (head >= tail)
    {
        return (head - tail) >= len;
    }
    else
    {
        return (BUF_SIZE - (tail - head)) >= len;
    }
}

// Returns if there is space of at least `len` bytes to add new data
bool circ_buf_has_avail(int len)
{
    int head = buf_head;
    int tail = buf_tail;

    if (head >= tail)
    {
        return (BUF_SIZE - (head - tail)) > len;
    }
    else
    {
        return (tail - head) > len;
    }
}

int main()
{
    init();
    usb_init();
    tft_init();

    int y = 0;

    while (true)
    {
        // check for sufficient data for entire line
        if (!circ_buf_has_data(ROW_LEN))
            continue;

        // draw line
        tft_draw(0, y, 128, 1, pixel_buf + buf_tail);

        y++;
        if (y == 160)
            y = 0;

        // update tail of circular buffer
        int tail = buf_tail + ROW_LEN;
        if (tail >= BUF_SIZE)
            tail -= BUF_SIZE;
        buf_tail = tail;

        usb_update_nak();
    }
}

// USB interrupt handler
extern "C" void usb_lp_can_rx0_isr()
{
    usbd_poll(usb_device);
}
