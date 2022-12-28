/*
 * MpListCachedInternals.c
 *
 *  Created on: Dec 12, 2017
 *      Author: mrabeda
 */

#include "MpCommon.h"

//
// Add data element to the end of the list
//
EFI_STATUS
MpListCachedPushBackI (
  MP_LIST_CACHED    *List,
  VOID              *Data,
  BOOLEAN           ShouldLock
  )
{
  MP_LIST_ENTRY     *Entry = NULL;

  ASSERT (List != NULL);
  if (Data == NULL) {
    DEBUG ((EFI_D_INFO, "Null data to be pushed to cached list: %lX\n", List));
    ASSERT (Data != NULL);
  }

  Entry = MpListCachedEntryAllocate (List);

  if (Entry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Entry->Data = Data;
  MpListEntryPushBackI (&List->InList, Entry, ShouldLock);

  return EFI_SUCCESS;
}

//
// Add data element to the end of the list
//
EFI_STATUS
MpListCachedPushFrontI (
  MP_LIST_CACHED    *List,
  VOID              *Data,
  BOOLEAN           ShouldLock
  )
{
  MP_LIST_ENTRY     *Entry = NULL;

  ASSERT (List != NULL);
  if (Data == NULL) {
    DEBUG ((EFI_D_INFO, "Null data to be pushed to cached list: %lX\n", List));
    ASSERT (Data != NULL);
  }

  Entry = MpListCachedEntryAllocate (List);

  if (Entry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Entry->Data = Data;
  MpListEntryPushFrontI (&List->InList, Entry, ShouldLock);

  return EFI_SUCCESS;
}

//
// Obtain data element from the front of the list
//
VOID*
MpListCachedPopFrontI (
  MP_LIST_CACHED    *List,
  BOOLEAN           ShouldLock
  )
{
  MP_LIST_ENTRY *Entry  = NULL;
  VOID          *Result = NULL;

  ASSERT (List != NULL);

  Entry = MpListEntryPopFrontI (&List->InList, ShouldLock);

  if (Entry) {
    Result = Entry->Data;
    MpListCachedEntryFreeI (List, Entry, ShouldLock);
  }

  return Result;
}

VOID
MpListCachedEntryFreeI (
  MP_LIST_CACHED  *List,
  MP_LIST_ENTRY   *Entry,
  BOOLEAN         ShouldLock
  )
{
  ASSERT (List != NULL);
  ASSERT (Entry != NULL);

  ZeroMem (Entry, sizeof (MP_LIST_ENTRY));
  if (ShouldLock) {
    MpListLock (&List->Cache);
  }

  MpListEntryPushBackI (&List->Cache, Entry, FALSE);

  if (ShouldLock) {
    MpListUnlock (&List->Cache);
  }
}
