/**

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "LwipSocket.h"
#include "LwipMp.h"
#include "MpTcpIp.h"

extern EFI_COMPONENT_NAME_PROTOCOL   gMpTcpIpComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gMpTcpIpComponentName2;

EFI_EVENT   gMpTcpIpExitBootServicesEvent;

EFI_STATUS
MpTcpIpInitializeNetworking (
  VOID
)
{
  EFI_STATUS  Status;

  DBG ("[MP TCPIP] Initializing networking...\n");
  tcpip_init(NULL, NULL);

  //
  // Create a child node with Socket protocol on it
  //
  Status = LwipInitializeSocketProtocol ();

  return Status;
}

VOID
EFIAPI
MpTcpIpNotifyExitBootServices (
  EFI_EVENT   Event,
  VOID        *Context
  )
{
  DBG ("[MP TCPIP] ExitBootServices: Stopping devices.\n");
  LwipStopAllDevices ();
  DBG ("[MP TCPIP] ExitBootServices: Stopping thread.\n");
  tcpip_stop ();
  DBG ("[MP TCPIP] ExitBootServices: Done.\n");
}

EFI_STATUS
EFIAPI
MpTcpIpDriverEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS    Status;

  LwipDeviceResourcesInit ();

  Status = MpTcpIpInitializeNetworking ();
  ASSERT_EFI_ERROR (Status);

  Status = LwipInitializePollingThreads ();
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  MpTcpIpNotifyExitBootServices,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &gMpTcpIpExitBootServicesEvent
                  );

  ASSERT_EFI_ERROR (Status);

  Status = EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gMpTcpIpDriverBinding,
           ImageHandle,
           &gMpTcpIpComponentName,
           &gMpTcpIpComponentName2
           );

  DBG ("[MP TCPIP] Driver protocol installation done.\n");

  return Status;
}

//
// TODO: Unload driver with proper thread cleanup.
// + uninstall socket protocol
//
