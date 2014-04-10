//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _QUEUE_H_
#define _QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "btr.h"
#include "probe.h"

typedef struct _BTR_RECORD_QUEUE {
	LIST_ENTRY ListEntry;
	PLIST_ENTRY QueueList;
	ULONG NumberOfQueues;
} BTR_RECORD_QUEUE, *PBTR_RECORD_QUEUE;

typedef enum _BTR_QUEUE_BUFFER_TYPE {
	QueueBufferNormal,
	QueueBufferLarge,
} BTR_QUEUE_BUFFER_TYPE;

typedef struct _BTR_QUEUE_BUFFER {
	
	union {
		SLIST_ENTRY SListEntry;
		LIST_ENTRY ListEntry;
	} u;

	USHORT NumberOfQueues;
	USHORT BufferType;
	ULONG Spare0;

#if defined (_M_X64)
	ULONGLONG Spare1;
#endif

	SLIST_HEADER Queue[ANYSIZE_ARRAY];

} BTR_QUEUE_BUFFER, *PBTR_QUEUE_BUFFER;

typedef struct _BTR_BUFFER_LOOKASIDE {
	SLIST_HEADER NormalList;
	ULONG NormalAllocateHits;
	ULONG NormalFreeHits;
	ULONG LargeAllocateHits;
	ULONG LargeFreeHits;
} BTR_BUFFER_LOOKASIDE, *PBTR_BUFFER_LOOKASIDE;

#define BYTES_PER_PAGE  4096

PBTR_QUEUE_BUFFER
BtrAllocateQueueBuffer(
	IN NumberOfQueues
	);

VOID
BtrFreeQueueBuffer(
	IN PBTR_QUEUE_BUFFER Buffer
	);

ULONG
BtrInitializeQueue(
	VOID
	);

ULONG 
BtrQueueRecord(
	IN PBTR_THREAD Thread,
	IN PBTR_RECORD_HEADER Header
	);

ULONG
BtrQueueRecordPerThread(
	IN PBTR_THREAD Thread,
	IN PBTR_RECORD_HEADER Header,
	IN PBTR_PROBE Probe
	);

ULONG
BtrQueueSharedCacheRecord(
	IN PBTR_THREAD Thread,
	IN PBTR_RECORD_HEADER Header,
	IN PBTR_PROBE Probe
	);

ULONG
BtrDeQueueRecord(
	IN PVOID Buffer,
	IN ULONG BufferLength,
	OUT PULONG Length,
	OUT PULONG Count
	);

ULONG
BtrQueueUpdateStackTrace(
	IN PBTR_STACKTRACE_ENTRY Trace
	);

ULONG
BtrDeQueueStackTrace(
	IN PVOID Buffer, 
	IN ULONG BufferLength,
	OUT PULONG Length,
	OUT PULONG Count
	);

ULONG
BtrDeQueueThreadQueue(
	IN PVOID Buffer,
	IN ULONG BufferLength,
	OUT PULONG Length,
	OUT PULONG Count
	);

typedef struct _BTR_THREAD_STATE {
	ULONG ActiveCount;
	ULONG RundownCount;
	PULONG ActiveId;
	PULONG RundownId;
	SYSTEMTIME TimeStamp;
} BTR_THREAD_STATE, *PBTR_THREAD_STATE;

ULONG
BtrScanRundown(
	VOID
	);

VOID
BtrScanLookaside(
	VOID
	);

VOID
BtrTrimWorkingSet(
	VOID
	);

extern BTR_LOCK BtrRecordQueueLock;
extern LIST_ENTRY BtrOrderedRecordList;


#ifdef __cplusplus
}
#endif

#endif  // _QUEUE_H_