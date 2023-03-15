/******************************************************************************************
  Filename    : jtag.h
  
  Core        : ARM Cortex-M0+
  
  MCU         : RP2040
    
  Author      : Chalandi Amine
 
  Owner       : Chalandi Amine
  
  Date        : 13.03.2023
  
  Description : jtag protocol implementation
  
******************************************************************************************/

#ifndef __JTAG_H__
#define __JTAG_H__

//=============================================================================
// Includes
//=============================================================================
#include "Platform_Types.h"

//=============================================================================
// Prototypes
//=============================================================================
void jtag_SetClock(uint32 jtag_freq);
void jtag_init(void);
uint64 jtag_transfer(uint64 IrReg, uint32 IrRegLen, uint64 DrData, uint32 DrDataLen);

#endif
