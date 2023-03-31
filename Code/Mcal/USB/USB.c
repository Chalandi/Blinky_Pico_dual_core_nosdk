
#include "RP2040.h"
#include "USB.h"
#include "usb_hwreg.h"
#include "usb_types.h"

volatile unsigned long long _EP0_[100] = {0};
volatile unsigned int _EP0_index = 0;
volatile unsigned int BusResetCounter = 0;
volatile unsigned int UsbDeviceAddress = 0;

volatile unsigned int UsbReceived_EP0_OUT_count = 0;
volatile unsigned int UsbReceived_EP0_IN_count = 0;

volatile unsigned int UsbNotSupportedRequestCount = 0;

static void UsbDriver_HandleSetupPacket(const tUsbSetupPacket* const pUsbSetupPacket);
static boolean UsbDriver_SendDataToHost(uint8 endpoint, uint8 pid, uint8* buffer, uint8 size);
static boolean UsbDriver_PrepareOutBufForReceiveDataFromHost(uint8 endpoint, uint8 pid, uint8 size);
static boolean UsbDriver_SendStallToHost(uint8 endpoint, uint8 pid);

typedef void (*pStandardRequestHandler)(const tUsbSetupPacket* const pUsbSetupPacket);

static void UsbDriver_Req_get_status        (const tUsbSetupPacket* const pUsbSetupPacket);
static void UsbDriver_Req_clear_feature     (const tUsbSetupPacket* const pUsbSetupPacket);
static void UsbDriver_Req_set_feature       (const tUsbSetupPacket* const pUsbSetupPacket);
static void UsbDriver_Req_set_address       (const tUsbSetupPacket* const pUsbSetupPacket);
static void UsbDriver_Req_get_descriptor    (const tUsbSetupPacket* const pUsbSetupPacket);
static void UsbDriver_Req_set_descriptor    (const tUsbSetupPacket* const pUsbSetupPacket);
static void UsbDriver_Req_get_configuration (const tUsbSetupPacket* const pUsbSetupPacket);
static void UsbDriver_Req_set_configuration (const tUsbSetupPacket* const pUsbSetupPacket);
static void UsbDriver_Req_get_interface     (const tUsbSetupPacket* const pUsbSetupPacket);
static void UsbDriver_Req_set_interface     (const tUsbSetupPacket* const pUsbSetupPacket);
static void UsbDriver_Req_synch_frame       (const tUsbSetupPacket* const pUsbSetupPacket);

volatile unsigned int __DEBUG_HALT__ = 1;

const pStandardRequestHandler StandardRequestHandlerLockupTable[13] = {
                                                                        UsbDriver_Req_get_status,
                                                                        UsbDriver_Req_clear_feature,
                                                                        NULL,
                                                                        UsbDriver_Req_set_feature,
                                                                        NULL,
                                                                        UsbDriver_Req_set_address,
                                                                        UsbDriver_Req_get_descriptor,
                                                                        UsbDriver_Req_set_descriptor,
                                                                        UsbDriver_Req_get_configuration,
                                                                        UsbDriver_Req_set_configuration,
                                                                        UsbDriver_Req_get_interface,
                                                                        UsbDriver_Req_set_interface,
                                                                        UsbDriver_Req_synch_frame
                                                                      };

//-----------------------------------------------------------------------------------------
/// \brief  
///
/// \param  
///
/// \return 
//-----------------------------------------------------------------------------------------
void USBCTRL_IRQ(void)
{
  /* handle SETUP packets */
  if(USBCTRL_REGS->INTS.bit.SETUP_REQ)
  {
    /* clear the interrupt */
    USBCTRL_REGS->SIE_STATUS.bit.SETUP_REC = 1U;

    /* get the SETUP packet data */
    const tUsbSetupPacket* const UsbSetupPacket = (const tUsbSetupPacket* const)USBCTRL_DPRAM_BASE;
    
    /* call the appropriate SETUP packet handler */
    UsbDriver_HandleSetupPacket(UsbSetupPacket);

    //save the data
    _EP0_[_EP0_index++] = *(volatile unsigned long long*)(USBCTRL_DPRAM_BASE);
  }
  
  /* handle bus reset */
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
      UsbDriver_PrepareOutBufForReceiveDataFromHost(EP0, DATA1_PID, 0);
    }
  }
}

