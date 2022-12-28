/*
 * MpFairLock.c
 *
 *  Created on: Dec 5, 2018
 *      Author: mrabeda
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/ThreadingLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/DxeThreadingStructLib.h>

enum MP_FAIR_LOCK_FLAGS {
  CPU_MUST_WAIT = 0,
  CPU_HAS_LOCK = 1
};

UINTN     mCpuCount           = 0;
UINTN     mEnabledCpuCount    = 0;

EFI_STATUS
EFIAPI
InitLocalData (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // Obtain number of CPUs
  //
  return GetCpuCount (&mCpuCount, &mEnabledCpuCount);
}

EFI_STATUS
MpFairLockInit (
  MP_FAIR_LOCK    *Lock
  )
{
  UINTN     i;

  ASSERT (Lock != NULL);

  if (mCpuCount == 0 &&
      GetCpuCount (&mCpuCount, &mEnabledCpuCount) != EFI_SUCCESS) {
    return EFI_NOT_READY;
  }

  Lock->Flags = AllocateZeroPool (sizeof (UINT8) * mCpuCount);

  if (Lock->Flags == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Lock->States = AllocateZeroPool (sizeof (UINT8) * mCpuCount);

  if (Lock->States == NULL) {
    FreePool ((UINT8*)Lock->Flags);
    return EFI_OUT_OF_RESOURCES;
  }

  Lock->Flags[0]  = CPU_HAS_LOCK;
  Lock->States[0] = FALSE;

  for (i = 1; i < mCpuCount; i++) {
    Lock->Flags[i]  = CPU_MUST_WAIT;
    Lock->States[i] = FALSE;
  }

  Lock->Owner = (UINTN)-1;
  Lock->Last  = 0;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MpFairLockAcquire (
  IN  MP_FAIR_LOCK          *Lock
  )
{
  UINT32    MyPlace;
  UINT32    CpuId;
  BOOLEAN   IsBsp;

  ASSERT (Lock != NULL);

  if (mCpuCount == 0 || mEnabledCpuCount == 0) {
    return EFI_NOT_READY;
  }

  IdentifyCpu (&CpuId, &IsBsp);

  if (Lock->States[CpuId] == TRUE) {
    //
    // Already owned
    // Note: speed of this functionality bases on CPU identification method
    //
    return EFI_SUCCESS;
  }

  MyPlace = InterlockedIncrement (&Lock->Last);
  MyPlace = (MyPlace - 1) % mCpuCount;

//  DEBUG ((EFI_D_INFO, "Fair lock ACQ. CPU %d. Place: %d\n", CpuId, MyPlace));

  while (Lock->Flags[MyPlace] == CPU_MUST_WAIT);
  Lock->Owner = MyPlace;
  Lock->States[CpuId] = TRUE;

  return EFI_SUCCESS;
}

EFI_STATUS
MpFairLockRelease (
  IN  MP_FAIR_LOCK            *Lock
  )
{
  UINTN     Owner;
  UINTN     NewOwner;
  UINT32    CpuId;
  BOOLEAN   IsBsp;

  ASSERT (Lock != NULL);

  if (mCpuCount == 0 || mEnabledCpuCount == 0) {
    return EFI_NOT_READY;
  }

  Owner = Lock->Owner;
  ASSERT (Owner != (UINTN)-1);

  IdentifyCpu (&CpuId, &IsBsp);

  ASSERT (Lock->States[CpuId] == TRUE);
  NewOwner              = (Owner + 1) % mCpuCount;
  Lock->Flags[Owner]    = CPU_MUST_WAIT;
  Lock->Owner           = (UINTN)-1;
  Lock->States[CpuId]   = FALSE;
  Lock->Flags[NewOwner] = CPU_HAS_LOCK;

  return EFI_SUCCESS;

//  DEBUG ((EFI_D_INFO, "Fair lock RLS. CPU %d. Current owner: %d, New owner: %d\n", CpuId, Owner, NewOwner));
}

VOID
MpFairLockDestroy (
  MP_FAIR_LOCK    *Lock
  )
{
  FreePool((UINT8*)Lock->Flags);
  FreePool((UINT8*)Lock->States);
  Lock->Owner = (UINTN)-1;
  Lock->Last = 0;
}
