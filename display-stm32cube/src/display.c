/*
 * USB Tutorial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * Color TFT display (ST7735)
 */

#include "display.h"
#include "stm32f1xx_hal.h"

#define MOSI_PIN GPIO_PIN_7
#define SCK_PIN GPIO_PIN_5
#define DC_PIN GPIO_PIN_3
#define RESET_PIN GPIO_PIN_2
#define CS_PIN GPIO_PIN_1

#define CMD_NOP 0x00
#define CMD_SWRESET 0x01
#define CMD_RDDID 0x04
#define CMD_RDDST 0x09

#define CMD_SLPIN 0x10
#define CMD_SLPOUT 0x11
#define CMD_PTLON 0x12
#define CMD_NORON 0x13

#define CMD_INVOFF 0x20
#define CMD_INVON 0x21
#define CMD_DISPOFF 0x28
#define CMD_DISPON 0x29
#define CMD_CASET 0x2A
#define CMD_RASET 0x2B
#define CMD_RAMWR 0x2C
#define CMD_RAMRD 0x2E

#define CMD_PTLAR 0x30
#define CMD_COLMOD 0x3A
#define CMD_MADCTL 0x36

#define CMD_FRMCTR1 0xB1
#define CMD_FRMCTR2 0xB2
#define CMD_FRMCTR3 0xB3
#define CMD_INVCTR 0xB4
#define CMD_DISSET5 0xB6

#define CMD_PWCTR1 0xC0
#define CMD_PWCTR2 0xC1
#define CMD_PWCTR3 0xC2
#define CMD_PWCTR4 0xC3
#define CMD_PWCTR5 0xC4
#define CMD_VMCTR1 0xC5

#define CMD_RDID1 0xDA
#define CMD_RDID2 0xDB
#define CMD_RDID3 0xDC
#define CMD_RDID4 0xDD

#define CMD_PWCTR6 0xFC

#define CMD_GMCTRP1 0xE0
#define CMD_GMCTRN1 0xE1

#define CMD_SLEEP 0xFE
#define CMD_EOS 0xFF

static const uint8_t init_data[] = {
    CMD_SWRESET, 0,                                     /* software reset */
    CMD_SLEEP, 150,                                     /* sleep 150ms */
    CMD_SLPOUT, 0,                                      /* turn off sleep mode */
    CMD_SLEEP, 250,                                     /* sleep 250ms */
    CMD_SLEEP, 250,                                     /* sleep 250ms */
    CMD_FRMCTR1, 3, 0x01, 0x2C, 0x2D,                   /* frame rate control */
    CMD_FRMCTR2, 3, 0x01, 0x2C, 0x2D,                   /* frame rate control */
    CMD_FRMCTR3, 6, 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D, /* frame rate control */
    CMD_INVCTR, 1, 0x07,                                /* inversion control: no inversion */
    CMD_PWCTR1, 3, 0xA2, 0x02, 0x84,                    /* power control */
    CMD_PWCTR2, 1, 0xC5,                                /* power control */
    CMD_PWCTR3, 2, 0x0A, 0x00,                          /* power control */
    CMD_PWCTR4, 2, 0x8A, 0x2A,                          /* power control */
    CMD_PWCTR5, 2, 0x8A, 0xEE,                          /* power control */
    CMD_VMCTR1, 1, 0x0E,                                /* VCOM control */
    CMD_INVOFF, 0,                                      /* no display inversion */
    CMD_MADCTL, 1, 0xC8,                                /* memory read order*/
    CMD_COLMOD, 1, 0x05,                                /* color mode: 16 bit / pixel */
    CMD_CASET, 4, 0x00, 0x00, 0x00, 0x7F,               /* colum: 0 to 127 */
    CMD_RASET, 4, 0x00, 0x00, 0x00, 0x9F,               /* rows: 0 to 159 */
    CMD_GMCTRP1, 16, 0x02, 0x1C, 0x07, 0x12,            /* gamma correction */
    0x37, 0x32, 0x29, 0x2D,                             /* gamma correction (cont.) */
    0x29, 0x25, 0x2B, 0x39,                             /* gamma correction (cont.) */
    0x00, 0x01, 0x03, 0x10,                             /* gamma correction (cont.) */
    CMD_GMCTRN1, 16, 0x03, 0x1D, 0x07, 0x06,            /* gamma correction*/
    0x2E, 0x2C, 0x29, 0x2D,                             /* gamma correction (cont.) */
    0x2E, 0x2E, 0x37, 0x3F,                             /* gamma correction (cont.) */
    0x00, 0x00, 0x02, 0x10,                             /* gamma correction (cont.) */
    CMD_NORON, 0,                                       /* normal display on */
    CMD_SLEEP, 10,                                      /* sleep 10ms */
    CMD_DISPON, 0,                                      /* display on */
    CMD_SLEEP, 100,                                     /* sleep 100ms */
    CMD_EOS                                             /* end of sequence */
};

