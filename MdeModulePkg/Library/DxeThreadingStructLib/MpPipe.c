/*
 * MpPipe.c
 *
 *  Created on: Mar 27, 2018
 *      Author: mrabeda
 */

#include "MpPipeInternals.h"

typedef struct _SNAPSHOT {
  UINT8   *Buffer;
  UINT8   *BufferEnd;
  UINT8   *Begin;
  UINT8   *End;
  UINTN   ElementSize;
} SNAPSHOT;

#define PIPE_MIN_CAPACITY       1024
#define CONVERT_TO_PIPE(p)      ((PIPE*)(p))

UINT32
GetNextPowerOfTwo32 (
  UINT32      Number
  )
{
  return 1ul << (HighBitSet32 (Number) + 1);
}

//
// Pipe internal functions
//
UINTN
__PipeGetElementSize (
  PIPE        *Pipe
  )
{
  ASSERT (Pipe != NULL);
  //
  // TODO: Check pipe signature
  //
  return Pipe->ElementSize;
}

inline
VOID*
OffsetCopyMem (
  VOID      *Destination,
  VOID      *Source,
  UINTN     Length
  )
{
  ASSERT (Destination != NULL);
  ASSERT (Source != NULL);

  CopyMem (Destination, Source, Length);
  return (UINT8*)Destination + Length;
}

inline
SNAPSHOT
SnapshotMake (
  PIPE      *Pipe
  )
{
  SNAPSHOT    Snapshot;

  ASSERT (Pipe != NULL);
  //
  // TODO: Check pipe signature
  //

  Snapshot.Buffer       = Pipe->Buffer;
  Snapshot.BufferEnd    = Pipe->BufferEnd;
  Snapshot.Begin        = Pipe->Begin;
  Snapshot.End          = Pipe->End;
  Snapshot.ElementSize  = __PipeGetElementSize(Pipe);

  return Snapshot;
}

inline
BOOLEAN
DoesSnapshotWrapAround (
  SNAPSHOT      Snapshot
  )
{
  return (Snapshot.Begin >= Snapshot.End);
}

inline
UINT8*
PipeCopyIntoNewBuffer (
  SNAPSHOT      Snapshot,
  UINT8         *Buffer
  )
{
  ASSERT (Buffer != NULL);

  if (DoesSnapshotWrapAround (Snapshot)) {
    Buffer = OffsetCopyMem (Buffer, Snapshot.Begin, Snapshot.BufferEnd - Snapshot.Begin);
    Buffer = OffsetCopyMem (Buffer, Snapshot.Buffer, Snapshot.End - Snapshot.Buffer);
  } else {
    Buffer = OffsetCopyMem (Buffer, Snapshot.Begin, Snapshot.End - Snapshot.Begin);
  }

  return Buffer;
}

inline
UINTN
SnapshotBytesInUse (
  SNAPSHOT      Snapshot
  )
{
  UINTN     Result;

  if (DoesSnapshotWrapAround (Snapshot)) {
    Result = ((Snapshot.End - Snapshot.Buffer) + (Snapshot.BufferEnd - Snapshot.Begin));
  } else {
    Result = (Snapshot.End - Snapshot.Begin);
  }

  return Result - Snapshot.ElementSize;
}

inline
UINTN
SnapshotCapacity (
  SNAPSHOT      Snapshot
  )
{
  return Snapshot.BufferEnd - Snapshot.Buffer - Snapshot.ElementSize;
}

