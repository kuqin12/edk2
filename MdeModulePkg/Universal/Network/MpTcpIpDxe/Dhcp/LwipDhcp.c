/*
 * LwipDhcp.c
 *
 *  Created on: Aug 21, 2018
 *      Author: mrabeda
 */

#include "LwipDhcp.h"
#include "LwipDhcpInternal.h"

typedef struct {
  EFI_DHCP4_PACKET_OPTION       **Option;
  UINT32                        OptionCount;
  UINT32                        Index;
} DHCP_PARSE_CONTEXT;

UINT8
LwipDhcpGetState (
  LWIP_NET_DEVICE       *Device
  )
{
  struct dhcp           *LwipDhcp;

  LwipDhcp = netif_dhcp_data (&Device->NetIf);
  if (LwipDhcp) {
    return LwipDhcp->state;
  } else {
    return (UINT8)(-1);
  }
}

EFI_STATUS
EFIAPI
LwipDhcp4GetModeData (
  IN  EFI_DHCP4_PROTOCOL    *This,
  OUT EFI_DHCP4_MODE_DATA   *Dhcp4ModeData
  )
{
  EFI_STATUS            Status = EFI_SUCCESS;
  LWIP_DHCP4_INSTANCE   *Instance;
  LWIP_NET_DEVICE       *Device;
  EFI_TPL               OldTpl;
  struct dhcp           *LwipDhcp;
  EFI_DHCP4_PACKET      *DhcpPacket;
  UINT32                DhcpPacketSize;

//  DBG ("[MP TCPIP] DHCP->GetModeData()\n");

  if (This == NULL || Dhcp4ModeData == NULL) {
    DBG ("[MP TCPIP] DHCP->GetModeData(): Parameters invalid.\n");
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP4_INSTANCE_FROM_PROTOCOL (This);
  Device = Instance->Device;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  LwipDhcp = netif_dhcp_data (&Device->NetIf);

  //
  // DHCP state
  //
  if (LwipDhcp) {
    switch (LwipDhcp->state) {
    case DHCP_STATE_OFF:
      Dhcp4ModeData->State = Dhcp4Stopped;
      break;
    case DHCP_STATE_INIT:
      Dhcp4ModeData->State = Dhcp4Init;
      break;
    case DHCP_STATE_CHECKING:
    case DHCP_STATE_SELECTING:
    case DHCP_STATE_BINDING:
      Dhcp4ModeData->State = Dhcp4Selecting;
      break;
    case DHCP_STATE_REQUESTING:
      Dhcp4ModeData->State = Dhcp4Requesting;
      break;
    case DHCP_STATE_BOUND:
      Dhcp4ModeData->State = Dhcp4Bound;
      break;
    case DHCP_STATE_RENEWING:
      Dhcp4ModeData->State = Dhcp4Renewing;
      break;
    case DHCP_STATE_REBINDING:
      Dhcp4ModeData->State = Dhcp4Rebinding;
      break;
    case DHCP_STATE_REBOOTING:
      Dhcp4ModeData->State = Dhcp4Rebooting;
      break;
    default:
      DBG ("[MP TCPIP] DHCP->GetModeData(): Unknown state of LWIP DHCP: %d!\n", LwipDhcp->state);
      ASSERT (FALSE);
    }
  } else {
    Dhcp4ModeData->State = Dhcp4Stopped;
  }

  //
  // MAC address
  //
  CopyMem (&Dhcp4ModeData->ClientMacAddress, Device->NetIf.hwaddr, Device->NetIf.hwaddr_len);
  Dhcp4ModeData->ClientMacAddressLength = Device->NetIf.hwaddr_len;

//  DBG ("[MP TCPIP] DHCP->GetModeData(): MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
//      Dhcp4ModeData->ClientMacAddress.Addr[0], Dhcp4ModeData->ClientMacAddress.Addr[1],
//      Dhcp4ModeData->ClientMacAddress.Addr[2], Dhcp4ModeData->ClientMacAddress.Addr[3],
//      Dhcp4ModeData->ClientMacAddress.Addr[4], Dhcp4ModeData->ClientMacAddress.Addr[5]);
//
//  DBG ("[MP TCPIP] DHCP->GetModeData(): MAC2: %02X:%02X:%02X:%02X:%02X:%02X\n",
//      Device->NetIf.hwaddr[0], Device->NetIf.hwaddr[1],
//      Device->NetIf.hwaddr[2], Device->NetIf.hwaddr[3],
//      Device->NetIf.hwaddr[4], Device->NetIf.hwaddr[5]);

  //
  // IP addresses
  //
  CopyMem (&Dhcp4ModeData->ClientAddress, &Device->NetIf.ip_addr, sizeof (Device->NetIf.ip_addr));
  CopyMem (&Dhcp4ModeData->SubnetMask, &Device->NetIf.netmask, sizeof (Device->NetIf.netmask));
  CopyMem (&Dhcp4ModeData->ServerAddress, &Device->NetIf.gw, sizeof (Device->NetIf.gw));

  Dhcp4ModeData->ReplyPacket = NULL;
  Dhcp4ModeData->DiscoverPacket = NULL;
  Dhcp4ModeData->AckPacket = NULL;

  if (LwipDhcp) {
    if (LwipDhcp->server_ip_addr.addr != 0) {
      CopyMem (&Dhcp4ModeData->RouterAddress, &LwipDhcp->server_ip_addr, sizeof (LwipDhcp->server_ip_addr));
    } else {
      ZeroMem (&Dhcp4ModeData->RouterAddress, sizeof (Dhcp4ModeData->RouterAddress));
    }

    //
    // Cached packet (if bound)
    //
    if (LwipDhcp->state == DHCP_STATE_BOUND) {
      if (LwipDhcp->p_offer != NULL && LwipDhcp->offer != NULL) {
        DhcpPacketSize = LwipDhcp->p_offer->len + sizeof (EFI_DHCP4_PACKET) - sizeof(((EFI_DHCP4_PACKET*)0)->Dhcp4);
        DhcpPacket = AllocateZeroPool (DhcpPacketSize);
        if (DhcpPacket != NULL) {
          DhcpPacket->Size = DhcpPacketSize;
          DhcpPacket->Length = LwipDhcp->p_offer->len;
          CopyMem (&DhcpPacket->Dhcp4, LwipDhcp->offer, DhcpPacket->Length);
          Dhcp4ModeData->ReplyPacket = DhcpPacket;
        } else {
          Status = EFI_OUT_OF_RESOURCES;
        }
      }
      if (LwipDhcp->p_discover != NULL && LwipDhcp->discover != NULL) {
        DhcpPacketSize = LwipDhcp->p_discover->len + sizeof (EFI_DHCP4_PACKET) - sizeof(((EFI_DHCP4_PACKET*)0)->Dhcp4);
        DhcpPacket = AllocateZeroPool (DhcpPacketSize);
        if (DhcpPacket != NULL) {
          DhcpPacket->Size = DhcpPacketSize;
          DhcpPacket->Length = LwipDhcp->p_discover->len;
          CopyMem (&DhcpPacket->Dhcp4, LwipDhcp->discover, DhcpPacket->Length);
          Dhcp4ModeData->DiscoverPacket = DhcpPacket;
        } else {
          Status = EFI_OUT_OF_RESOURCES;
        }
      }
      if (LwipDhcp->p_ack != NULL && LwipDhcp->ack != NULL) {
        DhcpPacketSize = LwipDhcp->p_ack->len + sizeof (EFI_DHCP4_PACKET) - sizeof(((EFI_DHCP4_PACKET*)0)->Dhcp4);
        DhcpPacket = AllocateZeroPool (DhcpPacketSize);
        if (DhcpPacket != NULL) {
          DhcpPacket->Size = DhcpPacketSize;
          DhcpPacket->Length = LwipDhcp->p_ack->len;
          CopyMem (&DhcpPacket->Dhcp4, LwipDhcp->ack, DhcpPacket->Length);
          Dhcp4ModeData->AckPacket = DhcpPacket;
        } else {
          Status = EFI_OUT_OF_RESOURCES;
        }
      }
    }
  }

  if (EFI_ERROR (Status)) {
    if (Dhcp4ModeData->ReplyPacket != NULL) {
      FreePool (Dhcp4ModeData->ReplyPacket);
    }
    if (Dhcp4ModeData->DiscoverPacket != NULL) {
      FreePool (Dhcp4ModeData->DiscoverPacket);
    }
    if (Dhcp4ModeData->AckPacket != NULL) {
      FreePool (Dhcp4ModeData->AckPacket);
    }
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

#define DHCP_OPTION_LENGTH(option) ((sizeof(struct dhcp_option) - 1) + (option)->Length)

EFI_STATUS
DhcpParseOptions (
  IN EFI_DHCP4_PACKET_OPTION    **Options,
  IN UINT32                     OptionCount,
  OUT struct dhcp_option        **LwipOptions,
  OUT UINT32                    *LwipOptionsLength
  )
{
  EFI_STATUS                Status;
  EFI_DHCP4_PACKET_OPTION   **DhcpOptionList;
  UINTN                     i;
  UINT8                     DhcpOptionCode;
  UINT32                    DhcpOptionLength;
  struct dhcp_option        *LwipDhcpOptions;
  struct dhcp_option        *LwipDhcpOption;

  if (Options == NULL || LwipOptions == NULL || LwipOptionsLength == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (OptionCount == 0) {
    return EFI_SUCCESS;
  }

  //
  // This will hold pointers to options ordered by option opcode.
  // Allows us to detect option duplication in input parameters.
  //
  DhcpOptionList = AllocateZeroPool (sizeof(EFI_DHCP4_PACKET_OPTION*) * 256);

  if (DhcpOptionList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DhcpOptionLength = 0;

  for (i = 0; i < OptionCount; i++) {
    DhcpOptionCode = Options[i]->OpCode;

    DBG ("[MP TCPIP] DhcpParseOptions: Option: %d\n", Options[i]->OpCode);
    DBG ("[MP TCPIP] DhcpParseOptions: Length: %d\n", Options[i]->Length);

    if (DhcpOptionList[DhcpOptionCode] != NULL) {
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }

    DhcpOptionList[DhcpOptionCode] = Options[i];
    DhcpOptionLength += DHCP_OPTION_LENGTH (Options[i]);
  }

  DBG ("[MP TCPIP] DhcpParseOptions: Option length: %d\n", DhcpOptionLength);

  if (DhcpOptionLength > 0) {
    LwipDhcpOptions = AllocateZeroPool (DhcpOptionLength);
    if (LwipDhcpOptions == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    LwipDhcpOption = LwipDhcpOptions;

    for (i = 0; i < 256; i++) {
      if (DhcpOptionList[i] == NULL) {
        continue;
      }

      LwipDhcpOption->opcode = DhcpOptionList[i]->OpCode;
      LwipDhcpOption->length = DhcpOptionList[i]->Length;
      CopyMem (LwipDhcpOption->data, DhcpOptionList[i]->Data, DhcpOptionList[i]->Length);
      LwipDhcpOption = NEXT_DHCP_OPTION (LwipDhcpOption);
    }

    *LwipOptions        = LwipDhcpOptions;
    *LwipOptionsLength  = DhcpOptionLength;
  }

  Status = EFI_SUCCESS;

ON_EXIT:
  FreePool (DhcpOptionList);
  return Status;
}

EFI_STATUS
EFIAPI
LwipDhcp4Configure (
  IN EFI_DHCP4_PROTOCOL     *This,
  IN EFI_DHCP4_CONFIG_DATA  *Dhcp4CfgData       OPTIONAL
  )
{
  EFI_STATUS            Status;
  err_t                 LwipStatus;
  LWIP_DHCP4_INSTANCE   *Instance;
  LWIP_NET_DEVICE       *Device;
  EFI_TPL               OldTpl;
  struct dhcp           *LwipDhcp;
  struct dhcp_option    *LwipDhcpOptions;
  struct dhcp_callback  *LwipDhcpCallback;
  UINT32                LwipDhcpOptionsLength;

  DBG ("[MP TCPIP] DHCP->Configure()\n");

  if (This == NULL) {
    DBG ("[MP TCPIP] DHCP->Configure(): Invalid parameters\n");
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP4_INSTANCE_FROM_PROTOCOL (This);
  Device = Instance->Device;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
//  MpFairLockAcquire (&Device->Dhcp4SbLock);

  LwipDhcp          = netif_dhcp_data (&Device->NetIf);
  LwipDhcpCallback  = netif_dhcp_callback_data (&Device->NetIf);

  Status  = EFI_ACCESS_DENIED;

  DBG ("[MP TCPIP] DHCP->Configure(): State: %d\n", LwipDhcp->state);

  switch (LwipDhcp->state) {
  case DHCP_STATE_OFF:
  case DHCP_STATE_INIT:
  case DHCP_STATE_BOUND:
    break;
  default:
    goto ON_EXIT;
  }

  if (Device->Dhcp4ActiveChild != NULL && Device->Dhcp4ActiveChild != Instance) {
    goto ON_EXIT;
  }

  if (Dhcp4CfgData) {
    DBG ("[MP TCPIP] DHCP->Configure(): Got config to process.\n");

    if (Dhcp4CfgData->OptionCount != 0 && Dhcp4CfgData->OptionList == NULL) {
      Status = EFI_INVALID_PARAMETER;
      goto ON_EXIT;
    }

    //
    // Initiate structure for DHCP event callbacks
    //
    if (LwipDhcpCallback == NULL) {
      LwipDhcpCallback = AllocateZeroPool (sizeof (struct dhcp_callback));
    }

    if (LwipDhcpCallback == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    } else {
      netif_set_client_data(
        &Device->NetIf,
        LWIP_NETIF_CLIENT_DATA_INDEX_DHCP_CALLBACK,
        LwipDhcpCallback
        );
    }

    //
    // Clear configuration first
    //
    Status = netifapi_dhcp_set_options (
               &Device->NetIf,
               NULL,
               0
               );

    //
    // Parse new configuration
    //
    Status = DhcpParseOptions (
               Dhcp4CfgData->OptionList,
               Dhcp4CfgData->OptionCount,
               &LwipDhcpOptions,
               &LwipDhcpOptionsLength
               );

    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    //
    // Set new options
    //
    LwipStatus = netifapi_dhcp_set_options (
                   &Device->NetIf,
                   LwipDhcpOptions,
                   LwipDhcpOptionsLength
                   );

    switch (LwipStatus) {
    case ERR_IF:
      //
      // DHCP not started yet. We will save the config on LWIP device to
      // be used upon DHCP->Start() call.
      //
      Status = EFI_SUCCESS;
      break;
    case ERR_VAL:
      Status = EFI_INVALID_PARAMETER;
      break;
    case ERR_MEM:
      Status = EFI_OUT_OF_RESOURCES;
      break;
    case ERR_OK:
      Status = EFI_SUCCESS;
      break;
    default:
      Status = EFI_DEVICE_ERROR;
      break;
    }

    if (EFI_ERROR (Status)) {
      FreePool (LwipDhcpOptions);
      goto ON_EXIT;
    }

    //
    // Save options on LWIP device for further use (on DHCP->Start() call)
    //
    Device->DhcpOptions       = LwipDhcpOptions;
    Device->DhcpOptionsLength = LwipDhcpOptionsLength;

    DBG ("[MP TCPIP] DHCP->Configure(): set options: %d\n", LwipStatus);

    //
    // Set this instance to active instance
    //
    Device->Dhcp4ActiveChild = Instance;

    //
    // Setup callbacking mechanism
    //
    if (Dhcp4CfgData->Dhcp4Callback) {
      Status = gBS->CreateEvent (
                      EVT_NOTIFY_SIGNAL,
                      TPL_CALLBACK,
                      DhcpNotifyUser,
                      Device,
                      &Device->DhcpCallbackEvent
                      );

      ASSERT_EFI_ERROR (Status);

      LwipDhcpCallback->callback = DhcpStatusCallback;
    }
  } else if (Device->Dhcp4ActiveChild == Instance) {
    //
    // TODO: Config reset
    //
    if (LwipDhcpCallback) {
      LwipDhcpCallback->callback = NULL;
      FreePool (LwipDhcpCallback);
      netif_set_client_data (
        &Device->NetIf,
        LWIP_NETIF_CLIENT_DATA_INDEX_DHCP_CALLBACK,
        NULL
        );
    }
    if (Device->DhcpCallbackEvent) {
      gBS->CloseEvent (Device->DhcpCallbackEvent);
    }
    Device->Dhcp4ActiveChild = NULL;
  }

  Status = EFI_SUCCESS;

ON_EXIT:
//  MpFairLockRelease (&Device->Dhcp4SbLock);
  gBS->RestoreTPL (OldTpl);

  DBG ("[MP TCPIP] DHCP->Configure(): %r\n", Status);

  return Status;
}

EFI_STATUS
EFIAPI
LwipDhcp4Start (
  IN EFI_DHCP4_PROTOCOL     *This,
  IN EFI_EVENT              CompletionEvent   OPTIONAL
  )
{
  EFI_STATUS            Status;
  LWIP_DHCP4_INSTANCE   *Instance;
  LWIP_NET_DEVICE       *Device;
  EFI_TPL               OldTpl;
  struct dhcp           *LwipDhcp;

  DBG ("[MP TCPIP] DHCP->Start()\n");

  if (This == NULL) {
    DBG ("[MP TCPIP] DHCP->Start(): Invalid parameters\n");
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP4_INSTANCE_FROM_PROTOCOL (This);
  Device = Instance->Device;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
//  MpFairLockAcquire (&Instance->Lock);

  LwipDhcp = netif_dhcp_data (&Device->NetIf);

  if (LwipDhcp && LwipDhcp->state != DHCP_STATE_INIT) {
    Status = EFI_ALREADY_STARTED;
    goto ON_ERROR;
  }

  netifapi_dhcp_start (&Device->NetIf, Device->DhcpOptions, Device->DhcpOptionsLength);

  if (CompletionEvent == NULL) {
    DBG ("[MP TCPIP] DHCP->Start(): Waiting for binding...\n");

    do {
      gBS->RestoreTPL (OldTpl);
      OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
    } while (LwipDhcpGetState (Device) != DHCP_STATE_BOUND);

    DBG ("[MP TCPIP] DHCP->Start(): Bound\n");
  }

  Status = EFI_SUCCESS;

ON_ERROR:
  gBS->RestoreTPL (OldTpl);

  DBG ("[MP TCPIP] DHCP->Start(): %r\n", Status);

  return Status;
}

EFI_STATUS
EFIAPI
LwipDhcp4RenewRebind (
  IN EFI_DHCP4_PROTOCOL     *This,
  IN BOOLEAN                RebindRequest,
  IN EFI_EVENT              CompletionEvent   OPTIONAL
  )
{
  EFI_STATUS            Status;
  LWIP_DHCP4_INSTANCE   *Instance;
  LWIP_NET_DEVICE       *Device;
  EFI_TPL               OldTpl;
  struct dhcp           *LwipDhcp;

  DBG ("[MP TCPIP] DHCP->RenewRebind()\n");

  if (This == NULL) {
    DBG ("[MP TCPIP] DHCP->RenewRebind(): Invalid parameters\n");
    return EFI_INVALID_PARAMETER;
  }

  if (RebindRequest) {
    DBG ("[MP TCPIP] DHCP->RenewRebind(): Rebind is unsupported\n");
    return EFI_UNSUPPORTED;
  }

  Instance = DHCP4_INSTANCE_FROM_PROTOCOL (This);
  Device = Instance->Device;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  LwipDhcp = netif_dhcp_data (&Device->NetIf);

  if (LwipDhcp->state == DHCP_STATE_OFF) {
    Status = EFI_NOT_STARTED;
    goto ON_EXIT;
  }

  if (LwipDhcp->state != DHCP_STATE_BOUND) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  netifapi_dhcp_renew (&Device->NetIf);

  Status = EFI_SUCCESS;

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  DBG ("[MP TCPIP] DHCP->RenewRebind(): %r\n", Status);

  return Status;
}

EFI_STATUS
EFIAPI
LwipDhcp4Release (
  IN EFI_DHCP4_PROTOCOL     *This
  )
{
  EFI_STATUS            Status;
  LWIP_DHCP4_INSTANCE   *Instance;
  LWIP_NET_DEVICE       *Device;
  EFI_TPL               OldTpl;
  struct dhcp           *LwipDhcp;

  DBG ("[MP TCPIP] DHCP->Release()\n");

  if (This == NULL) {
    DBG ("[MP TCPIP] DHCP->Release(): Invalid parameters\n");
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP4_INSTANCE_FROM_PROTOCOL (This);
  Device = Instance->Device;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  LwipDhcp = netif_dhcp_data (&Device->NetIf);

  if (LwipDhcp->state != DHCP_STATE_BOUND) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  netifapi_dhcp_release (&Device->NetIf);

  Status = EFI_SUCCESS;

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  DBG ("[MP TCPIP] DHCP->Release(): %r\n", Status);

  return Status;
}

EFI_STATUS
EFIAPI
LwipDhcp4Stop (
  IN EFI_DHCP4_PROTOCOL     *This
  )
{
  EFI_STATUS            Status;
  LWIP_DHCP4_INSTANCE   *Instance;
  LWIP_NET_DEVICE       *Device;
  EFI_TPL               OldTpl;
  struct dhcp           *LwipDhcp;

  DBG ("[MP TCPIP] DHCP->Stop()\n");

  if (This == NULL) {
    DBG ("[MP TCPIP] DHCP->Stop(): Invalid parameters\n");
    return EFI_INVALID_PARAMETER;
  }

  Instance = DHCP4_INSTANCE_FROM_PROTOCOL (This);
  Device = Instance->Device;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  LwipDhcp = netif_dhcp_data (&Device->NetIf);

  netifapi_dhcp_stop (&Device->NetIf);

  //
  // TODO: Clean configuration: options etc...
  //

  Status = EFI_SUCCESS;

//ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  DBG ("[MP TCPIP] DHCP->Stop(): %r\n", Status);

  return Status;
}

EFI_STATUS
EFIAPI
LwipDhcp4Build (
  IN EFI_DHCP4_PROTOCOL       *This,
  IN EFI_DHCP4_PACKET         *SeedPacket,
  IN UINT32                   DeleteCount,
  IN UINT8                    *DeleteList OPTIONAL,
  IN UINT32                   AppendCount,
  IN EFI_DHCP4_PACKET_OPTION  *AppendList[] OPTIONAL,
  OUT EFI_DHCP4_PACKET        **NewPacket
  )
{
  DBG ("[MP TCPIP] DHCP->Build()\n");

  if (This == NULL || NewPacket == NULL) {
    DBG ("[MP TCPIP] DHCP->Build(): Invalid param 1\n");
    return EFI_INVALID_PARAMETER;
  }

  if (SeedPacket == NULL || SeedPacket->Dhcp4.Magik != DHCP_OPTION_MAGIC ||
      EFI_ERROR (DhcpValidateOptions (SeedPacket, NULL))) {
    DBG ("[MP TCPIP] DHCP->Build(): Invalid param 2\n");
    return EFI_INVALID_PARAMETER;
  }

  if (((DeleteCount == 0) && (AppendCount == 0)) ||
      ((DeleteCount != 0) && (DeleteList == NULL)) ||
      ((AppendCount != 0) && (AppendList == NULL))) {
    DBG ("[MP TCPIP] DHCP->Build(): Invalid param 3\n");
    return EFI_INVALID_PARAMETER;
  }

  return DhcpBuild (
           SeedPacket,
           DeleteCount,
           DeleteList,
           AppendCount,
           AppendList,
           NewPacket
           );
}

VOID
EFIAPI
DhcpDummyExtFree (
  IN VOID                   *Arg
  )
{
}

EFI_STATUS
EFIAPI
LwipDhcp4TransmitReceive (
  IN EFI_DHCP4_PROTOCOL                *This,
  IN EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN  *Token
  )
{
  EFI_STATUS            Status;
  LWIP_DHCP4_INSTANCE   *Instance;
  LWIP_NET_DEVICE       *Device;
  EFI_TPL               OldTpl;
  int                   Socket;
  int                   LwipStatus;
  struct sockaddr_in    Address;
  struct sockaddr_in    ServerAddress;
  socklen_t             ServerAddressLen;
  EFI_DHCP4_PACKET      *ResponsePacket;
  IP4_ADDR              ClientAddress;
  UINT32                Xid;
  struct dhcp           *LwipDhcp;

  DBG ("[MP TCPIP] DHCP->TransmitReceive()\n");

  if (This == NULL || Token == NULL || Token->Packet == NULL) {
    DBG ("[MP TCPIP] DHCP->TransmitReceive(): Invalid parameters\n");
    return EFI_INVALID_PARAMETER;
  }

  Instance  = DHCP4_INSTANCE_FROM_PROTOCOL (This);
  Device    = Instance->Device;
  Xid       = htonl (Token->Packet->Dhcp4.Header.Xid);
  LwipDhcp  = netif_dhcp_data (&Device->NetIf);

  if ((Token->Packet->Dhcp4.Magik != DHCP_OPTION_MAGIC)             ||
      (Xid == LwipDhcp->xid)                                        ||
      (Token->TimeoutValue == 0)                                    ||
      (Token->ListenPointCount != 0 && Token->ListenPoints == NULL) ||
      EFI_ERROR (DhcpValidateOptions (Token->Packet, NULL))         ||
      (*(UINT32*)&Token->RemoteAddress == 0)
      ) {
    //
    // Packet is not well-formed or token is invalid.
    //
    DBG ("[MP TCPIP] DHCP->TransmitReceive(): Packet or/and token invalid\n");
    return EFI_INVALID_PARAMETER;
  }

  ClientAddress = htonl (*(UINT32*)&Token->Packet->Dhcp4.Header.ClientAddr);

  if (ClientAddress == 0) {
    return EFI_NO_MAPPING;
  }

  OldTpl          = gBS->RaiseTPL (TPL_CALLBACK);
  Socket          = -1;
  ResponsePacket  = NULL;

  //
  // Check the gateway, if it needs a switch, switch it.
  //
//  if (!IP4_NET_EQUAL (ClientAddress, htonl(*(UINT32*)&Token->RemoteAddress), Device->NetIf.netmask.addr)) {
//    DBG ("[MP TCPIP] DHCP Gateway was set.\n");
//    ASSERT (FALSE);
//    if (!NetIp4IsUnicast (*(UINT32*)&Token->GatewayAddress, Device->NetIf.netmask.addr)) {
//      Status = EFI_INVALID_PARAMETER;
//      goto ON_EXIT;
//    }
//  }

  //
  // Check for necessary resources
  //
  ResponsePacket = AllocateZeroPool (Device->NetIf.mtu);
  DBG ("[MP TCPIP] DHCP->TransmitReceive(): MTU: %d\n", Device->NetIf.mtu);

  if (ResponsePacket == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  //
  // Open new UDP socket on the same DHCP ports.
  // Make sure the XIDs don't mix up with LWIP DHCP.
  //
  Socket = -1;
  Socket = lwip_socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  ASSERT (Socket != -1);

  DBG ("[MP TCPIP] DHCP->TransmitReceive(): Socket created.\n");

  ZeroMem (&Address, sizeof (Address));

  if (Token->ListenPointCount == 0 || Token->ListenPoints[0].ListenPort == 0) {
    Address.sin_port = htons(DHCP_CLIENT_PORT);
  } else {
    Address.sin_port = htons(Token->ListenPoints[0].ListenPort);
  }

  Address.sin_family        = AF_INET;
  Address.sin_addr.s_addr   = *(UINT32*)&Token->Packet->Dhcp4.Header.ClientAddr;
  Address.sin_len           = sizeof(Address);

  ZeroMem (&ServerAddress, sizeof (ServerAddress));
  ServerAddress.sin_family = AF_INET;

  if (Token->RemotePort == 0) {
    ServerAddress.sin_port = htons(DHCP_SERVER_PORT);
  } else {
    ServerAddress.sin_port = htons(Token->RemotePort);
  }
  ServerAddress.sin_addr.s_addr   = *(UINT32*)&Token->RemoteAddress;
  ServerAddress.sin_len           = sizeof(ServerAddress);

  DBG ("[MP TCPIP] DHCP->TransmitReceive(): Binding to %d.%d.%d.%d:%d\n",
      Token->Packet->Dhcp4.Header.ClientAddr.Addr[0], Token->Packet->Dhcp4.Header.ClientAddr.Addr[1],
      Token->Packet->Dhcp4.Header.ClientAddr.Addr[2], Token->Packet->Dhcp4.Header.ClientAddr.Addr[3],
      htons(Address.sin_port));
  LwipStatus = lwip_bind (Socket, (struct sockaddr*)&Address, sizeof(Address));

  if (LwipStatus != 0) {
    DBG ("[MP TCPIP] DHCP->TransmitReceive(): lwip_bind: %d\n", LwipStatus);
    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  DBG ("[MP TCPIP] DHCP->TransmitReceive(): Bound.\n");
  DBG ("[MP TCPIP] DHCP->TransmitReceive(): Sending to: %d.%d.%d.%d:%d\n",
      Token->RemoteAddress.Addr[0], Token->RemoteAddress.Addr[1],
      Token->RemoteAddress.Addr[2], Token->RemoteAddress.Addr[3],
      htons(ServerAddress.sin_port));

  //
  // Transmit the given packet
  //
  LwipStatus = lwip_sendto (
                 Socket,
                 (void*)&Token->Packet->Dhcp4,
                 Token->Packet->Length,
                 0,
                 (struct sockaddr*) &ServerAddress,
                 sizeof (ServerAddress)
                 );

  if (LwipStatus == -1) {
    DBG ("[MP TCPIP] DHCP->TransmitReceive(): lwip_sendto: %d\n", LwipStatus);
    Status = EFI_DEVICE_ERROR;
    goto ON_EXIT;
  }

  //
  // Wait for packet receive
  //
  do {
    LwipStatus = lwip_recvfrom (
                   Socket,
                   &ResponsePacket->Dhcp4,
                   Device->NetIf.mtu - sizeof(EFI_DHCP4_PACKET) + sizeof(((EFI_DHCP4_PACKET*)0)->Dhcp4),
                   0,
                   (struct sockaddr*) &ServerAddress,
                   &ServerAddressLen
                   );

    DBG ("[MP TCPIP] DHCP->TransmitReceive(): lwip_recvfrom: %d\n", LwipStatus);

    if (LwipStatus != -1) {
      //
      // Packet received, TODO: validate (IP address, XID)
      //
      DBG ("[MP TCPIP] DHCP->TransmitReceive(): Packet received\n");
    }
  } while (LwipStatus == -1);

  //
  // Fill up token, signal
  //
  Token->ResponseList = ResponsePacket;
  Token->ResponseList->Size = Device->NetIf.mtu;
  Token->ResponseList->Length = LwipStatus;
  Token->ResponseCount = 1;
  Token->Status = EFI_SUCCESS;

  DBG ("[MP TCPIP] DHCP->TransmitReceive(): Success\n");

  Status = EFI_SUCCESS;

ON_EXIT:
  if (Socket != -1) {
    DBG ("[MP TCPIP] DHCP->TransmitReceive(): Closing socket\n");
    lwip_close (Socket);
    DBG ("[MP TCPIP] DHCP->TransmitReceive(): Socket closed\n");
  }

  if (Status == EFI_SUCCESS) {
    DBG ("[MP TCPIP] DHCP->TransmitReceive(): Signalling CompletionEvent\n");
    gBS->SignalEvent (Token->CompletionEvent);
  } else if (ResponsePacket != NULL) {
    DBG ("[MP TCPIP] DHCP->TransmitReceive(): Freeing ResponsePacket\n");
    FreePool (ResponsePacket);
    Token->ResponseList = NULL;
    Token->ResponseCount = 0;
    Token->Status = Status;
  }

  gBS->RestoreTPL (OldTpl);

  DBG ("[MP TCPIP] DHCP->TransmitReceive(): %r\n", Status);

  return Status;
}

EFI_STATUS
Dhcp4ParseCheckOption (
  IN UINT8                  Tag,
  IN UINT8                  Len,
  IN UINT8                  *Data,
  IN VOID                   *Context
  )
{
  DHCP_PARSE_CONTEXT        *Parse;

  Parse = (DHCP_PARSE_CONTEXT *) Context;
  Parse->Index++;

  if (Parse->Index <= Parse->OptionCount) {
    //
    // Use BASE_CR to get the memory position of EFI_DHCP4_PACKET_OPTION for
    // the EFI_DHCP4_PACKET_OPTION->Data because DhcpIterateOptions only
    // pass in the point to option data.
    //
    Parse->Option[Parse->Index - 1] = BASE_CR (Data, EFI_DHCP4_PACKET_OPTION, Data);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LwipDhcp4Parse (
  IN EFI_DHCP4_PROTOCOL       *This,
  IN EFI_DHCP4_PACKET         *Packet,
  IN OUT UINT32               *OptionCount,
  OUT EFI_DHCP4_PACKET_OPTION *PacketOptionList[] OPTIONAL
  )
{
  DHCP_PARSE_CONTEXT        Context;
  EFI_STATUS                Status;

  DBG ("[MP TCPIP] DHCP->Parse()\n");

  //
  // First validate the parameters
  //
  if ((This == NULL) || (Packet == NULL) || (OptionCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Packet->Size < Packet->Length + 2 * sizeof (UINT32)) ||
      (Packet->Dhcp4.Magik != DHCP_OPTION_MAGIC) ||
      EFI_ERROR (DhcpValidateOptions (Packet, NULL))) {

    return EFI_INVALID_PARAMETER;
  }

  if ((*OptionCount != 0) && (PacketOptionList == NULL)) {
    return EFI_BUFFER_TOO_SMALL;
  }

  ZeroMem (PacketOptionList, *OptionCount * sizeof (EFI_DHCP4_PACKET_OPTION *));

  Context.Option      = PacketOptionList;
  Context.OptionCount = *OptionCount;
  Context.Index       = 0;

  Status              = DhcpIterateOptions (Packet, Dhcp4ParseCheckOption, &Context);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  *OptionCount = Context.Index;

  if (Context.Index > Context.OptionCount) {
    return EFI_BUFFER_TOO_SMALL;
  }

  return EFI_SUCCESS;
}
