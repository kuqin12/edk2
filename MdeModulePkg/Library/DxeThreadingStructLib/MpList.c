/*
 * MpList.c
 *
 *  Created on: Oct 6, 2017
 *      Author: mrabeda
 */

#include "MpCommon.h"

//
// Initialize MP_LIST internals
//
EFI_STATUS
MpListInitialize (
  MP_LIST           *List
  )
{
  ASSERT (List != NULL);

  List->Front = NULL;
  List->Back = NULL;
  List->Count = 0;
  InitializeSpinLock (&List->Lock);

  return EFI_SUCCESS;
}

//
// Lock the list
//
VOID
MpListLock (
  MP_LIST           *List
  )
{
  ASSERT (List != NULL);

  AcquireSpinLock (&List->Lock);
}

//
// Unlock the list
//
VOID
MpListUnlock (
  MP_LIST           *List
  )
{
  ASSERT (List != NULL);

  ReleaseSpinLock (&List->Lock);
}

//
// Allocating new raw entry
//
MP_LIST_ENTRY*
MpListEntryAllocate (
  )
{
  return AllocateZeroPool (sizeof (MP_LIST_ENTRY));
}

MP_LIST_ENTRY*
MpListEntryAllocateEx (
  UINTN       Count
  )
{
  return AllocateZeroPool (sizeof (MP_LIST_ENTRY) * Count);
}

//
// Freeing up the raw entry
//
VOID
MpListEntryFree (
  MP_LIST_ENTRY     *Entry
  )
{
  FreePool (Entry);
}

//
// Test whether list is empty
//
BOOLEAN
MpListIsEmpty (
  MP_LIST           *List
  )
{
  return (List->Count == 0);
}

//
// Lock the list and add element to the back of the list
//
VOID
MpListEntryPushBack (
  MP_LIST           *List,
  MP_LIST_ENTRY     *Entry
  )
{
  MpListEntryPushBackI (List, Entry, TRUE);
}

//
// Add element to the back of the list without locking the list
//
VOID
MpListEntryPushBackUnsafe (
  MP_LIST           *List,
  MP_LIST_ENTRY     *Entry
  )
{
  MpListEntryPushBackI (List, Entry, FALSE);
}

//
// Lock the list and add element to the front of the list
//
VOID
MpListEntryPushFront (
  MP_LIST           *List,
  MP_LIST_ENTRY     *Entry
  )
{
  MpListEntryPushFrontI (List, Entry, TRUE);
}

//
// Add element to the front of the list without locking the list
//
VOID
MpListEntryPushFrontUnsafe (
  MP_LIST           *List,
  MP_LIST_ENTRY     *Entry
  )
{
  MpListEntryPushFrontI (List, Entry, FALSE);
}

//
// Lock the list and remove last entry from the list
//
MP_LIST_ENTRY*
MpListEntryPopBack (
  MP_LIST           *List
  )
{
  return MpListEntryPopBackI (List, TRUE);
}

//
// Remove last entry from the list without locking the list
//
MP_LIST_ENTRY*
MpListEntryPopBackUnsafe (
  MP_LIST           *List
  )
{
  return MpListEntryPopBackI (List, FALSE);
}

//
// Lock the list and remove first entry from the list
//
MP_LIST_ENTRY*
MpListEntryPopFront (
  MP_LIST           *List
  )
{
  return MpListEntryPopFrontI (List, TRUE);
}

//
// Remove first entry from the list without locking the list
//
MP_LIST_ENTRY*
MpListEntryPopFrontUnsafe (
  MP_LIST           *List
  )
{
  return MpListEntryPopFrontI (List, FALSE);
}

//
// Lock the list and add element to the back of the list
//
EFI_STATUS
MpListPushBack (
  MP_LIST           *List,
  VOID              *Data
  )
{
  return MpListPushBackI (List, Data, TRUE);
}

