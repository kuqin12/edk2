/** @file
  MM Unblock Memory Library Interface.

  This library provides an interface to request non-MMRAM pages to be mapped/unblocked
  from inside MM environment.

  For MM modules that need to access regions outside of MMRAMs, the agents that set up
  these regions are responsible for invoking this API in order for these memory areas
  to be accessed from inside MM.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_UNBLOCK_MEMORY_LIB_H_
#define MM_UNBLOCK_MEMORY_LIB_H_

/**
  This API provides a way to unblock certain data pages to be accessible inside MM environment.

  @param  UnblockAddress          The address of buffer caller requests to unblock, the address
                                  has to be page aligned.
  @param  NumberOfPages           The number of pages requested to be unblocked from MM
                                  environment.

  @return EFI_SUCCESS             The request goes through successfully.
  @return EFI_NOT_AVAILABLE_YET   The requested functionality is not produced yet.
  @return EFI_UNSUPPORTED         The requested functionality is not supported on current platform.
  @return EFI_SECURITY_VIOLATION  The requested address failed to pass security check for
                                  unblocking.
  @return EFI_INVALID_PARAMETER   Input address either NULL pointer or not page aligned.
  @return EFI_ACCESS_DENIED       The request is rejected due to system has passed certain boot
                                  phase.

**/
EFI_STATUS
EFIAPI
MmUnblockMemoryRequest (
  IN EFI_PHYSICAL_ADDRESS   UnblockAddress,
  IN UINT64                 NumberOfPages
);

#endif // MM_UNBLOCK_MEMORY_LIB_H_
