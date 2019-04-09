/*
 * sys_arch.c
 *
 *  Created on: Jun 19, 2018
 *      Author: mrabeda
 */

#include "MpTcpIp.h"
#include "lwip/sys.h"


sys_mutex_t   lock_tcpip_core;
int           errno;

sys_sem_t     *ThreadSemaphores = NULL;

typedef struct _SEM_WAIT_ENTRY {
  LIST_ENTRY          Entry;
  volatile BOOLEAN    Trigger;
} SEM_WAIT_ENTRY;

//
// System init
// Called by lwip_init()
//
void sys_init (void)
{
  EFI_STATUS    Status;
  UINTN   CpuCount, EnabledCpuCount, i;

  sys_mutex_new(&lock_tcpip_core);
  Status = GetCpuCount (&CpuCount, &EnabledCpuCount);
  ASSERT_EFI_ERROR (Status);

  ThreadSemaphores = AllocateZeroPool (sizeof (sys_sem_t) * CpuCount);
  ASSERT (ThreadSemaphores != NULL);

  for (i = 0; i < CpuCount; i++) {
    Status = sys_sem_new (&ThreadSemaphores[i], 0);
    ASSERT_EFI_ERROR (Status);
  }
}

sys_sem_t*
sys_get_sem_by_core () {
  UINT32    CpuId;
  BOOLEAN   IsBsp;

  IdentifyCpu (&CpuId, &IsBsp);
//  DEBUG ((EFI_D_INFO, "[LWIP] Getting semaphore for core %d: %lX\n", CpuId, &ThreadSemaphores[CpuId]));

  return &ThreadSemaphores[CpuId];
}

//
// Return current system time (in miliseconds)
//
u32_t
sys_now (
  )
{
  UINT64  EfiSystemTime;

  //
  // EFI system time is defined as 100ns jiffies. Scale to miliseconds.
  //
  EfiSystemTime = gBS->GetTime ();

  return (u32_t)(EfiSystemTime / (10 * 1000));
}

//
// Locks
//
VOID
SysArchProtect (
  MULTIENTRY_SPINLOCK   *Lock
  )
{
  UINT32      CpuId;
  BOOLEAN     IsBsp;

  IdentifyCpu (&CpuId, &IsBsp);

  if (CpuId != Lock->CpuId) {
    AcquireSpinLock (&Lock->Lock);
    Lock->CpuId = CpuId;
  }
}

VOID
SysArchUnprotect (
  MULTIENTRY_SPINLOCK   *Lock
  )
{
  UINT32      CpuId;
  BOOLEAN     IsBsp;

  IdentifyCpu (&CpuId, &IsBsp);

  ASSERT (CpuId == Lock->CpuId);
  Lock->CpuId = -1;
  ReleaseSpinLock (&Lock->Lock);
}

//
// Mutexes
//
err_t sys_mutex_new (sys_mutex_t *mutex)
{
  InitializeSpinLock(mutex);
  return ERR_OK;
}

void sys_mutex_lock(sys_mutex_t *mutex)
{
  AcquireSpinLock(mutex);
}

void sys_mutex_unlock(sys_mutex_t *mutex)
{
  ReleaseSpinLock(mutex);
}

//
// Semaphores
//
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
  if (sem == NULL) {
    return ERR_ARG;
  }

  if (MpFairLockInit (&sem->Lock) != EFI_SUCCESS) {
    return ERR_MEM;
  }
  
  InitializeListHead (&sem->WaitList);

  sem->Count        = count;
  sem->IsValid      = TRUE;

  return ERR_OK;
}

void sys_sem_free(sys_sem_t *sem)
{
  ASSERT (sem != NULL);

  //
  // Block further acquiring
  //
  sys_sem_set_invalid (sem);

  //
  // Wait until semaphore counter resets to max value
  // TODO: Rework to signal waiters to unlock them from waiting.
  while (!IsListEmpty (&sem->WaitList)) {
    CpuPause ();
  }

  MpFairLockDestroy (&sem->Lock);
  sem->Count    = 0;
}

int sys_sem_valid(sys_sem_t *sem)
{
  if (sem->IsValid) {
    return 1;
  } else {
    return 0;
  }
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
  u32_t             Count;
  SEM_WAIT_ENTRY    Entry;

  Count = 0;

  if (!sys_sem_valid (sem)) {
    DBG ("ASSERT: sem invalid.\n");
    ASSERT (FALSE);
  }
  
  MpFairLockAcquire (&sem->Lock);
  if (sem->Count > 0) {
    //
    // Got access. Instant exit
    //
    sem->Count--;
    Entry.Trigger = TRUE;
  } else {
    Entry.Trigger = FALSE;
    InitializeListHead (&Entry.Entry);
    InsertTailList (&sem->WaitList, &Entry.Entry);
  }

  MpFairLockRelease (&sem->Lock);

  // 
  // TODO: Take timeouts into consideration...
  //
  while (!Entry.Trigger) {
    CpuPause ();
    Count++;
  }

  return Count;
}

