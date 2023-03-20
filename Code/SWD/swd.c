/******************************************************************************************
  Filename    : swd.c
  
  Core        : ARM Cortex-M0+
  
  MCU         : RP2040
    
  Author      : Chalandi Amine
 
  Owner       : Chalandi Amine
  
  Date        : 13.03.2023
  
  Description : swd protocol implementation
  
******************************************************************************************/

//=============================================================================
// Includes
//=============================================================================
#include "swd.h"
#include "RP2040.h"
#define PICO_NO_HARDWARE 1
#include "swd_pio.h"

//-----------------------------------------------------------------------------------------
/// \brief
///
/// \param
///
/// \return
//-----------------------------------------------------------------------------------------
void swd_SetClock(uint32 swd_freq)
{
  uint32 cpu_freq  = 133;

  uint32 x_int  = cpu_freq / (PIO_SWD_TICK_PER_SWD_PERIOD * swd_freq);
  float  x_frac = ((float)(cpu_freq % (PIO_SWD_TICK_PER_SWD_PERIOD * swd_freq)) / (float)(PIO_SWD_TICK_PER_SWD_PERIOD * swd_freq)) * 256;

  /* configure the SM0 cycle */
  PIO1->SM0_CLKDIV.bit.FRAC = (uint8)x_frac;
  PIO1->SM0_CLKDIV.bit.INT  = (uint16)x_int;
}

//-----------------------------------------------------------------------------------------
/// \brief
///
/// \param
///
/// \return
//-----------------------------------------------------------------------------------------
void swd_init(void)
{
  /* configure PIO swd pin */
  IO_BANK0->GPIO14_CTRL.bit.FUNCSEL = IO_BANK0_GPIO14_CTRL_FUNCSEL_pio1_14;
  IO_BANK0->GPIO15_CTRL.bit.FUNCSEL = IO_BANK0_GPIO15_CTRL_FUNCSEL_pio1_15;

  PADS_BANK0->GPIO14.bit.SLEWFAST = 1;
  PADS_BANK0->GPIO14.bit.DRIVE    = 3;

  PADS_BANK0->GPIO15.bit.PUE      = 1;
  PADS_BANK0->GPIO15.bit.PDE      = 0;
  PADS_BANK0->GPIO15.bit.SLEWFAST = 1;
  PADS_BANK0->GPIO15.bit.DRIVE    = 3;

  /* Release PIO1 reset */
  RESETS->RESET.bit.pio1 = 0;
  while(RESETS->RESET_DONE.bit.pio1 == 0U);

  /* copy the compiled pio assembly code into the pio memory instruction */
  for(uint32 i=0; i < (sizeof(pio_swd_reset_program_instructions) / sizeof(uint16_t)); i++)
  {
    ((uint32*)&(PIO1->INSTR_MEM0.reg))[i] = (uint32)pio_swd_reset_program_instructions[i];
  }

  /* configure the swd clock to 4MHz */
  swd_SetClock(1);

  /* enable side-set opt */
  PIO1->SM0_EXECCTRL.bit.SIDE_EN = 1;

  /* configure pins */
  PIO1->SM0_PINCTRL.reg = 0;
  PIO1->SM0_PINCTRL.bit.SIDESET_BASE  = 14;
  PIO1->SM0_PINCTRL.bit.SIDESET_COUNT = 2; // Physical side-set pins (+ 1 if side-set opt is used)
  
  PIO1->SM0_PINCTRL.bit.OUT_BASE      = 15;
  PIO1->SM0_PINCTRL.bit.OUT_COUNT     = 1;

  PIO1->SM0_PINCTRL.bit.SET_BASE      = 14;
  PIO1->SM0_PINCTRL.bit.SET_COUNT     = 2;

  PIO1->SM0_PINCTRL.bit.IN_BASE       = 15;

  /* configure wrap */
  PIO1->SM0_EXECCTRL.bit.WRAP_TOP    = pio_swd_reset_wrap;
  PIO1->SM0_EXECCTRL.bit.WRAP_BOTTOM = pio_swd_reset_wrap_target;

  /* configure the shift reg */
  PIO1->SM0_SHIFTCTRL.reg = 0;
  PIO1->SM0_SHIFTCTRL.bit.IN_SHIFTDIR  = 1;
  PIO1->SM0_SHIFTCTRL.bit.OUT_SHIFTDIR = 1;

  /* enable the PIO1 interrupt */
  PIO1->IRQ0_INTE.bit.SM0_RXNEMPTY = 1;

  /* enable the SM0 */
  PIO1->CTRL.bit.SM_ENABLE = 1;

  /* perform the SWD reset sequence (JTAG-to-SWD) */
  PIO1->TXF0 = 0xe79e;
  PIO1->TXF0 = 0xA5;
  while(PIO1->FSTAT.bit.TXFULL);

  for(uint32 i=0; i<10000; i++)
  {
    __asm volatile("nop");
  }

  /* disable the SM0 */
  PIO1->CTRL.bit.SM_ENABLE  = 0;
  PIO1->CTRL.bit.SM_RESTART = 1;

  /* copy the compiled pio assembly code into the pio memory instruction */
  for(uint32 i=0; i < (sizeof(pio_swd_cmd_program_instructions) / sizeof(uint16_t)); i++)
  {
    ((uint32*)&(PIO1->INSTR_MEM0.reg))[i] = (uint32)pio_swd_cmd_program_instructions[i];
  }

  /* configure wrap */
  PIO1->SM0_EXECCTRL.bit.WRAP_TOP    = pio_swd_cmd_wrap;
  PIO1->SM0_EXECCTRL.bit.WRAP_BOTTOM = pio_swd_cmd_wrap_target;

  /* enable the SM0 */
  PIO1->CTRL.bit.SM_ENABLE = 1;

}

//-----------------------------------------------------------------------------------------
/// \brief
///
/// \param
///
/// \return
//-----------------------------------------------------------------------------------------
uint64 swd_read(uint8 header, uint8* Ack)
{
    (void) header;
    (void) Ack;
    return(0);
}

//-----------------------------------------------------------------------------------------
/// \brief
///
/// \param
///
/// \return
//-----------------------------------------------------------------------------------------
void swd_write(uint8 header, uint8* Ack, uint64 data)
{
    (void) header;
    (void) Ack;
    (void) data;
}


