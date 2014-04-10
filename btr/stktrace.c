//
// lan.john@gmail.com 
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "stktrace.h"
#include "util.h"
#include "cache.h"
#include "heap.h"
#include "list.h"
#include "hal.h"
#include "probe.h"
#include <windows.h>
#include "trap.h"

HANDLE BtrStackTraceHandle;
BTR_STACKTRACE_DATABASE BtrStackTraceDatabase;
PBTR_STACKTRACE_ENTRY BtrStackTraceBuckets;
BOOLEAN BtrStackTracePushUpdate;

SLIST_HEADER BtrStackTraceQueue;
SLIST_HEADER BtrStackTraceUpdateQueue;
SLIST_HEADER BtrStackTraceLookaside;
ULONG BtrStackTraceLookasideThreshold;

ULONG
BtrInitializeStackTrace(
	IN BOOLEAN PushUpdate	
	)
{
	ULONG Size;
	ULONG Number;

	RtlZeroMemory(&BtrStackTraceDatabase, sizeof(BTR_STACKTRACE_DATABASE));
	BtrStackTraceDatabase.ProcessId = GetCurrentProcessId();
	BtrStackTraceDatabase.NumberOfTotalBuckets = BTR_STACKTRACE_BUCKET_NUMBER;
	BtrStackTraceDatabase.NumberOfFixedBuckets = BTR_STACKTRACE_BUCKET_NUMBER;
	BtrStackTraceDatabase.ChainedBucketsThreshold = BTR_STACKTRACE_BUCKET_NUMBER;

	BtrStackTracePushUpdate = PushUpdate;
	InitializeSListHead(&BtrStackTraceQueue);
	InitializeSListHead(&BtrStackTraceUpdateQueue);
	InitializeSListHead(&BtrStackTraceLookaside);
	BtrStackTraceLookasideThreshold = 1000;
	
	//
	// Allocate the stack trace database from page file and map its whole region 
	//

	Size = BtrUlongRoundUp(BTR_STACKTRACE_BUCKET_NUMBER * sizeof(BTR_STACKTRACE_ENTRY), 1024 * 64);
	BtrStackTraceHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 
		                                    0, Size, NULL);
	if (!BtrStackTraceHandle) {
		return BTR_E_CREATEFILEMAPPING;
	}

	BtrStackTraceBuckets = (PBTR_STACKTRACE_ENTRY)MapViewOfFile(BtrStackTraceHandle, 
		                                             FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
	if (!BtrStackTraceBuckets) {
		CloseHandle(BtrStackTraceHandle);
		return BTR_E_MAPVIEWOFFILE;
	}

	//
	// N.B. Initialize spinlock, note that every access of first level bucket must
	// acquire spinlock first, because stack trace may be dynamically trimmed to
	// limit resource usage.
	//

	for(Number = 0; Number < BtrStackTraceDatabase.NumberOfTotalBuckets; Number += 1) {
		BtrInitSpinLock(&BtrStackTraceBuckets[Number].SpinLock, 10);
	}
	
	return S_OK;
}

ULONG
BtrScanStackTrace(
	VOID
	)
{
	ULONG Number;
	PBTR_STACKTRACE_ENTRY Bucket;

	if (BtrStackTraceDatabase.NumberOfChainedBuckets < 
		BtrStackTraceDatabase.ChainedBucketsThreshold) {
		return S_OK;
	}

	DebugTrace("scan stktrace: total %d, firsthit: %d, chained: %d", 
			BtrStackTraceDatabase.NumberOfTotalBuckets,
			BtrStackTraceDatabase.NumberOfFirstHitBuckets,
			BtrStackTraceDatabase.NumberOfChainedBuckets);
		
	Bucket = BtrStackTraceBuckets;

	for(Number = 0; Number < BtrStackTraceDatabase.NumberOfFixedBuckets; Number += 1) {

		BtrAcquireSpinLock(&Bucket[Number].SpinLock);

		if (Bucket[Number].ListEntry.Next != NULL) {
			BtrFreeStackTraceBucket(Bucket[Number].ListEntry.Next);
			Bucket[Number].ListEntry.Next = NULL;
			Bucket[Number].Length = 0; 
		}

		BtrReleaseSpinLock(&Bucket[Number].SpinLock);
	}

	return S_OK;	
}

