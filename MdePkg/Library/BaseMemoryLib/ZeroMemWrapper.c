/** @file
  ZeroMem() implementation.

  The following BaseMemoryLib instances contain the same copy of this file:

    BaseMemoryLib
    BaseMemoryLibMmx
    BaseMemoryLibSse2
    BaseMemoryLibRepStr
    BaseMemoryLibOptDxe
    BaseMemoryLibOptPei
    PeiMemoryLib
    UefiMemoryLib

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MemLibInternals.h"

#if defined (_MSC_VER)
void
_ReadWriteBarrier (
  void
  );

  #pragma intrinsic(_ReadWriteBarrier)
#endif

/**
  A helper function to emit a compiler barrier and treat Buffer as an input to
  discourage reordering and call-site elimination.

  @param  Buffer  Pointer to the buffer being zeroed.

**/
STATIC
VOID
SecureZeroMemoryBarrier (
  IN VOID  *Buffer
  )
{
 #if defined (_MSC_VER)
  _ReadWriteBarrier ();
  (VOID)Buffer;
 #elif defined (__GNUC__) || defined (__clang__)
  __asm__ __volatile__ ("" : : "r"(Buffer) : "memory");
 #else
  (VOID)Buffer;
 #endif
}

/**
  Internal worker function for SecureZeroMemory().

  The zeroing goes through volatile stores (so it is not elided) and is followed
  by a compiler "memory" barrier (so a later store cannot be scheduled ahead of
  the wipe). Both properties hold even when this function is inlined under LTO, so
  inlining is intentionally permitted.

  @param  Buffer  Pointer to the buffer to clear.
  @param  Length  Number of bytes to clear.

**/
STATIC
VOID
SecureZeroMemoryInternal (
  IN VOID   *Buffer,
  IN UINTN  Length
  )
{
  volatile UINT8  *Pointer;

  Pointer = (volatile UINT8 *)Buffer;
  while (Length-- != 0) {
    *Pointer++ = 0;
  }

  //
  // Compiler barrier + also treat Buffer as used.
  //
  SecureZeroMemoryBarrier (Buffer);
}

/**
  Securely zero a buffer.

  This function attempts to ensure the buffer is actually cleared and that the
  compiler does not optimize away the writes.

  @param  Buffer  Pointer to the buffer to clear.
  @param  Length  Number of bytes to clear.

  @return Buffer (same pointer passed in).
**/
VOID *
EFIAPI
ZeroMem (
  OUT VOID  *Buffer,
  IN UINTN  Length
  )
{
  if ((Buffer == NULL) || (Length == 0)) {
    return Buffer;
  }

  SecureZeroMemoryInternal (Buffer, Length);

  //
  // A second barrier to discourage call-site elimination.
  //
  SecureZeroMemoryBarrier (Buffer);

  return Buffer;
}
