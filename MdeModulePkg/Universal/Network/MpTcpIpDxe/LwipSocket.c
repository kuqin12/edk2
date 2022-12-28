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
#include "lwip/sockets.h"

EFI_HANDLE      mEfiHandle = NULL;

EFI_STATUS
EFIAPI
LwipSocketCreate (
  IN  EFI_LWIP_SOCKET_PROTOCOL    *This,
  IN  EFI_LWIP_SOCKET_DOMAIN      Domain,
  IN  EFI_LWIP_SOCKET_TYPE        Type,
  IN  LWIP_SOCKET_PROTOCOL        Protocol,
  OUT EFI_LWIP_SOCKET             *Socket
  )
{
  INT32 LwipSocket;
  INT32 LwipDomain;
  INT32 LwipType;
  INT32 LwipProtocol;
  int   LwipOption;
  int   LwipResult;

  DBG ("[MP TCPIP] Sockets->Create()\n");

  if (This == NULL || Socket == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Domain) {
  case LWIP_AF_INET:
    LwipDomain = AF_INET;
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  switch (Type) {
  case LWIP_SOCK_STREAM:
    LwipType = SOCK_STREAM;
    break;
  case LWIP_SOCK_DGRAM:
    LwipType = SOCK_DGRAM;
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  switch (Protocol) {
  case LWIP_IPPROTO_TCP:
    LwipProtocol = IPPROTO_TCP;
    break;
  case LWIP_IPPROTO_UDP:
    LwipProtocol = IPPROTO_UDP;
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  LwipSocket = lwip_socket (LwipDomain, LwipType, LwipProtocol);

  if (LwipSocket == -1) {
    return EFI_DEVICE_ERROR;
  }

  LwipOption = 1;
  LwipResult = lwip_setsockopt (LwipSocket, SOL_SOCKET, SO_REUSEADDR, &LwipOption, sizeof(LwipOption));

  if (LwipResult == -1) {
    lwip_close (LwipSocket);
    return EFI_DEVICE_ERROR;
  }

  *Socket = LwipSocket;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LwipSocketBind (
  IN  EFI_LWIP_SOCKET_PROTOCOL    *This,
  IN  EFI_LWIP_SOCKET             Socket,
  IN  EFI_IPv4_ADDRESS            *ClientIp,
  IN  UINT16                      ClientPort
  )
{
  int                 LwipStatus;
  struct sockaddr_in  ClientAddress;

  DBG ("[MP TCPIP] Sockets->Bind()\n");

  if (This == NULL || ClientIp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&ClientAddress, sizeof (ClientAddress));
  ClientAddress.sin_family      = AF_INET;
  ClientAddress.sin_port        = htons (ClientPort);
  ClientAddress.sin_addr.s_addr = *(UINT32*)ClientIp->Addr;
  ClientAddress.sin_len         = sizeof (ClientIp->Addr);

  LwipStatus = lwip_bind (
                 Socket,
                 (struct sockaddr*) &ClientAddress,
                 sizeof (ClientAddress)
                 );

  if (LwipStatus == -1) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LwipSocketConnect (
  IN  EFI_LWIP_SOCKET_PROTOCOL    *This,
  IN  EFI_LWIP_SOCKET             Socket,
  IN  EFI_IPv4_ADDRESS            *ServerIp,
  IN  UINT16                      ServerPort
  )
{
  int                 LwipStatus;
  struct sockaddr_in  ServerAddress;

  DBG ("[MP TCPIP] Sockets->Connect()\n");

  if (This == NULL || ServerIp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&ServerAddress, sizeof (ServerAddress));
  ServerAddress.sin_family      = AF_INET;
  ServerAddress.sin_port        = htons (ServerPort);
  ServerAddress.sin_addr.s_addr = *(UINT32*)ServerIp->Addr;
  ServerAddress.sin_len         = sizeof (ServerIp->Addr);

  LwipStatus = lwip_connect (
                 Socket,
                 (struct sockaddr*) &ServerAddress,
                 sizeof (ServerAddress)
                 );

  if (LwipStatus == -1) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LwipSocketReceive (
  IN      EFI_LWIP_SOCKET_PROTOCOL  *This,
  IN      EFI_LWIP_SOCKET           Socket,
  IN      UINT8                     *Buffer,
  IN OUT  UINT32                    *BufferSize
  )
{
  int LwipStatus;

  DBG ("[MP TCPIP] Sockets->Receive()\n");

  if (This == NULL || Buffer == NULL ||
      BufferSize == NULL || *BufferSize == 0) {
    return EFI_INVALID_PARAMETER;
  }

  LwipStatus = lwip_read (Socket, Buffer, *BufferSize);

  if (LwipStatus == -1) {
    return EFI_DEVICE_ERROR;
  }

  *BufferSize = LwipStatus;

  if (LwipStatus == 0) {
    return EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LwipSocketSend (
  IN  EFI_LWIP_SOCKET_PROTOCOL    *This,
  IN  EFI_LWIP_SOCKET             Socket,
  IN  UINT8                       *Buffer,
  IN  UINT32                      BufferLength
  )
{
  INT32 LwipStatus;

  DBG ("[MP TCPIP] Sockets->Send()\n");

  if (This == NULL || Buffer == NULL || BufferLength == 0) {
    return EFI_INVALID_PARAMETER;
  }

  LwipStatus = lwip_send (Socket, Buffer, BufferLength, 0);

  if (LwipStatus == -1) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LwipSocketReceiveFrom (
  IN      EFI_LWIP_SOCKET_PROTOCOL  *This,
  IN      EFI_LWIP_SOCKET           Socket,
  IN      BOOLEAN                   ShouldBlock,
  IN      UINT8                     *Buffer,
  IN      UINT32                    *BufferSize,
  IN OUT  EFI_IPv4_ADDRESS          *SourceIp,
  IN OUT  UINT16                    *SourcePort
  )
{
  INT32 LwipStatus;
  struct sockaddr_in SourceAddress;
  socklen_t SourceAddressLength;

//  DBG ("[MP TCPIP] Sockets->ReceiveFrom()\n");

  if (This == NULL || Buffer == NULL || BufferSize == NULL ||
      *BufferSize == 0 || SourceIp == NULL || SourcePort == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SourceAddressLength = sizeof (SourceAddress);

  LwipStatus = lwip_recvfrom (
                 Socket,
                 Buffer,
                 *BufferSize,
                 MSG_DONTWAIT,
                 (struct sockaddr*)&SourceAddress,
                 &SourceAddressLength
                 );

  if (LwipStatus == -1) {
    return EFI_NOT_READY;
  }

  CopyMem (SourceIp, &SourceAddress.sin_addr, sizeof (*SourceIp));
  *SourcePort = ntohs(SourceAddress.sin_port);
  *BufferSize = LwipStatus;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LwipSocketSendTo (
  IN  EFI_LWIP_SOCKET_PROTOCOL    *This,
  IN  EFI_LWIP_SOCKET             Socket,
  IN  UINT8                       *Buffer,
  IN  UINT32                      BufferLength,
  IN  EFI_IPv4_ADDRESS            *TargetIp,
  IN  UINT16                      TargetPort
  )
{
  INT32 LwipStatus;
  struct sockaddr_in TargetAddress;

  if (This == NULL || Buffer == NULL || BufferLength == 0 ||
      TargetIp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&TargetAddress, sizeof (TargetAddress));
  TargetAddress.sin_family = AF_INET;
  TargetAddress.sin_port = ntohs (TargetPort);
  TargetAddress.sin_addr.s_addr = ntohl (*(UINT32*)TargetIp->Addr);

  LwipStatus = lwip_sendto (
                 Socket,
                 Buffer,
                 BufferLength,
                 0,
                 (struct sockaddr*)&TargetAddress,
                 sizeof (TargetAddress)
                 );

  if (LwipStatus == -1) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LwipSocketClose (
  IN  EFI_LWIP_SOCKET_PROTOCOL    *This,
  IN  EFI_LWIP_SOCKET             Socket
  )
{
  int LwipStatus;

  DBG ("[MP TCPIP] Socket->Close()\n");

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  LwipStatus = lwip_close (Socket);

  if (LwipStatus) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_LWIP_SOCKET_PROTOCOL mEfiSocketProtocol = {
  LwipSocketCreate,
  LwipSocketBind,
  LwipSocketConnect,
  LwipSocketReceive,
  LwipSocketSend,
  LwipSocketReceiveFrom,
  LwipSocketSendTo,
  LwipSocketClose
};

EFI_STATUS
LwipInitializeSocketProtocol (
  VOID
  )
{
  EFI_STATUS    Status;

  if (mEfiHandle != NULL) {
    return EFI_ALREADY_STARTED;
  }

  DBG ("[MP TCPIP] Installing socket protocol...\n");

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mEfiHandle,
                  &gEfiMpSocketProtocolGuid,
                  &mEfiSocketProtocol,
                  NULL
                  );

  DBG ("[MP TCPIP] Socket protocol installation status: %r\n", Status);

  return Status;
}

EFI_STATUS
LwipStopSocketProtocol (
  VOID
  )
{
  EFI_STATUS    Status;

  if (mEfiHandle == NULL) {
    return EFI_SUCCESS;
  }

  DBG ("[MP TCPIP] Stopping socket protocol...\n");

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  mEfiHandle,
                  &gEfiMpSocketProtocolGuid,
                  &mEfiSocketProtocol,
                  NULL
                  );

  DBG ("[MP TCPIP] Socket protocol stopping result: %r\n", Status);

  return Status;
}
