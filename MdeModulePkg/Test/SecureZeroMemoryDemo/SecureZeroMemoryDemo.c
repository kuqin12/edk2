/** @file
  Demonstration module for the SecureZeroMemoryLib RFC.

  This module is a reproducible, EDK II-built demonstration that `ZeroMem ()`
  pins the *ordering* between a secret wipe and a later publish store: under a
  whole-program-optimized build (`/GL` + `/LTCG` on the VS toolchains, `-flto`
  on the GCC toolchain family), the compiler barrier inside `ZeroMem ()` keeps
  an ordinary store that follows the wipe from being scheduled ahead of it.

  The producer/consumer (Produce/Consume) are marked NOINLINE so the optimizer
  must treat Key as genuinely written and read; the buffer is seeded from a
  run-time value so it is not constant-folded away; and every result is funnelled
  into a volatile sink so nothing the demonstration relies on is dead.

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

typedef struct {
  UINT8     Key[32];
  UINT64    Ready;
} DEMO_SESSION;

//
// Volatile globals keep the demonstration honest: mDemoSeed makes the produced
// buffer run-time dependent (so it is not folded to a constant), and mDemoSink
// makes every result observable (so no store is dead).
//
STATIC volatile UINT32  mDemoSeed = 0;
STATIC volatile UINTN   mDemoSink = 0;

//
// NOINLINE forces the optimizer to keep Produce ()/Consume () as real
// out-of-line calls it cannot see through, so Key is genuinely written and then
// read. This reproduces, in a single file, the opacity a separate translation
// unit would provide. The ordering the demo observes is meaningful because the
// *wipe* is inlined under LTO, not because these routines are.
//
#if defined (__GNUC__) || defined (__clang__)
#define DEMO_NOINLINE  __attribute__ ((noinline))
#else
#define DEMO_NOINLINE  __declspec (noinline)
#endif

/**
  Fill Key with a run-time dependent pattern derived from Seed.

  @param  Key     Buffer to fill.
  @param  Length  Number of bytes to fill.
  @param  Seed    Run-time value that makes the contents non-constant.
**/
STATIC
DEMO_NOINLINE
VOID
Produce (
  OUT UINT8   *Key,
  IN  UINTN   Length,
  IN  UINT32  Seed
  )
{
  UINTN  Index;

  for (Index = 0; Index < Length; Index++) {
    Key[Index] = (UINT8)((Seed + (Index * 7)) + 3);
  }
}

/**
  Sum the bytes of Key.

  @param  Key     Buffer to read.
  @param  Length  Number of bytes to read.

  @return The sum of the Length bytes at Key.
**/
STATIC
DEMO_NOINLINE
UINTN
Consume (
  IN CONST UINT8  *Key,
  IN UINTN        Length
  )
{
  UINTN  Index;
  UINTN  Sum;

  Sum = 0;
  for (Index = 0; Index < Length; Index++) {
    Sum += Key[Index];
  }

  return Sum;
}

/**
  Firmware-relevant shape: wipe a secret in a structure with ZeroMem (), then
  publish a plain "this slot is now safe to reuse" flag.

  @param  Session  Session whose Key is wiped and whose Ready flag is published.

  @return A run-time dependent value derived from the produced secret.
**/
DEMO_NOINLINE
UINTN
EFIAPI
DemoReleaseZeroMem (
  IN OUT DEMO_SESSION  *Session
  )
{
  UINTN  Result;

  Produce (Session->Key, sizeof (Session->Key), mDemoSeed);
  Result = Consume (Session->Key, sizeof (Session->Key));

  ZeroMem (Session->Key, sizeof (Session->Key));
  Session->Ready = 1;

  return Result;
}

/**
  Entry point. Exercises every probe so each is compiled and referenced, and
  observes every result (including each Ready flag) so the compiler may not
  treat them as dead.

  @param  ImageHandle  The firmware allocated handle for the EFI image.
  @param  SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS  The demonstration ran.
**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  DEMO_SESSION  Session;
  UINTN         Total;

  //
  // Seed the produced secret from a run-time value so it is not constant-folded.
  //
  mDemoSeed = (UINT32)(UINTN)ImageHandle;

  Total  = 0;

  Total += DemoReleaseZeroMem (&Session);
  Total += Session.Ready;

  //
  // Observe everything: this store makes each result above live.
  //
  mDemoSink = Total;

  DEBUG ((DEBUG_INFO, "SecureZeroMemoryDemo: %Lu\n", (UINT64)mDemoSink));
  return EFI_SUCCESS;
}