//
// Add element to the back of the list without locking the list
//
EFI_STATUS
MpListPushBackUnsafe (
  MP_LIST           *List,
  VOID              *Data
  )
{
  return MpListPushBackI (List, Data, FALSE);
}

//
// Lock the list and add element to the front of the list
//
EFI_STATUS
MpListPushFront (
  MP_LIST           *List,
  VOID              *Data
  )
{
  return MpListPushFrontI (List, Data, TRUE);
}

//
// Add element to the front of the list without locking the list
//
EFI_STATUS
MpListPushFrontUnsafe (
  MP_LIST           *List,
  VOID              *Data
  )
{
  return MpListPushFrontI (List, Data, FALSE);
}

//
// Lock the list and obtain the element from the back of the list
//
VOID*
MpListPopBack (
  MP_LIST           *List
  )
{
  return MpListPopBackI (List, TRUE);
}

//
// Obtain the element from the back of the list without locking the list
//
VOID*
MpListPopBackUnsafe (
  MP_LIST           *List
  )
{
  return MpListPopBackI (List, FALSE);
}

//
// Lock the list and obtain the element from the front of the list
//
VOID*
MpListPopFront (
  MP_LIST           *List
  )
{
  return MpListPopFrontI (List, TRUE);
}

//
// Obtain the element from the front of the list without locking the list
//
VOID*
MpListPopFrontUnsafe (
  MP_LIST           *List
  )
{
  return MpListPopFrontI (List, FALSE);
}

//
// Lock the list and get the last element of the list
//
MP_LIST_ENTRY*
MpListGetBack (
  MP_LIST           *List
  )
{
  return MpListGetBackI (List, TRUE);
}

//
// Get the last element of the list without locking the list
//
MP_LIST_ENTRY*
MpListGetBackUnsafe (
  MP_LIST           *List
  )
{
  return MpListGetBackI (List, FALSE);
}

//
// Lock the list and get the first element of the list
//
MP_LIST_ENTRY*
MpListGetFront (
  MP_LIST           *List
  )
{
  return MpListGetFrontI (List, TRUE);
}

//
// Get the first element of the list without locking the list
//
MP_LIST_ENTRY*
MpListGetFrontUnsafe (
  MP_LIST           *List
  )
{
  return MpListGetFrontI (List, FALSE);
}

//
// Remove the element locking the list
//
EFI_STATUS
MpListRemove (
  MP_LIST_ENTRY     *Entry
  )
{
  return MpListRemoveI (Entry, TRUE);
}

//
// Remove the element without locking the list
//
EFI_STATUS
MpListRemoveUnsafe (
  MP_LIST_ENTRY     *Entry
  )
{
  return MpListRemoveI (Entry, FALSE);
}

//
// Call function for each entry of the list
//
EFI_STATUS
MpListIterate (
  MP_LIST                     *List,
  MP_LIST_ITERATE_FUNCTION    IterateFunction,
  VOID                        *Arg
  )
{
  return MpListIterateI (List, IterateFunction, Arg, TRUE);
}

EFI_STATUS
MpListIterateUnsafe (
  MP_LIST                     *List,
  MP_LIST_ITERATE_FUNCTION    IterateFunction,
  VOID                        *Arg
  )
{
  return MpListIterateI (List, IterateFunction, Arg, FALSE);
}

//
// Iteration function for clearing the list
//
EFI_STATUS
EFIAPI
__MpListClearIteration (
  MP_LIST_ENTRY   *Entry,
  VOID            *Arg
  )
{
  MpListRemoveUnsafe (Entry);

  ASSERT (Entry->Data != NULL);
  FreePool (Entry->Data);

  MpListEntryFree (Entry);

  return EFI_SUCCESS;
}

//
// Remove all data from the list
//
EFI_STATUS
MpListClear (
  MP_LIST     *List
  )
{
  return MpListIterate (List, __MpListClearIteration, NULL);
}
