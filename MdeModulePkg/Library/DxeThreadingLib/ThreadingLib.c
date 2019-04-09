/*
 * ThreadingLib.c
 *
 *  Created on: Jul 26, 2017
 *      Author: mrabeda
 */

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Threading.h>

EFI_THREADING_PROTOCOL    *mThreading = NULL;

EFI_STATUS
LocateThreadingProtocol (
  )
{
  EFI_STATUS  Status;

  if (gBS == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = gBS->LocateProtocol (
                  &gEfiThreadingProtocolGuid,
                  NULL,
                  (VOID **) &mThreading
                  );

  return Status;
}

EFI_STATUS
EFIAPI
IdentifyCpu (
  OUT UINT32    *CpuId,
  OUT BOOLEAN   *IsBsp
  )
{
  EFI_STATUS    Status;

  if (mThreading == NULL) {
    Status = LocateThreadingProtocol ();
    if (Status != EFI_SUCCESS) {
      return EFI_NOT_READY;
    }
  }
  return mThreading->IdentifyCpu (CpuId, IsBsp);
}

EFI_STATUS
EFIAPI
SpawnThread (
  IN  EFI_THREADING_PROCEDURE  ThreadProcedure,
  IN  VOID                     *ThreadArgument,
  IN  EFI_THREADING_PROCEDURE  OnThreadExit,
  IN  VOID                     *OnThreadExitArgument,
  IN  UINTN                    ThreadTimeout,
  OUT EFI_THREAD               *ThreadObj
  )
{
  EFI_STATUS  Status;

  if (mThreading == NULL) {
    Status = LocateThreadingProtocol ();
    if (Status != EFI_SUCCESS) {
      return EFI_NOT_READY;
    }
  }

  return mThreading->SpawnThread (
                       ThreadProcedure,
                       ThreadArgument,
                       OnThreadExit,
                       OnThreadExitArgument,
                       ThreadTimeout,
                       ThreadObj
                       );
}

EFI_STATUS
EFIAPI
WaitForThread (
  IN  EFI_THREAD      *Thread
  )
{
  EFI_STATUS  Status;

  if (mThreading == NULL) {
    Status = LocateThreadingProtocol ();
    if (Status != EFI_SUCCESS) {
      return EFI_NOT_READY;
    }
  }

  return mThreading->WaitForThread (Thread);
}

VOID
EFIAPI
CleanupThread (
  IN  EFI_THREAD      *Thread
  )
{
  EFI_STATUS  Status;

  if (mThreading == NULL) {
    Status = LocateThreadingProtocol ();
    ASSERT_EFI_ERROR (Status);
  }

  mThreading->CleanupThread (Thread);
}

EFI_STATUS
EFIAPI
GetCpuCount (
  IN OUT  UINTN       *CpuCount,
  IN OUT  UINTN       *EnabledCpuCount
  )
{
  EFI_STATUS  Status;

  DEBUG ((EFI_D_INFO, "GetCpuCount\n"));

  if (mThreading == NULL) {
    DEBUG ((EFI_D_INFO, "GetCpuCount: mThreading == NULL\n"));
    Status = LocateThreadingProtocol ();
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_INFO, "GetCpuCount: Threading not located\n"));
      return EFI_NOT_READY;
    }
  }

  Status = mThreading->GetCpuCount (CpuCount, EnabledCpuCount);

  DEBUG ((EFI_D_INFO, "GetCpuCount: result = %r\n", Status));
  return Status;
}

//EFI_STATUS
//EFIAPI
//GetCpuCheckpoints (
//  IN      UINT32          CpuId,
//  IN OUT  CPU_CHECKPOINT  **Checkpoints,
//  IN OUT  UINTN           *Length
//  )
//{
//  return mThreading->GetCpuCheckpoints (CpuId, Checkpoints, Length);
//}

//VOID
//EFIAPI
//RegisterCpuCheckpoint (
//  IN  CHAR8*            File,
//  IN  UINTN             Line
//  )
//{
//  mThreading->RegisterCpuCheckpoint (File, Line);
//}

EFI_STATUS
EFIAPI
AbortThread (
  IN  EFI_THREAD        *Thread
  )
{
  EFI_STATUS  Status;

  if (mThreading == NULL) {
    Status = LocateThreadingProtocol ();
    if (Status != EFI_SUCCESS) {
      return EFI_NOT_READY;
    }
  }

  return mThreading->AbortThread (Thread);
}
