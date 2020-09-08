/*
 * USB Tutorial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * Microsoft WCID descriptors
 */

#ifndef WCID_H
#define WCID_H

#include <libopencm3/usb/usbd.h>

// Register control request handlers for Microsoft WCID descriptors
void register_wcid_desc(usbd_device *usb_dev);

#endif
