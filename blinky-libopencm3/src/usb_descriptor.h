/*
 * USB Tutorial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * USB descriptor
 */

#ifndef USB_DESCRIPTOR_H
#define USB_DESCRIPTOR_H

#define MAX_PACKET_SIZE 16

// Endpoint number for commands from host to device
#define EP_COMM_OUT 0x01

// Interface index
#define INTF_COMM 0

// USB descriptor string table
extern const char *usb_desc_strings[4];
// USB device descriptor
extern const struct usb_device_descriptor usb_device_desc;
// USB device configurations
extern const struct usb_config_descriptor usb_config_desc[];

#endif