VOID
BtrFreeStackTraceBucket(
	IN PSLIST_ENTRY ListEntry 
	)
{
	PSLIST_ENTRY Next;
	PSLIST_ENTRY Prior;

	Prior = ListEntry;

	__try {
		while (Prior != NULL) {
			Next = Prior->Next;
			BtrFree(Prior);
			InterlockedDecrement(&BtrStackTraceDatabase.NumberOfTotalBuckets);
			InterlockedDecrement(&BtrStackTraceDatabase.NumberOfChainedBuckets);
			Prior = Next;
		} 
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		DebugTrace("BtrFreeStackTraceBucket: exception occurred.");
	}
}

ULONG
BtrUninitializeStackTrace(
	VOID
	)
{
	if (BtrStackTraceBuckets != NULL) {
		UnmapViewOfFile(BtrStackTraceBuckets);
		CloseHandle(BtrStackTraceHandle);
	}

	return S_OK;
}

#define COPY_STACKTRACE_ENTRY(_D, _S) \
	_D->Hash = _S->Hash;              \
	_D->Depth = _S->Depth;            \
	_D->Probe = _S->Probe;            \
	RtlCopyMemory(_D->Frame, _S->Frame, sizeof(PVOID) * _S->Depth); \
	_D->Count = 1;                                                  \
	_D->ListEntry.Next = NULL;                                          


BOOLEAN
BtrInsertStackTrace(
	IN PBTR_STACKTRACE_ENTRY Trace 
	)
{
	ULONG Bucket;
	PBTR_STACKTRACE_ENTRY Root; 
	PBTR_STACKTRACE_ENTRY Entry; 
	PBTR_STACKTRACE_ENTRY Prior; 

	Bucket = BtrStackTraceBucket(Trace->Hash);

	//
	// N.B. Two stack trace are idential if they have same hash, same depth,
	// and, have same frame values.
	//

	Entry = &BtrStackTraceBuckets[Bucket];
	Root = Entry;

	BtrAcquireSpinLock(&Root->SpinLock);

	if (Entry->Hash == 0 && Entry->Depth == 0 && Entry->Count == 0) {

		//
		// This is an empty bucket we can use
		//

		COPY_STACKTRACE_ENTRY(Entry, Trace);
		Root->Length += 1;

		BtrReleaseSpinLock(&Root->SpinLock);
		InterlockedIncrement(&BtrStackTraceDatabase.NumberOfFirstHitBuckets);
		return TRUE;
	}

	while (Entry != NULL) {

		if (Entry->Hash == Trace->Hash && Entry->Depth == Trace->Depth) {
			if (BtrCompareMemoryPointer(Entry->Frame, Trace->Frame, Entry->Depth) == Entry->Depth) {
				Entry->Count += 1;
				BtrReleaseSpinLock(&Root->SpinLock);
				return FALSE;
			}
		}
		
		Prior = Entry;
		Entry = (PBTR_STACKTRACE_ENTRY)Entry->ListEntry.Next;
	}

	//
	// No luck to hit, allocate new bucket to hold the stack trace
	//

	Entry = (PBTR_STACKTRACE_ENTRY)BtrMalloc(sizeof(BTR_STACKTRACE_ENTRY));
	COPY_STACKTRACE_ENTRY(Entry, Trace);

	Prior->ListEntry.Next = (PSLIST_ENTRY)Entry;
	Root->Length += 1;

	BtrReleaseSpinLock(&Root->SpinLock);

	InterlockedIncrement(&BtrStackTraceDatabase.NumberOfTotalBuckets);
	InterlockedIncrement(&BtrStackTraceDatabase.NumberOfChainedBuckets);
	return TRUE;
}

