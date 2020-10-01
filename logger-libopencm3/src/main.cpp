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
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/usbd.h>

#define SAMPLES_PER_MESSAGE 10
static void usb_set_config(usbd_device *usbd_dev, uint16_t wValue);
static void usb_data_transmitted(usbd_device *usbd_dev, uint8_t ep);

usbd_device *usb_device;
uint8_t usbd_control_buffer[256];
int num_samples = 0;
uint16_t samples[SAMPLES_PER_MESSAGE];
bool is_configured = false;

void init()
{
    // Enable required clocks
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
    rcc_periph_clock_enable(RCC_GPIOA);

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

void adc_init()
{
    rcc_periph_clock_enable(RCC_ADC1);
    adc_power_off(ADC1);

    // configure for regular single conversion
    adc_disable_scan_mode(ADC1);
    adc_set_single_conversion_mode(ADC1);
    adc_disable_external_trigger_regular(ADC1);
    adc_set_right_aligned(ADC1);

    // power up
    adc_power_on(ADC1);
    delay(100);
    adc_reset_calibration(ADC1);
    adc_calibrate(ADC1);

    // configure A0 as ADC channel
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, 0);
    uint8_t channels[] = {ADC_CHANNEL0};
    adc_set_regular_sequence(ADC1, 1, channels);
}

// Called when the host connects to the device and selects a configuration
void usb_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
    usbd_ep_setup(usbd_dev, EP_DATA_IN, USB_ENDPOINT_ATTR_BULK, BULK_MAX_PACKET_SIZE, usb_data_transmitted);
    register_wcid_desc(usb_device);
    is_configured = wValue != 0;
}

// Sends a package of samples
static void send_samples()
{
    if (!is_configured)
        return;
    usbd_ep_write_packet(usb_device, EP_DATA_IN, &samples, sizeof(samples));
}

// Takes a single sample
static void adc_once()
{
    adc_start_conversion_direct(ADC1);
    while (!(adc_eoc(ADC1)))
        ;
    uint32_t value = adc_read_regular(ADC1);

    samples[num_samples++] = (uint16_t)value;
    if (num_samples == SAMPLES_PER_MESSAGE) {
        send_samples();
        num_samples = 0;
    }
}

// Called when data has been transmitted
void usb_data_transmitted(__attribute__((unused)) usbd_device *usbd_dev, __attribute__((unused)) uint8_t ep)
{
    // nothing to do
}

int main()
{
    init();
    usb_init();
    adc_init();

    uint32_t next_time = millis() + 1;
    while (true)
    {
        if (next_time != millis())
            continue;

        adc_once();
        next_time += 10; // next sampling in 10ms
    }
}

// USB interrupt handler
extern "C" void usb_lp_can_rx0_isr()
{
    usbd_poll(usb_device);
}
