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

static uint8_t USBD_Vendor_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    return USBD_OK;
}

static uint8_t USBD_Vendor_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    return USBD_OK;
}

static uint8_t USBD_Vendor_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    uint16_t status_info = 0U;
    USBD_StatusTypeDef ret = USBD_OK;

    switch (req->bmRequest & USB_REQ_TYPE_MASK)
    {
    case USB_REQ_TYPE_VENDOR:
        if (req->bRequest == LED_CONTROL_ID && req->wIndex == 0)
        {
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, req->wValue != 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }
        else
        {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
        }
        break;

    case USB_REQ_TYPE_STANDARD:
        switch (req->bRequest)
        {
        case USB_REQ_GET_STATUS:
            if (pdev->dev_state == USBD_STATE_CONFIGURED)
            {
                USBD_CtlSendData(pdev, (uint8_t *)(void *)&status_info, 2U);
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
        break;

    default:
        USBD_CtlError(pdev, req);
        ret = USBD_FAIL;
        break;
    }

    return ret;
}

static uint8_t *USBD_Vendor_GetConfigDesc(uint16_t *length)
{
    *length = sizeof(Configuration_Desc);
    return Configuration_Desc;
}