PBTR_STACKTRACE_ENTRY
BtrLookupStackTrace(
	IN PVOID Probe,
	IN ULONG Hash,
	IN ULONG Depth
	)
{
	ULONG Bucket;
	PBTR_STACKTRACE_ENTRY Entry;
	PBTR_STACKTRACE_ENTRY Root;

	Bucket = BtrStackTraceBucket(Hash);
	Entry = BtrStackTraceBuckets + Bucket;
	Root = Entry;

	BtrAcquireSpinLock(&Root->SpinLock);

	while (Entry != NULL) {
		if (Entry->Probe == Probe && Entry->Hash == Hash && Entry->Depth == Depth) {
			BtrReleaseSpinLock(&Root->SpinLock);
			return Entry;
		}
		Entry = (PBTR_STACKTRACE_ENTRY)Entry->ListEntry.Next;
	}

	BtrReleaseSpinLock(&Root->SpinLock);
	return NULL;
}

VOID
BtrFlushStackTrace(
	VOID
	)
{
	PSLIST_ENTRY ListEntry;
	PBTR_STACKTRACE_ENTRY Trace;
	BOOLEAN Update;

	while (ListEntry = InterlockedPopEntrySList(&BtrStackTraceQueue)) {
	
		Trace = CONTAINING_RECORD(ListEntry, BTR_STACKTRACE_ENTRY, ListEntry);

		Update = BtrInsertStackTrace(Trace);
		if (Update && BtrStackTracePushUpdate) {
			
			//
			// Queue new stack trace to update queue, it will be passed to msp side periodically.
			//

			InterlockedPushEntrySList(&BtrStackTraceUpdateQueue, 
				                      &Trace->ListEntry);
		} else {

			BtrFreeStackTraceLookaside(Trace);
		}
	}
}

VOID
BtrQueueStackTrace(
	IN PBTR_STACKTRACE_ENTRY Trace
	)
{
	InterlockedPushEntrySList(&BtrStackTraceQueue, &Trace->ListEntry);
}

VOID
BtrCaptureStackTrace(
	IN PVOID Frame, 
	IN PVOID Probe,
	OUT PULONG Hash,
	OUT PULONG Depth
	)
{
	PVOID Callers[MAX_STACK_DEPTH];
	PBTR_STACKTRACE_ENTRY StackTrace;

	//
	// N.B. Only x86 require start frame pointer, x64 has different
	// stack walk algorithm, don't use it.
	//

	BtrWalkCallStack(Frame, Probe, MAX_STACK_DEPTH, Depth, &Callers[0], Hash);

	StackTrace = BtrLookupStackTrace(Probe, *Hash, *Depth);
	if (!StackTrace) {

		StackTrace = (PBTR_STACKTRACE_ENTRY)BtrAllocateStackTraceLookaside();
		StackTrace->Probe = Probe;
		StackTrace->Hash = *Hash;
		StackTrace->Depth = (USHORT)*Depth;
		RtlCopyMemory(StackTrace->Frame, Callers, sizeof(PVOID) * (*Depth));

		BtrQueueStackTrace(StackTrace);
	}
}

BOOLEAN
BtrWalkCallStack(
	IN PVOID Fp,
	IN PVOID ProbeAddress,
	IN ULONG Count,
	OUT PULONG CapturedCount,
	OUT PVOID  *Frame,
	OUT PULONG Hash
	)
{
	ULONG Depth;
	ULONG CapturedDepth;
	ULONG ValidDepth;
	ULONG_PTR Address;
	PVOID Buffer[MAX_STACK_DEPTH];

	//
	// Valid pointer can be smaller than 64KB
	//

	#define PTR_64KB ((ULONG_PTR)(1024 * 64))

	Count = min(Count, MAX_STACK_DEPTH);

#if defined (_M_IX86)
	CapturedDepth = BtrWalkFrameChain((ULONG_PTR)Fp, Count, Buffer, Hash);
#elif defined (_M_X64)
	CapturedDepth = (*BtrStackTraceRoutine)(3, Count, Buffer, Hash);
#endif

	if (CapturedDepth == MAX_STACK_DEPTH) {
		CapturedDepth -= 1;
	}
	
	if (CapturedDepth < 3) {
		CapturedDepth = 0; 
	}

	ValidDepth = 1;
	Frame[0] = ProbeAddress;

	for(Depth = 0; Depth < CapturedDepth; Depth += 1) {

		Address = (ULONG_PTR)Buffer[Depth];			
		if (Address < PTR_64KB) {
			continue;
		}

		if (BtrFilterStackTrace(Address)) {
			Frame[ValidDepth] = (PVOID)Address;
			ValidDepth += 1;
		}
	}

	*CapturedCount = ValidDepth;
	return TRUE;
}

