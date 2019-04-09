/*
 * LwipDhcpSb.h
 *
 *  Created on: Aug 22, 2018
 *      Author: mrabeda
 */

#ifndef MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_DHCP_LWIPDHCPSB_H_
#define MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_DHCP_LWIPDHCPSB_H_

#include "MpTcpIp.h"
#include "LwipDhcp.h"

EFI_STATUS
EFIAPI
LwipDhcp4ServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                *ChildHandle
  );

EFI_STATUS
EFIAPI
LwipDhcp4ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  );

EFI_STATUS
LwipDhcp4Install (
  LWIP_NET_DEVICE     *Device
  );

VOID
LwipDhcp4Uninstall (
  LWIP_NET_DEVICE     *Device
  );

#endif /* MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_DHCP_LWIPDHCPSB_H_ */
