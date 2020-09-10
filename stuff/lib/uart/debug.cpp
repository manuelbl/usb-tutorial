#include "uart.h"
#include <libopencm3/usb/usbd.h>


extern "C" void debug_request(char type, usb_setup_data* req, usbd_request_return_codes result)
{
    uart.printf("SETUP: %c: type=%02x, request=%02x, value=%04x, index=%04x, len=%04x, res=%d\r\n", type, req->bmRequestType, req->bRequest, req->wValue, req->wIndex, req->wLength, result);
}

static uint8_t buf[4096];
static uint8_t* sbrkp = buf;
extern "C" void *_sbrk(intptr_t increment)
{
    uint8_t* old = sbrkp;
    sbrkp += increment;
    return old;
}
