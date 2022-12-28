/**

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "MpTcpIp.h"

//err_t
//LwipTcpAcceptCallback (
//  void            *Arg,
//  struct tcp_pcb  *NewPcb,
//  err_t           err
//  )
//{
//  DBG ("[MP TCPIP][TCP] Connection accepted.\n");
//  return ERR_OK;
//}

VOID
LwipTcpTest (
  )
{
  int     LwipStatus;
  int     Socket, NewSocket;
  struct sockaddr_in Address;
  struct sockaddr_in ClientAddress;
  int     ClientAddressLength;

  DBG ("[MP TCPIP][TCP] Running TCP test\n");

  Socket = lwip_socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
  ASSERT (Socket != -1);

  DBG ("[MP TCPIP][TCP] Socket created\n");

  ZeroMem (&Address, sizeof (Address));
  Address.sin_family = AF_INET;
  Address.sin_port = htons(8080);
  Address.sin_addr.s_addr = htonl(INADDR_ANY);
  Address.sin_len = sizeof(Address);

  LwipStatus = lwip_bind (Socket, (struct sockaddr*)&Address, sizeof(Address));
  ASSERT (LwipStatus == 0);

  DBG ("[MP TCPIP][TCP] Socket bound\n");

  LwipStatus = lwip_listen (Socket, TCP_DEFAULT_LISTEN_BACKLOG);
  ASSERT (LwipStatus == 0);

  DBG ("[MP TCPIP][TCP] Socket is listening...\n");

  NewSocket = -1;
  ClientAddressLength = sizeof (ClientAddress);
  NewSocket = lwip_accept (Socket, (struct sockaddr*)&ClientAddress, &ClientAddressLength);
  ASSERT (NewSocket != -1);

  DBG ("[MP TCPIP][TCP] Connection accepted.\n");
  DBG ("[MP TCPIP][TCP] Client address length: %d\n", ClientAddressLength);
  DBG ("[MP TCPIP][TCP] Client address: %X\n", ClientAddress.sin_addr.s_addr);
  DBG ("[MP TCPIP][TCP] Connection test done!\n");

//  TcpPcb = tcp_new ();
//  ASSERT (TcpPcb != NULL);
//
//  LwipStatus = tcp_bind (TcpPcb, IP_ADDR_ANY, 8080);
//  ASSERT (LwipStatus == ERR_OK);
//
//  DBG ("[MP TCPIP][TCP] Bound\n");
//
//  TcpPcb = tcp_listen (TcpPcb);
//  ASSERT (TcpPcb != NULL);
//
//  DBG ("[MP TCPIP][TCP] Listen OK\n");
//
//  tcp_accept (TcpPcb, LwipTcpAcceptCallback);
//
//  DBG ("[MP TCPIP][TCP] Accept OK\n");
//
//  while (TRUE) {};


}
