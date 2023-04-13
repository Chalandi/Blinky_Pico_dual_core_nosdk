/******************************************************************************************
  Filename    : USB.h
  
  Core        : ARM Cortex-M0+
  
  MCU         : RP2040
    
  Author      : Chalandi Amine
 
  Owner       : Chalandi Amine
  
  Date        : 01.04.2023
  
  Description : USB low level device driver header file
  
******************************************************************************************/

#ifndef __USB_H__
#define __USB_H__

#include "Platform_Types.h"

void USBCTRL_IRQ(void);
void UsbInit(void);

/* endpoints IDs */
#define EP0      0u
#define EP1      1u
#define EP2      2u
#define EP3      3u
#define EP4      4u
#define EP5      5u
#define EP6      6u
#define EP7      7u
#define EP8      8u
#define EP9      9u
#define EP10    10u
#define EP11    11u
#define EP12    12u
#define EP13    13u
#define EP14    14u
#define EP15    15u

/* endpoints direction */
#define EP_DIR_IN   0x80
#define EP_DIR_OUT  0x00

/* Data PIDs */
#define DATA0_PID      0u
#define DATA1_PID      1u

/* SETUP Packet - Standard requests */
#define USB_REQ_GET_STATUS         0u
#define USB_REQ_CLEAR_FEATURE      1u
#define USB_REQ_SET_FEATURE        3u
#define USB_REQ_SET_ADDRESS        5u
#define USB_REQ_GET_DESCRIPTOR     6u
#define USB_REQ_SET_DESCRIPTOR     7u
#define USB_REQ_GET_CONFIGURATION  8u
#define USB_REQ_SET_CONFIGURATION  9u
#define USB_REQ_GET_INTERFACE     10u
#define USB_REQ_SET_INTERFACE     11u
#define USB_REQ_SYNCH_FRAME       12u

#define USB_REQ_DIR_DEVICE_TO_HOST       1u
#define USB_REQ_DIR_HOST_TO_DEVICE       0u

#define USB_REQ_TYPE_STANDARD            0u
#define USB_REQ_TYPE_CLASS               1u
#define USB_REQ_TYPE_VENDOR              2u
#define USB_REQ_TYPE_RESERVED            3u

#define USB_REQ_RECIPIENT_DEVICE         0u
#define USB_REQ_RECIPIENT_INTERFACE      1u
#define USB_REQ_RECIPIENT_ENDPOINT       2u

/* Descriptor Types */
#define USB_DESCRIPTOR_TYPE_DEVICE                     1u
#define USB_DESCRIPTOR_TYPE_CONFIGURATION              2u
#define USB_DESCRIPTOR_TYPE_STRING                     3u
#define USB_DESCRIPTOR_TYPE_INTERFACE                  4u
#define USB_DESCRIPTOR_TYPE_ENDPOINT                   5u
#define USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER           6u
#define USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION  7u
#define USB_DESCRIPTOR_TYPE_INTERFACE_POWER            8u
#define USB_DESCRIPTOR_TYPE_HID_REPORT              0x22u











#endif
