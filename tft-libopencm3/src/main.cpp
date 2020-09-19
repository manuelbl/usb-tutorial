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

static void usb_set_config(usbd_device *usbd_dev, uint16_t wValue);
void usb_data_received(usbd_device *usbd_dev, uint8_t ep);

// USB device instance
usbd_device *usb_device;

// buffer for control requests
uint8_t usbd_control_buffer[256];

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
    // nothing to do
}

int main()
{
    init();
    usb_init();
    tft_init();

    while (true)
        ; // do nothing; all the action is in interrupt handlers
}

// USB interrupt handler
extern "C" void usb_lp_can_rx0_isr()
{
    usbd_poll(usb_device);
}
