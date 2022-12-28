/*
 * lwipopts.h
 *
 *  Created on: Jun 12, 2018
 *      Author: mrabeda
 */

#ifndef MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_LWIPOPTS_H_
#define MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_LWIPOPTS_H_

#define NO_SYS                      0                   // Compile for OS environment
#define MEM_LIBC_MALLOC             0                   // Utilize LWIP memory allocators
#define MEMP_MEM_MALLOC             1                   // Allow resizing of memory pool allocated by malloc
#define MEM_ALIGNMENT               4                   // Alignment of memory in bytes
#define MEM_SIZE                    (4 * 1024 * 1024)   // Memory available for the stack
#define MEMP_NUM_PBUF               2048                // Number of packet buffers within memory pool
#define MEMP_NUM_UDP_PCB            20                  // Number of UDP protocol control blocks
#define MEMP_NUM_TCP_PCB            20                  // Number of simultaneously active TCP connections
#define MEMP_NUM_TCP_PCB_LISTEN     16                  // Number of listening TCP connections
#define MEMP_NUM_TCP_SEG            128                 // Number of consecutive TCP segments
#define MEMP_NUM_REASSDATA          32                  // Number of IP packets simultaneously queued for reassembly
#define MEMP_NUM_ARP_QUEUE          10                  // Number of simultaneously queued outgoing packets waiting for ARP request to finish
#define PBUF_POOL_SIZE              2048                // Number of buffers in pbuf pool
#define LWIP_ARP                    1                   // Enable ARP functionality
#define IP_REASS_MAX_PBUFS          64                  // Total max of pbufs waiting to be reassembled
#define IP_FRAG_USES_STATIC_BUF     1                   // Do not base pbuf size on MTU size
#define IP_DEFAULT_TTL              255                 // Default TTL value for transport layers
#define IP_SOF_BROADCAST            1                   // Enable broadcast filters per pcb on UDP and raw send operations.
#define IP_SOF_BROADCAST_RECV       1                   // Enable broadcast filter on receive operations
#define LWIP_ICMP                   1                   // Enable ICMP within LWIP
#define LWIP_BROADCAST_PING         1                   // Respond to broadcast ping
#define LWIP_MULTICAST_PING         1                   // Respond to multicast ping
#define LWIP_RAW                    0                   // Enable application layer to hook into IP layer
#define TCP_WND                     (4 * TCP_MSS)       // TCP window size
#define TCP_MSS                     1460                // TCP max segment size
#define TCP_SND_BUF                 (8 * TCP_MSS)       // TCP sender buffer space
#define TCP_LISTEN_BACKLOG          1                   // Enable backlog option for TCP
#define LWIP_NETIF_STATUS_CALLBACK  1                   // Support callback function for net interface status change
#define LWIP_NETIF_LINK_CALLBACK    1                   // Support callback function for link change
#define LWIP_NETIF_HWADDRHINT       1                   // Cache link layer address hints
#define LWIP_NETCONN                0                   // Netconn API on/off
#define LWIP_SOCKET                 1                   // Socket API on
#define LWIP_STATS_DISPLAY          0                   // Compile in statistic functions
#define MEM_STATS                   0                   // mem.c stats on/off
#define SYS_STATS                   0                   // System stats on/off
#define MEMP_STATS                  0                   // memp.c stats on/off
#define LINK_STATS                  0                   // Link stats on/off
#define ETHARP_TRUST_IP_MAC         0                   // Update (or not) ARP tables with IPs from incoming packets
#define ETH_PAD_SIZE                0                   // Number of bytes added before Ethernet header
#define LWIP_CHKSUM_ALGORITHM       2                   // Checksum algorithm
#define LWIP_DHCP                   1                   // Compile in DHCP
#define LWIP_PROVIDE_ERRNO          1                   // Use LWIP standard error codes
#define LWIP_NETIF_API              1                   // Compile NetIf API
#define LWIP_ETHERNET               1                   // Use Ethernet in higher level APIs
#define LWIP_TCPIP_CORE_LOCKING     0
#define SO_REUSE                    1
#define LWIP_STATS                  0
#define LWIP_CHECKSUM_ON_COPY       1
#define MEMP_NUM_TCPIP_MSG_API      1024
#define MEMP_NUM_TCPIP_MSG_INPKT    1024

#define LWIP_NO_STDDEF_H            1                   // stddef.h include disable
#define LWIP_NO_STDINT_H            1
#define LWIP_NO_INTTYPES_H          1
#define LWIP_NO_LIMITS_H            1
#define LWIP_NO_CTYPE_H             1

#define LWIP_TCP_KEEPALIVE          1                   // Keep alive TCP sessions

#define TCP_KEEIDLE_DEFAULT         10000UL             // Default KEEPALIVE timer in ms
#define TCP_KEPPINTVL_DEFAULT       2000UL              // Default time between KEEPALIVE probes in ms
#define TCP_KEEPCNT_DEFAULT         9U                  // Default counter for KEEPALIVE probes

#define TCPIP_MBOX_SIZE             1024                // Size of mailbox structure
#define DEFAULT_ACCEPTMBOX_SIZE     32                  // Size of TCP connection mbox

#define DEFAULT_UDP_RECVMBOX_SIZE   128                 // Size of UDP receive mbox for connection

#define DHCP_OPTIONS_LEN            400

#define LWIP_NETCONN_SEM_PER_THREAD     1

#include "MpTcpIpCommon.h"

//#define LWIP_DEBUG                  1
#define ETHARP_DEBUG                LWIP_DBG_OFF
#define NETIF_DEBUG                 LWIP_DBG_OFF /*LWIP_DBG_ON*/
#define PBUF_DEBUG                  LWIP_DBG_OFF
#define API_LIB_DEBUG               LWIP_DBG_OFF
#define API_MSG_DEBUG               LWIP_DBG_OFF /*LWIP_DBG_ON*/
#define SOCKETS_DEBUG               LWIP_DBG_OFF /*LWIP_DBG_ON*/
#define ICMP_DEBUG                  LWIP_DBG_OFF
#define INET_DEBUG                  LWIP_DBG_OFF
#define IP_DEBUG                    LWIP_DBG_OFF/*LWIP_DBG_ON*/
#define IP_REASS_DEBUG              LWIP_DBG_OFF
#define RAW_DEBUG                   LWIP_DBG_OFF/*LWIP_DBG_ON*/
#define MEM_DEBUG                   LWIP_DBG_OFF
#define MEMP_DEBUG                  LWIP_DBG_OFF
#define SYS_DEBUG                   LWIP_DBG_OFF
#define TCP_DEBUG                   LWIP_DBG_OFF
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
#define TCP_RTO_DEBUG               LWIP_DBG_OFF
#define TCP_CWND_DEBUG              LWIP_DBG_OFF
#define TCP_WND_DEBUG               LWIP_DBG_OFF
#define TCP_FR_DEBUG                LWIP_DBG_OFF
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
#define TCP_RST_DEBUG               LWIP_DBG_OFF
#define UDP_DEBUG                   LWIP_DBG_OFF/*LWIP_DBG_ON*/
#define TCPIP_DEBUG                 LWIP_DBG_OFF/*LWIP_DBG_ON*/
#define PPP_DEBUG                   LWIP_DBG_OFF
#define SLIP_DEBUG                  LWIP_DBG_OFF
#define DHCP_DEBUG                  LWIP_DBG_OFF/*LWIP_DBG_ON*/

#endif /* MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_LWIPOPTS_H_ */
