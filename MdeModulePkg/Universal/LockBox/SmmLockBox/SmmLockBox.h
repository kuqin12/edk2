/** @file
  LockBox SMM driver.

  Caution: This module requires additional review when modified.
  This driver will have external input - communicate buffer in SMM mode.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  SmmLockBoxHandler(), SmmLockBoxRestore(), SmmLockBoxUpdate(), SmmLockBoxSave()
  will receive untrusted input and do basic validation.

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMM_LOCK_BOX_H_
#define SMM_LOCK_BOX_H_

/**
  Notify function for LockBox protocol.

  This function is used to install the LockBox protocol in the DXE phase.

  @param ImageHandle  The image handle of this driver.

  @retval EFI_SUCCESS  The function completed successfully.
  @return Others      Some error occurs.
**/
EFI_STATUS
NotifyLockBoxProtocol (
  IN EFI_HANDLE  ImageHandle
  );

/**
  This function check if the buffer is valid per processor architecture and not overlap with MMRAM.

  @param Buffer  The buffer start address to be checked.
  @param Length  The buffer length to be checked.

  @retval TRUE  This buffer is valid per processor architecture and not overlap with MMRAM.
  @retval FALSE This buffer is not valid per processor architecture or overlap with MMRAM.
**/
BOOLEAN
EFIAPI
IsBufferOutsideMmValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  );

/**
  This function checks if the Primary Buffer (CommBuffer) is valid.

  @param Buffer The buffer start address to be checked.
  @param Length The buffer length to be checked.

  @retval TRUE  This buffer is valid.
  @retval FALSE This buffer is not valid.
**/
BOOLEAN
IsPrimaryBufferValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  );

/**
  Entry Point for LockBox SMM driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  A Pointer to the EFI System Table.

  @retval EFI_SUCEESS
  @return Others          Some error occurs.
**/
EFI_STATUS
EFIAPI
CommonMmLockBoxEntryPoint (
  IN EFI_HANDLE        ImageHandle
  );

#endif // SMM_LOCK_BOX_H_
