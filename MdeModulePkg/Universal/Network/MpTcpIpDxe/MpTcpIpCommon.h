/**

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __MP_TCPIP_COMMON_H__
#define __MP_TCPIP_COMMON_H__

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ThreadingLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeThreadingStructLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NetLib.h>
#include <Library/BaseLib.h>

#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/Dhcp4.h>
#include <Protocol/MpSocket.h>

#define DBG(...)        DEBUG ((EFI_D_WARN, __VA_ARGS__));
#define DBG_MACADDR(x)  DBG ("%02X:%02X:%02X:%02X:%02X:%02X", (x)[0], (x)[1], (x)[2], (x)[3], (x)[4], (x)[5])

#endif /* __MP_TCPIP_COMMON_H__ */
