/*
 * MpReentryLock.c
 *
 *  Created on: Dec 4, 2018
 *      Author: mrabeda
 */

#include "MpCommon.h"

VOID
MpReentryLockInit (
  MP_REENTRY_LOCK   *Lock
  )
{
  InitializeSpinLock (&Lock->Lock);
  Lock->OwnerAP = -1;
}

VOID
MpReentryLockAcquire (
  MP_REENTRY_LOCK   *Lock
  )
{
  UINT32      CpuId;
  BOOLEAN     IsBsp;
  EFI_STATUS  Status;

  Status = IdentifyCpu (&CpuId, &IsBsp);

  if (Status == EFI_NOT_READY) {
    //
    // Threading protocol not yet installed. Only BSP is running.
    //
    CpuId = 0;
    IsBsp = TRUE;
  }

  if (Lock->OwnerAP != -1 && (UINT32)Lock->OwnerAP == CpuId) {
    return;
  }

  AcquireSpinLock (&Lock->Lock);
  Lock->OwnerAP = (INTN)CpuId;
}

VOID
MpReentryLockRelease (
  MP_REENTRY_LOCK   *Lock
  )
{
  UINT32      CpuId;
  BOOLEAN     IsBsp;
  EFI_STATUS  Status;

  Status = IdentifyCpu (&CpuId, &IsBsp);
  if (Status == EFI_NOT_READY) {
    //
    // Threading protocol not yet installed. Only BSP is running.
    //
    CpuId = 0;
    IsBsp = TRUE;
  }
  ASSERT (Lock->OwnerAP != -1);
  ASSERT ((UINT32)Lock->OwnerAP == CpuId);

  Lock->OwnerAP = -1;
  ReleaseSpinLock (&Lock->Lock);
}
