/*
 * LwipDhcpSb.c
 *
 *  Created on: Aug 22, 2018
 *      Author: mrabeda
 */

#include "LwipDhcpSb.h"

#define LWIP_NET_DEVICE_FROM_DHCP4_SB(s) \
  CR ( \
  (s), \
  LWIP_NET_DEVICE, \
  Dhcp4Sb, \
  LWIP_NET_DEVICE_SIGNATURE \
  )

EFI_DHCP4_PROTOCOL  mDhcp4ProtocolTemplate = {
  LwipDhcp4GetModeData,
  LwipDhcp4Configure,
  LwipDhcp4Start,
  LwipDhcp4RenewRebind,
  LwipDhcp4Release,
  LwipDhcp4Stop,
  LwipDhcp4Build,
  LwipDhcp4TransmitReceive,
  LwipDhcp4Parse
};

EFI_STATUS
EFIAPI
LwipDhcp4ServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                *ChildHandle
  )
{
  EFI_STATUS            Status;
  LWIP_NET_DEVICE       *Device;
  LWIP_DHCP4_INSTANCE   *Instance;

  Device = LWIP_NET_DEVICE_FROM_DHCP4_SB (This);

  //
  // Test if child creation is not allowed anymore
  //
  if (!Device->DhcpSbEnabled) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Acquire ownership of whole DHCP SB
  //
  MpFairLockAcquire (&Device->Dhcp4SbLock);

  //
  // Test again (could be changed while waiting for the lock)
  //
  if (!Device->DhcpSbEnabled) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  //
  // At this point, we are sure that DHCP SB has not been yet unloaded and
  // we are the only ones owning it right now.
  //
  // Carry on with instance installation
  //
  Instance = AllocateZeroPool (sizeof (LWIP_DHCP4_INSTANCE));

  if (Instance == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Instance->Signature = LWIP_DHCP4_INSTANCE_SIGNATURE;
  CopyMem (&Instance->Protocol, &mDhcp4ProtocolTemplate, sizeof (Instance->Protocol));
  Instance->Device = Device;
  Device->Dhcp4ActiveChild = NULL;

  Status = MpFairLockInit (&Instance->Lock);

  if (EFI_ERROR (Status)) {
    FreePool (Instance);
    goto ON_EXIT;
  }

  MpFairLockAcquire (&Instance->Lock);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiDhcp4ProtocolGuid,
                  &Instance->Protocol,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DBG ("[MP TCPIP][DHCP4] Failed to install DHCP4 protocol on child handle: %r\n", Status);
    FreePool (Instance);
    goto ON_EXIT;
  }

  Instance->Handle = *ChildHandle;

  MpListPushBack (&Device->Dhcp4ChildList, Instance);
  MpFairLockRelease (&Instance->Lock);
  Status = EFI_SUCCESS;

ON_EXIT:
  MpFairLockRelease (&Device->Dhcp4SbLock);
  return Status;
}

EFI_STATUS
EFIAPI
LwipDhcp4ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  EFI_STATUS            Status;
  EFI_DHCP4_PROTOCOL    *Dhcp;
  LWIP_NET_DEVICE       *Device;
  BOOLEAN               InstanceFound;
  LWIP_DHCP4_INSTANCE   *Instance;
  MP_LIST_ENTRY         *Entry;

  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiDhcp4ProtocolGuid,
                  (VOID **) &Dhcp,
                  gMpTcpIpDriverBinding.DriverBindingHandle,
                  ChildHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Device = LWIP_NET_DEVICE_FROM_DHCP4_SB (This);

  //
  // Test if DHCP SB is still operational
  //
  if (!Device->DhcpSbEnabled) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Take ownership over DHCP SB
  //
  MpFairLockAcquire (&Device->Dhcp4SbLock);

  //
  // Test again if DHCP SB is still operational. Might have been changed while
  // we were waiting for the lock.
  //
  if (!Device->DhcpSbEnabled) {
    Status = EFI_ACCESS_DENIED;
    goto ON_EXIT;
  }

  //
  // Find the instance within DHCP SB
  // If it's not there, it was already killed.
  //
  InstanceFound = FALSE;

  MP_LIST_FOREACH (&Device->Dhcp4ChildList, Entry, Instance, LWIP_DHCP4_INSTANCE*) {
    ASSERT (Instance != NULL);
    if (Instance->Handle == ChildHandle) {
      InstanceFound = TRUE;
      break;
    }
  } MP_LIST_FOREACH_END (&Device->Dhcp4ChildList);

  if (!InstanceFound) {
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  Instance->Device->Dhcp4ActiveChild = NULL;

  //
  // Remove instance from the list
  //
  MpListRemove (Entry);

  gBS->UninstallProtocolInterface (
         ChildHandle,
         &gEfiDhcp4ProtocolGuid,
         Dhcp
         );

  FreePool (Instance);

  Status = EFI_SUCCESS;

ON_EXIT:
  MpFairLockRelease (&Device->Dhcp4SbLock);
  return Status;
}