//-----------------------------------------------------------------------------------------
/// \brief  
///
/// \param  
///
/// \return 
//-----------------------------------------------------------------------------------------
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

    /* enable endpoint 1 */
    USBCTRL_DPRAM->EP1_IN_CONTROL.reg                = 0;
    USBCTRL_DPRAM->EP1_IN_CONTROL.bit.ENDPOINT_TYPE  = 3;
    USBCTRL_DPRAM->EP1_IN_CONTROL.bit.BUFFER_ADDRESS = (USBCTRL_DPRAM_BASE + 0x100ul + (1 * 0x80ul)) >> 6;
    USBCTRL_DPRAM->EP1_IN_CONTROL.bit.ENABLE         = 1u;

    USBCTRL_DPRAM->EP1_OUT_CONTROL.reg                = 0;
    USBCTRL_DPRAM->EP1_OUT_CONTROL.bit.ENDPOINT_TYPE  = 3;
    USBCTRL_DPRAM->EP1_OUT_CONTROL.bit.BUFFER_ADDRESS = (USBCTRL_DPRAM_BASE + 0x100ul + (1 * 0x80ul)) >> 6;
    USBCTRL_DPRAM->EP1_OUT_CONTROL.bit.ENABLE         = 1u;

    //while(__DEBUG_HALT__);
    USBCTRL_REGS->SIE_CTRL.bit.PULLUP_EN      = 1U;

}

//-----------------------------------------------------------------------------------------
/// \brief  
///
/// \param  
///
/// \return 
//-----------------------------------------------------------------------------------------
static boolean UsbDriver_SendDataToHost(uint8 endpoint, uint8 pid, uint8* buffer, uint8 size)
{
  boolean status = FALSE;

  EPx_BUFFER_CONTROL* epx_in_buffer_control = (EPx_BUFFER_CONTROL*)(USBCTRL_DPRAM_BASE + EPx_IN_BUFFER_CONTROL_OFFSET + (endpoint * 8ul));

  if((size < 65u) && (endpoint < 16u) && (pid < 2u))
  {
    if(buffer != NULL)
    {
      for(uint8 i = 0; i < size; i++)
      {
        ((volatile uint8*)(USBCTRL_DPRAM_BASE + 0x100ul + (endpoint * 0x80ul)))[i] = buffer[i];
      }
    }

    epx_in_buffer_control->reg             = 0;
    epx_in_buffer_control->bit.LENGTH_0    = size;
    epx_in_buffer_control->bit.AVAILABLE_0 = 1U;
    epx_in_buffer_control->bit.PID_0       = (pid == 0 ? 0 : 1);
    epx_in_buffer_control->bit.FULL_0      = 1U;

    status = TRUE;
  }

  return(status);
}

//-----------------------------------------------------------------------------------------
/// \brief  
///
/// \param  
///
/// \return 
//-----------------------------------------------------------------------------------------
static boolean UsbDriver_SendStallToHost(uint8 endpoint, uint8 pid)
{
  boolean status = FALSE;

  EPx_BUFFER_CONTROL* epx_in_buffer_control = (EPx_BUFFER_CONTROL*)(USBCTRL_DPRAM_BASE + EPx_IN_BUFFER_CONTROL_OFFSET + (endpoint * 8ul));

  if((endpoint < 16u) && (pid < 2u))
  {
    epx_in_buffer_control->reg             = 0;
    epx_in_buffer_control->bit.LENGTH_0    = 0;
    epx_in_buffer_control->bit.AVAILABLE_0 = 1U;
    epx_in_buffer_control->bit.PID_0       = (pid == 0 ? 0 : 1);
    epx_in_buffer_control->bit.STALL       = 1U;

    if(endpoint == EP0)
    {
      USBCTRL_REGS->EP_STALL_ARM.bit.EP0_IN  = 1U;
    }
    epx_in_buffer_control->bit.FULL_0      = 1U;

    status = TRUE;
  }

  return(status);
}
//-----------------------------------------------------------------------------------------
/// \brief  
///
/// \param  
///
/// \return 
//-----------------------------------------------------------------------------------------
static boolean UsbDriver_PrepareOutBufForReceiveDataFromHost(uint8 endpoint, uint8 pid, uint8 size)
{
  boolean status = FALSE;

  EPx_BUFFER_CONTROL* epx_out_buffer_control = (EPx_BUFFER_CONTROL*)(USBCTRL_DPRAM_BASE + EPx_OUT_BUFFER_CONTROL_OFFSET + (endpoint * 8ul));
  
  if((size < 65u) && (endpoint < 16u) && (pid < 2u))
  {
    /* configure the expected OUT packet */
    epx_out_buffer_control->reg             = 0;
    epx_out_buffer_control->bit.LENGTH_0    = size;
    epx_out_buffer_control->bit.AVAILABLE_0 = 1U;
    epx_out_buffer_control->bit.PID_0       = (pid == 0 ? 0 : 1);
    epx_out_buffer_control->bit.FULL_0      = 0U;
    status = TRUE;
  }

  return(status);
}

