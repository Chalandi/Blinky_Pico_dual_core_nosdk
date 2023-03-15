/******************************************************************************************
  Filename    : swd.h
  
  Core        : ARM Cortex-M0+
  
  MCU         : RP2040
    
  Author      : Chalandi Amine
 
  Owner       : Chalandi Amine
  
  Date        : 13.03.2023
  
  Description : swd protocol implementation
  
******************************************************************************************/

#ifndef __SWD_H__
#define __SWD_H__

//=============================================================================
// Includes
//=============================================================================
#include "Platform_Types.h"

//=============================================================================
// Prototypes
//=============================================================================
void   swd_SetClock(uint32 swd_freq);
void   swd_init(void);
uint64 swd_read(uint8 header, uint8* Ack);
void   swd_write(uint8 header, uint8* Ack, uint64 data);

#endif
