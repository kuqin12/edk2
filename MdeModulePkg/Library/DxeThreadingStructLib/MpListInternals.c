/*
 * MpListInternals.c
 *
 *  Created on: Dec 12, 2017
 *      Author: mrabeda
 */

#include "MpCommon.h"

//
// Add entry to the back of the list
//
VOID
MpListEntryPushBackI (
  MP_LIST           *List,
  MP_LIST_ENTRY     *Entry,
  BOOLEAN           ShouldLock
  )
{
  ASSERT (List != NULL);
  ASSERT (Entry != NULL);
//  ASSERT (Entry->Data != NULL);

  if (ShouldLock) {
    MpListLock (List);
  }

  Entry->Next = NULL;
  Entry->Parent = List;
  Entry->Previous = List->Back;

  if (List->Back) {
    //
    // List not empty. Pushing back.
    //
    if (List->Count == 0) {
      DEBUG ((EFI_D_INFO, "LIST ASSERT: Something wrong with list %lX\nList->Front: %lX, List->Back: %lX, List->Count: %lX\n", List, List->Front, List->Back, List->Count));
      ASSERT (FALSE);
    }

    List->Back->Next = Entry;
    List->Back = Entry;
  }
  else {
    //
    // List empty. Adding first element
    //
    if (List->Count != 0) {
      DEBUG ((EFI_D_INFO, "LIST ASSERT: Something wrong with list %lX\nList->Front: %lX, List->Back: %lX, List->Count: %lX\n", List, List->Front, List->Back, List->Count));
      ASSERT (FALSE);
    }
    List->Front = List->Back = Entry;
  }

  List->Count++;

  if (ShouldLock) {
    MpListUnlock (List);
  }
}

//
// Add entry to the front of the list
//
VOID
MpListEntryPushFrontI (
  MP_LIST           *List,
  MP_LIST_ENTRY     *Entry,
  BOOLEAN           ShouldLock
  )
{
  ASSERT (List != NULL);
  ASSERT (Entry != NULL);
//  ASSERT (Entry->Data != NULL);

  if (ShouldLock) {
    MpListLock (List);
  }

  Entry->Previous = NULL;
  Entry->Parent = List;
  Entry->Next = List->Front;

  if (List->Front) {
    //
    // List not empty. Pushing front.
    //
    ASSERT (List->Count > 0);
    List->Front->Previous = Entry;
    List->Front = Entry;
  } else {
    //
    // List empty. Adding first element
    //
    if (List->Count != 0) {
      DEBUG ((EFI_D_INFO, "LIST ASSERT: Something wrong with list %lX\nList->Front: %lX, List->Back: %lX, List->Count: %lX\n", List, List->Front, List->Back, List->Count));
      ASSERT (FALSE);
    }
    List->Front = List->Back = Entry;
  }

  List->Count++;

  if (ShouldLock) {
    MpListUnlock (List);
  }
}

//
// Remove entry from a list it was attached to
//
MP_LIST_ENTRY*
MpListEntryPop (
  MP_LIST_ENTRY     *Entry
  )
{
  MP_LIST           *List;

  if (Entry != NULL) {
    List = Entry->Parent;

    ASSERT (List != NULL);
    ASSERT (List->Count > 0);

//    Entry = List->Back;

    if (Entry->Next) {
      Entry->Next->Previous = Entry->Previous;
    }

    if (Entry->Previous) {
      Entry->Previous->Next = Entry->Next;
    }

    if (Entry == List->Front) {
      List->Front = Entry->Next;
    }

    if (Entry == List->Back) {
      List->Back = Entry->Previous;
    }

    List->Count--;

    if (List->Count == 0) {
      if (List->Front != NULL || List->Back != NULL) {
        DEBUG ((EFI_D_INFO, "LIST Corruption: %lX\n", List));
        ASSERT (FALSE);
      }
    }
//    Entry->Parent = NULL;
  }

  return Entry;
}

//
// Remove entry from the back of the list
//
MP_LIST_ENTRY*
MpListEntryPopBackI (
  MP_LIST           *List,
  BOOLEAN           ShouldLock
  )
{
  MP_LIST_ENTRY     *Result;

  if (ShouldLock) {
    MpListLock (List);
  }

  Result = MpListEntryPop (List->Back);

  if (ShouldLock) {
    MpListUnlock (List);
  }

  return Result;
}

//
// Remove entry from the front of the list
//
MP_LIST_ENTRY*
MpListEntryPopFrontI (
  MP_LIST           *List,
  BOOLEAN           ShouldLock
  )
{
  MP_LIST_ENTRY     *Result;

  if (ShouldLock) {
    MpListLock (List);
  }

  Result = MpListEntryPop (List->Front);

  if (ShouldLock) {
    MpListUnlock (List);
  }

  return Result;
}

