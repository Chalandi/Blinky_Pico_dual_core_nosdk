/******************************************************************************************
  Filename    : main.c
  
  Core        : ARM Cortex-M0+
  
  MCU         : RP2040
    
  Author      : Chalandi Amine
 
  Owner       : Chalandi Amine
  
  Date        : 07.02.2023
  
  Description : Application main function
  
******************************************************************************************/

//=============================================================================
// Includes
//=============================================================================
#include "Platform_Types.h"
#include "Cpu.h"
#include "Gpio.h"
#include "SysTickTimer.h"

#define PICO_NO_HARDWARE 1
#include "jtag.h"
//=============================================================================
// Macros
//=============================================================================

//=============================================================================
// Prototypes
//=============================================================================
void main_Core0(void);
void main_Core1(void);
void BlockingDelay(uint32 delay);

//=============================================================================
// Globals
//=============================================================================
#ifdef DEBUG
  volatile boolean boHaltCore0 = TRUE;
  volatile boolean boHaltCore1 = TRUE;
#endif

//-----------------------------------------------------------------------------------------
/// \brief  main function
///
/// \param  void
///
/// \return void
//-----------------------------------------------------------------------------------------
int main(void)
{
  /* Run the main function of the core 0, it will start the core 1 */
  main_Core0();

  /* Synchronize with core 1 */
  RP2040_MulticoreSync(SIO->CPUID);

  /* endless loop on the core 0 */
  for(;;);

  /* never reached */
  return(0);
}

//-----------------------------------------------------------------------------------------
/// \brief  main_Core0 function
///
/// \param  void
///
/// \return void
//-----------------------------------------------------------------------------------------
void main_Core0(void)
{
#ifdef DEBUG
  while(boHaltCore0);
#endif

  /* Disable interrupts on core 0 */
  __asm volatile("CPSID i");

  /* Output disable on pin 25 */
  LED_GREEN_CFG();

  /* Start the Core 1 and turn on the led to be sure that we passed successfully the core 1 initiaization */
  if(TRUE == RP2040_StartCore1())
  {
    LED_GREEN_ON();
  }
  else
  {
    /* Loop forever in case of error */
    while(1)
    {
      __asm volatile("NOP");
    }
  }

}

//-----------------------------------------------------------------------------------------
/// \brief  main_Core1 function
///
/// \param  void
///
/// \return void
//-----------------------------------------------------------------------------------------
void main_Core1(void)
{
#ifdef DEBUG
  while(boHaltCore1);
#endif

  /* Note: Core 1 is started with interrupt enabled by the BootRom */

  /* Clear the stiky bits of the FIFO_ST on core 1 */
  SIO->FIFO_ST.reg = 0xFFu;
  __asm volatile("DSB");

  /* Clear all pending interrupts on core 1 */
  NVIC->ICPR[0] = (uint32)-1;

  /* Synchronize with core 0 */
  RP2040_MulticoreSync(SIO->CPUID);

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

  /* configure the SM0 cycle to 0.5 us (2MHz) */
  PIO0->SM0_CLKDIV.bit.FRAC = 0;
  PIO0->SM0_CLKDIV.bit.INT  = 133/2;

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

  /* enable the SM0 */
  PIO0->CTRL.bit.SM_ENABLE = 1;

  /* fill the TX fifo */
  PIO0->TXF0 = 0; /* init */
  PIO0->TXF0 = 2; /* 4 - 2 */
  PIO0->TXF0 = 15;
  PIO0->TXF0 = 30; /* 32 - 2 */
  PIO0->TXF0 = 0xb15b00b5;


  for(uint32 i=0;i<4;i++)
  {
    volatile uint32 x = PIO0->RXF0;
    x = x;
  }



  while(1)
  {
    LED_GREEN_TOGGLE();
    BlockingDelay(10000000);
    
    PIO0->TXF0 = 2; /* 4 - 2 */
    PIO0->TXF0 = 5;
    PIO0->TXF0 = 62; /* 64 - 2 */
    PIO0->TXF0 = 0xdeadbeef; /* LSB of 64-bit */
    //for(uint32 i=0;i<10;i++)
      __asm volatile("nop");
    while(PIO0->FSTAT.bit.TXFULL);
    PIO0->TXF0 = 0xb15b00b5; /* MSB of 64-bit */
  
  for(uint32 i=0;i<4;i++)
  {
    volatile uint32 x = PIO0->RXF0;
    x = x;
  }
  }


}
