/******************************************************************************************
  Filename    : Memory_Map.ld
  
  Core        : ARM Cortex-M0+
  
  MCU         : RP2040
    
  Author      : Chalandi Amine
 
  Owner       : Chalandi Amine
  
  Date        : 07.02.2023
  
  Description : Linker description file for Raspberry Pi Pico
  
******************************************************************************************/

/******************************************************************************************
 ELF Entrypoint
******************************************************************************************/
ENTRY(Startup_Init)

/******************************************************************************************
 Globals
******************************************************************************************/
__STACK_SIZE_CORE0 = 2K;
__STACK_SIZE_CORE1 = 2K;

/******************************************************************************************
 Memory configuration
******************************************************************************************/

MEMORY
{

  SBL(rx)     : ORIGIN = 0x10000000, LENGTH = 0x200
  INTVECT(rx) : ORIGIN = 0x10000200, LENGTH = 0x200
  FLASH(rx)   : ORIGIN = 0x10000400, LENGTH = 2M - 0x400
  RAM(rwx)    : ORIGIN = 0x20000000, LENGTH = 264K

}

/******************************************************************************************
 Sections definition
******************************************************************************************/
SECTIONS
{
  /* Pico Secondary bootloader */
  .SBL :
  {
    . = ALIGN(4);
    PROVIDE(__SBL_BASE_ADDRESS = .);
    *(.SBL)
    KEEP(*(.SBL))
  } > SBL

  /* Interrupt vector table */
  .intvect :
  {
    . = ALIGN(4);
    PROVIDE(__INTVECT_BASE_ADDRESS = .);
    *(.intvect_c0)
    *(.intvect_c1)
    KEEP(*(.intvect_c0))
    KEEP(*(.intvect_c1))
  } > INTVECT

  /* Program code (text) */
  .text :
  {
    . = ALIGN(4);
    PROVIDE(__CODE_BASE_ADDRESS = .);
    *(.text)
  } > FLASH

  /* Read-only data (.rodata) */
  .rodata :
  {
    . = ALIGN(4);
    PROVIDE(__RODATA_BASE_ADDRESS = .);
    *(.rodata)
  } > FLASH

 /* Section for constructors */
  .ctors :
  {
    . = ALIGN(4);
    PROVIDE(__CPPCTOR_LIST__ = .);
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array))
    LONG(-1) ;
    PROVIDE(__CPPCTOR_END__ = .);
  }  > FLASH


  /* Section for destructors */
  .dtors :
  {
    . = ALIGN(4);
    PROVIDE(__CPPDTOR_LIST__ = .);
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array))
    LONG(-1) ;
    PROVIDE(__CPPDTOR_END__ = .);
  } > FLASH

  /* Runtime clear table */
  .clear_sec :
  {
    . = ALIGN(4);
    PROVIDE(__RUNTIME_CLEAR_TABLE = .) ;
    LONG(0 + ADDR(.bss));   LONG(SIZEOF(.bss));
    LONG(-1);                 LONG(-1);
  } > FLASH

  /* Runtime copy table */
  .copy_sec :
  {
    . = ALIGN(4);
    PROVIDE(__RUNTIME_COPY_TABLE = .) ;
    LONG(LOADADDR(.data));  LONG(0 + ADDR(.data));  LONG(SIZEOF(.data));
    LONG(-1);                 LONG(-1);                  LONG(-1);
  } > FLASH 

  /* The ROM-to-RAM initialized data section */
  .data :
  {
    . = ALIGN(4);
    PROVIDE(__DATA_BASE_ADDRESS = .);
    *(.data)
  } > RAM  AT>FLASH

  /* The uninitialized (zero-cleared) bss section */
  .bss :
  {
    . = ALIGN(4);
    PROVIDE(__BSS_BASE_ADDRESS = .);
    *(.bss)
  } > RAM

  /* stack definition */
  .stack_core0 :
  {
    . = ALIGN(MAX(__STACK_SIZE_CORE0 , .), 8);
    PROVIDE(__CORE0_STACK_TOP = .) ;
  } > RAM

  .stack_core1 :
  {
    . = ALIGN(MAX(__STACK_SIZE_CORE1 , .), 8);
    PROVIDE(__CORE1_STACK_TOP = .) ;
  } > RAM

}
