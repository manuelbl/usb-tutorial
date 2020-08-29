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
static usbd_request_return_codes led_control(usbd_device *usbd_dev, usb_setup_data *req,
                                             uint8_t **buf, uint16_t *len,
                                             usbd_control_complete_callback *complete);

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
    register_wcid_desc(usb_device);

    // Enable interrupt
    nvic_set_priority(NVIC_USB_LP_CAN_RX0_IRQ, 2 << 6);
    nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
}

// Called when the host connects to the device and selects a configuration
void usb_set_config(usbd_device *usbd_dev, __attribute__((unused)) uint16_t wValue)
{
    register_wcid_desc(usbd_dev);
    usbd_register_control_callback(usbd_dev,
                                   USB_REQ_TYPE_VENDOR, USB_REQ_TYPE_TYPE,
                                   led_control);
}

// Called when a vendor request has been received
usbd_request_return_codes led_control(__attribute__((unused)) usbd_device *usbd_dev, usb_setup_data *req,
                                      uint8_t **buf, uint16_t *len,
                                      __attribute__((unused)) usbd_control_complete_callback *complete)
{
    if (req->bRequest == LED_VENDOR_ID && req->wIndex == 1)
    {
        if (req->wValue == 0)
            gpio_clear(GPIOC, GPIO13);
        else
            gpio_set(GPIOC, GPIO13);

        *buf = nullptr;
        *len = 0;

        return USBD_REQ_HANDLED;
    }

    return USBD_REQ_NEXT_CALLBACK;
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
