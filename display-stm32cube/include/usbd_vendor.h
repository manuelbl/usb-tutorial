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

#define DATA_OUT_EP 0x01U
#define DATA_PACKET_SIZE 64

extern USBD_ClassTypeDef USBD_Vendor_Class;

#endif
