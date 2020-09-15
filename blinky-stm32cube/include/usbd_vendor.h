/*
 * USB Tutorial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * Vendor specific interface
 */

#ifndef USB_VENDOR_H
#define USB_VENDOR_H

#include "usbd_ioreq.h"

#define LED_CONTROL_ID 0x33

extern USBD_ClassTypeDef USBD_Vendor_Class;

#endif