static SPI_HandleTypeDef hspi;

static void reset();
static void init_seq();
static void send_cmd(uint8_t cmd, int len, const uint8_t *buf);

void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(hspi->Instance==SPI1)
    {
        __HAL_RCC_SPI1_CLK_ENABLE();

        GPIO_InitStruct.Pin = SCK_PIN|MOSI_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

void display_init()
{
    // DC = PA3, RESET = PA2, CS = PA1
    HAL_GPIO_WritePin(GPIOA, DC_PIN | RESET_PIN | CS_PIN, GPIO_PIN_SET);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DC_PIN | RESET_PIN | CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hspi.Instance = SPI1;
    hspi.Init.Mode = SPI_MODE_MASTER;
    hspi.Init.Direction = SPI_DIRECTION_2LINES;
    hspi.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi.Init.NSS = SPI_NSS_SOFT;
    hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi.Init.CRCPolynomial = 10;
    HAL_SPI_Init(&hspi);

    reset();
    init_seq();
}

void reset()
{
    HAL_GPIO_WritePin(GPIOA, CS_PIN, GPIO_PIN_RESET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(GPIOA, RESET_PIN, GPIO_PIN_RESET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(GPIOA, RESET_PIN, GPIO_PIN_SET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(GPIOA, CS_PIN, GPIO_PIN_SET);
}

void init_seq()
{
    const uint8_t *p = init_data;
    uint8_t cmd = *p++;
    while (cmd != CMD_EOS)
    {
        uint8_t val = *p++;
        if (cmd == CMD_SLEEP)
        {
            HAL_Delay(val);
        }
        else
        {
            send_cmd(cmd, val, p);
            p += val;
        }
        cmd = *p++;
    }
}

void send_cmd(uint8_t cmd, int len, const uint8_t *buf)
{
    // select command mode
    HAL_GPIO_WritePin(GPIOA, DC_PIN, GPIO_PIN_RESET);

    // select chip
    HAL_GPIO_WritePin(GPIOA, CS_PIN, GPIO_PIN_RESET);

    // send command
    HAL_SPI_Transmit(&hspi, &cmd, 1, 100);

    // select data mode
    HAL_GPIO_WritePin(GPIOA, DC_PIN, GPIO_PIN_SET);

    // send data
    HAL_SPI_Transmit(&hspi, (uint8_t*)buf, len, 100);

    // deselect chip
    HAL_GPIO_WritePin(GPIOA, CS_PIN, GPIO_PIN_SET);
}

void set_address_window(int x, int y, int w, int h)
{
    uint8_t param[] = {0, 0, 0, 0};
    param[1] = x;
    param[3] = x + w - 1;
    send_cmd(CMD_CASET, 4, param);
    param[1] = y;
    param[3] = y + h - 1;
    send_cmd(CMD_RASET, 4, param);
}

void display_draw(int x, int y, int row_len, int num_rows, const uint8_t *pixels)
{
    set_address_window(x, y, row_len, num_rows);
    send_cmd(CMD_RAMWR, row_len * num_rows * 2, pixels);
}
