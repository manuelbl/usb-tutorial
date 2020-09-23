/*
 * USB Tutorial
 * 
 * Copyright (c) 2020 Manuel Bleichenbacher
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 * 
 * Interrupt handlers
 */

#include "main.h"

extern PCD_HandleTypeDef usb_pcd;

void SysTick_Handler()
{
    HAL_IncTick();
}

void USB_LP_CAN1_RX0_IRQHandler()
{
    HAL_PCD_IRQHandler(&usb_pcd);
}

void NMI_Handler()
{
}

void HardFault_Handler()
{
    while (1)
    {
    }
}

void MemManage_Handler()
{
    while (1)
    {
    }
}

void BusFault_Handler()
{
    while (1)
    {
    }
}

void UsageFault_Handler()
{
    while (1)
    {
    }
}

void SVC_Handler()
{
}

void DebugMon_Handler()
{
}

void PendSV_Handler()
{
}
