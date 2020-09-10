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
import usb.util
import time

COMM_EP = 1

# find device
dev = usb.core.find(idVendor=0xcafe, idProduct=0xcafe)
if dev is None:
    raise ValueError('Device not found')

# set configuration
dev.set_configuration()

led_on = False

#while True:
#    led_on = not led_on
#    dev.write(COMM_EP, bytes([0x01, int(led_on)]))
#    time.sleep(0.6)

#print (dev)
#ret = usb.util.get_string(dev, 0xee)
#print (ret)
#ret = dev.ctrl_transfer(0x80, 0x06, 0x03ee, 1033, 255)
#print (ret)
ret = dev.ctrl_transfer(0x80, 0x06, 0x03ee, 0, 0x12)
print (ret)
ret = dev.ctrl_transfer(0xc0, 0x37, 0, 0x0004, 0x10)
print (ret)
ret = dev.ctrl_transfer(0xc0, 0x37, 0, 0x0004, 0x28)
print (ret)
ret = dev.ctrl_transfer(0xc1, 0x37, 0, 0x0004, 0x10)
print (ret)
ret = dev.ctrl_transfer(0xc1, 0x37, 0, 0x0004, 0x28)
print (ret)
