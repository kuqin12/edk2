/** @file
  DXE Core library services.

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"

//
// Lock Stuff
//

/**
  Initialize a basic mutual exclusion lock.   Each lock
  provides mutual exclusion access at it's task priority
  level.  Since there is no-premption (at any TPL) or
  multiprocessor support, acquiring the lock only consists
  of raising to the locks TPL.

  @param  Lock               The EFI_LOCK structure to initialize

  @retval EFI_SUCCESS        Lock Owned.
  @retval EFI_ACCESS_DENIED  Reentrant Lock Acquisition, Lock not Owned.

**/
EFI_STATUS
CoreAcquireLockOrFail (
  IN EFI_LOCK  *Lock
  )
{
  ASSERT (Lock != NULL);
  ASSERT (Lock->Lock != EfiLockUninitialized);

  if (Lock->Lock == EfiLockAcquired) {
    //
    // Lock is already owned, so bail out
    //
    return EFI_ACCESS_DENIED;
  }

  Lock->OwnerTpl = CoreRaiseTpl (Lock->Tpl);

  Lock->Lock = EfiLockAcquired;
  return EFI_SUCCESS;
}

/**
  Raising to the task priority level of the mutual exclusion
  lock, and then acquires ownership of the lock.

  @param  Lock               The lock to acquire

  @return Lock owned

**/
VOID
CoreAcquireLock (
  IN EFI_LOCK  *Lock
  )
{
  ASSERT (Lock != NULL);
  //ASSERT (Lock->Lock == EfiLockReleased);

  while (Lock->Lock == EfiLockAcquired) {
    CpuPause ();
  }

  Lock->OwnerTpl = CoreRaiseTpl (Lock->Tpl);
  Lock->Lock     = EfiLockAcquired;
}

/**
  Releases ownership of the mutual exclusion lock, and
  restores the previous task priority level.

  @param  Lock               The lock to release

  @return Lock unowned

**/
VOID
CoreReleaseLock (
  IN EFI_LOCK  *Lock
  )
{
  EFI_TPL  Tpl;

  ASSERT (Lock != NULL);
  //ASSERT (Lock->Lock == EfiLockAcquired);

  Tpl = Lock->OwnerTpl;

  Lock->Lock = EfiLockReleased;

  CoreRestoreTpl (Tpl);
}

VOID
CoreInitializeSpinLock (
  EFI_DEBUG_SPIN_LOCK     *DebugLock,
  EFI_TPL                 LockTpl
  )
{
  InitializeSpinLock (&DebugLock->Lock);
  DebugLock->Tpl = LockTpl;
  DebugLock->CpuId = 0xFFFFFFFF;
}

VOID
CoreAcquireSpinLock (
  IN EFI_DEBUG_SPIN_LOCK  *DebugLock,
  IN CONST CHAR8          *OwnerFile,
  IN UINTN                Line
  )
{
  UINT32      CpuId = 0;
  BOOLEAN     IsBsp = TRUE;
  EFI_TPL     OriginalTpl;

  if (gThreading != NULL) {
    gThreading->IdentifyCpu (&CpuId, &IsBsp);

    if (DebugLock->CpuId == CpuId) {
      //
      // Already acquired by this CPU
      //
      return;
    }
  }

  OriginalTpl = CoreRaiseTpl (TPL_HIGH_LEVEL);

  if (AcquireSpinLock (&DebugLock->Lock) == (SPIN_LOCK*)(UINTN)0xDEADBEEF) {
//    DEBUG ((EFI_D_ERROR, "--- Deadlock detected.---\nLock %lX is acquired by: [CPU %d] %a:%d\nWants to acquire: [CPU %d] %a:%d\n",
//            DebugLock, DebugLock->CpuId, DebugLock->OwnerFile, DebugLock->Line,
//            CpuId, OwnerFile, Line));

    DEBUG ((EFI_D_ERROR, "--- Deadlock detected ---\n"));
    DEBUG ((EFI_D_ERROR, "[DEADLOCK %X][ACQUIRED][CPU %d] %a:%d\n", DebugLock, DebugLock->CpuId, DebugLock->OwnerFile, DebugLock->Line));
    DEBUG ((EFI_D_ERROR, "[DEADLOCK %X][TRYING][CPU %d] %a:%d\n", DebugLock, CpuId, OwnerFile, Line));

    ASSERT (FALSE);
  }

  DebugLock->OwnerTpl = OriginalTpl;
  CoreRestoreTpl (DebugLock->Tpl);
  DebugLock->Line = Line;
  DebugLock->OwnerFile = OwnerFile;
  DebugLock->CpuId = CpuId;
//  DEBUG ((EFI_D_INFO, "Spinlock %lX acquired by CPU %d\n", DebugLock, CpuId));
}

VOID
CoreReleaseSpinLock (
  IN EFI_DEBUG_SPIN_LOCK  *DebugLock
  )
{
  UINT32      CpuId = 0;
  BOOLEAN     IsBsp = TRUE;
  EFI_TPL     Tpl;

  if (gThreading != NULL) {
    gThreading->IdentifyCpu (&CpuId, &IsBsp);

    if (DebugLock->CpuId != 0xFFFFFFFF && CpuId != DebugLock->CpuId) {
      DEBUG ((EFI_D_ERROR, "--- Deadlock detected ---. Lock %lX is acquired by: [CPU %d] %a:%d\nWants to release [CPU %d]\n",
          DebugLock, DebugLock->CpuId, DebugLock->OwnerFile, DebugLock->Line,
          CpuId));
      ASSERT (FALSE);
    }
  }

  DebugLock->CpuId = 0xFFFFFFFF;
  DebugLock->Line = 0;
  DebugLock->OwnerFile = "Nobody";
  Tpl = DebugLock->OwnerTpl;
  ReleaseSpinLock (&DebugLock->Lock);
  CoreRestoreTpl (Tpl);
//  DEBUG ((EFI_D_INFO, "Spinlock %lX released by CPU %d\n", DebugLock, CpuId));
}
