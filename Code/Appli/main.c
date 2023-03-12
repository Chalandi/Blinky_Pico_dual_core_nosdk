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

  volatile uint64 x = (uint64)-1;

  jtag_init();

  while(1)
  {
    LED_GREEN_TOGGLE();
    BlockingDelay(10000000);

    /* communicate with the jtag */
    x = jtag_transfer(0x01, 5, x, 32);

  }
}
