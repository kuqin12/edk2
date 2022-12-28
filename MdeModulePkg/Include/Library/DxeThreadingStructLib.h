/*
 * DxeThreadingStructLib.h
 *
 *  Created on: Nov 26, 2018
 *      Author: mrabeda
 */

#ifndef MDEMODULEPKG_INCLUDE_LIBRARY_DXETHREADINGSTRUCTLIB_H_
#define MDEMODULEPKG_INCLUDE_LIBRARY_DXETHREADINGSTRUCTLIB_H_

//
// MRA: MP Linked List
//
#include <Library/SynchronizationLib.h>

//------------------------------------------------------------------------------
// MP_LIST
//------------------------------------------------------------------------------
typedef struct _MP_LIST_ENTRY     MP_LIST_ENTRY;

#define MP_LIST_FOREACH(List, Entry, Value, Type) \
    AcquireSpinLock (&(List)->Lock); \
    for (Entry = (List)->Front, Value = (Entry != NULL) ? (Type)Entry->Data : NULL; Entry != NULL; Entry = Entry->Next)

#define MP_LIST_FOREACH_END(List) \
    ReleaseSpinLock (&(List)->Lock)

typedef struct _MP_LIST {
  MP_LIST_ENTRY     *Front;
  MP_LIST_ENTRY     *Back;
  UINTN             Count;
  SPIN_LOCK         Lock;
} MP_LIST;

typedef struct _MP_LIST_ENTRY {
  MP_LIST           *Parent;
  MP_LIST_ENTRY     *Previous;
  MP_LIST_ENTRY     *Next;
  VOID              *Data;
} MP_LIST_ENTRY;

//
// Initialize MP_LIST internals
//
EFI_STATUS
MpListInitialize (
  MP_LIST           *List
  );

//
// Lock the list
//
VOID
MpListLock (
  MP_LIST           *List
  );

//
// Unlock the list
//
VOID
MpListUnlock (
  MP_LIST           *List
  );

//
// Allocating new raw entry
//
MP_LIST_ENTRY*
MpListEntryAllocate (
  );

MP_LIST_ENTRY*
MpListEntryAllocateEx (
  UINTN       Count
  );

//
// Freeing up the raw entry
//
VOID
MpListEntryFree (
  MP_LIST_ENTRY     *Entry
  );

//
// Test whether list is empty
//
BOOLEAN
MpListIsEmpty (
  MP_LIST           *List
  );

//
// Lock the list and add element to the back of the list
//
VOID
MpListEntryPushBack (
  MP_LIST           *List,
  MP_LIST_ENTRY     *Entry
  );

//
// Add element to the back of the list without locking the list
//
VOID
MpListEntryPushBackUnsafe (
  MP_LIST           *List,
  MP_LIST_ENTRY     *Entry
  );

//
// Lock the list and add element to the front of the list
//
VOID
MpListEntryPushFront (
  MP_LIST           *List,
  MP_LIST_ENTRY     *Entry
  );

//
// Add element to the front of the list without locking the list
//
VOID
MpListEntryPushFrontUnsafe (
  MP_LIST           *List,
  MP_LIST_ENTRY     *Entry
  );

//
// Lock the list and remove last entry from the list
//
MP_LIST_ENTRY*
MpListEntryPopBack (
  MP_LIST           *List
  );

//
// Remove last entry from the list without locking the list
//
MP_LIST_ENTRY*
MpListEntryPopBackUnsafe (
  MP_LIST           *List
  );

//
// Lock the list and remove first entry from the list
//
MP_LIST_ENTRY*
MpListEntryPopFront (
  MP_LIST           *List
  );

//
// Remove first entry from the list without locking the list
//
MP_LIST_ENTRY*
MpListEntryPopFrontUnsafe (
  MP_LIST           *List
  );

//
// Lock the list and add element to the back of the list
//
EFI_STATUS
MpListPushBack (
  MP_LIST           *List,
  VOID              *Data
  );

//
// Add element to the back of the list without locking the list
//
EFI_STATUS
MpListPushBackUnsafe (
  MP_LIST           *List,
  VOID              *Data
  );

//
// Lock the list and add element to the front of the list
//
EFI_STATUS
MpListPushFront (
  MP_LIST           *List,
  VOID              *Data
  );

//
// Add element to the front of the list without locking the list
//
EFI_STATUS
MpListPushFrontUnsafe (
  MP_LIST           *List,
  VOID              *Data
  );

//
// Lock the list and obtain the element from the back of the list
//
VOID*
MpListPopBack (
  MP_LIST           *List
  );

//
// Obtain the element from the back of the list without locking the list
//
VOID*
MpListPopBackUnsafe (
  MP_LIST           *List
  );

//
// Lock the list and obtain the element from the front of the list
//
VOID*
MpListPopFront (
  MP_LIST           *List
  );

//
// Obtain the element from the front of the list without locking the list
//
VOID*
MpListPopFrontUnsafe (
  MP_LIST           *List
  );

