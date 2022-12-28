/*
 * MpCommon.h
 *
 *  Created on: Dec 12, 2017
 *      Author: mrabeda
 */

#ifndef MDEPKG_LIBRARY_BASETHREADSAFESTRUCTLIB_MPCOMMON_H_
#define MDEPKG_LIBRARY_BASETHREADSAFESTRUCTLIB_MPCOMMON_H_

#include <Uefi.h>
#include <Library/DxeThreadingStructLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/ThreadingLib.h>

#include "MpListInternals.h"
#include "MpListCachedInternals.h"

#define DEBUGPOINT()                    DEBUG ((EFI_D_ERROR, "[%a:%a:%d] Point.\n", __FILE__, __FUNCTION__, __LINE__));

#endif /* MDEPKG_LIBRARY_BASETHREADSAFESTRUCTLIB_MPCOMMON_H_ */
