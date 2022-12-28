/*
 * string.h
 *
 *  Created on: Jun 18, 2018
 *      Author: mrabeda
 */

#ifndef MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_INCLUDE_ARCH_STRING_H_
#define MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_INCLUDE_ARCH_STRING_H_

#define memset(b, c, n)       SetMem(b, n, c)
#define memcpy(d, s, n)       CopyMem(d, s, n)
#define strlen(s)             AsciiStrLen(s)
#define strncmp(s1, s2, n)    AsciiStrnCmp(s1, s2, n)
#define memmove(d, s, n)      CopyMem(d, s, n)
#define memcmp(s1, s2, n)     CompareMem(s1, s2, n)

#endif /* MDEMODULEPKG_UNIVERSAL_NETWORK_LWIPDXE_INCLUDE_ARCH_STRING_H_ */
