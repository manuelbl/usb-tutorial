#
# USB Tutorial
# 
# Copyright (c) 2020 Manuel Bleichenbacher
# Licensed under MIT License
# https://opensource.org/licenses/MIT
# 
# Voltage logger (from host)
#

import usb.core
import time
import struct

COMM_EP = 1
DATA_EP = 129

# find device
dev = usb.core.find(idVendor=0xcafe, idProduct=0xbabe)
if dev is None:
    raise ValueError('Device not found')

# set configuration
dev.set_configuration()

while True:
    samples = struct.unpack('HHHHHHHHHH', dev.read(DATA_EP, 20))
    for sample in samples:
        voltage = sample * 3.3 / 4095
        print("%0.2fV" % voltage)