//
// Add element to the back of the list
//
EFI_STATUS
MpListPushBackI (
  MP_LIST           *List,
  VOID              *Data,
  BOOLEAN           ShouldLock
  )
{
  MP_LIST_ENTRY     *Entry;

  ASSERT (List != NULL);
  ASSERT (Data != NULL);

  Entry = MpListEntryAllocate ();

  if (Entry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Entry->Data = Data;
  MpListEntryPushBackI (List, Entry, ShouldLock);

  return EFI_SUCCESS;
}

//
// Add element to the front of the list
//
EFI_STATUS
MpListPushFrontI (
  MP_LIST           *List,
  VOID              *Data,
  BOOLEAN           ShouldLock
  )
{
  MP_LIST_ENTRY     *Entry;

  ASSERT (List != NULL);
  ASSERT (Data != NULL);

  Entry = MpListEntryAllocate ();

  if (Entry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Entry->Data = Data;
  MpListEntryPushFrontI (List, Entry, ShouldLock);

  return EFI_SUCCESS;
}

//
// Obtain data from the back of the list
//
VOID*
MpListPopBackI (
  MP_LIST           *List,
  BOOLEAN           ShouldLock
  )
{
  MP_LIST_ENTRY   *Entry  = NULL;
  VOID            *Result = NULL;

  ASSERT (List != NULL);

  Entry = MpListEntryPopBackI (List, ShouldLock);

  if (Entry) {
    Result = Entry->Data;
    MpListEntryFree (Entry);
  }

  return Result;
}

//
// Obtain data from the front of the list
//
VOID*
MpListPopFrontI (
  MP_LIST           *List,
  BOOLEAN           ShouldLock
  )
{
  MP_LIST_ENTRY *Entry  = NULL;
  VOID          *Result = NULL;

  ASSERT (List != NULL);

  Entry = MpListEntryPopFrontI (List, ShouldLock);

  if (Entry) {
    Result = Entry->Data;
    MpListEntryFree (Entry);
  }

  return Result;
}

//
// Get last entry of the list
//
MP_LIST_ENTRY*
MpListGetBackI (
  MP_LIST           *List,
  BOOLEAN           ShouldLock
  )
{
  MP_LIST_ENTRY *Entry  = NULL;

  if (ShouldLock) {
    MpListLock (List);
  }

  if (List->Count == 0) {
    ASSERT (List->Back == NULL);
  } else {
    ASSERT (List->Back != NULL);
  }

  if (List->Back) {
    Entry = List->Back;
  }

  if (ShouldLock) {
    MpListUnlock (List);
  }

  return Entry;
}

//
// Get first entry of the list
//
MP_LIST_ENTRY*
MpListGetFrontI (
  MP_LIST           *List,
  BOOLEAN           ShouldLock
  )
{
  MP_LIST_ENTRY *Entry  = NULL;

  if (ShouldLock) {
    MpListLock (List);
  }

  if (List->Count == 0) {
    ASSERT (List->Front == NULL);
  } else {
    ASSERT (List->Front != NULL);
  }

  if (List->Front) {
    Entry = List->Front;
  }

  if (ShouldLock) {
    MpListUnlock (List);
  }

  return Entry;
}

//
// Removal of single raw entry
// Entry must have a valid pointer to parent list
// ShouldLock can be set if spinlock acquiring is required
//
EFI_STATUS
MpListRemoveI (
  MP_LIST_ENTRY     *Entry,
  BOOLEAN           ShouldLock
  )
{
  ASSERT (Entry != NULL);
  ASSERT (Entry->Parent != NULL);

  if (ShouldLock) {
    MpListLock (Entry->Parent);
  }

  MpListEntryPop (Entry);

  if (ShouldLock) {
    MpListUnlock (Entry->Parent);
  }

  return EFI_SUCCESS;
}

//
// Call function for each entry of the list
//
EFI_STATUS
MpListIterateI (
  MP_LIST                     *List,
  MP_LIST_ITERATE_FUNCTION    IterateFunction,
  VOID                        *Arg,
  BOOLEAN                     ShouldLock
  )
{
  MP_LIST_ENTRY   *Entry;
  MP_LIST_ENTRY   *NextEntry = NULL;
  EFI_STATUS      Status = EFI_SUCCESS;

  if (ShouldLock) {
    MpListLock (List);
  }

  Entry = List->Front;

  if (Entry != NULL) {
    NextEntry = Entry->Next;
  }

  while (Entry != NULL) {
    Status = IterateFunction (Entry, Arg);

    if (EFI_ERROR (Status)) {
      break;
    }

    Entry = NextEntry;

    if (Entry != NULL) {
      NextEntry = Entry->Next;
    }
  }

  if (ShouldLock) {
    MpListUnlock (List);
  }

  return Status;
}