//-----------------------------------------------------------------------------------------
/// \brief  
///
/// \param  
///
/// \return 
//-----------------------------------------------------------------------------------------
static void UsbDriver_HandleSetupPacket(const tUsbSetupPacket* const pUsbSetupPacket)
{
  const uint8 Request = pUsbSetupPacket->bRequest;

  if(Request < 13u && StandardRequestHandlerLockupTable[Request] != NULL)
  {
    /* call the appropriate handler */
    StandardRequestHandlerLockupTable[Request](pUsbSetupPacket);
  }
}

//-----------------------------------------------------------------------------------------
/// \brief  
///
/// \param  
///
/// \return 
//-----------------------------------------------------------------------------------------
static void UsbDriver_Req_get_status(const tUsbSetupPacket* const pUsbSetupPacket)
{
}

//-----------------------------------------------------------------------------------------
/// \brief  
///
/// \param  
///
/// \return 
//-----------------------------------------------------------------------------------------
static void UsbDriver_Req_clear_feature(const tUsbSetupPacket* const pUsbSetupPacket)
{
}

//-----------------------------------------------------------------------------------------
/// \brief  
///
/// \param  
///
/// \return 
//-----------------------------------------------------------------------------------------
static void UsbDriver_Req_set_feature(const tUsbSetupPacket* const pUsbSetupPacket)
{
}

//-----------------------------------------------------------------------------------------
/// \brief  
///
/// \param  
///
/// \return 
//-----------------------------------------------------------------------------------------
static void UsbDriver_Req_set_address(const tUsbSetupPacket* const pUsbSetupPacket)
{
  UsbDeviceAddress = pUsbSetupPacket->wValue;
  UsbDriver_SendDataToHost(EP0, DATA1_PID, NULL, 0);
}