//
// Lock the list and get the last element of the list
//
MP_LIST_ENTRY*
MpListGetBack (
  MP_LIST           *List
  );

//
// Get the last element of the list without locking the list
//
MP_LIST_ENTRY*
MpListGetBackUnsafe (
  MP_LIST           *List
  );

//
// Lock the list and get the first element of the list
//
MP_LIST_ENTRY*
MpListGetFront (
  MP_LIST           *List
  );

//
// Get the first element of the list without locking the list
//
MP_LIST_ENTRY*
MpListGetFrontUnsafe (
  MP_LIST           *List
  );

//
// Removal of single raw entry
// Entry must have a valid pointer to parent list
// ShouldLock can be set if spinlock acquiring is required
//
EFI_STATUS
MpListRemove (
  MP_LIST_ENTRY     *Entry
  );

//
// Remove the element without locking the list
//
EFI_STATUS
MpListRemoveUnsafe (
  MP_LIST_ENTRY     *Entry
  );

typedef
EFI_STATUS
(EFIAPI *MP_LIST_ITERATE_FUNCTION) (
  MP_LIST_ENTRY               *Entry,
  VOID                        *Arg
  );

//
// Call function for each entry of the list
//
EFI_STATUS
MpListIterate (
  MP_LIST                     *List,
  MP_LIST_ITERATE_FUNCTION    IterateFunction,
  VOID                        *Arg
  );

EFI_STATUS
MpListIterateUnsafe (
  MP_LIST                     *List,
  MP_LIST_ITERATE_FUNCTION    IterateFunction,
  VOID                        *Arg
  );

//
// Remove all data from the list
//
EFI_STATUS
MpListClear (
  MP_LIST     *List
  );

//------------------------------------------------------------------------------
// MP_LIST_CACHED
//------------------------------------------------------------------------------
typedef struct _MP_LIST_CACHED {
  MP_LIST           InList;
  MP_LIST           Cache;
} MP_LIST_CACHED;

//
// Initialize MP_LIST_CACHED internals
//
UINTN
MpListCachedInitialize (
  MP_LIST_CACHED    *List,
  UINTN             InitCacheLen
  );

//
// Lock the cached list
//
VOID
MpListCachedLock (
  MP_LIST_CACHED      *List
  );

//
// Unlock the cached list
//
VOID
MpListCachedUnlock (
  MP_LIST_CACHED      *List
  );

//
// Allocate entry within cached list
// Either from entry cache or memory pool
//
MP_LIST_ENTRY*
MpListCachedEntryAllocate (
  MP_LIST_CACHED    *List
  );

//
// Cache the entry
//
VOID
MpListCachedEntryFree (
  MP_LIST_CACHED  *List,
  MP_LIST_ENTRY   *Entry
  );

//
// Test whether cached list is empty
//
BOOLEAN
MpListCachedIsEmpty (
  MP_LIST_CACHED    *List
  );

//
// Lock the list and add the data element to the end of the list
//
EFI_STATUS
MpListCachedPushBack (
  MP_LIST_CACHED    *List,
  VOID              *Data
  );

//
// Add the data element to the end of the list without locking the list
//
EFI_STATUS
MpListCachedPushBackUnsafe (
  MP_LIST_CACHED    *List,
  VOID              *Data
  );

//
// Lock the list and add the data element to the front of the list
//
EFI_STATUS
MpListCachedPushFront (
  MP_LIST_CACHED    *List,
  VOID              *Data
  );

//
// Add the data element to the front of the list without locking the list
//
EFI_STATUS
MpListCachedPushFrontUnsafe (
  MP_LIST_CACHED    *List,
  VOID              *Data
  );

//
// Lock the list and obtain the data element from the front of the list
//
VOID*
MpListCachedPopFront (
  MP_LIST_CACHED    *List
  );

//
// Obtain the data element from the front of the list without locking the list
//
VOID*
MpListCachedPopFrontUnsafe (
  MP_LIST_CACHED    *List
  );

//
// Clean up the list (cache + data - optional freeing of data memory)
//
EFI_STATUS
MpListCachedFree (
  MP_LIST_CACHED            *List,
  MP_LIST_ITERATE_FUNCTION  Callback
  );

//------------------------------------------------------------------------------
// MP_PIPE - TODO: Protect the PIPE with signatures. Remake functions to work
//                 with EFI_STATUS.
//------------------------------------------------------------------------------
typedef struct _PIPE            PIPE;
typedef struct _PIPE_PRODUCER   PIPE_PRODUCER;
typedef struct _PIPE_CONSUMER   PIPE_CONSUMER;
typedef struct _PIPE_GENERIC    PIPE_GENERIC;
typedef struct _THREAD_FQUEUE   THREAD_FQUEUE;

typedef struct _THREAD_QUEUE {
  SPIN_LOCK     Lock;
  LIST_ENTRY    List;
} THREAD_QUEUE;

VOID
ThreadQueueInit (
  THREAD_QUEUE    *ThreadQueue
  );

