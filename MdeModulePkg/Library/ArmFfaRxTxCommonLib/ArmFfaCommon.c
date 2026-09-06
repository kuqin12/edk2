/** @file
  Arm Ffa library common code.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  Copyright (c) Qualcomm Technologies, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

   @par Glossary:
     - FF-A - Firmware Framework for Arm A-profile

   @par Reference(s):
     - Arm Firmware Framework for Arm A-Profile [https://developer.arm.com/documentation/den0077/latest]

**/
#include <Uefi.h>
#include <Pi/PiMultiPhase.h>

#include <Library/ArmLib.h>
#include <Library/ArmFfaLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/ArmSvcLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/SafeIntLib.h>

#include <IndustryStandard/ArmFfaSvc.h>
#include <IndustryStandard/ArmFfaPartInfo.h>
#include <IndustryStandard/ArmStdSmc.h>

#include <Guid/ArmFfaRxTxBufferInfo.h>

#include "ArmFfaCommon.h"

/**
  Common ArmFfaLib init.

  @param [out] PartId            PartitionId
  @param [out] IsFfaSupported    FF-A supported flag

  @retval EFI_SUCCESS            Success
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval Others                 Error

**/
EFI_STATUS
EFIAPI
ArmFfaLibCommonInit (
  OUT UINT16   *PartId,
  OUT BOOLEAN  *IsFfaSupported
  )
{
  EFI_STATUS  Status;

  if ((PartId == NULL) || (IsFfaSupported == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *IsFfaSupported = ArmFfaLibIsFfaSupported ();

  Status = ArmFfaLibPartitionIdGet (PartId);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS  Status;
  VOID        *TxBuffer;
  UINT64      TxBufferSize;
  VOID        *RxBuffer;
  UINT64      RxBufferSize;
  UINT32      Count;
  UINT32      Size;
  UINT16      PartId;

  if (PartDesc == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Count = 1;

  Status = ArmFfaLibPartitionInfoGetRegs (ServiceGuid, &Count, PartDesc);
  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  Status = ArmFfaLibGetRxTxBuffers (
             &TxBuffer,
             &TxBufferSize,
             &RxBuffer,
             &RxBufferSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get Rx/Tx Buffer. Status: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  Status = ArmFfaLibPartitionInfoGet (
             ServiceGuid,
             FFA_PART_INFO_FLAG_TYPE_DESC,
             &Count,
             &Size
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ArmFfaLibGetPartId (&PartId);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Size < sizeof (EFI_FFA_PART_INFO_DESC))) {
    ArmFfaLibRxRelease (PartId);
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (PartDesc, RxBuffer, sizeof (EFI_FFA_PART_INFO_DESC));
  ArmFfaLibRxRelease (PartId);

  return EFI_SUCCESS;
}

/**
  Get first Rx/Tx Buffer allocation hob.
  If UseGuid is TRUE, BufferAddr and BufferSize parameters are ignored.

  @param[in]  BufferAddr       Buffer address
  @param[in]  BufferSize       Buffer Size
  @param[in]  UseGuid          Find MemoryAllocationHob using gArmFfaRxTxBufferInfoGuid.

  @retval     NULL             Not found
  @retval     Other            MemoryAllocationHob related to Rx/Tx buffer

**/
EFI_HOB_MEMORY_ALLOCATION *
EFIAPI
GetRxTxBufferAllocationHob (
  IN EFI_PHYSICAL_ADDRESS  BufferAddr,
  IN UINT64                BufferSize,
  IN BOOLEAN               UseGuid
  )
{
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryAllocationHob;
  EFI_PHYSICAL_ADDRESS       MemoryBase;
  UINT64                     MemorySize;

  if (!UseGuid && (BufferAddr == 0x00)) {
    return NULL;
  }

  MemoryAllocationHob = NULL;
  Hob.Raw             = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);

  while (Hob.Raw != NULL) {
    if (Hob.MemoryAllocation->AllocDescriptor.MemoryType == EfiConventionalMemory) {
      continue;
    }

    MemoryBase = Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress;
    MemorySize = Hob.MemoryAllocation->AllocDescriptor.MemoryLength;

    if ((!UseGuid && (BufferAddr >= MemoryBase) &&
         ((BufferAddr + BufferSize) <= (MemoryBase + MemorySize))) ||
        (UseGuid && CompareGuid (
                      &gArmFfaRxTxBufferInfoGuid,
                      &Hob.MemoryAllocation->AllocDescriptor.Name
                      )))
    {
      MemoryAllocationHob = (EFI_HOB_MEMORY_ALLOCATION *)Hob.Raw;
      break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw);
  }

  return MemoryAllocationHob;
}

/**
  Get Rx/Tx buffer MinSizeAndAign and MaxSize

  @param[out] MinSizeAndAlign  Minimum size of Buffer.

  @retval EFI_SUCCESS
  @retval EFI_UNSUPPORTED          Wrong min size received from SPMC
  @retval EFI_INVALID_PARAMETER    Wrong buffer size
  @retval Others                   Failure of ArmFfaLibGetFeatures()

**/
EFI_STATUS
EFIAPI
GetRxTxBufferMinSizeAndAlign (
  OUT UINTN  *MinSizeAndAlign
  )
{
  EFI_STATUS  Status;
  UINTN       MinAndAlign;
  UINTN       MaxSize;
  UINTN       Property1;
  UINTN       Property2;

  Status = ArmFfaLibGetFeatures (
             ARM_FID_FFA_RXTX_MAP,
             FFA_RXTX_MAP_INPUT_PROPERTY_DEFAULT,
             &Property1,
             &Property2
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to get RX/TX buffer property... Status: %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  MinAndAlign =
    ((Property1 >>
      ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_SHIFT) &
     ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_MASK);

  switch (MinAndAlign) {
    case ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_4K:
      MinAndAlign = SIZE_4KB;
      break;
    case ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_16K:
      MinAndAlign = SIZE_16KB;
      break;
    case ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_64K:
      MinAndAlign = SIZE_64KB;
      break;
    default:
      DEBUG ((DEBUG_ERROR, "%a: Invalid MinSizeAndAlign: 0x%x\n", __func__, MinAndAlign));
      return EFI_UNSUPPORTED;
  }

  MaxSize =
    (((Property1 >>
       ARM_FFA_BUFFER_MAXSIZE_PAGE_COUNT_SHIFT) &
      ARM_FFA_BUFFER_MAXSIZE_PAGE_COUNT_MASK));

  MaxSize = ((MaxSize == 0) ? MAX_UINTN : (MaxSize * MinAndAlign));

  if ((MinAndAlign > (EFI_PAGES_TO_SIZE (PcdGet64 (PcdFfaTxRxPageCount)))) ||
      (MaxSize < (EFI_PAGES_TO_SIZE (PcdGet64 (PcdFfaTxRxPageCount)))))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Buffer is too small! MinSize: 0x%x, MaxSize: 0x%x, PageCount: %d\n",
      __func__,
      MinAndAlign,
      MaxSize,
      PcdGet64 (PcdFfaTxRxPageCount)
      ));
    return EFI_INVALID_PARAMETER;
  }

  *MinSizeAndAlign = MinAndAlign;

  return EFI_SUCCESS;
}

/**
  Determine if FF-A is supported

  @retval TRUE if FF-A is supported, FALSE otherwise.

**/
BOOLEAN
EFIAPI
ArmFfaLibIsFfaSupported (
  IN VOID
  )
{
  EFI_STATUS  Status;
  UINT32      CurrentVersion;

  Status = ArmFfaLibGetVersion (
             ARM_FFA_CREATE_VERSION (ARM_FFA_MAJOR_VERSION, ARM_FFA_MINOR_VERSION),
             &CurrentVersion
             );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if (!ARM_FFA_ABI_COMPATIBLE (CurrentVersion, ARM_FFA_MAJOR_VERSION, ARM_FFA_MINOR_VERSION)) {
    DEBUG ((
      DEBUG_INFO,
      "Incompatible FF-A Versions.\n" \
      "Request Version: Major=0x%x, Minor=0x%x.\n" \
      "Current Version: Major=0x%x, Minor=0x%x.\n",
      ARM_FFA_MAJOR_VERSION,
      ARM_FFA_MINOR_VERSION,
      ARM_FFA_MAJOR_VERSION_GET (CurrentVersion),
      ARM_FFA_MINOR_VERSION_GET (CurrentVersion)
      ));
    return FALSE;
  }

  return TRUE;
}