//-----------------------------------------------------------------------------------------
/// \brief  
///
/// \param  
///
/// \return 
//-----------------------------------------------------------------------------------------
static void UsbDriver_Req_get_descriptor(const tUsbSetupPacket* const pUsbSetupPacket)
{
  const uint8 DescriptorType = (uint8)(pUsbSetupPacket->wValue >> 8);

  if(USB_DESCRIPTOR_TYPE_DEVICE == DescriptorType)
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
                                                0x0a,0x00, // idProduct
                                                0x01,0x00, // bcdDevice
                                                0x01,      // iManufacturer
                                                0x02,      // iProduct
                                                0x03,      // iSerialNumber
                                                0x01       // bNumConfigurations
                                              };
      UsbDriver_SendDataToHost(EP0, DATA1_PID, (uint8*)device_dsc, sizeof(device_dsc));
    }
    else if(USB_DESCRIPTOR_TYPE_CONFIGURATION == DescriptorType)
    {
      /* send configuration descriptor */
      const unsigned char configuration_dsc[]={
                                                 // Configuration Descriptor
                                                 0x09,                   // bLength             - Descriptor size in bytes
                                                 0x02,                   // bDescriptorType     - The constant CONFIGURATION (02h)
                                                 0x29,0x00,              // wTotalLength        - The number of bytes in the configuration descriptor and all of its subordinate descriptors
                                                 1,                      // bNumInterfaces      - Number of interfaces in the configuration
                                                 1,                      // bConfigurationValue - Identifier for Set Configuration and Get Configuration requests
                                                 0,                      // iConfiguration      - Index of string descriptor for the configuration
                                                 0x80,                   // bmAttributes        - Self/bus power and remote wakeup settings (Self powered 0xC0,  0x80 bus powered)
                                                 50,                     // bMaxPower           - Bus power required in units of 2 mA
                                             
                                                 // Interface Descriptor
                                                 0x09,                   // bLength - Descriptor size in bytes (09h)
                                                 0x04,                   // bDescriptorType - The constant Interface (04h)
                                                 0,                      // bInterfaceNumber - Number identifying this interface
                                                 0,                      // bAlternateSetting - A number that identifies a descriptor with alternate settings for this bInterfaceNumber.
                                                 2,                      // bNumEndpoint - Number of endpoints supported not counting endpoint zero
                                                 0x03,                   // bInterfaceClass - Class code
                                                 0,                      // bInterfaceSubclass - Subclass code
                                                 0,                      // bInterfaceProtocol - Protocol code
                                                 0,                      // iInterface - Interface string index
                                             
                                                 // HID Class-Specific Descriptor
                                                 0x09,                   // bLength - Descriptor size in bytes.
                                                 0x21,                   // bDescriptorType - This descriptor's type: 21h to indicate the HID class.
                                                 0x01,0x01,              // bcdHID - HID specification release number (BCD).
                                                 0x00,                   // bCountryCode - Numeric expression identifying the country for localized hardware (BCD) or 00h.
                                                 1,                      // bNumDescriptors - Number of subordinate report and physical descriptors.
                                                 0x22,                   // bDescriptorType - The type of a class-specific descriptor that follows
                                                 33,0x00,                // wDescriptorLength - Total length of the descriptor identified above.
                                             
                                                 // Endpoint Descriptor
                                                 0x07,                   // bLength - Descriptor size in bytes (07h)
                                                 0x05,                   // bDescriptorType - The constant Endpoint (05h)
                                                 1 | 0x80,               // bEndpointAddress - Endpoint number and direction
                                                 0x03,                   // bmAttributes - Transfer type and supplementary information    
                                                 0x40,0x00,              // wMaxPacketSize - Maximum packet size supported
                                                 255,                    // bInterval - Service interval or NAK rate
                                             
                                                 // Endpoint Descriptor
                                                 0x07,                   // bLength - Descriptor size in bytes (07h)
                                                 0x05,                   // bDescriptorType - The constant Endpoint (05h)
                                                 1,                      // bEndpointAddress - Endpoint number and direction
                                                 0x03,                   // bmAttributes - Transfer type and supplementary information
                                                 0x40,0x00,              // wMaxPacketSize - Maximum packet size supported
                                                 255                     // bInterval - Service interval or NAK rate
                                             };

      /* check the configuration size requested by the host 
         note: we must send exactly the requested size otherwise the host will abort the enumeration process */
      const uint8 size = (uint8)(pUsbSetupPacket->wLength) > sizeof(configuration_dsc) ? sizeof(configuration_dsc) : (uint8)(pUsbSetupPacket->wLength);

      /* send the configuration to the host */
      UsbDriver_SendDataToHost(EP0, DATA1_PID, (uint8*)configuration_dsc, (uint8)size);
    }
    else if(USB_DESCRIPTOR_TYPE_HID_REPORT == DescriptorType)
    {
      const uint8 hid_rpt_desc[] =
        {
            0x06, 0x00, 0xFF,       // Usage Page = 0xFF00 (Vendor Defined Page 1)
            0x09, 0x01,             // Usage (Vendor Usage 1)
            0xA1, 0x01,             // Collection (Application)
            // Input report
            0x19, 0x01,             // Usage Minimum
            0x29, 0x40,             // Usage Maximum
            0x15, 0x00,             // Logical Minimum (data bytes in the report may have minimum value = 0x00)
            0x26, 0xFF, 0x00,       // Logical Maximum (data bytes in the report may have maximum value = 0x00FF = unsigned 255)
            0x75, 0x08,             // Report Size: 8-bit field size
            0x95, 64,               // Report Count
            0x81, 0x02,             // Input (Data, Array, Abs)
            // Output report
            0x19, 0x01,             // Usage Minimum
            0x29, 0x40,             // Usage Maximum
            0x75, 0x08,             // Report Size: 8-bit field size
            0x95, 64,               // Report Count
            0x91, 0x02,             // Output (Data, Array, Abs)
            0xC0                    // End Collection
        };

         UsbDriver_SendDataToHost(EP0, DATA1_PID, (uint8*)hid_rpt_desc, sizeof(hid_rpt_desc));

    }
    else if(USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER == DescriptorType)
    {
      /* As DEVICE_QUALIFIER is used only for high speed we will not support it on RP2040 (has only a full speed device controller) */
      UsbDriver_SendStallToHost(EP0, DATA1_PID);
    }
    else if(USB_DESCRIPTOR_TYPE_STRING == DescriptorType)
    {
      const tUsbStringDescriptor UsbStringDescriptor = {4, 0x03, {0x0409}};
      const tUsbSubsequentStringDescriptor UsbSubsequentStringDescriptor_0 = {30, 0x03, {'C','H','A','L','A','N','D','I',' ','A','M','I','N','E'}};              /* Manufacturer string descriptor */
      const tUsbSubsequentStringDescriptor UsbSubsequentStringDescriptor_1 = {36, 0x03, {'C','H','A','L','A','N','D','I',' ','D','E','B','U','G','G','E','R'}};  /* Product string descriptor */
      const tUsbSubsequentStringDescriptor UsbSubsequentStringDescriptor_2 = {10, 0x03, {'2','0','2','3'}};                                                      /* SerialNumber */
     
      const uint8 StrIdx = (uint8)pUsbSetupPacket->wValue;

      if(StrIdx == 0u)
      {
        UsbDriver_SendDataToHost(EP0, DATA1_PID, (uint8*)&UsbStringDescriptor,UsbStringDescriptor.bLength);
      }
      else if(StrIdx == 1u)
      {
        UsbDriver_SendDataToHost(EP0, DATA1_PID, (uint8*)&UsbSubsequentStringDescriptor_0, UsbSubsequentStringDescriptor_0.bLength);
      }
      else if(StrIdx == 2u)
      {
        UsbDriver_SendDataToHost(EP0, DATA1_PID, (uint8*)&UsbSubsequentStringDescriptor_1, UsbSubsequentStringDescriptor_1.bLength);
      }
      else if(StrIdx == 3u)
      {
        UsbDriver_SendDataToHost(EP0, DATA1_PID, (uint8*)&UsbSubsequentStringDescriptor_2, UsbSubsequentStringDescriptor_2.bLength);
      }
      else
      {
        //for(;;);
        UsbDriver_SendStallToHost(EP0, DATA1_PID);
      }
    }
    else
    {

    }
}

