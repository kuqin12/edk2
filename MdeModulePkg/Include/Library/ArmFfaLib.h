/** @file
  Arm FF-A library Header file

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
     - FF-A - Firmware Framework for Arm A-profile
     - spmc - Secure Partition Manager Core
     - spmd - Secure Partition Manager Dispatcher

  @par Reference(s):
     - Arm Firmware Framework for Arm A-Profile [https://developer.arm.com/documentation/den0077/latest]

**/

#pragma once

#include <IndustryStandard/ArmFfaSvc.h>
#include <IndustryStandard/ArmFfaBootInfo.h>
#include <IndustryStandard/ArmFfaPartInfo.h>

#include <Library/ArmSmcLib.h>

/**
  Check FF-A support or not.

  @retval TRUE                   Supported
  @retval FALSE                  Not supported

**/
BOOLEAN
EFIAPI
IsFfaSupported (
  IN VOID
  );

/**
  Helper to retrieve the first partition information associated with
  a service GUID via registers.

  @param [in]       ServiceGuid       Service guid.
  @param [in, out]  PartDescCount     Return number of partition info related to
                                      Service guid when PartDesc == NULL.
                                      Otherwise return number of partition info
                                      copied in ParcDesc
  @param [out]      PartDesc          Partition information Buffer

  @retval EFI_SUCCESS
  @retval EFI_UNSUPPORTED
  @retval EFI_INVALID_PARAMETER
  @retval Other                       Error

**/
EFI_STATUS
EFIAPI
ArmFfaLibGetPartitionInfo (
  IN EFI_GUID                 *ServiceGuid,
  OUT EFI_FFA_PART_INFO_DESC  *PartDesc
  );
