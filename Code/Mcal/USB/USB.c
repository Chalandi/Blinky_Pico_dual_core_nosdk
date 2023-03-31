
#include "RP2040.h"
#include "USB.h"

volatile unsigned long long _EP0_[100] = {0};
volatile unsigned int _EP0_index = 0;
volatile unsigned int BusResetCounter = 0;
volatile unsigned int UsbDeviceAddress = 0;

volatile unsigned int UsbReceived_EP0_OUT_count = 0;
volatile unsigned int UsbReceived_EP0_IN_count = 0;

volatile unsigned int UsbNotSupportedRequestCount = 0;

void USBCTRL_IRQ(void)
{
  if(USBCTRL_REGS->INTS.bit.SETUP_REQ)
  {
    //clear the interrupt
    USBCTRL_REGS->SIE_STATUS.bit.SETUP_REC = 1U;
    __asm("DSB");

    //check the received request in the SETUP Packet
    if(USBCTRL_DPRAM->SETUP_PACKET_LOW.bit.BMREQUESTTYPE == 0x80U &&
       USBCTRL_DPRAM->SETUP_PACKET_LOW.bit.BREQUEST      == 0x06U
       )
    {
      if(USBCTRL_DPRAM->SETUP_PACKET_LOW.bit.WVALUE == 0x0100)
      {
          /* send the device descriptor */
          const unsigned char device_dsc[0x12] = {
                                                   0x12,      // bLength
                                                   0x01,      // bDescriptorType
                                                   0x00,0x02, // bcdUSB
                                                   0x00,      // bDeviceClass
                                                   0x00,      // bDeviceSubClass
                                                   0x00,      // bDeviceProtocol
                                                   64,        // bMaxPacketSize0
                                                   0x8a,0x2e, // idVendor
                                                   0x01,0x00, // idProduct
                                                   0x00,0x00, // bcdDevice
                                                   0x00,      // iManufacturer
                                                   0x00,      // iProduct
                                                   0x00,      // iSerialNumber
                                                   0x01       // bNumConfigurations
                                                 };
           for(int i =0; i < 0x12; i++)
           {
             ((volatile unsigned char*)(USBCTRL_DPRAM_BASE+0x100UL))[i] = device_dsc[i];
           }
          
           USBCTRL_DPRAM->EP0_IN_BUFFER_CONTROL.reg = 0;
           USBCTRL_DPRAM->EP0_IN_BUFFER_CONTROL.bit.LENGTH_0    = 0x12u;
           USBCTRL_DPRAM->EP0_IN_BUFFER_CONTROL.bit.AVAILABLE_0 = 1U;
           USBCTRL_DPRAM->EP0_IN_BUFFER_CONTROL.bit.PID_0       = 1U;
           USBCTRL_DPRAM->EP0_IN_BUFFER_CONTROL.bit.FULL_0      = 1U;
       }
       else if(USBCTRL_DPRAM->SETUP_PACKET_LOW.bit.WVALUE == 0x0200)
       {
         /* send configuration descriptor */
         const unsigned char configuration_dsc[9]={
                                                    // Configuration Descriptor
                                                    0x09,                   // bLength             - Descriptor size in bytes
                                                    0x02,                   // bDescriptorType     - The constant CONFIGURATION (02h)
                                                    0x29,0x00,              // wTotalLength        - The number of bytes in the configuration descriptor and all of its subordinate descriptors
                                                    1,                      // bNumInterfaces      - Number of interfaces in the configuration
                                                    1,                      // bConfigurationValue - Identifier for Set Configuration and Get Configuration requests
                                                    0,                      // iConfiguration      - Index of string descriptor for the configuration
                                                    0x80,                   // bmAttributes        - Self/bus power and remote wakeup settings (Self powered 0xC0,  0x80 bus powered)
                                                    50                      // bMaxPower           - Bus power required in units of 2 mA
                                                  };
           for(int i =0; i < 9; i++)
           {
             ((volatile unsigned char*)(USBCTRL_DPRAM_BASE+0x100UL))[i] = configuration_dsc[i];
           }
          
           USBCTRL_DPRAM->EP0_IN_BUFFER_CONTROL.reg = 0;
           USBCTRL_DPRAM->EP0_IN_BUFFER_CONTROL.bit.LENGTH_0    = 0x9u;
           USBCTRL_DPRAM->EP0_IN_BUFFER_CONTROL.bit.AVAILABLE_0 = 1U;
           USBCTRL_DPRAM->EP0_IN_BUFFER_CONTROL.bit.PID_0       = 1U;
           USBCTRL_DPRAM->EP0_IN_BUFFER_CONTROL.bit.FULL_0      = 1U;
       
       }
       else
       {
       }

    }
    else if(USBCTRL_DPRAM->SETUP_PACKET_LOW.bit.BMREQUESTTYPE == 0U &&
            USBCTRL_DPRAM->SETUP_PACKET_LOW.bit.BREQUEST      == 0x05U)
    {
      UsbDeviceAddress = USBCTRL_DPRAM->SETUP_PACKET_LOW.bit.WVALUE;

       USBCTRL_DPRAM->EP0_IN_BUFFER_CONTROL.reg = 0;
       USBCTRL_DPRAM->EP0_IN_BUFFER_CONTROL.bit.LENGTH_0    = 0u;
       USBCTRL_DPRAM->EP0_IN_BUFFER_CONTROL.bit.AVAILABLE_0 = 1U;
       USBCTRL_DPRAM->EP0_IN_BUFFER_CONTROL.bit.PID_0       = 1U;
       USBCTRL_DPRAM->EP0_IN_BUFFER_CONTROL.bit.FULL_0      = 1U;
    }
    else
    {
      UsbNotSupportedRequestCount++;
    }



    //save the data
    _EP0_[_EP0_index++] = *(volatile unsigned long long*)(USBCTRL_DPRAM_BASE);
  }
  
  /* handel bus reset */
  if(USBCTRL_REGS->INTS.bit.BUS_RESET)
  {
    /* clear the bus reset interrupt flag */
    USBCTRL_REGS->SIE_STATUS.bit.BUS_RESET = 1;
    USBCTRL_REGS->ADDR_ENDP.bit.ADDRESS = 0;
    UsbDeviceAddress = 0;
    BusResetCounter++;
  }

  /* handle OUT and IN packets */
  if(USBCTRL_REGS->INTS.bit.BUFF_STATUS)
  {
    if(USBCTRL_REGS->BUFF_STATUS.bit.EP0_OUT)
    {
        UsbReceived_EP0_OUT_count++;

       /* clear the EP0_OUT buffer status */
       USBCTRL_REGS->BUFF_STATUS.bit.EP0_OUT = 1;
    }

    if(USBCTRL_REGS->BUFF_STATUS.bit.EP0_IN)
    {
        /* this bit indicate that the DATA is received by the HOST and we received ACK from HOST, the EP0_IN buffer is empty 
           (USBCTRL_DPRAM->EP0_IN_BUFFER_CONTROL.bit.FULL_0 is cleared to indicate that data has been sent).
        */

        UsbReceived_EP0_IN_count++;
       /* clear the EP0_IN buffer status */
       USBCTRL_REGS->BUFF_STATUS.bit.EP0_IN = 1;

      if(UsbDeviceAddress != 0)
      {
        /* setup device address */
        USBCTRL_REGS->ADDR_ENDP.reg |= (unsigned int)(UsbDeviceAddress & 0x7Ful);
      }

      /* configure the expected OUT packet */
       USBCTRL_DPRAM->EP0_OUT_BUFFER_CONTROL.reg = 0;
       USBCTRL_DPRAM->EP0_OUT_BUFFER_CONTROL.bit.LENGTH_0    = 0u;
       USBCTRL_DPRAM->EP0_OUT_BUFFER_CONTROL.bit.AVAILABLE_0 = 1U;
       USBCTRL_DPRAM->EP0_OUT_BUFFER_CONTROL.bit.PID_0       = 1U;
       USBCTRL_DPRAM->EP0_OUT_BUFFER_CONTROL.bit.FULL_0      = 0U;
    }
  }

}

