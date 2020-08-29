#
# USB Tutorial
# 
# Copyright (c) 2020 Manuel Bleichenbacher
# Licensed under MIT License
# https://opensource.org/licenses/MIT
# 
# Blink LED (from host)
#

import usb.core
import time

COMM_EP = 1

# find device
dev = usb.core.find(idVendor=0xcafe, idProduct=0xcafe)
if dev is None:
    raise ValueError('Device not found')

# set configuration
dev.set_configuration()

led_on = False

while True:
    led_on = not led_on
    dev.ctrl_transfer(0x41, 0x33, int(led_on), 1)
    time.sleep(0.6)
