/*
 * LwipDhcpInternal.h
 *
 *  Created on: Aug 29, 2018
 *      Author: mrabeda
 */

#ifndef MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_DHCP_LWIPDHCPINTERNAL_H_
#define MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_DHCP_LWIPDHCPINTERNAL_H_

#include "LwipDhcp.h"

#define DHCP_OPTION_MAGIC         0x63538263 // Network byte order
#define DHCP_MAX_OPTIONS          256

typedef struct {
  UINT8                     Tag;
  UINT16                    Len;
  UINT8                     *Data;
} DHCP_OPTION;

typedef struct {
  IP4_ADDR                  NetMask;  // DHCP4_TAG_NETMASK
  IP4_ADDR                  Router;   // DHCP4_TAG_ROUTER, only the first router is used

  //
  // DHCP specific options
  //
  UINT8                     DhcpType; // DHCP4_TAG_MSG_TYPE
  UINT8                     Overload; // DHCP4_TAG_OVERLOAD
  IP4_ADDR                  ServerId; // DHCP4_TAG_SERVER_ID
  UINT32                    Lease;    // DHCP4_TAG_LEASE
  UINT32                    T1;       // DHCP4_TAG_T1
  UINT32                    T2;       // DHCP4_TAG_T2
} DHCP_PARAMETER;

typedef
EFI_STATUS
(*DHCP_CHECK_OPTION) (
  IN UINT8                  Tag,
  IN UINT8                  Len,
  IN UINT8                  *Data,
  IN VOID                   *Context
  );

EFI_STATUS
DhcpBuild (
  IN  EFI_DHCP4_PACKET        *SeedPacket,
  IN  UINT32                  DeleteCount,
  IN  UINT8                   *DeleteList     OPTIONAL,
  IN  UINT32                  AppendCount,
  IN  EFI_DHCP4_PACKET_OPTION *AppendList[]   OPTIONAL,
  OUT EFI_DHCP4_PACKET        **NewPacket
  );

EFI_STATUS
DhcpValidateOptions (
  IN  EFI_DHCP4_PACKET      *Packet,
  OUT DHCP_PARAMETER        **Para       OPTIONAL
  );

EFI_STATUS
DhcpIterateOptions (
  IN  EFI_DHCP4_PACKET      *Packet,
  IN  DHCP_CHECK_OPTION     Check         OPTIONAL,
  IN  VOID                  *Context
  );

err_t
DhcpStatusCallback (
  IN  struct netif          *NetIf,
  IN  dhcp_event            DhcpEvent,
  IN  struct pbuf           *Packet,
  IN  struct pbuf           **NewPacket
  );

VOID
DhcpNotifyUser (
  IN  EFI_EVENT             Event,
  IN  VOID                  *Context
  );

#endif /* MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_DHCP_LWIPDHCPINTERNAL_H_ */
