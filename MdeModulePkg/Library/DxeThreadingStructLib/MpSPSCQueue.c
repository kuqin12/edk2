/*
 * MpSPSCQueue.c
 *
 *  Created on: Aug 8, 2018
 *      Author: mrabeda
 */

#include "MpCommon.h"

EFI_STATUS
MpSPSCQueueInit (
  UINTN             ElementSize,
  UINTN             Count,
  MP_SPSC_QUEUE     *Queue
  )
{
  ASSERT (Queue != NULL);

  Queue->Elements = AllocateZeroPool (ElementSize * (Count + 1));

  if (Queue->Elements == NULL) {
    FreePool (Queue);
    return EFI_OUT_OF_RESOURCES;
  }

  Queue->Head = 0;
  Queue->Tail = 0;
  Queue->ElementSize = ElementSize;
  Queue->Capacity = Count + 1;

//  DEBUG ((EFI_D_INFO, "[SPSCQ][%lX] Init. Head: %d, Tail: %d, Elements: %lX, ElementSize: %d, Capacity: %d\n",
//      Queue, Queue->Head, Queue->Tail, Queue->Elements, Queue->ElementSize, Queue->Capacity));

  return EFI_SUCCESS;
}

UINTN
_MpSPSCQueueIncrement (
  MP_SPSC_QUEUE     *Queue,
  UINTN             Value
  )
{
  ASSERT (Queue != NULL);
  ASSERT (Queue->Capacity != 0);

  return (Value + 1) % Queue->Capacity;
}

EFI_STATUS
MpSPSCQueuePush (
  MP_SPSC_QUEUE     *Queue,
  VOID              *Element
  )
{
  UINTN   CurrentTail;
  UINTN   NextTail;
  ASSERT (Queue != NULL);
  ASSERT (Element != NULL);

  CurrentTail = Queue->Tail;
  NextTail = _MpSPSCQueueIncrement (Queue, CurrentTail);

//  DEBUG ((EFI_D_INFO, "[SPSCQ][%lX] Push. Current tail: %d, Next tail: %d\n", Queue, CurrentTail, NextTail));

  if (NextTail != Queue->Head) {
    UINT8   *ElementPtr;

    ElementPtr = Queue->Elements + CurrentTail * Queue->ElementSize;
    CopyMem (ElementPtr, Element, sizeof (Queue->ElementSize));
    Queue->Tail = NextTail;

//    DEBUG ((EFI_D_INFO, "[SPSCQ][%lX] Push. ElementPtr: %lX, OK.\n", Queue, ElementPtr));
    return EFI_SUCCESS;
  }

//  DEBUG ((EFI_D_INFO, "[SPSCQ][%lX] Push. Out of resources.\n", Queue));

  //
  // Queue is full
  //
  return EFI_OUT_OF_RESOURCES;
}

EFI_STATUS
MpSPSCQueuePop (
  MP_SPSC_QUEUE     *Queue,
  VOID              *Element
  )
{
  UINTN     CurrentHead;
  UINT8     *ElementPtr;

  ASSERT (Queue != NULL);
  ASSERT (Element != NULL);

  CurrentHead = Queue->Head;

//  DEBUG ((EFI_D_INFO, "[SPSCQ][%lX] Pop. Current head: %d, Current tail: %d\n", Queue, CurrentHead, Queue->Tail));

  if (CurrentHead == Queue->Tail) {
    //
    // Queue is empty
    //
//    DEBUG ((EFI_D_INFO, "[SPSCQ][%lX] Pop. Queue empty.\n", Queue));
    return EFI_NOT_READY;
  }

  ElementPtr = Queue->Elements + CurrentHead * Queue->ElementSize;
  CopyMem (Element, ElementPtr, Queue->ElementSize);
  Queue->Head = _MpSPSCQueueIncrement (Queue, CurrentHead);
//  DEBUG ((EFI_D_INFO, "[SPSCQ][%lX] Pop. ElementPtr: %lX, new head: %d. OK.\n", Queue, ElementPtr, Queue->Head));
  return EFI_SUCCESS;
}

EFI_STATUS
MpSPSCQueueDestroy (
  MP_SPSC_QUEUE     *Queue
  )
{
  FreePool (Queue->Elements);
  return EFI_SUCCESS;
}

