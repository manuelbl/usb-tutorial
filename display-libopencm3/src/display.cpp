/*
 * USB Tutorial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * Color TFT display (ST7735)
 */

#include "common.h"
#include "display.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>

#define MOSI_PIN GPIO7
#define SCK_PIN GPIO5
#define DC_PIN GPIO3
#define RESET_PIN GPIO2
#define CS_PIN GPIO1

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

static void reset();
static void init_seq();
static void send_cmd(uint8_t cmd, int len, const uint8_t *buf);

void display_init()
{
    // MOSI = PA7, SCK = PA5
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, MOSI_PIN | SCK_PIN);

    // DC = PA3, RESET = PA2, CS = PA1
    gpio_set(GPIOA, DC_PIN | RESET_PIN | CS_PIN);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, DC_PIN | RESET_PIN | CS_PIN);

    // Initialize SPI
    spi_reset(SPI1);
    spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_32, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
    spi_enable_software_slave_management(SPI1);
    spi_set_nss_high(SPI1);
    spi_enable(SPI1);

    reset();
    init_seq();
}

void reset()
{
    gpio_clear(GPIOA, CS_PIN);
    delay(500);
    gpio_clear(GPIOA, RESET_PIN);
    delay(500);
    gpio_set(GPIOA, RESET_PIN);
    delay(500);
    gpio_set(GPIOA, CS_PIN);
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
            delay(val);
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
    gpio_clear(GPIOA, DC_PIN);

    // select chip
    gpio_clear(GPIOA, CS_PIN);

    // send command
    spi_xfer(SPI1, cmd);

    // select data mode
    gpio_set(GPIOA, DC_PIN);

    // send data
    for (int i = 0; i < len; i++)
    {
        spi_xfer(SPI1, buf[i]);
    }

    // deselect chip
    gpio_set(GPIOA, CS_PIN);
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
