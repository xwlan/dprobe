//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _COUNTER_H_
#define _COUNTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include "btr.h"

ULONG
BtrInitializeCounter(
	IN HANDLE PerfHandle	
	);

ULONG
BtrUninitializeCounter(
	VOID
	);

PBTR_COUNTER_ENTRY
BtrAllocateCounter(
	IN ULONG ObjectId
	);

VOID
BtrRemoveCounter(
	IN PBTR_COUNTER_ENTRY Entry 
	);

VOID
BtrUpdateCounter(
	IN PBTR_COUNTER_ENTRY Entry,
	IN PBTR_RECORD_HEADER Header
	);

ULONG
BtrGetCounterBucket(
	IN PBTR_COUNTER_ENTRY Entry
	);

//
// Perf data is shared by btr and msp via page file
// backed file mapping.
//

extern HANDLE BtrCounterHandle;

#ifdef __cplusplus
}
#endif

#endif