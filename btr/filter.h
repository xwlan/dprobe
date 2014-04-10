//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009 - 2011
//

#ifndef _FILTER_H_
#define _FILTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "btr.h"

ULONG
BtrCreateFilter(
	IN PWSTR FilterFullPath,
	OUT PBTR_FILTER *Result
	);

PBTR_FILTER
BtrQueryFilter(
	IN PWSTR FilterName
	);

ULONG
BtrReadFilterRecords(
	IN PVOID Buffer,
	IN ULONG Length,
	OUT PULONG UsedLength,
	OUT PULONG NumberOfRecords
	);

ULONG
BtrInitializeFlt(
	VOID
	);

ULONG
BtrUnregisterFilters(
	VOID
	);

extern LIST_ENTRY BtrFilterList;

#ifdef __cplusplus
}
#endif
#endif  // _FILTER_H_