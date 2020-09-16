/*
 * USB Tutorial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * Vendor specific interface
 */

#include "main.h"
#include "usbd_vendor.h"
#include "usbd_ctlreq.h"

static uint8_t USBD_Vendor_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_Vendor_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_Vendor_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t *USBD_Vendor_GetConfigDesc(uint16_t *length);
static uint8_t *USBD_Vendor_GetStringDesc(USBD_HandleTypeDef *pdev, uint8_t index, uint16_t *length);

USBD_ClassTypeDef USBD_Vendor_Class =
    {
        USBD_Vendor_Init,
        USBD_Vendor_DeInit,
        USBD_Vendor_Setup,
        NULL, /* EP0_TxSent */
        NULL, /* EP0_RxReady */
        NULL, /* DataIn */
        NULL, /* DataOut */
        NULL, /* SOF */
        NULL,
        NULL,
        NULL,
        USBD_Vendor_GetConfigDesc,
        NULL,
        NULL,
        USBD_Vendor_GetStringDesc,
};

#define CONFIG_DESC_SIZE 18U

/* USB blinky device configuration descriptor */
__ALIGN_BEGIN static uint8_t Configuration_Desc[CONFIG_DESC_SIZE] __ALIGN_END =
    {
        0x09,                        /* bLength: Configuration Descriptor size */
        USB_DESC_TYPE_CONFIGURATION, /* bDescriptorType: Configuration */
        CONFIG_DESC_SIZE,            /* wTotalLength: Bytes returned */
        0x00,
        0x01,                /* bNumInterfaces: 1 interface */
        0x01,                /* bConfigurationValue: Configuration value */
        USBD_IDX_CONFIG_STR, /* iConfiguration: Index of string descriptor for configuration */
        0x80,                /* bmAttributes: bus powered */
        0xfa,                /* MaxPower 500 mA: this current is used for detecting Vbus */

        /* Interface 0 descriptor */
        /* 09 */
        0x09,                    /* bLength: Interface Descriptor size */
        USB_DESC_TYPE_INTERFACE, /* bDescriptorType: Interface descriptor type */
        0x00,                    /* bInterfaceNumber: Number of Interface */
        0x00,                    /* bAlternateSetting: Alternate setting */
        0x00,                    /* bNumEndpoints */
        0xff,                    /* bInterfaceClass: Vendor-specific */
        0x00,                    /* bInterfaceSubClass : 1=BOOT, 0=no boot */
        0x00,                    /* nInterfaceProtocol : 0=vendor specific */
        USBD_IDX_INTERFACE_STR,  /* iInterface: Index of string descriptor */
                                 /* 18 */
};

#define WCID_VENDOR_CODE 0x37

// Microsoft WCID string descriptor (string index 0xee)
static const uint8_t msft_sig_desc[] = {
    0x12,                           /* length = 18 bytes */
    USB_DESC_TYPE_STRING,           /* descriptor type string */
    'M', 0, 'S', 0, 'F', 0, 'T', 0, /* 'M', 'S', 'F', 'T' */
    '1', 0, '0', 0, '0', 0,         /* '1', '0', '0' */
    WCID_VENDOR_CODE,               /* vendor code */
    0                               /* padding */
};

// Microsoft WCID feature descriptor (index 0x0004)
static const uint8_t wcid_feature_desc[] = {
    0x28, 0x00, 0x00, 0x00,                         /* length = 40 bytes */
    0x00, 0x01,                                     /* version 1.0 (in BCD) */
    0x04, 0x00,                                     /* compatibility descriptor index 0x0004 */
    0x01,                                           /* number of sections */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       /* reserved (7 bytes) */
    0x00,                                           /* interface number 0 */
    0x01,                                           /* reserved */
    0x57, 0x49, 0x4E, 0x55, 0x53, 0x42, 0x00, 0x00, /* Compatible ID "WINUSB\0\0" */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Subcompatible ID (unused) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00              /* reserved 6 bytes */
};

uint8_t USBD_Vendor_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    return USBD_OK;
}

uint8_t USBD_Vendor_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    return USBD_OK;
}

uint8_t USBD_Vendor_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    USBD_StatusTypeDef ret = USBD_OK;

    switch (req->bmRequest & USB_REQ_TYPE_MASK)
    {
    case USB_REQ_TYPE_VENDOR:
        /* WCID feature request */
        if (req->bRequest == WCID_VENDOR_CODE && req->wIndex == 0x0004)
        {
            uint16_t len = sizeof(wcid_feature_desc);
            if (len > req->wLength)
                len = req->wLength;
            USBD_CtlSendData(pdev, (uint8_t *)wcid_feature_desc, len);
        }
        /* LED command */
        else if (req->bRequest == LED_CONTROL_ID && req->wIndex == 0)
        {
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, req->wValue == 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }
        else
        {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
        }
        break;

    default:
        USBD_CtlError(pdev, req);
        ret = USBD_FAIL;
        break;
    }

    return ret;
}

uint8_t *USBD_Vendor_GetConfigDesc(uint16_t *length)
{
    *length = sizeof(Configuration_Desc);
    return Configuration_Desc;
}

static uint8_t *USBD_Vendor_GetStringDesc(USBD_HandleTypeDef *pdev, uint8_t index, uint16_t *length)
{
    if (index == 0xee)
    {
        *length = sizeof(msft_sig_desc);
        return (uint8_t *)msft_sig_desc;
    }
    else
    {
        *length = 0;
        USBD_CtlError(pdev, NULL);
        return NULL;
    }
}
