/** @file
  Global data used in memory service

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"

//
// MemoryLock - synchronizes access to the memory map and pool lists
//
EFI_DEBUG_SPIN_LOCK gMemoryLock;

//
// MemoryMap - the current memory map
//
LIST_ENTRY  gMemoryMap = INITIALIZE_LIST_HEAD_VARIABLE (gMemoryMap);
