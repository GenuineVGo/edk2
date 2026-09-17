/*++ @file

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PlatformBm.h"

#include <Guid/SerialPortLibVendor.h>
typedef struct {
  VENDOR_DEVICE_PATH        VendorHw;
  UART_DEVICE_PATH          Uart;
  VENDOR_DEVICE_PATH        TerminalType;
  EFI_DEVICE_PATH_PROTOCOL  End;
} EMU_PLATFORM_SERIAL_CONSOLE_DEVICE_PATH;
EMU_PLATFORM_SERIAL_CONSOLE_DEVICE_PATH  gSerialConsoleDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH, HW_VENDOR_DP,
      { (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8) }
    },
    EDKII_SERIAL_PORT_LIB_VENDOR_GUID
  },
  {
    {
      MESSAGING_DEVICE_PATH, MSG_UART_DP,
      { (UINT8)(sizeof (UART_DEVICE_PATH)),
        (UINT8)((sizeof (UART_DEVICE_PATH)) >> 8) }
    },
    0,        // Reserved
    115200,   // BaudRate
    8,        // DataBits
    1,        // Parity   = NoParity
    1         // StopBits = OneStopBit
  },
  {
    {
      MESSAGING_DEVICE_PATH, MSG_VENDOR_DP,
      { (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8) }
    },
    DEVICE_PATH_MESSAGING_VT_100
  },
  gEndEntire
};
EMU_PLATFORM_GRAPHICS_WINDOW_DEVICE_PATH  gGopDevicePath = {
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8)(sizeof (EMU_VENDOR_DEVICE_PATH_NODE)),
          (UINT8)((sizeof (EMU_VENDOR_DEVICE_PATH_NODE)) >> 8)
        }
      },
      EMU_THUNK_PROTOCOL_GUID
    },
    0
  },
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8)(sizeof (EMU_VENDOR_DEVICE_PATH_NODE)),
          (UINT8)((sizeof (EMU_VENDOR_DEVICE_PATH_NODE)) >> 8)
        },
      },
      EMU_GRAPHICS_WINDOW_PROTOCOL_GUID,
    },
    0
  },
  gEndEntire
};

EMU_PLATFORM_GRAPHICS_WINDOW_DEVICE_PATH  gGopDevicePath2 = {
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8)(sizeof (EMU_VENDOR_DEVICE_PATH_NODE)),
          (UINT8)((sizeof (EMU_VENDOR_DEVICE_PATH_NODE)) >> 8)
        }
      },
      EMU_THUNK_PROTOCOL_GUID
    },
    0
  },
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8)(sizeof (EMU_VENDOR_DEVICE_PATH_NODE)),
          (UINT8)((sizeof (EMU_VENDOR_DEVICE_PATH_NODE)) >> 8)
        },
      },
      EMU_GRAPHICS_WINDOW_PROTOCOL_GUID,
    },
    1
  },
  gEndEntire
};

//
// Predefined platform default console device path
//
BDS_CONSOLE_CONNECT_ENTRY  gPlatformConsole[] = {
  {
    (EFI_DEVICE_PATH_PROTOCOL *)&gGopDevicePath,
    (CONSOLE_OUT | CONSOLE_IN)
  },
  {
    (EFI_DEVICE_PATH_PROTOCOL *)&gGopDevicePath2,
    (CONSOLE_OUT | CONSOLE_IN)
  },
  {
    (EFI_DEVICE_PATH_PROTOCOL *)&gSerialConsoleDevicePath,
    (CONSOLE_OUT | CONSOLE_IN)
  },
  {
    NULL,
    0
  }
};
