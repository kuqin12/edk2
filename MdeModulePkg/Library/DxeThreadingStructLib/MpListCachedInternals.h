/*
 * MpListCachedInternals.h
 *
 *  Created on: Dec 12, 2017
 *      Author: mrabeda
 */

#ifndef MDEPKG_LIBRARY_BASETHREADSAFESTRUCTLIB_MPLISTCACHEDINTERNALS_H_
#define MDEPKG_LIBRARY_BASETHREADSAFESTRUCTLIB_MPLISTCACHEDINTERNALS_H_

//
// Add data element to the end of the list
//
EFI_STATUS
MpListCachedPushBackI (
  MP_LIST_CACHED    *List,
  VOID              *Data,
  BOOLEAN           ShouldLock
  );

//
// Add data element to the front of the list
//
EFI_STATUS
MpListCachedPushFrontI (
  MP_LIST_CACHED    *List,
  VOID              *Data,
  BOOLEAN           ShouldLock
  );

//
// Obtain data element from the front of the list
//
VOID*
MpListCachedPopFrontI (
  MP_LIST_CACHED    *List,
  BOOLEAN           ShouldLock
  );

VOID
MpListCachedEntryFreeI (
  MP_LIST_CACHED  *List,
  MP_LIST_ENTRY   *Entry,
  BOOLEAN         ShouldLock
  );

#endif /* MDEPKG_LIBRARY_BASETHREADSAFESTRUCTLIB_MPLISTCACHEDINTERNALS_H_ */
