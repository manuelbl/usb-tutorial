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

#include "stm32f1xx_hal.h"

#define LED_Pin GPIO_PIN_13
#define LED_GPIO_Port GPIOC

void Error_Handler();

#endif
