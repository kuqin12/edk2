/**

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __MP_TCPIP_H__
#define __MP_TCPIP_H__

#include "lwip/netif.h"
#include "lwip/init.h"
#include "lwip/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"
#include "lwip/tcpip.h"
#include "lwip/netifapi.h"
#include "lwip/ip_addr.h"
#include "lwip/sockets.h"

#include "lwip/prot/dhcp.h"

#include "LwipDevice.h"
#include "MpTcpIpCommon.h"

#pragma warning (disable : 4702)    /* unreachable code warning */

extern EFI_DRIVER_BINDING_PROTOCOL   gMpTcpIpDriverBinding;

#endif /* __MP_TCPIP_H__ */
