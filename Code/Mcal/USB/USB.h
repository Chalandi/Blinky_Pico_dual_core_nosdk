
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

/* Data PIDs */
#define DATA0_PID      0u
#define DATA1_PID      1u

#endif
