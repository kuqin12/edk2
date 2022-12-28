/*
 * sys_arch.h
 *
 *  Created on: Jun 12, 2018
 *      Author: mrabeda
 */

#ifndef MDEMODULEPKG_UNIVERSAL_NETWORK_SYS_ARCH_H_
#define MDEMODULEPKG_UNIVERSAL_NETWORK_SYS_ARCH_H_

#include "MpTcpIpCommon.h"

#define SYS_MBOX_NULL     NULL
#define SYS_SEM_NULL      NULL

typedef struct _MULTIENTRY_SPINLOCK {
  SPIN_LOCK   Lock;
  INTN        CpuId;
} MULTIENTRY_SPINLOCK;

typedef struct _LWIP_SEMAPHORE {
  MP_FAIR_LOCK      Lock;
  LIST_ENTRY        WaitList;
  volatile UINT32   Count;
  BOOLEAN           IsValid;
} LWIP_SEMAPHORE;

#define MP_QUEUE_SIGNATURE    SIGNATURE_32('m', 'B', 'o', 'x')

typedef struct _MP_QUEUE {
  UINT32            Signature;
  MP_SPSC_QUEUE     Queue;
  MP_FAIR_LOCK      Lock;
} MP_QUEUE;

typedef MULTIENTRY_SPINLOCK sys_prot_t;
typedef SPIN_LOCK           sys_mutex_t;
typedef LWIP_SEMAPHORE      sys_sem_t;
typedef MP_QUEUE            sys_mbox_t;
typedef EFI_THREAD*         sys_thread_t;

#define SYS_ARCH_DECL_PROTECT(var)  static sys_prot_t  var = { .Lock = 1, .CpuId = -1 }
#define SYS_ARCH_PROTECT(var)       SysArchProtect(&var)
#define SYS_ARCH_UNPROTECT(var)     SysArchUnprotect(&var)

VOID
SysArchProtect (
  MULTIENTRY_SPINLOCK   *Lock
  );

VOID
SysArchUnprotect (
  MULTIENTRY_SPINLOCK   *Lock
  );

#define LWIP_NETCONN_THREAD_SEM_GET     sys_get_sem_by_core
#define LWIP_NETCONN_THREAD_SEM_ALLOC()
#define LWIP_NETCONN_THREAD_SEM_FREE()

sys_sem_t* sys_get_sem_by_core();

#endif /* MDEMODULEPKG_UNIVERSAL_NETWORK_SYS_ARCH_H_ */