volatile unsigned int __DEBUG_HALT__ = 1;

void UsbInit(void)
{
   //release the reset of PLL_USB
   RESETS->RESET.bit.pll_usb = 0U;
   while(RESETS->RESET_DONE.bit.pll_usb != 1);

   //configure the PLL_USB
   PLL_USB->CS.bit.REFDIV           = 1U;
   PLL_USB->FBDIV_INT.bit.FBDIV_INT = 40U;
   PLL_USB->PRIM.bit.POSTDIV1       = 5U;
   PLL_USB->PRIM.bit.POSTDIV2       = 2U;

   PLL_USB->PWR.bit.PD        = 0U;
   PLL_USB->PWR.bit.VCOPD     = 0U;

   while(PLL_USB->CS.bit.LOCK != 1U);

   PLL_USB->PWR.bit.POSTDIVPD = 0U;

   // switch the system clock to use the PLL
   CLOCKS->CLK_SYS_CTRL.bit.AUXSRC = CLOCKS_CLK_SYS_CTRL_AUXSRC_clksrc_pll_sys;
   CLOCKS->CLK_SYS_CTRL.bit.SRC    = CLOCKS_CLK_SYS_CTRL_SRC_clksrc_clk_sys_aux;

   //switch on the USB clock
   CLOCKS->CLK_USB_CTRL.bit.AUXSRC = CLOCKS_CLK_USB_CTRL_AUXSRC_clksrc_pll_usb;
   CLOCKS->CLK_USB_CTRL.bit.ENABLE = 1U;

   // switch off the ROSC clock

   //release reset of usb
    RESETS->RESET.bit.usbctrl = 0U;
    while(RESETS->RESET_DONE.bit.usbctrl != 1);

    //clear the DPRAM
    for(unsigned int i=0; i < 4096U; i = i+8)
    {
      *(volatile unsigned long long*)(USBCTRL_DPRAM_BASE + i) = 0;
    }

    //enable USB
    USBCTRL_REGS->USB_MUXING.bit.TO_PHY       = 1U;
    USBCTRL_REGS->USB_MUXING.bit.SOFTCON      = 0U;
    USBCTRL_REGS->MAIN_CTRL.bit.HOST_NDEVICE  = 0U;

    USBCTRL_REGS->USB_PWR.bit.VBUS_DETECT = 1U;
    USBCTRL_REGS->USB_PWR.bit.VBUS_DETECT_OVERRIDE_EN = 1U;

    USBCTRL_REGS->MAIN_CTRL.bit.CONTROLLER_EN = 1U;

    USBCTRL_REGS->SIE_CTRL.bit.EP0_INT_1BUF   = 1U;

    //enable usb interrupt
    USBCTRL_REGS->INTE.bit.BUFF_STATUS = 1U; // note: this interrupt is needed to detect OUT and IN requests send by the host.
    USBCTRL_REGS->INTE.bit.BUS_RESET   = 1U; // note: this interrupt is needed to detect a reset state on the USB bus.
    USBCTRL_REGS->INTE.bit.SETUP_REQ   = 1U; // note: this interrupt is needed to notify about a received SETUP packet.

    //enable NVIC
    NVIC_EnableIRQ(USBCTRL_IRQ_IRQn);
    __enable_irq();


    //while(__DEBUG_HALT__);
    USBCTRL_REGS->SIE_CTRL.bit.PULLUP_EN      = 1U;

}
