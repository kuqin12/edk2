/*
 * MpListInternals.h
 *
 *  Created on: Dec 12, 2017
 *      Author: mrabeda
 */

#ifndef MDEPKG_LIBRARY_BASETHREADSAFESTRUCTLIB_MPLISTINTERNALS_H_
#define MDEPKG_LIBRARY_BASETHREADSAFESTRUCTLIB_MPLISTINTERNALS_H_

//
// Add entry to the back of the list
//
VOID
MpListEntryPushBackI (
  MP_LIST           *List,
  MP_LIST_ENTRY     *Entry,
  BOOLEAN           ShouldLock
  );

//
// Add entry to the front of the list
//
VOID
MpListEntryPushFrontI (
  MP_LIST           *List,
  MP_LIST_ENTRY     *Entry,
  BOOLEAN           ShouldLock
  );

//
// Remove entry from the back of the list
//
MP_LIST_ENTRY*
MpListEntryPopBackI (
  MP_LIST           *List,
  BOOLEAN           ShouldLock
  );

//
// Remove entry from the front of the list
//
MP_LIST_ENTRY*
MpListEntryPopFrontI (
  MP_LIST           *List,
  BOOLEAN           ShouldLock
  );

//
// Add element to the back of the list
//
EFI_STATUS
MpListPushBackI (
  MP_LIST           *List,
  VOID              *Data,
  BOOLEAN           ShouldLock
  );

//
// Add element to the front of the list
//
EFI_STATUS
MpListPushFrontI (
  MP_LIST           *List,
  VOID              *Data,
  BOOLEAN           ShouldLock
  );

//
// Obtain data from the front of the list
//
VOID*
MpListPopFrontI (
  MP_LIST           *List,
  BOOLEAN           ShouldLock
  );

//
// Obtain data from the back of the list
//
VOID*
MpListPopBackI (
  MP_LIST           *List,
  BOOLEAN           ShouldLock
  );

//
// Get last entry of the list
//
MP_LIST_ENTRY*
MpListGetBackI (
  MP_LIST           *List,
  BOOLEAN           ShouldLock
  );

//
// Get first entry of the list
//
MP_LIST_ENTRY*
MpListGetFrontI (
  MP_LIST           *List,
  BOOLEAN           ShouldLock
  );

//
// Removal of single raw entry
// Entry must have a valid pointer to parent list
// ShouldLock can be set if spinlock acquiring is required
//
EFI_STATUS
MpListRemoveI (
  MP_LIST_ENTRY     *Entry,
  BOOLEAN           ShouldLock
  );

//
// Call function for each entry of the list
//
EFI_STATUS
MpListIterateI (
  MP_LIST                     *List,
  MP_LIST_ITERATE_FUNCTION    IterateFunction,
  VOID                        *Arg,
  BOOLEAN                     ShouldLock
  );

#endif /* MDEPKG_LIBRARY_BASETHREADSAFESTRUCTLIB_MPLISTINTERNALS_H_ */
