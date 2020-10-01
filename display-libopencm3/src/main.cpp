/*
 * USB Tutorial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * Main program
 */

#include "circ_buf.h"
#include "common.h"
#include "display.h"
#include "usb_descriptor.h"
#include "wcid.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/usbd.h>
#include <algorithm>
#include <string.h>

static void usb_set_config(usbd_device *usbd_dev, uint16_t wValue);
static void usb_data_received(usbd_device *usbd_dev, uint8_t ep);
static void usb_update_nak();

// USB device instance
static usbd_device *usb_device;

// buffer for control requests
static uint8_t usbd_control_buffer[256];

// Circular buffer for data
static circ_buf<1024> buffer;

// Minimum free space in circular buffer for requesting more packets
static constexpr int MIN_FREE_SPACE = 2 * BULK_MAX_PACKET_SIZE;

static constexpr int ROW_LEN = 256; /* 128 pixels x 2 byte = 256 bytes */

// indicates if the endpoint is forced to NAK to prevent receiving further data
static volatile bool is_forced_nak = false;

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
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);
    gpio_clear(GPIOA, GPIO12);
    delay(80);

    usb_init_serial_num();

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

    buffer.reset();
    is_forced_nak = false;
}

// Called when data has been received
void usb_data_received(__attribute__((unused)) usbd_device *usbd_dev, __attribute__((unused)) uint8_t ep)
{
    // Retrieve USB data (has side effect of setting endpoint to VALID)
    uint8_t packet[BULK_MAX_PACKET_SIZE] __attribute__((aligned(4)));
    int len = usbd_ep_read_packet(usb_device, EP_DATA_OUT, packet, sizeof(packet));

    // copy data into circular buffer
    buffer.add_data(packet, len);

    // check if there is space for less than 2 packets
    if (!is_forced_nak && buffer.avail_size() < MIN_FREE_SPACE)
    {
        // set endpoint from VALID to NAK
        usbd_ep_nak_set(usbd_dev, EP_DATA_OUT, 1);
        is_forced_nak = true;
    }
}

// Check if endpoints can be reset from NAK to VALID
void usb_update_nak()
{
    // can be set from NAK to VALID if there is space for 2 more packets
    if (is_forced_nak && buffer.avail_size() >= MIN_FREE_SPACE)
    {
        usbd_ep_nak_set(usb_device, EP_DATA_OUT, 0);
        is_forced_nak = false;
    }
}

int main()
{
    init();
    usb_init();
    display_init();

    int y = 0;

    while (true)
    {
        // check for sufficient data for entire row
        if (buffer.data_size() < ROW_LEN)
            continue;

        // draw pixel row
        uint8_t row[ROW_LEN];
        buffer.get_data(row, ROW_LEN);
        display_draw(0, y, 128, 1, row);

        y++;
        if (y == 160)
            y = 0;

        usb_update_nak();
    }
}

// USB interrupt handler
extern "C" void usb_lp_can_rx0_isr()
{
    usbd_poll(usb_device);
}
