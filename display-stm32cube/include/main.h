/*
 * USB Tutorial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * Common pin / peripheral definitions
 */

#ifndef MAIN_H
#define MAIN_H

#include "usbd_def.h"
#include "stm32f1xx_hal.h"
#include <stdbool.h>

void Error_Handler();

/* Called when data has been received.
 * Returns if USB should continue to receive data on this endpoint. */
bool usb_data_received(uint8_t* buf, int len);

/* Continue receiving data (if it has been stopped previously). */
void usb_continue_rx(USBD_HandleTypeDef *pdev);

#endif
