/**

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_LWIPSOCKET_H_
#define MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_LWIPSOCKET_H_

#include "MpTcpIpCommon.h"

EFI_STATUS
LwipInitializeSocketProtocol (
  VOID
  );

#endif /* MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_LWIPSOCKET_H_ */