void sys_sem_signal(sys_sem_t *sem)
{
  SEM_WAIT_ENTRY  *Entry;

  MpFairLockAcquire (&sem->Lock);
  if (!IsListEmpty (&sem->WaitList)) {
    Entry = (SEM_WAIT_ENTRY*) GetFirstNode (&sem->WaitList);
    RemoveEntryList (&Entry->Entry);
    Entry->Trigger = TRUE;
  } else {
    sem->Count++;
  }
  MpFairLockRelease (&sem->Lock);
}

void sys_sem_set_invalid(sys_sem_t *sem)
{
  if (sys_sem_valid (sem)) {
    MpFairLockAcquire (&sem->Lock);
    sem->IsValid = FALSE;
    MpFairLockRelease (&sem->Lock);
  }
}

//
// Mailboxes
//
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
  EFI_STATUS  Status;

  Status = MpFairLockInit (&mbox->Lock);

  Status = MpSPSCQueueInit (sizeof(VOID*), size, &mbox->Queue);
  if (EFI_ERROR (Status)) {
    return ERR_MEM;
  }

  mbox->Signature = MP_QUEUE_SIGNATURE;

  return ERR_OK;
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
  EFI_STATUS  Status;

  MpFairLockAcquire(&mbox->Lock);
  Status = MpSPSCQueuePush (&mbox->Queue, &msg);
  MpFairLockRelease(&mbox->Lock);

//  DBG ("sys_mbox_trypost: Status: %r\n", Status);

  if (EFI_ERROR (Status)) {
    return ERR_MEM;
  }

//  DBG ("sys_mbox_trypost: Added msg %lX to box %lX\n", msg, mbox);

  return ERR_OK;
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
  while (sys_mbox_trypost(mbox, msg) != ERR_OK) {}
}

int sys_mbox_valid(sys_mbox_t *mbox)
{
  if (mbox->Signature == MP_QUEUE_SIGNATURE) {
    return 1;
  } else {
    return 0;
  }
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
  EFI_STATUS    Status;

  //
  // NOTE: If this function gets to be called outside the LWIP thread
  //       this queue pop will have to be converted to 
  //       a critical section.
  //
  Status = MpSPSCQueuePop(&mbox->Queue, msg);

  if (EFI_ERROR (Status)) {
    return SYS_MBOX_EMPTY;
  }

//  DBG ("sys_mbox_tryfetch: Obtained msg %lX from box %lX\n", *msg, mbox);

  return 0;
}

#define SYS_MBOX_FETCH_SCALAR     1000

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
  UINT32        Result;
  UINTN         Scalar;

  Result = 0;

//  DBG ("sys_mbox_fetch: Beginning attempt.\n");
  Scalar = SYS_MBOX_FETCH_SCALAR;

  while (sys_arch_mbox_tryfetch(mbox, msg) == SYS_MBOX_EMPTY) {
    if (timeout != 0) {
      Scalar--;
      if (Scalar == 0) {
        Result++;
        Scalar = SYS_MBOX_FETCH_SCALAR;
      }
      if (Result == ((UINT64)timeout)) {
        break;
      }
    }
  }

  if (timeout != 0 && Result >= (UINT64)timeout) {
    //DBG ("sys_mbox_fetch: Timeout.\n");
    return SYS_ARCH_TIMEOUT;
  }
//  DBG ("sys_mbox_fetch: Attempt OK.\n");

  return (UINT32)(Result);
}

void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
  mbox->Signature = 0;
}

void sys_mbox_free(sys_mbox_t *mbox)
{
  MpSPSCQueueDestroy (&mbox->Queue);
  MpFairLockDestroy (&mbox->Lock);
}

//
// Thread spawning
//
sys_thread_t
sys_thread_new (
  const char        *name,
  lwip_thread_fn    thread,
  void              *arg,
  int               stacksize,
  int               prio
  )
{
  EFI_STATUS    Status;
  EFI_THREAD    *Result;

  Status = SpawnThread (
             thread,
             arg,
             NULL,
             NULL,
             0,
             &Result
             );

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return Result;
}

void
sys_thread_stop (
  sys_thread_t    thread
  )
{
  AbortThread (thread);
//  WaitForThread (thread);
}


