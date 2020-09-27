#
# USB Tutorial
#
# Copyright (c) 2020 Manuel Bleichenbacher
# Licensed under MIT License
# https://opensource.org/licenses/MIT
#
# Display image on TFT color display
#

import usb.core
from PIL import Image

def rgb888_to_rgb565(r, g, b):
    """Converts a 8-bit RGB tuple to 16-bit RGB565 value"""
    w = (r & 0xf8) << 8
    w |= (g & 0xfc) << 3
    w |= b >> 3
    return w

def convert_rgb565(image):
    """Converts the RGB image into a byte array in RGB565 format"""
    data = bytearray(128 * 160 * 2)
    i = 0
    for (r, g, b) in image.getdata():
        w = rgb888_to_rgb565(r, g, b)
        data[i] = w >> 8
        i += 1
        data[i] = w & 0xff
        i += 1
    return data

DATA_EP = 1

# load image
im = Image.open("parrot.png")
pixels = convert_rgb565(im)

# find device
dev = usb.core.find(idVendor=0xcafe, idProduct=0xceaf)
if dev is None:
    raise ValueError('Device not found')

# set configuration
dev.set_configuration()

# send pixel data
dev.write(DATA_EP, pixels, 2000)