//-----------------------------------------------------------------------------------------
/// \brief  
///
/// \param  
///
/// \return 
//-----------------------------------------------------------------------------------------
static void UsbDriver_Req_set_descriptor(const tUsbSetupPacket* const pUsbSetupPacket)
{
}

//-----------------------------------------------------------------------------------------
/// \brief  
///
/// \param  
///
/// \return 
//-----------------------------------------------------------------------------------------
static void UsbDriver_Req_get_configuration(const tUsbSetupPacket* const pUsbSetupPacket)
{
}

//-----------------------------------------------------------------------------------------
/// \brief  
///
/// \param  
///
/// \return 
//-----------------------------------------------------------------------------------------
static void UsbDriver_Req_set_configuration(const tUsbSetupPacket* const pUsbSetupPacket)
{
  /* as we have only one configuration just send ACK to the host */
  UsbDriver_SendDataToHost(EP0, DATA1_PID, NULL, 0);
}

//-----------------------------------------------------------------------------------------
/// \brief  
///
/// \param  
///
/// \return 
//-----------------------------------------------------------------------------------------
static void UsbDriver_Req_get_interface(const tUsbSetupPacket* const pUsbSetupPacket)
{
}

//-----------------------------------------------------------------------------------------
/// \brief  
///
/// \param  
///
/// \return 
//-----------------------------------------------------------------------------------------
static void UsbDriver_Req_set_interface(const tUsbSetupPacket* const pUsbSetupPacket)
{
}

//-----------------------------------------------------------------------------------------
/// \brief  
///
/// \param  
///
/// \return 
//-----------------------------------------------------------------------------------------
static void UsbDriver_Req_synch_frame(const tUsbSetupPacket* const pUsbSetupPacket)
{
}


