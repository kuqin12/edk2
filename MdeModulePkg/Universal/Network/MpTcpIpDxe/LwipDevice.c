/**

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "LwipDevice.h"
#include "LwipDhcpSb.h"
#include "LwipMp.h"
#include "MpTcpIp.h"

#define LWIP_NET_DEVICE_PACKET_QUEUE_SIZE     1024

MP_LIST   DeviceList;

#pragma pack(1)
typedef struct {
  EFI_MANAGED_NETWORK_TRANSMIT_DATA     Data;
  EFI_MANAGED_NETWORK_FRAGMENT_DATA     Fragments[16];
} EFI_MANAGED_NETWORK_TRANSMIT_DATA2;

typedef struct _LWIP_TRANSMIT_CONTEXT {
  LWIP_NET_DEVICE                       *Device;
  EFI_MANAGED_NETWORK_COMPLETION_TOKEN  Token;
  EFI_MANAGED_NETWORK_TRANSMIT_DATA2    TxData;
} LWIP_TRANSMIT_CONTEXT;
#pragma pack()


err_t
LwipDeviceInitialize (
  struct netif *netif
  );

VOID
LwipDeviceDestroy (
  LWIP_NET_DEVICE     *Device
  );

VOID
LwipPacketInput (
  EFI_MANAGED_NETWORK_PROTOCOL    *Mnp,
  VOID                            *Packet,
  VOID                            *Context
  );

err_t
LwipPacketOutput (
  struct netif *netif,
  struct pbuf *p
  );

EFI_STATUS
LwipDeviceResourcesInit (
  )
{
  MpListInitialize (&DeviceList);

  return EFI_SUCCESS;
}



EFI_STATUS
LwipStopAllDevices (
  )
{
  MP_LIST_ENTRY   *Entry;
  LWIP_NET_DEVICE *Device;

  DBG ("[MP TCPIP] LwipStopAllDevices: Entered.\n");

  MpListLock (&DeviceList);

  DBG ("[MP TCPIP] LwipStopAllDevices: Got list lock.\n");

  Entry = MpListGetFrontUnsafe (&DeviceList);
  while (Entry != NULL) {
    Device = (LWIP_NET_DEVICE*)Entry->Data;
    ASSERT (Device != NULL);
    NET_CHECK_SIGNATURE (Device, LWIP_NET_DEVICE_SIGNATURE);
    DBG ("[MP TCPIP] LwipStopAllDevices: Setting IsUp to FALSE...\n");
    Device->IsUp = FALSE;
    DBG ("[MP TCPIP] LwipStopAllDevices: Setting interface DOWN...\n");
    netifapi_netif_set_down (&Device->NetIf);
    DBG ("[MP TCPIP] LwipStopAllDevices: Setting interface link DOWN...\n");
    netifapi_netif_set_link_down (&Device->NetIf);
    DBG ("[MP TCPIP] LwipStopAllDevices: Getting next entry...\n");
    Entry = Entry->Next;
  }

  DBG ("[MP TCPIP] LwipStopAllDevices: Unlocking device list...\n");

  MpListUnlock (&DeviceList);

  DBG ("[MP TCPIP] LwipStopAllDevices: Done.\n");

  return EFI_SUCCESS;
}

EFI_STATUS
LwipDeviceAdd (
  EFI_HANDLE    ImageHandle,
  EFI_HANDLE    ControllerHandle
  )
{
  EFI_STATUS        Status;
  LWIP_NET_DEVICE   *Device;
  err_t             LwipStatus;

  //
  // Create device structure
  //
  Device = AllocateZeroPool (sizeof (LWIP_NET_DEVICE));

  if (Device == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Device->Signature           = LWIP_NET_DEVICE_SIGNATURE;
  Device->ControllerHandle    = ControllerHandle;
  Device->ImageHandle         = ImageHandle;

  //
  // Add interface to LWIP
  //
  LwipStatus = netifapi_netif_add(
                 &Device->NetIf,         // netif
                 IP4_ADDR_ANY,           // ipaddr
                 IP4_ADDR_ANY,           // netmask
                 IP4_ADDR_ANY,           // gw
                 Device,
                 LwipDeviceInitialize,
                 tcpip_input
                 );

  if (LwipStatus != ERR_OK) {
    Status = EFI_DEVICE_ERROR;
    DBG ("[MP TCPIP] netif_add failed.\n");
    goto Cleanup;
  }

  DBG ("[MP TCPIP] netif_add OK.\n");

  //
  // Enable Rx & Tx on the device
  //
  netifapi_netif_set_up (&Device->NetIf);
  netifapi_netif_set_link_up (&Device->NetIf);
  Device->IsUp = TRUE;

  //
  // Add device to the list
  //
  MpListPushBack (&DeviceList, Device);

  //
  // Assign device to one of the Poll APs
  //
  LwipAssignDevice (Device);

  return EFI_SUCCESS;

Cleanup:
  LwipDeviceDestroy (Device);

  return Status;
}

typedef struct _DEVICE_LIST_FIND_DEVICE_ARGS {
  EFI_HANDLE        ControllerHandle;
  MP_LIST_ENTRY     *DeviceEntry;
  LWIP_NET_DEVICE   *Device;
} DEVICE_LIST_FIND_DEVICE_ARGS;

EFI_STATUS
DeviceListFindDevice (
  MP_LIST_ENTRY     *Entry,
  VOID              *Arg
  )
{
  DEVICE_LIST_FIND_DEVICE_ARGS  *Args;
  LWIP_NET_DEVICE               *Device;

  Args = (DEVICE_LIST_FIND_DEVICE_ARGS*) Arg;
  Device = (LWIP_NET_DEVICE*) Entry->Data;

  NET_CHECK_SIGNATURE (Device, LWIP_NET_DEVICE_SIGNATURE);
  ASSERT (Args->ControllerHandle != NULL);

  if (Device->ControllerHandle == Args->ControllerHandle) {
    //
    // Return error code to stop further iterating
    //
    Args->DeviceEntry = Entry;
    Args->Device      = Device;
    return EFI_ALREADY_STARTED;
  }

  return EFI_SUCCESS;
}

VOID
LwipDeviceDestroy (
  LWIP_NET_DEVICE     *Device
  )
{
  if (Device == NULL) {
    return;
  }

  NET_CHECK_SIGNATURE (Device, LWIP_NET_DEVICE_SIGNATURE);

  Device->IsUp = FALSE;

  //
  // Release device from Poll AP
  //
  LwipReleaseDevice (Device);

  //
  // Uninstall DHCP (if present)
  //
  LwipDhcp4Uninstall (Device);

  if (Device->Snp != NULL) {
    netifapi_netif_set_down (&Device->NetIf);
    netifapi_netif_remove (&Device->NetIf);
    Device->Snp->Stop (Device->Snp);

    gBS->CloseProtocol (
           Device->ControllerHandle,
           &gEfiSimpleNetworkProtocolGuid,
           Device->ImageHandle,
           Device->ControllerHandle
           );
  }

  if (Device->PbufCache != NULL) {
    pbuf_free (Device->PbufCache);
  }

  if (Device->TxBuffer) {
    FreePool (Device->TxBuffer);
  }

  FreePool (Device);
}

VOID
LwipDeviceRemove (
  EFI_HANDLE    ControllerHandle
  )
{
  LWIP_NET_DEVICE     *Device;
  MP_LIST_ENTRY       *Entry;
  BOOLEAN             DeviceFound;

  //
  // Find device within device list (with MpListIterate)
  //
  DeviceFound = FALSE;

  DBG ("ControllerHandle to match: %lX\n", ControllerHandle);

  MP_LIST_FOREACH (&DeviceList, Entry, Device, LWIP_NET_DEVICE*) {

    DBG ("ControllerHandle of device: %lX\n", Device->ControllerHandle);
    if (Device->ControllerHandle == ControllerHandle) {
      DeviceFound = TRUE;
      break;
    }
  } MP_LIST_FOREACH_END (&DeviceList);

  ASSERT (DeviceFound);

  MpListRemove (Entry);
  LwipDeviceDestroy (Device);
}

err_t
LwipDeviceInitialize (
  struct netif *netif
  )
{
  EFI_STATUS        Status;
  LWIP_NET_DEVICE   *Device;
  UINT32            EnableFilterBits;
  UINT32            DisableFilterBits;

  Device = LWIP_DEVICE_FROM_NETIF (netif);

  ASSERT (Device != NULL);

  //
  // Open SNP interface
  //
  Status = gBS->OpenProtocol (
                  Device->ControllerHandle,
                  &gEfiSimpleNetworkProtocolGuid,
                  (VOID **) &Device->Snp,
                  Device->ImageHandle,
                  Device->ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return ERR_IF;
  }

  //
  // Start SNP interface
  //
  Status = Device->Snp->Start (Device->Snp);

  if (EFI_ERROR (Status)) {
    return ERR_IF;
  }

  //
  // Initialize SNP interface
  //
  Status = Device->Snp->Initialize (Device->Snp, 0, 0);

  if (EFI_ERROR (Status)) {
    return ERR_IF;
  }

  //
  // Setup receive filters
  //
  EnableFilterBits = EFI_SIMPLE_NETWORK_RECEIVE_UNICAST;
  EnableFilterBits |= EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST;
  DisableFilterBits = Device->Snp->Mode->ReceiveFilterMask;
  DisableFilterBits ^= EnableFilterBits;

  Status = Device->Snp->ReceiveFilters (
                          Device->Snp,
                          EnableFilterBits,
                          DisableFilterBits,
                          TRUE,
                          0,
                          NULL
                          );

  if (EFI_ERROR (Status)) {
    return ERR_IF;
  }

  //
  // Setup info based on SNP
  //
  Device->TxBuffer = AllocateZeroPool (Device->Snp->Mode->MaxPacketSize);
  if (Device->TxBuffer == NULL) {
    return ERR_IF;
  }

  netif->linkoutput = LwipPacketOutput;
  netif->output     = etharp_output;
  netif->mtu        = 1500;                 // ETHERNET_MTU
  netif->flags      |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;

  CopyMem (netif->hwaddr, Device->Snp->Mode->CurrentAddress.Addr, sizeof (netif->hwaddr));
  netif->hwaddr_len   = sizeof (netif->hwaddr);

  //
  // Add DHCP4 service binding
  //
  Status = LwipDhcp4Install (Device);

  if (EFI_ERROR (Status)) {
    return ERR_IF;
  }

  DBG ("[MP TCPIP] Device initialized successfully.\n");
  DBG ("[MP TCPIP] MAC address: "); DBG_MACADDR(netif->hwaddr); DBG ("\n");

  return ERR_OK;
}

err_t
LwipPacketOutput (
  struct netif *netif,
  struct pbuf *p
  )
{
  EFI_STATUS              Status;
  LWIP_NET_DEVICE         *Device;
  UINTN                   TotalSize;
  VOID                    *Buffer;
  struct pbuf             *Cp;
  UINT8                   *Position;
  VOID                    *TxBuf;

  Device = LWIP_DEVICE_FROM_NETIF (netif);

  if (!Device->IsUp) {
    return ERR_IF;
  }

//  DBG ("[MP TCPIP] Packet transmit attempt. Length: %d, Total length: %d\n", p->len, p->tot_len);

  if (p->len != p->tot_len) {
    Cp          = p;
    TotalSize   = 0;
    Position    = Device->TxBuffer;

    while (Cp && TotalSize + Cp->len < Device->Snp->Mode->MaxPacketSize) {
      CopyMem (Position, Cp->payload, Cp->len);
      Position += Cp->len;
      TotalSize += Cp->len;
      Cp = Cp->next;
    }
    ZeroMem (Position, Device->Snp->Mode->MaxPacketSize - TotalSize);
    Buffer = Device->TxBuffer;

//    DBG ("[MP TCPIP] Total length obtained: %d\n", TotalSize);
    ASSERT (TotalSize == p->tot_len);
    if (TotalSize != p->tot_len) {
      return ERR_IF;
    }
  } else {
    Buffer      = p->payload;
    TotalSize   = p->tot_len;
  }

  Status = Device->Snp->Transmit (
                          Device->Snp,
                          0,
                          TotalSize,
                          Buffer,
                          NULL,
                          NULL,
                          NULL
                          );

  if (Status == EFI_SUCCESS) {
    //
    // Wait for packet to be transmitted
    //
//    DEBUG ((EFI_D_INFO, "[MP TCPIP] Packet placed on transmit queue.\n"));
    TxBuf = NULL;

    do {
      Status = Device->Snp->GetStatus (
                              Device->Snp,
                              NULL,
                              &TxBuf
                              );
    } while (TxBuf != Buffer);

//    DEBUG ((EFI_D_INFO, "[MP TCPIP] Packet sent successfully.\n"));
    return ERR_OK;
  } else {
    return ERR_IF;
  }
}