EFI_STATUS
LwipDhcp4Install (
  LWIP_NET_DEVICE     *Device
  )
{
  EFI_STATUS    Status;

  Status = MpFairLockInit (&Device->Dhcp4SbLock);

  if (EFI_ERROR (Status)) {
    DBG ("[MP TCPIP] Cannot initialize Dhcp4SbLock on LWIP device.\n");
    return Status;
  }

  Device->Dhcp4Sb.CreateChild  = LwipDhcp4ServiceBindingCreateChild;
  Device->Dhcp4Sb.DestroyChild = LwipDhcp4ServiceBindingDestroyChild;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Device->ControllerHandle,
                  &gEfiDhcp4ServiceBindingProtocolGuid,
                  &Device->Dhcp4Sb,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DBG ("[MP TCPIP] Installing DHCP4 SB failed with: %r\n", Status);
    MpFairLockDestroy (&Device->Dhcp4SbLock);
    return Status;
  }

  MpListInitialize (&Device->Dhcp4ChildList);

  //
  // Trigger that DHCP Service Binding is operational
  // If it stops being operational (upon DriverBinding->Stop()), it should
  // trigger this to false to forbid creation of new child handles.
  //
  Device->DhcpSbEnabled = TRUE;
  DBG ("[MP TCPIP] DHCP now operational!\n");

  return EFI_SUCCESS;

}

VOID
LwipDhcp4Uninstall (
  LWIP_NET_DEVICE     *Device
  )
{
  MP_LIST_ENTRY         *Entry;
  LWIP_DHCP4_INSTANCE   *Instance;

  if (!Device->DhcpSbEnabled) {
    //
    // DHCP SB not operational.
    //
    return;
  }
  DBG ("[MP TCPIP] Stopping DHCP...\n");

  //
  // Trigger that DHCP Service Binding should not be deemed operational
  // This does not allow further child creation/destruction by forcing
  // Service Binding functions to return EFI_ACCESS_DENIED.
  //
  Device->DhcpSbEnabled = FALSE;

  MpFairLockAcquire (&Device->Dhcp4SbLock);

  //
  // At this point we are the only owners of DHCP SB and nobody else is
  // waiting for the lock.
  //
  MP_LIST_FOREACH (&Device->Dhcp4ChildList, Entry, Instance, LWIP_DHCP4_INSTANCE*) {
    ASSERT (Instance != NULL);

    //
    // Clean up fields and uninstall DHCP protocol.
    //
    gBS->UninstallProtocolInterface (
           Instance->Handle,
           &gEfiDhcp4ProtocolGuid,
           &Instance->Protocol
           );
  } MP_LIST_FOREACH_END (&Device->Dhcp4ChildList);

  //
  // This clears entries and data
  //
  MpListClear (&Device->Dhcp4ChildList);

  gBS->UninstallMultipleProtocolInterfaces (
         &Device->ControllerHandle,
         &gEfiDhcp4ServiceBindingProtocolGuid,
         &Device->Dhcp4Sb,
         NULL
         );

  if (Device->DhcpOptions != NULL) {
    FreePool (Device->DhcpOptions);
  }
  Device->DhcpOptionsLength = 0;

  if (Device->DhcpCallbackEvent) {
    gBS->CloseEvent (Device->DhcpCallbackEvent);
  }

  MpFairLockDestroy (&Device->Dhcp4SbLock);
  DBG ("[MP TCPIP] DHCP stopped!\n");
}
