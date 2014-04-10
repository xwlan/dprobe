//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _UTIL_H_
#define _UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include "lock.h"

VOID
DebugTrace(
	IN PSTR Format,
	...
	);

VOID WINAPI
BtrFormatError(
	IN ULONG ErrorCode,
	IN BOOLEAN SystemError,
	OUT PWCHAR Buffer,
	IN ULONG BufferLength, 
	OUT PULONG ActualLength
	);

ULONG_PTR
BtrUlongPtrRoundDown(
	IN ULONG_PTR Value,
	IN ULONG_PTR Align
	);

ULONG_PTR
BtrUlongPtrRoundUp(
	IN ULONG_PTR Value,
	IN ULONG_PTR Align
	);

ULONG
BtrUlongRoundDown(
	IN ULONG Value,
	IN ULONG Align
	);

ULONG
BtrUlongRoundUp(
	IN ULONG Value,
	IN ULONG Align
	);

ULONG64
BtrUlong64RoundDown(
	IN ULONG64 Value,
	IN ULONG64 Align
	);

ULONG64
BtrUlong64RoundUp(
	IN ULONG64 Value,
	IN ULONG64 Align
	);

BOOLEAN
BtrIsExecutableAddress(
	IN PVOID Address
	);

BOOLEAN
BtrIsValidAddress(
	IN PVOID Address
	);

ULONG
BtrGetByteOffset(
	IN PVOID Address
	);

SIZE_T 
BtrConvertUnicodeToUTF8(
	IN PWSTR Source, 
	OUT PCHAR *Destine
	);

VOID 
BtrConvertUTF8ToUnicode(
	IN PCHAR Source,
	OUT PWCHAR *Destine
	);

ULONG 
BtrCompareMemoryPointer(
	IN PVOID Source,
	IN PVOID Destine,
	IN ULONG NumberOfUlongPtr
	);

ULONG
BtrComputeAddressHash(
	IN PVOID Address,
	IN ULONG Limit
	);

#ifdef __cplusplus
}
#endif

#endif // _UTIL_H_