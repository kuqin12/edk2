/*
 * MpListCached.c
 *
 *  Created on: Dec 12, 2017
 *      Author: mrabeda
 */

#include "MpCommon.h"

//
// Initialize MP_LIST_CACHED internals
//
UINTN
MpListCachedInitialize (
  MP_LIST_CACHED    *List,
  UINTN             InitCacheLen
  )
{
  UINTN           i;
  MP_LIST_ENTRY   *Entry;

  ASSERT (List != NULL);

  MpListInitialize (&List->InList);
  InitializeSpinLock (&List->InList.Lock);
  MpListInitialize (&List->Cache);
  MpListLock (&List->Cache);

//  Entry = MpListEntryAllocateEx (InitCacheLen);
//  ASSERT (Entry != NULL);

  for (i = 0; i < InitCacheLen; i++) {
    Entry = AllocateZeroPool (sizeof (MP_LIST_ENTRY));
    if (Entry == NULL) {
      break;
    }
    MpListEntryPushBackUnsafe (&List->Cache, Entry);
  }
  MpListUnlock (&List->Cache);

  return i;
}

//
// Lock the cached list
//
VOID
MpListCachedLock (
  MP_LIST_CACHED      *List
  )
{
  ASSERT (List != NULL);

  MpListLock (&List->InList);
}

//
// Unlock the cached list
//
VOID
MpListCachedUnlock (
  MP_LIST_CACHED      *List
  )
{
  ASSERT (List != NULL);

  MpListUnlock (&List->InList);
}

//
// Allocate entry within cached list
// Either from entry cache or memory pool
//
MP_LIST_ENTRY*
MpListCachedEntryAllocate (
  MP_LIST_CACHED    *List
  )
{
  MP_LIST_ENTRY   *Entry;

  ASSERT (List != NULL);
  Entry = NULL;

  if (!MpListIsEmpty (&List->Cache)) {
    MpListLock (&List->Cache);
    Entry = MpListEntryPopFrontUnsafe (&List->Cache);
    MpListUnlock (&List->Cache);
  }

  if (Entry == NULL) {
    Entry = MpListEntryAllocate ();
  }

  return Entry;
}

//
// Cache the entry
//
VOID
MpListCachedEntryFree (
  MP_LIST_CACHED  *List,
  MP_LIST_ENTRY   *Entry
  )
{
  MpListCachedEntryFreeI (List, Entry, TRUE);
}

//
// Test whether cached list is empty
//
BOOLEAN
MpListCachedIsEmpty (
  MP_LIST_CACHED    *List
  )
{
  return MpListIsEmpty (&List->InList);
}

//
// Lock the list and add the data element to the end of the list
//
EFI_STATUS
MpListCachedPushBack (
  MP_LIST_CACHED    *List,
  VOID              *Data
  )
{
  return MpListCachedPushBackI (List, Data, TRUE);
}

//
// Add the data element to the end of the list without locking the list
//
EFI_STATUS
MpListCachedPushBackUnsafe (
  MP_LIST_CACHED    *List,
  VOID              *Data
  )
{
  return MpListCachedPushBackI (List, Data, FALSE);
}

//
// Lock the list and add the data element to the front of the list
//
EFI_STATUS
MpListCachedPushFront (
  MP_LIST_CACHED    *List,
  VOID              *Data
  )
{
  return MpListCachedPushFrontI (List, Data, TRUE);
}

//
// Add the data element to the front of the list without locking the list
//
EFI_STATUS
MpListCachedPushFrontUnsafe (
  MP_LIST_CACHED    *List,
  VOID              *Data
  )
{
  return MpListCachedPushFrontI (List, Data, FALSE);
}

//
// Lock the list and obtain the data element from the front of the list
//
VOID*
MpListCachedPopFront (
  MP_LIST_CACHED    *List
  )
{
  return MpListCachedPopFrontI (List, TRUE);
}

//
// Obtain the data element from the front of the list without locking the list
//
VOID*
MpListCachedPopFrontUnsafe (
  MP_LIST_CACHED    *List
  )
{
  return MpListCachedPopFrontI (List, FALSE);
}

//
// Clean up the list (cache + data - optional freeing of data memory)
//
EFI_STATUS
MpListCachedFree (
  MP_LIST_CACHED            *List,
  MP_LIST_ITERATE_FUNCTION  Callback
  )
{
  VOID *Data;

  if (List == 0) {
    return EFI_INVALID_PARAMETER;
  }

  MpListCachedLock (List);

  //
  // Remove the data (free memory if necessary)
  //
  while (!MpListIsEmpty (&List->InList)) {
    Data = MpListCachedPopFrontUnsafe (List);

    if (Data == NULL) {
      DEBUG ((EFI_D_ERROR, "Unexpected cached list entry with data being NULL.\n"));
      ASSERT (FALSE);
    }

    if (Callback) {
      Callback (NULL, Data);
    }
  }

  //
  // Free cache entries
  //
  MpListLock (&List->Cache);
  while (!MpListIsEmpty (&List->Cache)) {
    Data = MpListEntryPopFrontUnsafe (&List->Cache);
    ASSERT (Data != NULL);
    FreePool (Data);
  }
  MpListUnlock (&List->Cache);

  MpListCachedUnlock (List);
  return EFI_SUCCESS;
}
