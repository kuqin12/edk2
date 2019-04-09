/**

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#define LWIP_POLL_AP_COUNT    2

#include "LwipMp.h"

typedef enum _LWIP_POLL_AP_STATE {
  LWIP_POLL_AP_STATE_SPAWNED,
  LWIP_POLL_AP_STATE_READY,
  LWIP_POLL_AP_STATE_STARTED
} LWIP_POLL_AP_STATE;

typedef struct _LWIP_POLL_AP_CONTEXT {
  LWIP_POLL_AP_STATE    State;
  UINTN                 Id;
  MP_LIST               DeviceList;
} LWIP_POLL_AP_CONTEXT;

//
// Global information
//
LWIP_POLL_AP_CONTEXT    *PollApContexts   = NULL;
EFI_THREAD              **PollApThreads   = NULL;
UINTN                   NextAssignee      = 0;


//
// Polling iteration through whole list of devices the thread owns
//
EFI_STATUS
EFIAPI
LwipPollingIteration (
  IN     MP_LIST_ENTRY    *Entry,
  IN     VOID             *Arg
  )
{
  LWIP_POLL_AP_CONTEXT  *Context;
  LWIP_NET_DEVICE       *Device;
  EFI_STATUS            Status;
  UINTN                 PacketLength;
  UINTN                 MaxPacketLength;
  struct pbuf           *Packet;

  Context = (LWIP_POLL_AP_CONTEXT*) Arg;
  Device  = (LWIP_NET_DEVICE*) Entry->Data;

  MaxPacketLength = Device->Snp->Mode->MaxPacketSize + Device->Snp->Mode->MediaHeaderSize;

  if (Device->PbufCache == NULL) {
    Device->PbufCache = pbuf_alloc (PBUF_RAW, (UINT16)MaxPacketLength, PBUF_POOL);
  }

  if (Device->IsUp && Device->PbufCache != NULL) {

    PacketLength = Device->PbufCache->tot_len;

    Status = Device->Snp->Receive (
                            Device->Snp,
                            NULL,
                            &PacketLength,
                            Device->PbufCache->payload,
                            NULL,
                            NULL,
                            NULL
                            );

    if (Status == EFI_SUCCESS) {
//      DEBUG ((EFI_D_INFO, "[MP TCPIP] Packet received. Length: %d\n", PacketLength));

      Packet = Device->PbufCache;
      Device->PbufCache = NULL;
      Packet->len = (UINT16)PacketLength;

      Device->NetIf.input (Packet, &Device->NetIf);
    } else if (Status == EFI_BUFFER_TOO_SMALL) {
      DEBUG ((EFI_D_INFO, "[MP TCPIP] Packet receive failed. Buffer is too small. Expected: %d, Got: %d\n", PacketLength, MaxPacketLength));
    } else if (Status != EFI_NOT_READY) {
      //DEBUG ((EFI_D_INFO, "[MP TCPIP] Packet receive failed: %r\n", Status));
    }
  }

  return EFI_SUCCESS;
}

//
// Polling thread task
//
VOID
LwipPollingThreadTask (
  IN VOID*    Arg
  )
{
  LWIP_POLL_AP_CONTEXT *Context;

  Context = (LWIP_POLL_AP_CONTEXT*) Arg;

  DEBUG ((EFI_D_INFO, "[PollAP %d] Hi.\n", Context->Id));

  while (1) {
    MpListIterate (&Context->DeviceList, LwipPollingIteration, Context);
  }
}

EFI_STATUS
LwipAssignDeviceToPollAp (
  IN  LWIP_NET_DEVICE   *Device,
  IN  UINTN             PollApId
  )
{
  EFI_STATUS    Status;

  if (Device == NULL || PollApId >= LWIP_POLL_AP_COUNT) {
    return EFI_INVALID_PARAMETER;
  }

  if (Device->Entry != NULL) {
    return EFI_ACCESS_DENIED;
  }

  MpListLock (&PollApContexts[PollApId].DeviceList);
  Status = MpListPushBackUnsafe (&PollApContexts[PollApId].DeviceList, Device);
  if (Status == EFI_SUCCESS) {
    Device->Entry = MpListGetBackUnsafe (&PollApContexts[PollApId].DeviceList);
  }
  MpListUnlock (&PollApContexts[PollApId].DeviceList);
  return Status;
}

EFI_STATUS
LwipAssignDevice (
  IN  LWIP_NET_DEVICE   *Device
  )
{
  EFI_STATUS    Status;

  Status = LwipAssignDeviceToPollAp (Device, NextAssignee);
  if (Status == EFI_SUCCESS) {
    NextAssignee++;
    if (NextAssignee == LWIP_POLL_AP_COUNT) {
      NextAssignee = 0;
    }
  }
  return Status;
}

EFI_STATUS
LwipReleaseDevice (
  IN  LWIP_NET_DEVICE   *Device
  )
{
  if (Device == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Device->Entry != NULL) {
    MpListRemove (Device->Entry);
    MpListEntryFree (Device->Entry);
    Device->Entry = NULL;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
LwipInitializePollingThreads (
  )
{
  EFI_STATUS  Status;
  UINTN       CpuCount, EnabledCpuCount;
  UINTN       i;

  Status = GetCpuCount (&CpuCount, &EnabledCpuCount);

  //
  // 1 core for BSP
  // 1 core for LWIP AP
  // n cores for Poll APs
  //
  if (EnabledCpuCount < (2 + LWIP_POLL_AP_COUNT)) {
    DEBUG ((EFI_D_ERROR, "[MP TCPIP] Not enough cores. Total: %d, Enabled: %d, Needed: %d\n",
           CpuCount, EnabledCpuCount, 2 + LWIP_POLL_AP_COUNT));
    return EFI_OUT_OF_RESOURCES;
  }

  PollApContexts = AllocateZeroPool (LWIP_POLL_AP_COUNT * sizeof (LWIP_POLL_AP_CONTEXT));
  if (PollApContexts == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PollApThreads = AllocateZeroPool (LWIP_POLL_AP_COUNT * sizeof (EFI_THREAD*));
  if (PollApThreads == NULL) {
    FreePool (PollApContexts);
    return EFI_OUT_OF_RESOURCES;
  }

  for (i = 0; i < LWIP_POLL_AP_COUNT; i++) {
    PollApContexts[i].State = LWIP_POLL_AP_STATE_SPAWNED;
    PollApContexts[i].Id = i;
    MpListInitialize (&PollApContexts[i].DeviceList);

    Status = SpawnThread (
               LwipPollingThreadTask,
               &PollApContexts[i],
               NULL,
               NULL,
               0,
               &PollApThreads[i]
               );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
LwipUninstallPollingThreads (
  )
{
  UINTN     i;

  if (PollApThreads) {
    for (i = 0; i < LWIP_POLL_AP_COUNT; i++) {
      if (PollApThreads[i]) {
        AbortThread (PollApThreads[i]);
      }
    }

    FreePool (PollApThreads);
  }

  if (PollApContexts) {
    FreePool (PollApContexts);
  }

  return EFI_SUCCESS;
}