#define IS_FILTER_DLL(_F, _A) \
	(_A >= (ULONG_PTR)_F->BaseOfDll && _A <= ((ULONG_PTR)_F->BaseOfDll + _F->SizeOfImage))

#define IS_RUNTIME_DLL(_A) \
	(_A >= BtrDllBase && _A <= BtrDllBase + BtrDllSize)

FORCEINLINE
BOOLEAN
BtrFilterStackTrace(
	IN ULONG_PTR Address
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_FILTER Filter;

	if (IS_RUNTIME_DLL((ULONG_PTR)Address)) {
		return FALSE;
	}

	ListEntry = BtrFilterList.Flink;
	while (ListEntry != &BtrFilterList) {
		Filter = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);
		if (IS_FILTER_DLL(Filter, (ULONG_PTR)Address)) {
			return FALSE;
		}
		ListEntry = ListEntry->Flink;
	}

	return TRUE;
}

ULONG
BtrWalkFrameChain(
	IN ULONG_PTR Fp,
    IN ULONG Count, 
    OUT PVOID *Callers,
	OUT PULONG Hash
    )
{
	ULONG_PTR StackHigh;
	ULONG_PTR StackLow;
    ULONG_PTR BackwardFp;
	ULONG_PTR ReturnAddress;
	ULONG Number;

	*Hash = 0;
	StackLow = (ULONG_PTR)&Fp;
	StackHigh = (ULONG_PTR)HalGetCurrentStackBase();

	for (Number = 0; Number < Count; Number += 1) {

		if (Fp < StackLow || Fp > StackHigh) {
			break;
		}

		BackwardFp = *(PULONG_PTR)Fp;
		ReturnAddress = *((PULONG_PTR)(Fp + sizeof(ULONG_PTR)));

		if (Fp >= BackwardFp || BackwardFp > StackHigh) {
			break;
		}

		if (StackLow < ReturnAddress && ReturnAddress < StackHigh) {
			break;
		}

		Callers[Number] = (PVOID)ReturnAddress;
		*Hash += PtrToUlong(Callers[Number]);
		Fp = BackwardFp;
	}

	return Number;
}

PBTR_STACKTRACE_ENTRY
BtrAllocateStackTraceLookaside(
	VOID
	)
{
	PSLIST_ENTRY ListEntry;
	PBTR_STACKTRACE_ENTRY Entry;

	ListEntry = InterlockedPopEntrySList(&BtrStackTraceLookaside);
	if (ListEntry != NULL) {
		return (PBTR_STACKTRACE_ENTRY)ListEntry;
	}
	else {
		Entry = (PBTR_STACKTRACE_ENTRY)_aligned_malloc(sizeof(BTR_STACKTRACE_ENTRY), 16);
		return Entry;
	}
}

VOID
BtrFreeStackTraceLookaside(
	IN PBTR_STACKTRACE_ENTRY Entry
	)
{
	if (QueryDepthSList(&BtrStackTraceLookaside) < BtrStackTraceLookasideThreshold) {
		InterlockedPushEntrySList(&BtrStackTraceLookaside, &Entry->ListEntry);
	}
	else {
		_aligned_free(Entry);
	}
}