/*
 * USB Tutorial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * Main program
 */

#include "main.h"
#include "display.h"
#include "usbd_desc.h"
#include "usbd_vendor.h"
#include <stdbool.h>
#include <string.h>

static void SystemClock_Config(void);
static void GPIO_Init(void);
static void USB_Device_Init(void);
static bool circ_buf_has_data(int len);
static bool circ_buf_has_avail(int len);
static void usb_check_stop();

USBD_HandleTypeDef USBD_Device;

/* Circular buffer for data: */
#define ROW_LEN 256 /* 128 pixels x 2 byte = 256 bytes */
#define NUM_ROWS 4
#define BUF_SIZE (ROW_LEN * NUM_ROWS)

uint8_t pixel_buf[BUF_SIZE];

/* 0 <= head < BUF_SIZE
 * 0 <= tail < BUF_SIZE
 * head == tail: buffer is empty
 * Therefore, the buffer must never be filled completely. */
volatile int buf_head = 0; /* updated by USB callbacks */
volatile int buf_tail = 0; /* updated by TFT display functions */

volatile bool is_rx_stopped = false;


int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    USB_Device_Init();
    display_init();

    int y = 0;

    while (1)
    {
        // check for sufficient data for entire line
        if (!circ_buf_has_data(ROW_LEN))
            continue;

        // draw line
        display_draw(0, y, 128, 1, pixel_buf + buf_tail);

        y++;
        if (y == 160)
            y = 0;

        // update tail of circular buffer
        int tail = buf_tail + ROW_LEN;
        if (tail >= BUF_SIZE)
            tail -= BUF_SIZE;
        buf_tail = tail;

        usb_check_stop();
    }
}

void usb_check_stop()
{
    if (is_rx_stopped && circ_buf_has_avail(DATA_PACKET_SIZE))
    {
        usb_continue_rx(&USBD_Device);
        is_rx_stopped = false;
    }
}

bool usb_data_received(uint8_t* buf, int len)
{
    int head = buf_head;

    // copy first part (from head to end of circular buffer)
    int n = len;
    if (n > BUF_SIZE - head)
        n = BUF_SIZE - head;
    memcpy(pixel_buf + head, buf, n);

    // copy second part if needed (to start of circular buffer)
    if (n < len)
        memcpy(pixel_buf, buf + n, len - n);

    // update head
    head += len;
    if (head >= BUF_SIZE)
        head -= BUF_SIZE;
    buf_head = head;

    bool has_space = circ_buf_has_avail(DATA_PACKET_SIZE);
    if (!has_space)
        is_rx_stopped = true;
    return has_space;
}

void SystemClock_Config(void)
{
    /* External 8 MHz crystal with 9x multiplier (72 MHz) */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* Divide clock by 2 for AHB and APB buses (36 MHz) */
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);

    /* Divide clock by 1.5 for USB (48 MHz) */
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
}

static void GPIO_Init(void)
{
    /* Enable GPIO Port Clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
}

void USB_Device_Init(void)
{
    /* Pull USB D+ (A12) low for 80ms to trigger device reenumeration */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_Delay(80);

    /* Init USB Device Library, add supported class and start the library. */
    USBD_Init(&USBD_Device, &USBD_Descriptors, DEVICE_INDEX);
    USBD_RegisterClass(&USBD_Device, &USBD_Vendor_Class);
    USBD_Start(&USBD_Device);
}

/* MSP initialization callback */
void HAL_MspInit(void)
{
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_AFIO_REMAP_SWJ_NOJTAG();
}

void Error_Handler(void)
{
}

/* Returns if there are at least `len` bytes of data in buffer */
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

/* Returns if there is space of at least `len` bytes to add new data */
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
