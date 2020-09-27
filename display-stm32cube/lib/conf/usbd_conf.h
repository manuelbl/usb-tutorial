/*
 * USB Tutorial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * USB device configurtion
 * 
 * Located in 'conf' directory so it's
 * available to the usb_core library as well.
 */

#ifndef __USBD_CONF__H__
#define __USBD_CONF__H__

#include <stdio.h>
#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"

/* Max num interfaces: declare as 4 as Microsoft WCID will use index 4 */
#define USBD_MAX_NUM_INTERFACES 4
#define USBD_MAX_NUM_CONFIGURATION 1
#define USBD_MAX_STR_DESC_SIZ 512
#define USBD_DEBUG_LEVEL 0
#define USBD_SELF_POWERED 1
#define USBD_SUPPORT_USER_STRING_DESC 1

#define DEVICE_INDEX 0

/* DEBUG macros */

#if (USBD_DEBUG_LEVEL > 0)
#define USBD_UsrLog(...) \
    printf(__VA_ARGS__); \
    printf("\n");
#else
#define USBD_UsrLog(...)
#endif

#if (USBD_DEBUG_LEVEL > 1)

#define USBD_ErrLog(...) \
    printf("ERROR: ");   \
    printf(__VA_ARGS__); \
    printf("\n");
#else
#define USBD_ErrLog(...)
#endif

#if (USBD_DEBUG_LEVEL > 2)
#define USBD_DbgLog(...) \
    printf("DEBUG : ");  \
    printf(__VA_ARGS__); \
    printf("\n");
#else
#define USBD_DbgLog(...)
#endif

#endif
