/*
 * ThreadingLib.h
 *
 *  Created on: Jul 26, 2017
 *      Author: mrabeda
 */

#ifndef MDEMODULEPKG_INCLUDE_LIBRARY_THREADINGLIB_H_
#define MDEMODULEPKG_INCLUDE_LIBRARY_THREADINGLIB_H_

#include <Protocol/Threading.h>

//#define THREAD_CHECKPOINT() RegisterCpuCheckpoint (__FILE__, __LINE__)
#define THREAD_CHECKPOINT()

EFI_STATUS
EFIAPI
IdentifyCpu (
  OUT UINT32    *CpuId,
  OUT BOOLEAN   *IsBsp
  );

EFI_STATUS
EFIAPI
SpawnThread (
  IN  EFI_THREADING_PROCEDURE  ThreadProcedure,
  IN  VOID                     *ThreadArgument,
  IN  EFI_THREADING_PROCEDURE  OnThreadExit,
  IN  VOID                     *OnThreadExitArgument,
  IN  UINTN                    ThreadTimeout,
  OUT EFI_THREAD               **ThreadObj
  );

EFI_STATUS
EFIAPI
WaitForThread (
  IN  EFI_THREAD      *Thread
  );

VOID
EFIAPI
CleanupThread (
  IN  EFI_THREAD      *Thread
  );

EFI_STATUS
EFIAPI
GetCpuCount (
  IN OUT  UINTN       *CpuCount,
  IN OUT  UINTN       *EnabledCpuCount
  );

//EFI_STATUS
//EFIAPI
//GetCpuCheckpoints (
//  IN      UINT32          CpuId,
//  IN OUT  CPU_CHECKPOINT  **Checkpoints,
//  IN OUT  UINTN           *Length
//  );
//
//VOID
//EFIAPI
//RegisterCpuCheckpoint (
//  IN  CHAR8*            File,
//  IN  UINTN             Line
//  );

EFI_STATUS
EFIAPI
AbortThread (
  IN  EFI_THREAD        *Thread
  );

#endif /* MDEMODULEPKG_INCLUDE_LIBRARY_THREADINGLIB_H_ */
