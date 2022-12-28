/**

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_LWIPDEVICE_H_
#define MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_LWIPDEVICE_H_

#include "MpTcpIp.h"

extern MP_LIST   DeviceList;

typedef struct _LWIP_NET_DEVICE {
  //
  // EFI specific
  //
  UINT32                            Signature;
  EFI_HANDLE                        ControllerHandle;
  EFI_HANDLE                        ImageHandle;

  //
  // DHCP4
  //
  volatile BOOLEAN                  DhcpSbEnabled;
  EFI_SERVICE_BINDING_PROTOCOL      Dhcp4Sb;
  MP_FAIR_LOCK                      Dhcp4SbLock;
  MP_LIST                           Dhcp4ChildList;
  EFI_HANDLE                        Dhcp4ActiveChild;
  struct dhcp_option                *DhcpOptions;
  UINT32                            DhcpOptionsLength;

  volatile BOOLEAN                  DhcpCallbackSem;
  EFI_EVENT                         DhcpCallbackEvent;

  //
  // MNP
  //
//  EFI_HANDLE                        MnpChildHandle;
//  EFI_MANAGED_NETWORK_PROTOCOL      *Mnp;
//  BOOLEAN                           WaitingForTransmit;
//  EFI_MANAGED_NETWORK_CONFIG_DATA   MnpConfigData;

  //
  // SNP
  //
  EFI_SIMPLE_NETWORK_PROTOCOL       *Snp;
  EFI_SIMPLE_NETWORK_MODE           SnpModeData;
  UINT8                             *TxBuffer;

  //
  // LWIP
  //
  BOOLEAN                           IsUp;
  struct netif                      NetIf;
  struct pbuf                       *PbufCache;

  //
  // MP
  //
  MP_LIST_ENTRY                     *Entry;
} LWIP_NET_DEVICE;

#define LWIP_NET_DEVICE_SIGNATURE     SIGNATURE_32 ('L', 'd', 'e', 'v')

#define LWIP_DEVICE_FROM_NETIF(a)   \
  CR ( \
  (a), \
  LWIP_NET_DEVICE, \
  NetIf, \
  LWIP_NET_DEVICE_SIGNATURE \
  )


EFI_STATUS
LwipDeviceResourcesInit (
  );

EFI_STATUS
LwipDeviceAdd (
  EFI_HANDLE    ImageHandle,
  EFI_HANDLE    ControllerHandle
  );

VOID
LwipDeviceRemove (
  EFI_HANDLE    ControllerHandle
  );

EFI_STATUS
LwipStopAllDevices (
  );

#endif /* MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_LWIPDEVICE_H_ */
