/**

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_LWIPMP_H_
#define MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_LWIPMP_H_

#include "LwipDevice.h"

EFI_STATUS
LwipInitializePollingThreads (
  );

EFI_STATUS
LwipUninstallPollingThreads (
  );

EFI_STATUS
LwipAssignDevice (
  IN  LWIP_NET_DEVICE   *Device
  );

EFI_STATUS
LwipReleaseDevice (
  IN  LWIP_NET_DEVICE   *Device
  );

#endif /* MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_LWIPMP_H_ */
