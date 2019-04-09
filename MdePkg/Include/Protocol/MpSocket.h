/**

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_MP_SOCKET_PROTOCOL_H__
#define __EFI_MP_SOCKET_PROTOCOL_H__

#include <Uefi.h>

#define EFI_MP_SOCKET_PROTOCOL_GUID \
  { \
    0xBAF99B71, 0x2251, 0x4B77, { 0x9F, 0x32, 0x0C, 0xBE, 0xB3, 0xC6, 0x99, 0x20 } \
  }

typedef struct _EFI_LWIP_SOCKET_PROTOCOL EFI_LWIP_SOCKET_PROTOCOL;

typedef INT32 EFI_LWIP_SOCKET;

typedef enum {
  LWIP_AF_INET
} EFI_LWIP_SOCKET_DOMAIN;

typedef enum {
  LWIP_SOCK_STREAM,
  LWIP_SOCK_DGRAM
} EFI_LWIP_SOCKET_TYPE;

typedef enum {
  LWIP_IPPROTO_TCP,
  LWIP_IPPROTO_UDP
} LWIP_SOCKET_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EFI_LWIP_SOCKET_CREATE) (
  IN  EFI_LWIP_SOCKET_PROTOCOL    *This,
  IN  EFI_LWIP_SOCKET_DOMAIN      Domain,
  IN  EFI_LWIP_SOCKET_TYPE        Type,
  IN  LWIP_SOCKET_PROTOCOL        Protocol,
  OUT EFI_LWIP_SOCKET             *Socket
  );

typedef
EFI_STATUS
(EFIAPI *EFI_LWIP_SOCKET_BIND) (
  IN  EFI_LWIP_SOCKET_PROTOCOL    *This,
  IN  EFI_LWIP_SOCKET             Socket,
  IN  EFI_IPv4_ADDRESS            *ClientIp,
  IN  UINT16                      ClientPort
  );

typedef
EFI_STATUS
(EFIAPI *EFI_LWIP_SOCKET_CONNECT) (
  IN  EFI_LWIP_SOCKET_PROTOCOL    *This,
  IN  EFI_LWIP_SOCKET             Socket,
  IN  EFI_IPv4_ADDRESS            *ServerIp,
  IN  UINT16                      ServerPort
  );

typedef
EFI_STATUS
(EFIAPI *EFI_LWIP_SOCKET_RECEIVE) (
  IN  EFI_LWIP_SOCKET_PROTOCOL    *This,
  IN      EFI_LWIP_SOCKET         Socket,
  IN      UINT8                   *Buffer,
  IN OUT  UINT32                  *BufferSize
  );

typedef
EFI_STATUS
(EFIAPI *EFI_LWIP_SOCKET_SEND) (
  IN  EFI_LWIP_SOCKET_PROTOCOL    *This,
  IN  EFI_LWIP_SOCKET             Socket,
  IN  UINT8                       *Buffer,
  IN  UINT32                      BufferLength
  );

typedef
EFI_STATUS
(EFIAPI *EFI_LWIP_SOCKET_RECEIVEFROM) (
  IN      EFI_LWIP_SOCKET_PROTOCOL  *This,
  IN      EFI_LWIP_SOCKET           Socket,
  IN      BOOLEAN                   ShouldBlock,
  IN      UINT8                     *Buffer,
  IN      UINT32                    *BufferSize,
  IN OUT  EFI_IPv4_ADDRESS          *SourceIp,
  IN OUT  UINT16                    *SourcePort
  );

typedef
EFI_STATUS
(EFIAPI *EFI_LWIP_SOCKET_SENDTO) (
  IN  EFI_LWIP_SOCKET_PROTOCOL    *This,
  IN  EFI_LWIP_SOCKET             Socket,
  IN  UINT8                       *Buffer,
  IN  UINT32                      BufferLength,
  IN  EFI_IPv4_ADDRESS            *TargetIp,
  IN  UINT16                      TargetPort
  );

typedef
EFI_STATUS
(EFIAPI *EFI_LWIP_SOCKET_CLOSE) (
  IN  EFI_LWIP_SOCKET_PROTOCOL    *This,
  IN  EFI_LWIP_SOCKET             Socket
  );

struct _EFI_LWIP_SOCKET_PROTOCOL {
  EFI_LWIP_SOCKET_CREATE          Create;
  EFI_LWIP_SOCKET_BIND            Bind;
  EFI_LWIP_SOCKET_CONNECT         Connect;
  EFI_LWIP_SOCKET_RECEIVE         Receive;
  EFI_LWIP_SOCKET_SEND            Send;
  EFI_LWIP_SOCKET_RECEIVEFROM     ReceiveFrom;
  EFI_LWIP_SOCKET_SENDTO          SendTo;
  EFI_LWIP_SOCKET_CLOSE           Close;
};

extern EFI_GUID gEfiMpSocketProtocolGuid;

#endif /* __EFI_MP_SOCKET_PROTOCOL_H__ */
