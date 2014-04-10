//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _STKTRACE_H_
#define _STKTRACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "btr.h"

typedef struct _BTR_DLL_ENTRY {
	ULONG ProcessId;
	PVOID  BaseVa;
	SIZE_T Size;
	FILETIME LinkerTime;
	FILETIME StartTime;	
	FILETIME EndTime;	
	WCHAR Path[MAX_PATH];
} BTR_DLL_ENTRY, *PBTR_DLL_ENTRY;

typedef struct _BTR_DLL_DATABASE {
	
	ULONG ProcessId;
	ULONG NumberOfDlls;

	union {
		PBTR_DLL_ENTRY Dll;
		ULONG64 Offset;
	} u1;

	union {
		LIST_ENTRY ListEntry;
		ULONG64 NextOffset;
	} u2;

} BTR_DLL_DATABASE, *PBTR_DLL_DATABASE;

ULONG
BtrInitializeStackTrace(
	IN BOOLEAN PushUpdate	
	);

ULONG
BtrUninitializeStackTrace(
	VOID
	);

BOOLEAN
BtrInsertStackTrace(
	IN PBTR_STACKTRACE_ENTRY Trace 
	);

PBTR_STACKTRACE_ENTRY
BtrLookupStackTrace(
	IN PVOID Probe,
	IN ULONG Hash,
	IN ULONG Depth
	);

ULONG
BtrScanStackTrace(
	VOID
	);

VOID
BtrFreeStackTraceBucket(
	IN PSLIST_ENTRY ListEntry 
	);

PBTR_STACKTRACE_ENTRY
BtrAllocateStackTraceLookaside(
	VOID
	);

VOID
BtrFreeStackTraceLookaside(
	IN PBTR_STACKTRACE_ENTRY Entry
	);
VOID
BtrFlushStackTrace(
	VOID
	);

VOID
BtrQueueStackTrace(
	IN PBTR_STACKTRACE_ENTRY Trace
	);

VOID
BtrCaptureStackTrace(
	IN PVOID Frame, 
	IN PVOID Probe,
	OUT PULONG Hash,
	OUT PULONG Depth
	);

BOOLEAN
BtrWalkCallStack(
	IN PVOID Fp,
	IN PVOID ProbeAddress,
	IN ULONG Count,
	OUT PULONG CapturedCount,
	OUT PVOID  *Frame,
	OUT PULONG Hash
	);

ULONG
BtrWalkFrameChain(
	IN ULONG_PTR Fp,
    IN ULONG Count, 
    OUT PVOID *Callers,
	OUT PULONG Hash
    );

FORCEINLINE
BOOLEAN
BtrFilterStackTrace(
	IN ULONG_PTR Address
	);

extern SLIST_HEADER BtrStackTraceQueue;
extern SLIST_HEADER BtrStackTraceUpdateQueue;
extern BTR_STACKTRACE_DATABASE BtrStackTraceDatabase;

#ifdef __cplusplus
}
#endif
#endif