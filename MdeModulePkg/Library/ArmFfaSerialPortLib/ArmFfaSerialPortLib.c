/** @file
  Base Debug library instance based on Arm FF-A Console log.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>

#include <IndustryStandard/ArmFfaSvc.h>
#include <IndustryStandard/ArmStdSmc.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ArmFfaLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/ArmSvcLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugPrintErrorLevelLib.h>

//
// Define the maximum debug and assert message length that this library supports
//
#define MAX_DEBUG_MESSAGE_LENGTH  0x100

//
// Define Maxmium available number of registers per FF-A call.
//
#define MAX_REG64_COUNT  16

/**
  Write data from buffer to serial device.

  Writes NumberOfBytes data bytes from Buffer to the serial device.
  The number of bytes actually written to the serial device is returned.
  If the return value is less than NumberOfBytes, then the write operation failed.
  If Buffer is NULL, then ASSERT().
  If NumberOfBytes is zero, then return 0.

  @param  Buffer           Pointer to the data buffer to be written.
  @param  NumberOfBytes    Number of bytes to written to the serial device.

  @retval 0                NumberOfBytes is 0.
  @retval >0               The number of bytes written to the serial device.
                           If this value is less than NumberOfBytes, then the write operation failed.

**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  )
{
  RETURN_STATUS  Status;
  UINTN          MaxCharsPerArmFfaConsoleCall;
  UINTN          RemainBytes;
  UINTN          WriteBytes;
  ARM_FFA_ARGS   FfaArgs;

  MaxCharsPerArmFfaConsoleCall = sizeof (UINT64) * MAX_REG64_COUNT;
  RemainBytes                  = NumberOfBytes;

  while (RemainBytes > 0) {
    ZeroMem (&FfaArgs, sizeof (ARM_FFA_ARGS));
    WriteBytes = (RemainBytes > MaxCharsPerArmFfaConsoleCall) ? MaxCharsPerArmFfaConsoleCall : RemainBytes;

    /*
     * StandaloneMm in Arm is supported 64-bit only so
     * SPMC must be in the 64-bit execution state and
     * according to FF-A specification Chapter 11, bullet 6
     * SPMC must implement both SMC32 and SMC64 conventions of the ABI
     * if it runs in the 64-bit execution state.
     *
     * Therefore, use FFA_CONSOLE_LOG_AARH64 only.
     */
    Status = ArmFfaLibFfaConsoleLog (Buffer, WriteBytes, &WriteBytes);
    if ((Status == RETURN_TIMEOUT) || (Status == RETURN_SUCCESS)) {
      // If the call timed out, we still want to update the number of bytes written
      // and continue to write the remaining bytes.
      RemainBytes -= WriteBytes;
      Buffer      += WriteBytes;
    } else {
      break;
    }
  }

  return NumberOfBytes - RemainBytes;
}
