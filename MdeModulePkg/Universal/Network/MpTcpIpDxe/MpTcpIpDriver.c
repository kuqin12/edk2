/**

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "LwipTest.h"
#include "MpTcpIp.h"

EFI_STATUS
EFIAPI
MpTcpIpDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
  );

EFI_STATUS
EFIAPI
MpTcpIpDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

EFI_STATUS
EFIAPI
MpTcpIpDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

EFI_DRIVER_BINDING_PROTOCOL gMpTcpIpDriverBinding = {
  MpTcpIpDriverBindingSupported,
  MpTcpIpDriverBindingStart,
  MpTcpIpDriverBindingStop,
  0xa,
  NULL,
  NULL
};

//=============================================================================

EFI_STATUS
EFIAPI
MpTcpIpDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS    Status;

  //
  // Test for DHCP4 service binding protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDhcp4ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  if (Status == EFI_SUCCESS) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Test for SNP protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleNetworkProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  if (Status == EFI_SUCCESS) {
    DBG ("[MP TCPIP] SNP matched.\n");
  }

  return Status;
}

EFI_STATUS
EFIAPI
MpTcpIpDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS      Status;

  Status = LwipDeviceAdd (This->DriverBindingHandle, ControllerHandle);

  return Status;
}

EFI_STATUS
EFIAPI
MpTcpIpDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_HANDLE    NicHandle;

  NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiManagedNetworkProtocolGuid);

  if (NicHandle != NULL) {
    DBG ("[MP TCPIP] DriverBinding->Stop()\n");
    LwipDeviceRemove (NicHandle);
    DBG ("[MP TCPIP] DriverBinding->Stop(): Device removed.\n");
  }

  return EFI_SUCCESS;
}
