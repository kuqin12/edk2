/*
 * cc.h
 *
 *  Created on: Jun 12, 2018
 *      Author: mrabeda
 */

#ifndef MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_CC_H_
#define MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_CC_H_

#include "MpTcpIpCommon.h"

#define BYTE_ORDER      LITTLE_ENDIAN

typedef UINT8     u8_t;
typedef INT8      s8_t;
typedef UINT16    u16_t;
typedef INT16     s16_t;
typedef UINT32    u32_t;
typedef INT32     s32_t;

typedef UINTN     mem_ptr_t;

typedef UINT16    uint16_t;
typedef UINTN     ptrdiff_t;

#define X8_F      "x"
#define U16_F     "u"
#define S16_F     "d"
#define X16_F     "x"
#define U32_F     "u"
#define S32_F     "d"
#define X32_F     "x"
#define SZT_F     "lu"

#define PACK_STRUCT_FIELD(x)    x
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END
//#define PACK_STRUCT_STRUCT    __attribute__((packed))
//#define PACK_STRUCT_BEGIN     _Pragma("pack")
//#define PACK_STRUCT_END       _Pragma("pop")

#define NO_PARENTH(...)   __VA_ARGS__
#define LWIP_PLATFORM_DIAG(message)   DEBUG ((EFI_D_WARN, NO_PARENTH message));
#define LWIP_PLATFORM_ASSERT

#endif /* MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_CC_H_ */
