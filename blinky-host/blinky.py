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
    dev.write(COMM_EP, bytes([0x01, int(led_on)]))
    time.sleep(0.6)
