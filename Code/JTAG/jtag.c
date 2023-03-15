/******************************************************************************************
  Filename    : jtag.c
  
  Core        : ARM Cortex-M0+
  
  MCU         : RP2040
    
  Author      : Chalandi Amine
 
  Owner       : Chalandi Amine
  
  Date        : 13.03.2023
  
  Description : jtag protocol implementation
  
******************************************************************************************/

//=============================================================================
// Includes
//=============================================================================
#include "jtag.h"
#include "RP2040.h"
#define PICO_NO_HARDWARE 1
#include "jtag_pio.h"


//-----------------------------------------------------------------------------------------
/// \brief  jtag_SetClock function
///
/// \param  void
///
/// \return void
//-----------------------------------------------------------------------------------------
void jtag_SetClock(uint32 jtag_freq)
{
  uint32 cpu_freq  = 133;

  uint32 x_int  = cpu_freq / (6 * jtag_freq);
  float  x_frac = ((float)(cpu_freq % (6 * jtag_freq)) / (float)(6 * jtag_freq)) * 256;

  /* configure the SM0 cycle */
  PIO0->SM0_CLKDIV.bit.FRAC = (uint8)x_frac;
  PIO0->SM0_CLKDIV.bit.INT  = (uint16)x_int;
}

//-----------------------------------------------------------------------------------------
/// \brief  main function
///
/// \param  void
///
/// \return void
//-----------------------------------------------------------------------------------------
void jtag_init(void)
{
  /* configure PIO JTAG pin */
  IO_BANK0->GPIO0_CTRL.bit.FUNCSEL = IO_BANK0_GPIO0_CTRL_FUNCSEL_pio0_0;
  IO_BANK0->GPIO1_CTRL.bit.FUNCSEL = IO_BANK0_GPIO1_CTRL_FUNCSEL_pio0_1;
  IO_BANK0->GPIO2_CTRL.bit.FUNCSEL = IO_BANK0_GPIO2_CTRL_FUNCSEL_pio0_2;
  IO_BANK0->GPIO3_CTRL.bit.FUNCSEL = IO_BANK0_GPIO3_CTRL_FUNCSEL_pio0_3;
  IO_BANK0->GPIO4_CTRL.bit.FUNCSEL = IO_BANK0_GPIO4_CTRL_FUNCSEL_pio0_4;

  /* Release PIO0 reset */
  RESETS->RESET.bit.pio0 = 0;
  while(RESETS->RESET_DONE.bit.pio0 == 0U);

  /* copy the compiled pio assembly code into the pio memory instruction */
  for(uint32 i=0; i < (sizeof(PIO_JTAG_program_instructions) / sizeof(uint16_t)); i++)
  {
    ((uint32*)&(PIO0->INSTR_MEM0.reg))[i] = (uint32)PIO_JTAG_program_instructions[i];
  }

  /* configure the JTAG clock to 4MHz */
  jtag_SetClock(4);

  /* configure PIO pin direction */
  PIO0->SM0_PINCTRL.reg = 0;
  PIO0->SM0_PINCTRL.bit.SET_BASE  = 0;
  PIO0->SM0_PINCTRL.bit.SET_COUNT = 5;
  PIO0->SM0_INSTR.reg = (uint32_t)PIO_JTAG_SET_PIN_CONFIG_program_instructions[0];

  /* configure PIO outputs to logic 1 */
  PIO0->SM0_PINCTRL.reg = 0;
  PIO0->SM0_PINCTRL.bit.SET_BASE  = 0;
  PIO0->SM0_PINCTRL.bit.SET_COUNT = 4;
  PIO0->SM0_INSTR.reg = (uint32_t)PIO_JTAG_SET_PIN_OUTPUT_program_instructions[0];

  /* enable side-set opt */
  PIO0->SM0_EXECCTRL.bit.SIDE_EN = 1;

  /* configure wrap */
  PIO0->SM0_EXECCTRL.bit.WRAP_TOP    = PIO_JTAG_wrap;
  PIO0->SM0_EXECCTRL.bit.WRAP_BOTTOM = PIO_JTAG_wrap_target;

  /* configure pins */
  PIO0->SM0_PINCTRL.bit.SIDESET_BASE  = 1;
  PIO0->SM0_PINCTRL.bit.SIDESET_COUNT = 3; // Physical side-set pins (+ 1 if side-set opt is used)
  
  PIO0->SM0_PINCTRL.bit.OUT_BASE      = 3;
  PIO0->SM0_PINCTRL.bit.OUT_COUNT     = 1;

  PIO0->SM0_PINCTRL.bit.SET_BASE      = 0;
  PIO0->SM0_PINCTRL.bit.SET_COUNT     = 4;

  PIO0->SM0_PINCTRL.bit.IN_BASE       = 4;

  /* configure the shift reg */
  PIO0->SM0_SHIFTCTRL.reg = 0;
  PIO0->SM0_SHIFTCTRL.bit.IN_SHIFTDIR  = 1;
  PIO0->SM0_SHIFTCTRL.bit.OUT_SHIFTDIR = 1;
  PIO0->SM0_SHIFTCTRL.bit.AUTOPULL     = 1;
  PIO0->SM0_SHIFTCTRL.bit.AUTOPUSH     = 1;

  /* enable the PIO0 interrupt */
  PIO0->IRQ0_INTE.bit.SM0_RXNEMPTY = 1;

  /* enable the SM0 */
  PIO0->CTRL.bit.SM_ENABLE = 1;

  /* fill the TX fifo */
  PIO0->TXF0 = 0; /* init */
  while(PIO0->FSTAT.bit.TXFULL);
}

//-----------------------------------------------------------------------------------------
/// \brief  main function
///
/// \param  void
///
/// \return void
//-----------------------------------------------------------------------------------------
uint64 jtag_transfer(uint64 IrReg, uint32 IrRegLen, uint64 DrData, uint32 DrDataLen)
{
  uint64 DrRxData = 0;

  PIO0->TXF0 = IrRegLen - 2;
  PIO0->TXF0 = (uint32)IrReg;
  if(IrRegLen == 64)
  {
    PIO0->TXF0 = (uint32)(IrReg>>32);
    while((PIO0->FSTAT.bit.TXEMPTY & 0x01) == 0);
  }
  PIO0->TXF0 = DrDataLen - 2;
  PIO0->TXF0 = (uint32)DrData;
  while((PIO0->FSTAT.bit.TXEMPTY & 0x01) == 0);
  if(DrDataLen == 64)
  {
    PIO0->TXF0 = (uint32)(DrData>>32);
  }

  /* wait for the PIO0 interrupt */
  while((PIO0->IRQ.bit.IRQ & 0x01) != 1);

  /* clear the PIO0 interrupt */
  PIO0->IRQ.bit.IRQ |= 1;

  for(uint32 i=0;i<4;i++)
  {
    if(i==1)
      DrRxData = (uint64)PIO0->RXF0;
    if(i == 2 && DrDataLen == 64)
      DrRxData |= (uint64)PIO0->RXF0 << 32;
    else
      PIO0->RXF0;
  }

  return(DrRxData);
}