VOID
ThreadQueueSleep (
  THREAD_QUEUE    *ThreadQueue
  );

VOID
ThreadQueueWake (
  THREAD_QUEUE    *ThreadQueue
  );

VOID
ThreadQueueWakeAll (
  THREAD_QUEUE    *ThreadQueue
  );

typedef struct _CONDITION {
  THREAD_QUEUE  WaitQueue;
} CONDITION;

struct _PIPE {
  UINTN   ElementSize;
  UINTN   MinCap;
  UINTN   MaxCap;

  UINT8   *Buffer;
  UINT8   *BufferEnd;
  UINT8   *Begin;
  UINT8   *End;

  UINTN   ProducerRefCount;
  UINTN   ConsumerRefCount;

  SPIN_LOCK BeginLock;
  SPIN_LOCK EndLock;

  CONDITION JustPushed;
  CONDITION JustPopped;
};

EFI_STATUS
PipeNew (
  UINT32  ElementSize,
  UINT32  Limit,
  PIPE    **Pipe
  );

EFI_STATUS
PipeProducerNew (
  PIPE            *Pipe,
  PIPE_PRODUCER   **Producer
  );

EFI_STATUS
PipeConsumerNew (
  PIPE            *Pipe,
  PIPE_CONSUMER   **Consumer
  );

EFI_STATUS
PipeFree (
  PIPE            *Pipe
  );

EFI_STATUS
PipeProducerFree (
  PIPE_PRODUCER     *Producer
  );

EFI_STATUS
PipeConsumerFree (
  PIPE_CONSUMER     *Consumer
  );

EFI_STATUS
PipePush (
  PIPE_PRODUCER* Producer,
  VOID* Elements,
  UINTN Count
  );

EFI_STATUS
PipePop (
  PIPE_CONSUMER   *Consumer,
  VOID            *Target,
  UINTN           *Count
  );

EFI_STATUS
PipePopEager (
  PIPE_CONSUMER   *Consumer,
  VOID            *Target,
  UINTN           *Count,
  BOOLEAN         ShouldWait
  );

struct _THREAD_FQUEUE {
  volatile UINTN       *Flags;
  UINTN       Size;
  UINT32      LastQueue;
  UINTN       Owner;
};

VOID
ThreadFQueueInit (
  THREAD_FQUEUE     *FQueue,
  UINTN             Size
  );

VOID
ThreadFQueueLock (
  THREAD_FQUEUE     *FQueue,
  UINTN             Id
  );

VOID
ThreadFQueueUnlock (
  THREAD_FQUEUE     *FQueue,
  UINTN             Id
  );

//------------------------------------------------------------------------------
// Single Producer Single Consumer Queue
// Lock-free implementation
//------------------------------------------------------------------------------
typedef struct _MP_SPSC_QUEUE {
  UINTN             Increment;
  volatile UINTN    Tail;
  volatile UINTN    Head;
  UINT8             *Elements;
  UINTN             Capacity;
  UINTN             ElementSize;
} MP_SPSC_QUEUE;

EFI_STATUS
MpSPSCQueueInit (
  UINTN             ElementSize,
  UINTN             Count,
  MP_SPSC_QUEUE     *Queue
  );

EFI_STATUS
MpSPSCQueuePush (
  MP_SPSC_QUEUE     *Queue,
  VOID              *Element
  );

EFI_STATUS
MpSPSCQueuePop (
  MP_SPSC_QUEUE     *Queue,
  VOID              *Element
  );

EFI_STATUS
MpSPSCQueueDestroy (
  MP_SPSC_QUEUE     *Queue
  );

//------------------------------------------------------------------------------
// Re-entry spin lock
//------------------------------------------------------------------------------
typedef struct _MP_REENTRY_LOCK {
  SPIN_LOCK   Lock;
  INTN        OwnerAP;
} MP_REENTRY_LOCK;

VOID
MpReentryLockInit (
  MP_REENTRY_LOCK   *Lock
  );

VOID
MpReentryLockAcquire (
  MP_REENTRY_LOCK   *Lock
  );

VOID
MpReentryLockRelease (
  MP_REENTRY_LOCK   *Lock
  );

//------------------------------------------------------------------------------
// Fair lock
//------------------------------------------------------------------------------
typedef struct _MP_FAIR_LOCK {
  volatile UINT8    *Flags;
  volatile UINT8    *States;
  volatile UINTN    Owner;
  volatile UINT32   Last;
} MP_FAIR_LOCK;

EFI_STATUS
MpFairLockInit (
  MP_FAIR_LOCK    *Lock
  );

EFI_STATUS
EFIAPI
MpFairLockAcquire (
  IN  MP_FAIR_LOCK          *Lock
  );

EFI_STATUS
MpFairLockRelease (
  IN  MP_FAIR_LOCK            *Lock
  );

VOID
MpFairLockDestroy (
  MP_FAIR_LOCK    *Lock
  );

#endif /* MDEMODULEPKG_INCLUDE_LIBRARY_DXETHREADINGSTRUCTLIB_H_ */
