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
#include "usb_descriptor.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/usbd.h>

static void usb_set_config(usbd_device *usbd_dev, uint16_t wValue);
static void usb_data_received(usbd_device *usbd_dev, uint8_t ep);

usbd_device *usb_device;
uint8_t usbd_control_buffer[256];

void init()
{
    // Enable required clocks
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOC);

    // Initialize LED
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
    gpio_set(GPIOC, GPIO13);

    // Initialize systick services
    systick_init();
}

void usb_init()
{
    // enable USB clock
    rcc_periph_clock_enable(RCC_USB);

    // reset USB peripheral
    rcc_periph_reset_pulse(RST_USB);

    // Pull USB D+ (A12) low for 80ms to trigger device reenumeration
    // (hack for boards without proper USB pull up control)
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);
    gpio_clear(GPIOA, GPIO12);
    delay(80);

    // create USB device
    usb_device = usbd_init(&st_usbfs_v1_usb_driver, &usb_device_desc, usb_config_desc,
                           usb_desc_strings, sizeof(usb_desc_strings) / sizeof(usb_desc_strings[0]),
                           usbd_control_buffer, sizeof(usbd_control_buffer));

    // Set callback for config calls
    usbd_register_set_config_callback(usb_device, usb_set_config);

    // Enable interrupt
    nvic_set_priority(NVIC_USB_LP_CAN_RX0_IRQ, 2 << 6);
    nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
}

// Called when the host connects to the device and selects a configuration
void usb_set_config(usbd_device *usbd_dev, __attribute__((unused)) uint16_t wValue)
{
    usbd_ep_setup(usbd_dev, EP_COMM_OUT, USB_ENDPOINT_ATTR_INTERRUPT, MAX_PACKET_SIZE, usb_data_received);
}

// Called when data has been received
void usb_data_received(usbd_device *usbd_dev, uint8_t ep)
{
    uint8_t buf[MAX_PACKET_SIZE];
    uint16_t len = usbd_ep_read_packet(usbd_dev, ep, buf, sizeof(buf));

    if (len == 2 && buf[0] == 1)
    {
        uint8_t led_value = buf[1];
        if (led_value)
            gpio_clear(GPIOC, GPIO13);
        else
            gpio_set(GPIOC, GPIO13);
    }
}

int main()
{
    init();
    usb_init();

    while (true)
        ; // do nothing; all the action is in interrupt handlers
}

// USB interrupt handler
extern "C" void usb_lp_can_rx0_isr()
{
    usbd_poll(usb_device);
}
