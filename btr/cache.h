//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _CACHE_H_
#define _CACHE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "btr.h"
#include "hal.h"
#include "lock.h"

typedef struct _BTR_MAPPING_OBJECT {
	HANDLE Object;
	PVOID MappedVa;
	ULONG MappedLength;
	ULONG64 MappedOffset;
	ULONG64 FileLength;
	USHORT ObjectFlag;
	struct _BTR_THREAD *Thread;
} BTR_MAPPING_OBJECT, *PBTR_MAPPING_OBJECT;

typedef struct _BTR_CACHE_OBJECT {
	BOOLEAN IsConfigured;
	HANDLE IndexFileHandle;
	HANDLE DataFileHandle;
	HANDLE SharedDataHandle;
	HANDLE PerfDataHandle;
} BTR_CACHE_OBJECT, *PBTR_CACHE_OBJECT;

ULONG
BtrInitializeCache(
	VOID
	);

VOID
BtrUninitializeCache(
	VOID
	);

ULONG
BtrConfigureCache(
	PBTR_MESSAGE_CACHE Packet 
	);

ULONG
BtrCreateMappingPerThread(
	IN PBTR_THREAD Thread
	);

ULONG
BtrCloseMappingPerThread(
	IN PBTR_THREAD Thread
	);

ULONG
BtrAcquireWritePosition(
	IN ULONG BytesToWrite,
	OUT PULONG64 Position
	);

ULONG
BtrWriteSharedCacheRecord(
	IN PBTR_THREAD Thread,
	IN PBTR_RECORD_HEADER Record,
	OUT PULONG64 Offset
	);

ULONG
BtrWriteSharedCacheIndex(
	IN PBTR_THREAD Thread,
	IN ULONG64 RecordOffset,
	IN ULONG64 Location,
	IN ULONG Size
	);

ULONG
BtrExtendFileLength(
	IN HANDLE File,
	IN ULONG64 Length
	);

ULONG
BtrCopyMemory(
	IN PVOID Destine,
	IN PVOID Source,
	IN ULONG Length 
	);

ULONG
BtrAcquireObjectId(
	VOID
	);

extern PBTR_SHARED_DATA BtrSharedData;

#ifdef __cplusplus
}
#endif

#endif 