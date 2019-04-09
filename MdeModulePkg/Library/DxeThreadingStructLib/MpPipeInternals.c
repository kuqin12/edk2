/*
 * MpPipeInternals.c
 *
 *  Created on: Mar 27, 2018
 *      Author: mrabeda
 */

#include "MpPipeInternals.h"

typedef struct _THREAD_QUEUE_ENTRY {
  LIST_ENTRY      Link;
  UINT32          Trigger;
} THREAD_QUEUE_ENTRY;

VOID
ThreadQueueInit (
  THREAD_QUEUE    *ThreadQueue
  )
{
  ASSERT (ThreadQueue != NULL);

  InitializeSpinLock (&ThreadQueue->Lock);
  InitializeListHead (&ThreadQueue->List);
}

VOID
ThreadQueueSleep (
  THREAD_QUEUE    *ThreadQueue
  )
{
  THREAD_QUEUE_ENTRY    WaitEntry;
  UINT32                Val;

  ASSERT (ThreadQueue != NULL);

  InitializeListHead (&WaitEntry.Link);
  WaitEntry.Trigger = FALSE;

  AcquireSpinLock (&ThreadQueue->Lock);
  InsertTailList (&ThreadQueue->List, &WaitEntry.Link);
  ReleaseSpinLock (&ThreadQueue->Lock);

  while (TRUE) {
    _ReadWriteBarrier ();
    Val = InterlockedCompareExchange32 (&WaitEntry.Trigger, 1, 0);
    _ReadWriteBarrier ();
    if (Val == 1) {
      break;
    }
    CpuPause ();
  }
}

VOID
ThreadQueueWakeUnsafe (
  THREAD_QUEUE    *ThreadQueue
  )
{
  THREAD_QUEUE_ENTRY    *Entry;

  if (!IsListEmpty (&ThreadQueue->List)) {
    Entry = (THREAD_QUEUE_ENTRY*) GetFirstNode (&ThreadQueue->List);
    RemoveEntryList (&Entry->Link);
    _ReadWriteBarrier ();
    InterlockedIncrement (&Entry->Trigger);
    _ReadWriteBarrier ();
  }
}

VOID
ThreadQueueWake (
  THREAD_QUEUE    *ThreadQueue
  )
{
  ASSERT (ThreadQueue != NULL);

  AcquireSpinLock (&ThreadQueue->Lock);
  ThreadQueueWakeUnsafe (ThreadQueue);
  ReleaseSpinLock (&ThreadQueue->Lock);
}

VOID
ThreadQueueWakeAll (
  THREAD_QUEUE    *ThreadQueue
  )
{
  ASSERT (ThreadQueue != NULL);

  AcquireSpinLock (&ThreadQueue->Lock);
  while (!IsListEmpty (&ThreadQueue->List)) {
    ThreadQueueWakeUnsafe (ThreadQueue);
  }
  ReleaseSpinLock (&ThreadQueue->Lock);
}

VOID
ConditionInit (
  CONDITION *Condition
  )
{
  ASSERT (Condition != NULL);

  ThreadQueueInit (&Condition->WaitQueue);
}

VOID
ConditionSignal (
  CONDITION *Condition
  )
{
  ASSERT (Condition != NULL);

  ThreadQueueWake (&Condition->WaitQueue);
}

VOID
ConditionBroadcast (
  CONDITION *Condition
  )
{
  ASSERT (Condition != NULL);

  ThreadQueueWakeAll (&Condition->WaitQueue);
}

VOID
ConditionWait (
  CONDITION *Condition,
  SPIN_LOCK *Lock
  )
{
  ASSERT (Condition != NULL);

  ReleaseSpinLock (Lock);
  ThreadQueueSleep (&Condition->WaitQueue);
  AcquireSpinLock (Lock);
}

enum FQUEUE_FLAGS {
  FQUEUE_CPU_MUST_WAIT = 0,
  FQUEUE_CPU_HAS_LOCK = 1
};

VOID
ThreadFQueueInit (
  THREAD_FQUEUE     *FQueue,
  UINTN             Size
  )
{
  UINTN     i;

  ASSERT (FQueue != NULL);
  ASSERT (Size > 0);

  FQueue->Flags = AllocateZeroPool (sizeof (UINTN) * Size);
  FQueue->Flags[0] = FQUEUE_CPU_HAS_LOCK;

  for (i = 1; i < Size; i++) {
    FQueue->Flags[i] = FQUEUE_CPU_MUST_WAIT;
  }

  FQueue->Owner = (UINTN)-1;
  FQueue->Size = Size;
  FQueue->LastQueue = 0;
}

VOID
ThreadFQueueLock (
  THREAD_FQUEUE     *FQueue,
  UINTN             Id
  )
{
  UINT32    MyPlace;
  ASSERT (FQueue != NULL);

  MyPlace = InterlockedIncrement (&FQueue->LastQueue);
  MyPlace--;
//  DEBUG ((EFI_D_INFO, "[FQUEUE %d][Lock] MyPlace: %d. Wait on: %lX\n", Id, MyPlace, &FQueue->Flags[MyPlace % FQueue->Size]));

  while (FQueue->Flags[MyPlace % FQueue->Size] == FQUEUE_CPU_MUST_WAIT);
//  DEBUG ((EFI_D_INFO, "[FQUEUE %d][Lock] Got lock: %d\n", Id, MyPlace));
  FQueue->Owner = MyPlace;
}

VOID
ThreadFQueueUnlock (
  THREAD_FQUEUE     *FQueue,
  UINTN             Id
  )
{
  UINTN   Owner;
  ASSERT (FQueue != NULL);
  ASSERT (FQueue->Owner != (UINTN)-1);

  Owner = FQueue->Owner;

//  DEBUG ((EFI_D_INFO, "[FQUEUE %d][Unlock] Owner: %d\n", Id, Owner));

  FQueue->Flags[Owner % FQueue->Size] = FQUEUE_CPU_MUST_WAIT;
  FQueue->Owner = (UINTN)-1;
//  DEBUG ((EFI_D_INFO, "[FQUEUE %d][Unlock] New owner: %d. Field: %lX\n", Id, Owner + 1, &FQueue->Flags[(Owner + 1) % FQueue->Size]));
  FQueue->Flags[(Owner + 1) % FQueue->Size] = FQUEUE_CPU_HAS_LOCK;
}

//
// TODO: ThreadFQueueDestroy
//