EFI_STATUS
PipeResizeBuffer (
  PIPE      *Pipe,
  UINTN     NewSize,
  SNAPSHOT  *Snapshot
  )
{
  UINTN     MaxCap;
  UINTN     MinCap;
  UINTN     ElementSize;
  UINT8     *NewBuffer;

  if (Pipe == NULL || NewSize == 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // TODO: Check pipe signature
  //

  MaxCap        = Pipe->MaxCap;
  MinCap        = Pipe->MinCap;
  ElementSize   = __PipeGetElementSize (Pipe);

  if (NewSize >= MaxCap) {
    NewSize = MaxCap;
  }

  if (NewSize <= MinCap) {
    *Snapshot = SnapshotMake (Pipe);
    return EFI_SUCCESS;
  }

  NewBuffer = AllocateZeroPool (NewSize + ElementSize);
  if (NewBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Pipe->End = PipeCopyIntoNewBuffer (SnapshotMake (Pipe), NewBuffer);
  FreePool (Pipe->Buffer);
  Pipe->Begin = Pipe->Buffer = NewBuffer;
  Pipe->BufferEnd = NewBuffer + NewSize + ElementSize;

  *Snapshot = SnapshotMake(Pipe);
  return EFI_SUCCESS;
}

inline
SNAPSHOT
PipeWaitForRoom (
  PIPE      *Pipe,
  UINTN     *MaxCap
  )
{
  SNAPSHOT      Snapshot;
  UINTN         BytesUsed;
  UINTN         ConsumerRefCount;

  ASSERT (Pipe != NULL);
  ASSERT (MaxCap != NULL);
  //
  // TODO: Check pipe signature
  //

  Snapshot          = SnapshotMake (Pipe);
  BytesUsed         = SnapshotBytesInUse (Snapshot);
  ConsumerRefCount  = Pipe->ConsumerRefCount;
  *MaxCap           = Pipe->MaxCap;

  for (; (BytesUsed == *MaxCap) && (ConsumerRefCount > 0);
       Snapshot         = SnapshotMake (Pipe),
       BytesUsed        = SnapshotBytesInUse (Snapshot),
       ConsumerRefCount = Pipe->ConsumerRefCount,
       *MaxCap          = Pipe->MaxCap) {
    ConditionWait (&Pipe->JustPopped, &Pipe->EndLock);
  }

  return Snapshot;
}

inline
SNAPSHOT
PipeValidateSize (
  PIPE      *Pipe,
  SNAPSHOT  Snapshot,
  UINTN     NewBytes
  )
{
  UINTN     ElementSize;
  UINTN     Capacity;
  UINTN     BytesNeeded;
  UINTN     ElementsNeeded;

  ASSERT (Pipe != NULL);
  //
  // TODO: Check pipe signature
  //
  ElementSize   = __PipeGetElementSize (Pipe);
  Capacity      = SnapshotCapacity (Snapshot);
  BytesNeeded   = SnapshotBytesInUse (Snapshot) + NewBytes;

  if (BytesNeeded > Capacity) {
    AcquireSpinLock (&Pipe->BeginLock);
    Snapshot = SnapshotMake (Pipe);
    BytesNeeded = SnapshotBytesInUse (Snapshot) + NewBytes;
    ElementsNeeded = BytesNeeded / ElementSize;
    if (BytesNeeded > Capacity) {
      PipeResizeBuffer (Pipe, GetNextPowerOfTwo32((UINT32)ElementsNeeded + 1) * ElementSize, &Snapshot);
    }
    ReleaseSpinLock (&Pipe->BeginLock);
  }

  return Snapshot;
}

inline
UINT8*
PipeAdjustBufferWrapping (
  UINT8     *Buffer,
  UINT8     *Pointer,
  UINT8     *BufferEnd
  )
{
  UINTN     Delta;

  if (Pointer >= BufferEnd) {
    Delta = Pointer - BufferEnd;
    return Buffer + Delta;
  } else {
    return Pointer;
  }
}

inline
UINT8*
PipeReverseAdjustBufferWrapping (
  UINT8     *Buffer,
  UINT8     *Pointer,
  UINT8     *BufferEnd
  )
{
  UINTN     Delta;

  if (Pointer < Buffer) {
    Delta = Buffer - Pointer;
    return BufferEnd - Delta;
  } else {
    return Pointer;
  }
}

EFI_STATUS
PipeNew (
  UINT32  ElementSize,
  UINT32  Limit,
  PIPE    **Pipe
  )
{
  PIPE    *IPipe;
  UINT32  Capacity;
  UINT8   *Buffer;
  UINT32  LimitInBytes;

  if (ElementSize == 0 || Pipe == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IPipe = AllocateZeroPool (sizeof(PIPE));

  if (IPipe == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Capacity  = PIPE_MIN_CAPACITY * ElementSize;
  Buffer    = AllocateZeroPool (Capacity);

  if (Buffer == NULL) {
    FreePool (Buffer);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Calculate limit in bytes. Include the sentinel element.
  //
  LimitInBytes  = (Limit + 1) * ElementSize;

  //
  // Initialize pipe fields
  // TODO: Add signature
  //
  IPipe->ElementSize      = ElementSize;
  IPipe->MinCap           = Capacity;
  IPipe->MaxCap           = Limit ? GetNextPowerOfTwo32 (MAX (LimitInBytes, Capacity)) : ~(UINTN) 0;
  IPipe->Buffer           = Buffer;
  IPipe->BufferEnd        = Buffer + Capacity;
  IPipe->Begin            = Buffer;
  IPipe->End              = Buffer + ElementSize;
  IPipe->ProducerRefCount = 1;
  IPipe->ConsumerRefCount = 1;

  DEBUG ((EFI_D_INFO, "[Pipe] Max cap: %lX\n", IPipe->MaxCap));

  InitializeSpinLock (&IPipe->BeginLock);
  InitializeSpinLock (&IPipe->EndLock);
  DEBUG ((EFI_D_INFO, "[Pipe] Begin lock: %lX\n", &IPipe->BeginLock));
  DEBUG ((EFI_D_INFO, "[Pipe] End lock: %lX\n", &IPipe->EndLock));
  ConditionInit (&IPipe->JustPushed);
  ConditionInit (&IPipe->JustPopped);

  //
  // TODO: check_invariants(Pipe);
  //
  *Pipe = IPipe;
  return EFI_SUCCESS;
}

EFI_STATUS
PipeProducerNew (
  PIPE            *Pipe,
  PIPE_PRODUCER   **Producer
  )
{
  if (Pipe == NULL || Producer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // TODO: Check pipe signature
  //
  AcquireSpinLock (&Pipe->BeginLock);
  Pipe->ProducerRefCount++;
  ReleaseSpinLock (&Pipe->BeginLock);
  *Producer = (PIPE_PRODUCER*)Pipe;
  return EFI_SUCCESS;
}

EFI_STATUS
PipeConsumerNew (
  PIPE            *Pipe,
  PIPE_CONSUMER   **Consumer
  )
{
  if (Pipe == NULL || Consumer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // TODO: Check pipe signature
  //
  AcquireSpinLock (&Pipe->EndLock);
  Pipe->ConsumerRefCount++;
  ReleaseSpinLock (&Pipe->EndLock);
  *Consumer = (PIPE_CONSUMER*)Pipe;
  return EFI_SUCCESS;
}

EFI_STATUS
PipeDeallocate (
  PIPE            *Pipe
  )
{
  if (Pipe == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // TODO: Check pipe signature
  //
  if (Pipe->Buffer != NULL) {
    FreePool (Pipe->Buffer);
  }

  FreePool (Pipe);
  return EFI_SUCCESS;
}

EFI_STATUS
PipeFree (
  PIPE            *Pipe
  )
{
  UINTN   NewProducerRefCount;
  UINTN   NewConsumerRefCount;

  if (Pipe == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // TODO: Check pipe signature
  //

  AcquireSpinLock (&Pipe->BeginLock);
  NewProducerRefCount = --Pipe->ProducerRefCount;
  ReleaseSpinLock (&Pipe->BeginLock);

  AcquireSpinLock (&Pipe->EndLock);
  NewConsumerRefCount = --Pipe->ConsumerRefCount;
  ReleaseSpinLock (&Pipe->EndLock);

  if (NewConsumerRefCount == 0) {
    FreePool (Pipe->Buffer);
    Pipe->Buffer = NULL;

    if (NewProducerRefCount > 0) {
      ConditionBroadcast (&Pipe->JustPopped);
    } else {
      PipeDeallocate (Pipe);
    }
  } else if (NewProducerRefCount == 0) {
    ConditionBroadcast (&Pipe->JustPushed);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PipeProducerFree (
  PIPE_PRODUCER     *Producer
  )
{
  PIPE    *Pipe;
  UINTN   NewProducerRefCount;

  if (Producer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Pipe = CONVERT_TO_PIPE (Producer);

  //
  // TODO: Check the pipe signature
  //

  AcquireSpinLock (&Pipe->BeginLock);
  NewProducerRefCount = --Pipe->ProducerRefCount;
  ReleaseSpinLock (&Pipe->BeginLock);

  if (NewProducerRefCount == 0) {
    UINTN   ConsumerRefCount;

    AcquireSpinLock (&Pipe->EndLock);
    ConsumerRefCount = Pipe->ConsumerRefCount;
    ReleaseSpinLock (&Pipe->EndLock);

    if (ConsumerRefCount > 0) {
      ConditionBroadcast (&Pipe->JustPushed);
    } else {
      PipeDeallocate (Pipe);
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PipeConsumerFree (
  PIPE_CONSUMER     *Consumer
  )
{
  PIPE      *Pipe;
  UINTN     NewConsumerRefCount;

  if (Consumer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Pipe = CONVERT_TO_PIPE (Consumer);

  //
  // TODO: Check the pipe signature
  //

  AcquireSpinLock (&Pipe->EndLock);
  NewConsumerRefCount = --Pipe->ConsumerRefCount;
  ReleaseSpinLock (&Pipe->EndLock);

  if (NewConsumerRefCount == 0) {
    UINTN     ProducerRefCount;

    AcquireSpinLock (&Pipe->BeginLock);
    ProducerRefCount = Pipe->ProducerRefCount;
    ReleaseSpinLock (&Pipe->BeginLock);

    if (ProducerRefCount > 0) {
      ConditionBroadcast (&Pipe->JustPopped);
    } else {
      PipeDeallocate (Pipe);
    }
  }

  return EFI_SUCCESS;
}

inline
UINT8*
ProcessPush (
  SNAPSHOT    Snapshot,
  VOID        *Elements,
  UINTN       BytesToCopy
  )
{
  UINTN     AtEnd;

  if (!DoesSnapshotWrapAround (Snapshot)) {
    AtEnd = MIN (BytesToCopy, (UINTN)(Snapshot.BufferEnd - Snapshot.End));
    Snapshot.End = OffsetCopyMem (Snapshot.End, Elements, AtEnd);
    Elements = (UINT8*)Elements + AtEnd;
    BytesToCopy -= AtEnd;
  }

  if (BytesToCopy) {
    Snapshot.End = PipeAdjustBufferWrapping (Snapshot.Buffer, Snapshot.End, Snapshot.BufferEnd);
    Snapshot.End = OffsetCopyMem (Snapshot.End, Elements, BytesToCopy);
  }

  Snapshot.End = PipeAdjustBufferWrapping (Snapshot.Buffer, Snapshot.End, Snapshot.Buffer);

  return Snapshot.End;
}


EFI_STATUS
__PipePush (
  PIPE      *Pipe,
  VOID      *Elements,
  UINTN     Count
  )
{
  UINTN     ElementSize;
  UINTN     Pushed;
  UINTN     MaxCap;
  SNAPSHOT  Snapshot;
  UINTN     BytesRemaining;

  if (Pipe == NULL || Elements == NULL || Count == 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // TODO: Check pipe signature
  //

  ElementSize = __PipeGetElementSize (Pipe);
  Pushed      = 0;

  AcquireSpinLock (&Pipe->EndLock);
  Snapshot = PipeWaitForRoom (Pipe, &MaxCap);
  if (Pipe->ConsumerRefCount == 0) {
    ReleaseSpinLock (&Pipe->EndLock);
    return EFI_SUCCESS;
  }

  Snapshot  = PipeValidateSize (Pipe, Snapshot, Count);
  Pushed    = MIN (Count, MaxCap - SnapshotBytesInUse (Snapshot));
  Pipe->End = ProcessPush (Snapshot, Elements, Pushed);
  ReleaseSpinLock (&Pipe->EndLock);

  if (Pushed == ElementSize) {
    ConditionSignal (&Pipe->JustPushed);
  } else {
    ConditionBroadcast (&Pipe->JustPushed);
  }

  BytesRemaining = Count - Pushed;

  if (BytesRemaining) {
    return __PipePush (Pipe, (UINT8*)Elements + Pushed, BytesRemaining);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PipePush (
  PIPE_PRODUCER   *Producer,
  VOID            *Elements,
  UINTN           Count
  )
{
  PIPE      *Pipe;

  if (Producer == NULL || Elements == NULL || Count == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Pipe = CONVERT_TO_PIPE (Producer);

  Count *= __PipeGetElementSize (Pipe);
  return __PipePush (Pipe, Elements, Count);
}

inline
VOID
TrimBuffer (
  PIPE      *Pipe,
  SNAPSHOT  Snapshot
  )
{
  UINTN     Capacity;
  SNAPSHOT  TempSnapshot;

  ASSERT (Pipe != NULL);

  //
  // TODO: Check pipe signature
  //

  Capacity = SnapshotCapacity (Snapshot);

  if (SnapshotBytesInUse (Snapshot) > Capacity / 4) {
    ReleaseSpinLock (&Pipe->BeginLock);
    return;
  }

  ReleaseSpinLock (&Pipe->BeginLock);
  AcquireSpinLock (&Pipe->EndLock);
  AcquireSpinLock (&Pipe->BeginLock);

  Snapshot = SnapshotMake (Pipe);
  Capacity = SnapshotCapacity (Snapshot);

  if (SnapshotBytesInUse (Snapshot) <= Capacity / 4) {
    //
    // TODO: OUT_OF_RESOURCES can be present here.
    //
    PipeResizeBuffer (Pipe, Capacity / 2, &TempSnapshot);
  }

  ReleaseSpinLock (&Pipe->BeginLock);
  ReleaseSpinLock (&Pipe->EndLock);
}

inline
SNAPSHOT
PipePopWithoutLocking (
  SNAPSHOT      Snapshot,
  VOID          *Target,
  UINTN         BytesToCopy,
  UINT8         **Begin
  )
{
  UINTN ElementSize;
  UINTN FirstBytesToCopy;

  ElementSize       = Snapshot.ElementSize;
  FirstBytesToCopy  = MIN (BytesToCopy, (UINTN)(Snapshot.BufferEnd - Snapshot.Begin - ElementSize));
  Target            = OffsetCopyMem (Target, Snapshot.Begin + ElementSize, FirstBytesToCopy);
  BytesToCopy       -= FirstBytesToCopy;
  Snapshot.Begin    += FirstBytesToCopy;
  Snapshot.Begin    = PipeAdjustBufferWrapping (Snapshot.Buffer, Snapshot.Begin, Snapshot.BufferEnd);

  if (BytesToCopy > 0) {
    Snapshot.Begin  += ElementSize;
    Snapshot.Begin  = PipeAdjustBufferWrapping (Snapshot.Buffer, Snapshot.Begin, Snapshot.BufferEnd);
    CopyMem (Target, Snapshot.Begin, BytesToCopy);
    Snapshot.Begin  += BytesToCopy;
    Snapshot.Begin  = PipeAdjustBufferWrapping (Snapshot.Buffer, Snapshot.Begin, Snapshot.BufferEnd);
    Snapshot.Begin  -= ElementSize;
    Snapshot.Begin  = PipeReverseAdjustBufferWrapping (Snapshot.Buffer, Snapshot.Begin, Snapshot.BufferEnd);
  }

  *Begin = Snapshot.Begin;

  return Snapshot;
}

inline
SNAPSHOT
PipeWaitForElements (
  PIPE      *Pipe,
  BOOLEAN   Loop
  )
{
  SNAPSHOT  Snapshot;
  UINTN     BytesUsed;

  ASSERT (Pipe != NULL);
  //
  // TODO: Check pipe signature
  //

  Snapshot  = SnapshotMake (Pipe);
  BytesUsed = SnapshotBytesInUse (Snapshot);

  if (Loop) {
    for (; (BytesUsed == 0) && (Pipe->ProducerRefCount > 0);
      Snapshot  = SnapshotMake(Pipe),
      BytesUsed = SnapshotBytesInUse(Snapshot)) {
      ConditionWait (&Pipe->JustPushed, &Pipe->BeginLock);
    }
  }

  return Snapshot;
}

inline
UINTN
__PipePop (
  PIPE      *Pipe,
  VOID      *Target,
  UINTN     Requested,
  BOOLEAN   ShouldWait
  )
{
  UINTN     Popped;
  SNAPSHOT  Snapshot;
  UINTN     BytesUsed;

  ASSERT (Pipe != NULL);
  ASSERT (Target != NULL);

  //
  // TODO: Check pipe signature
  //

  if (Requested == 0) {
    return 0;
  }

  Popped = 0;
  AcquireSpinLock (&Pipe->BeginLock);
  Snapshot = PipeWaitForElements (Pipe, ShouldWait);
  BytesUsed = SnapshotBytesInUse (Snapshot);

  if (BytesUsed == 0) {
    ReleaseSpinLock (&Pipe->BeginLock);
    return 0;
  }

  // check_invariants(Pipe);
  Popped    = MIN (Requested, BytesUsed);
  Snapshot  = PipePopWithoutLocking (Snapshot, Target, Popped, &Pipe->Begin);
  // check_invariants(Pipe);
  TrimBuffer (Pipe, Snapshot);  // Unlocks Pipe->BeginLock

  if (Popped == __PipeGetElementSize (Pipe)) {
    ConditionSignal (&Pipe->JustPopped);
  }
  else {
    ConditionBroadcast (&Pipe->JustPopped);
  }
  return Popped;
}

EFI_STATUS
PipePop (
  PIPE_CONSUMER   *Consumer,
  VOID            *Target,
  UINTN           *Count
  )
{
  UINTN     ElementSize;
  UINTN     BytesLeft;
  UINTN     BytesPopped;
  UINTN     Result;

  if (Consumer == NULL || Target == NULL || Count == NULL) {
    *Count = 0;
    return EFI_INVALID_PARAMETER;
  }

  ElementSize = __PipeGetElementSize (CONVERT_TO_PIPE(Consumer));
  Result      = (UINTN)-1;
  BytesPopped = 0;
  BytesLeft   = *Count * ElementSize;

  do {
    Result      = __PipePop (CONVERT_TO_PIPE (Consumer), Target, BytesLeft, TRUE);
    Target      = (VOID*)((UINT8*)Target + Result);
    BytesPopped += Result;
    BytesLeft   -= Result;
  } while (Result != 0 && BytesLeft);

  *Count = BytesPopped / ElementSize;
  return EFI_SUCCESS;
}

EFI_STATUS
PipePopEager (
  PIPE_CONSUMER   *Consumer,
  VOID            *Target,
  UINTN           *Count,
  BOOLEAN         ShouldWait
  )
{
  UINTN     ElementSize;
  UINTN     Result;

  if (Consumer == NULL || Target == NULL || Count == NULL) {
    *Count = 0;
    return EFI_INVALID_PARAMETER;
  }

  ElementSize = __PipeGetElementSize (CONVERT_TO_PIPE(Consumer));
  Result      = __PipePop (CONVERT_TO_PIPE (Consumer), Target, *Count * ElementSize, ShouldWait);

  *Count = Result / ElementSize;
  return EFI_SUCCESS;
}
