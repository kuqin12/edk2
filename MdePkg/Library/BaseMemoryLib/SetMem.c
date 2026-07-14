/** @file
  Implementation of the EfiSetMem routine. This function is broken
  out into its own source file so that it can be excluded from a
  build for a particular platform easily if an optimized version
  is desired.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2012 - 2013, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>

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
  Emit a compiler barrier that orders the preceding fill stores before any
  memory access that follows.

  The fill loops in InternalMemSetMem() use volatile stores so the writes are
  never elided.  However, volatile only orders volatile-versus-volatile
  accesses; a later plain store by the caller may still be scheduled ahead of
  the fill.  This barrier closes that gap: the "memory" clobber (or
  _ReadWriteBarrier() on MSVC) forbids the compiler from moving any subsequent
  memory operation before the fill.  It emits no instructions and holds even
  when InternalMemSetMem() is inlined under LTO, so it does not disturb the
  optimized fill.

  @param  Buffer  Pointer to the buffer that was just filled.

**/
STATIC
VOID
InternalMemBarrier (
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
  Set Buffer to Value for Size bytes.

  @param  Buffer   The memory to set.
  @param  Length   The number of bytes to set.
  @param  Value    The value of the set operation.

  @return Buffer

**/
VOID *
EFIAPI
InternalMemSetMem (
  OUT     VOID   *Buffer,
  IN      UINTN  Length,
  IN      UINT8  Value
  )
{
  //
  // Declare the local variables that actually move the data elements as
  // volatile to prevent the optimizer from replacing this function with
  // the intrinsic memset()
  //
  volatile UINT8   *Pointer8;
  volatile UINT32  *Pointer32;
  volatile UINT64  *Pointer64;
  UINT32           Value32;
  UINT64           Value64;

  if ((((UINTN)Buffer & 0x7) == 0) && (Length >= 8)) {
    // Generate the 64bit value
    Value32 = (Value << 24) | (Value << 16) | (Value << 8) | Value;
    Value64 = LShiftU64 (Value32, 32) | Value32;

    Pointer64 = (UINT64 *)Buffer;
    while (Length >= 8) {
      *(Pointer64++) = Value64;
      Length        -= 8;
    }

    // Finish with bytes if needed
    Pointer8 = (UINT8 *)Pointer64;
  } else if ((((UINTN)Buffer & 0x3) == 0) && (Length >= 4)) {
    // Generate the 32bit value
    Value32 = (Value << 24) | (Value << 16) | (Value << 8) | Value;

    Pointer32 = (UINT32 *)Buffer;
    while (Length >= 4) {
      *(Pointer32++) = Value32;
      Length        -= 4;
    }

    // Finish with bytes if needed
    Pointer8 = (UINT8 *)Pointer32;
  } else {
    Pointer8 = (UINT8 *)Buffer;
  }

  while (Length-- > 0) {
    *(Pointer8++) = Value;
  }

  //
  // Order the fill stores above before any later memory access so a caller's
  // subsequent store cannot be scheduled ahead of the buffer being cleared.
  //
  InternalMemBarrier (Buffer);

  return Buffer;
}
