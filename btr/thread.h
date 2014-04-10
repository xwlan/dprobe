//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _THREAD_H_
#define _THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "btr.h"
#include "lock.h"

#pragma pack(push, 1)

typedef struct _BTR_UNLOAD_BLOCK32 {
	LONG Nop;
	CHAR PushTimeOut[5];
	CHAR PushWaitHandle[5];
	CHAR CallWaitForSingleObject[5];
	CHAR PushRuntimeHandle[5];
	CHAR CallFreeLibrary[5];
	CHAR PopEax;
	CHAR SubEspUlong[3];
	CHAR PushHeapHandle[5];
	CHAR PushEax;
	CHAR JmpHeapDestroy[5];
	LONG Int3;
} BTR_UNLOAD_BLOCK32, *PBTR_UNLOAD_BLOCK32;

#pragma pack(pop)

//
// N.B. Systsem TEB size is 4K on x86, 8K on x64
// The size must be ensured by compiler assert
//

typedef struct _BTR_TEB {
	ULONG_PTR Teb[1023];
	PBTR_THREAD Thread;
} BTR_TEB, *PBTR_TEB;

#if defined (_M_IX86)

C_ASSERT(sizeof(BTR_TEB) == 4096);

#endif

#if defined (_M_X64) 

C_ASSERT(sizeof(BTR_TEB) == (4096 * 2));

#endif

struct _BTR_FILE_OBJECT;
struct _BTR_FILE_HEAD;

//
// N.B. Thread object must be allocated with 16 bytes aligned
// on x64, 8 bytes aligned on x86, this ensure InterlockedPushEntrySList
// work as expected on MP system.
//

typedef struct _BTR_THREAD {

	union {
		LIST_ENTRY ListEntry;
		SLIST_ENTRY SListEntry;
	};

	ULONG References;
	BTR_THREAD_TYPE Type;

	ULONG ThreadFlag;
	ULONG ThreadId;
	ULONG_PTR ThreadTag;
	PULONG_PTR StackBase;
	HANDLE RundownHandle;
	PVOID Buffer;
	BOOLEAN BufferBusy;

	union {
		struct {
			struct _BTR_FILE_OBJECT *FileObject;
			struct _BTR_FILE_OBJECT *IndexObject;
		};
		struct {
			struct _BTR_MAPPING_OBJECT *DataMapping;
			struct _BTR_MAPPING_OBJECT *IndexMapping;
		};
	};

	ULONG FrameDepth;
	LIST_ENTRY FrameList;
	ULONG64 CallCount;

} BTR_THREAD, *PBTR_THREAD;

typedef struct _BTR_ADDRESS_RANGE {
	LIST_ENTRY ListEntry;
	ULONG_PTR Address;
	ULONG_PTR Size;
} BTR_ADDRESS_RANGE, *PBTR_ADDRESS_RANGE;

typedef enum _BTR_THREAD_TYPE {
	ThreadWatch,
	ThreadWrite,
	ThreadSchedule,
	ThreadMessage,
	ThreadNumber,
	ThreadUser = -1,
} BTR_THREAD_TYPE, *PBTR_THREAD_TYPE;

typedef struct _BTR_QUEUE {
	HANDLE Object;
	PVOID Context;
	BTR_THREAD_TYPE Type;
	LIST_ENTRY ListEntry;
} BTR_QUEUE, *PBTR_QUEUE;


//
// Define thread tag used to verify that
// whether the thread object is corrupted. 
//

#define THREAD_TAG  'erhT'


PBTR_TEB 
BtrGetCurrentTeb(
	VOID
	);

VOID
BtrClearTeb(
	IN ULONG ThreadId 
	);

PBTR_THREAD
BtrAllocateThread(
	IN BTR_THREAD_TYPE Type
	);

PBTR_THREAD
BtrAllocateThreadLookaside(
	VOID
	);

VOID
BtrFreeThreadLookaside(
	IN PBTR_THREAD Thread
	);

VOID
BtrCleanThreadLookaside(
	VOID
	);

VOID
BtrQueueRundownThread(
	IN PBTR_THREAD Thread
	);

BOOLEAN
BtrIsThreadTerminated(
	IN PBTR_THREAD Thread
	);

PBTR_THREAD
BtrGetCurrentThread(
	VOID
	);

PBTR_UNLOAD_BLOCK32
BtrCreateUnloadBlock(
	OUT PHANDLE Heap	
	);

ULONG
BtrSuspendProcess(
	OUT PLIST_ENTRY ListHead	
	);

VOID
BtrResumeProcess(
	IN PLIST_ENTRY ListHead
	);

PBTR_THREAD
BtrGetFirstThread(
	VOID
	);

PBTR_THREAD
BtrGetNextThread(
	IN PBTR_THREAD Thread
	);

BOOLEAN
BtrIsExecutingAddress(
	IN PLIST_ENTRY ContextList,
	IN PLIST_ENTRY AddressList
	);

BOOLEAN
BtrIsExecutingRuntime(
	IN PLIST_ENTRY ContextList 
	);

BOOLEAN
BtrIsExecutingFilter(
	IN PLIST_ENTRY ContextList 
	);

BOOLEAN
BtrIsExecutingTrap(
	IN PLIST_ENTRY ContextList
	);

BOOLEAN
BtrIsPendingFrameExist(
	IN PBTR_THREAD Thread
	);

ULONG
BtrInitializeThread(
	VOID
	);

VOID
BtrUninitializeThread(
	VOID
	);

BOOLEAN
BtrIsRuntimeThread(
	IN ULONG ThreadId
	);

VOID
BtrInitializeRuntimeThread(
	VOID
	);

ULONG
BtrScanThreadFinal(
	VOID
	);

BOOLEAN
BtrIsExemptedAddress(
	IN ULONG_PTR Address
	);

VOID
BtrScanPendingThreads(
	OUT PLIST_ENTRY PendingListHead,
	OUT PLIST_ENTRY CompleteListHead
	);

VOID
BtrExecuteUnloadRoutine(
	VOID
	);

ULONG CALLBACK
BtrWatchProcedure(
	IN PVOID Context
	);

ULONG CALLBACK
BtrWriteProcedure(
	IN PVOID Context
	);

ULONG CALLBACK
BtrScheduleProcedure(
	IN PVOID Context
	);

BOOL
BtrOnThreadAttach(
	VOID
	);

BOOL
BtrOnThreadDetach(
	VOID
	);

BOOL
BtrOnProcessAttach(
	IN HMODULE DllHandle	
	);

BOOL
BtrOnProcessDetach(
	VOID
	);

VOID
BtrSetTlsValue(
	IN PBTR_THREAD Thread
	);

PBTR_THREAD
BtrGetTlsValue(
	VOID
	);

VOID
BtrClearThreadStack(
	IN PBTR_THREAD Thread
	);

extern DWORD BtrTlsIndex;
extern LIST_ENTRY BtrActiveThreadList;
extern LIST_ENTRY BtrRundownThreadList;
extern ULONG BtrActiveThreadCount;
extern BTR_LOCK BtrThreadLock;

extern PBTR_THREAD BtrNullThread;
extern HANDLE BtrSystemThread[ThreadNumber];
extern BTR_QUEUE BtrSystemQueue[ThreadNumber];
extern PTHREAD_START_ROUTINE BtrSystemProcedure[ThreadNumber];
extern SLIST_HEADER BtrThreadRundownQueue;

#ifdef __cplusplus
}
#endif

#endif