/*
 * LwipDhcp.h
 *
 *  Created on: Aug 21, 2018
 *      Author: mrabeda
 */

#ifndef MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_LWIPDHCP_H_
#define MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_LWIPDHCP_H_

#include "MpTcpIp.h"

typedef struct _LWIP_DHCP4_INSTANCE {
  UINT32                        Signature;
  EFI_HANDLE                    Handle;

  LWIP_NET_DEVICE               *Device;

  MP_FAIR_LOCK                  Lock;
  EFI_DHCP4_PROTOCOL            Protocol;
} LWIP_DHCP4_INSTANCE;

#define LWIP_DHCP4_INSTANCE_SIGNATURE     SIGNATURE_32 ('L', 'D', 'I', '4')
#define DHCP4_INSTANCE_FROM_PROTOCOL(p) \
  CR ( \
  (p), \
  LWIP_DHCP4_INSTANCE, \
  Protocol, \
  LWIP_DHCP4_INSTANCE_SIGNATURE \
  )

EFI_STATUS
EFIAPI
LwipDhcp4GetModeData (
  IN  EFI_DHCP4_PROTOCOL    *This,
  OUT EFI_DHCP4_MODE_DATA   *Dhcp4ModeData
  );

EFI_STATUS
EFIAPI
LwipDhcp4Configure (
  IN EFI_DHCP4_PROTOCOL     *This,
  IN EFI_DHCP4_CONFIG_DATA  *Dhcp4CfgData       OPTIONAL
  );

EFI_STATUS
EFIAPI
LwipDhcp4Start (
  IN EFI_DHCP4_PROTOCOL     *This,
  IN EFI_EVENT              CompletionEvent   OPTIONAL
  );

EFI_STATUS
EFIAPI
LwipDhcp4RenewRebind (
  IN EFI_DHCP4_PROTOCOL     *This,
  IN BOOLEAN                RebindRequest,
  IN EFI_EVENT              CompletionEvent   OPTIONAL
  );

EFI_STATUS
EFIAPI
LwipDhcp4Release (
  IN EFI_DHCP4_PROTOCOL     *This
  );

EFI_STATUS
EFIAPI
LwipDhcp4Stop (
  IN EFI_DHCP4_PROTOCOL     *This
  );

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
  );

EFI_STATUS
EFIAPI
LwipDhcp4TransmitReceive (
  IN EFI_DHCP4_PROTOCOL                *This,
  IN EFI_DHCP4_TRANSMIT_RECEIVE_TOKEN  *Token
  );

EFI_STATUS
EFIAPI
LwipDhcp4Parse (
  IN EFI_DHCP4_PROTOCOL       *This,
  IN EFI_DHCP4_PACKET         *Packet,
  IN OUT UINT32               *OptionCount,
  OUT EFI_DHCP4_PACKET_OPTION *PacketOptionList[] OPTIONAL
  );

#endif /* MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_LWIPDHCP_H_ */
