//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "btr.h"
#include "queue.h"
#include "hal.h"
#include "ipc.h"
#include "list.h"
#include "thread.h"
#include "heap.h"
#include "util.h"
#include "heap.h"
#include "status.h"
#include "sched.h"
#include <process.h>
#include "cache.h"
#include "counter.h"
#include "stktrace.h"

//
// N.B. This list hold pending send records under high io throughput,
// each time records are dequeued, we should first check this list
// to dequeue the records not yet sent last time.
//

LIST_ENTRY BtrQueueBufferList;
LIST_ENTRY BtrOrderedRecordList;
LIST_ENTRY BtrThreadQueueList;
BTR_LOCK BtrRecordQueueLock;

ULONG BtrRecordCounter;
ULONG BtrRecordCurrentCount;

DECLSPEC_CACHEALIGN
BTR_BUFFER_LOOKASIDE BtrBufferLookaside;

ULONG
BtrInitializeQueue(
	VOID
	)
{
	BtrRecordCounter = 0;
	BtrInitLockEx(&BtrRecordQueueLock, 4000);
	InitializeListHead(&BtrOrderedRecordList);
	InitializeListHead(&BtrThreadQueueList);

	InitializeListHead(&BtrQueueBufferList);
	return S_OK;
}

ULONG
BtrQueueSharedCacheRecord(
	IN PBTR_THREAD Thread,
	IN PBTR_RECORD_HEADER Header,
	IN PBTR_PROBE Probe
	)
{
	ULONG Status;
	ULONG64 Offset;
	ULONG64 Location;

	ASSERT(Thread->IndexMapping && Thread->DataMapping);

	//
	// Acquire the record's sequence number
	//

	Header->Sequence = _InterlockedIncrement(&BtrSharedData->Sequence);

	Status = BtrWriteSharedCacheRecord(Thread, Header, &Offset);
	if (Status != S_OK) {
		return Status;
	}

	Location = Header->Sequence * sizeof(BTR_FILE_INDEX);
	Status = BtrWriteSharedCacheIndex(Thread, Offset, Location, Header->TotalLength);
	if (Status != S_OK) {
		return Status;
	}

	//
	// Update the performance counters
	//

	BtrUpdateCounter(Probe->Counter, Header);
	return Status;
}

ULONG
BtrDeQueueRecord(
	IN PVOID Buffer,
	IN ULONG BufferLength,
	OUT PULONG Length,
	OUT PULONG Count
	)
{
	*Length = 0;
	*Count = 0;
	return S_OK;
}

ULONG
BtrDeQueueStackTrace(
	IN PVOID Buffer, 
	IN ULONG BufferLength,
	OUT PULONG Length,
	OUT PULONG Count
	)
{
	ULONG Status;
	ULONG CopyLength;
	ULONG NumberOfTraces;
	PSLIST_ENTRY ListEntry;
	PBTR_STACKTRACE_ENTRY Trace;

	Status = S_OK;
	CopyLength = 0;
	NumberOfTraces = 0;

	while (ListEntry = InterlockedPopEntrySList(&BtrStackTraceUpdateQueue)) {

		Trace = CONTAINING_RECORD(ListEntry, BTR_STACKTRACE_ENTRY, ListEntry);

		if (CopyLength + sizeof(BTR_STACKTRACE_ENTRY) > BufferLength) {
			Status = NumberOfTraces ? BTR_S_MORE_DATA : BTR_E_BUFFER_LIMITED;
			InterlockedPushEntrySList(&BtrStackTraceUpdateQueue, &Trace->ListEntry);
			break;
		}

		RtlCopyMemory(Buffer, Trace, sizeof(BTR_STACKTRACE_ENTRY));
		Buffer = (PCHAR)Buffer + sizeof(BTR_STACKTRACE_ENTRY);
		CopyLength = CopyLength + sizeof(BTR_STACKTRACE_ENTRY);
		NumberOfTraces += 1;

		BtrFreeStackTraceLookaside(Trace);

	} 

	*Length = CopyLength;
	*Count = NumberOfTraces;
	return Status;

}

ULONG
BtrDeQueueThreadQueue(
	IN PVOID Buffer,
	IN ULONG BufferLength,
	OUT PULONG Length,
	OUT PULONG Count
	)
{
	return S_FALSE;
}

ULONG CALLBACK
BtrWatchProcedure(
	IN PVOID Context
	)
{
	ULONG Status;
	PBTR_NOTIFICATION_PACKET Notification;
	PBTR_CONTROL Packet;
	ULONG ControlCode;
	
	BtrInitializeRuntimeThread();

	__try {

		while (TRUE) {

			Status = BtrDeQueueNotification(&BtrSystemQueue[ThreadWatch], 
											INFINITE, &Notification);
			if (Status != S_OK) {
				break;
			}

			Packet = (PBTR_CONTROL)Notification->Packet;
			ControlCode = Packet->ControlCode;

			switch (ControlCode) {

			case SCHED_SCAN_RUNDOWN:
				Packet->Complete = TRUE;
				break;	

			case SCHED_SCAN_LOOKASIDE:

				//
				// N.B. Lookaside and thread rundown share same quantum
				//

				BtrScanLookaside();
				BtrScanRundown();
				Packet->Complete = TRUE;
				break;

			case SCHED_SCAN_STACKTRACE:
				BtrScanStackTrace();
				Packet->Complete = TRUE;
				break;

			case SCHED_TRIM_WORKINGSET:
				BtrTrimWorkingSet();
				Packet->Complete = TRUE;
				break;

			case SCHED_STOP_RUNTIME:
				Packet->Complete = TRUE;
				return 0;
			}

			if (Packet->FreeRequired) {
				BtrFree(Notification);
				BtrFree(Packet);
			}
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

		//
		// If any exception occurs, we need request to stop runtime.
		//
	}
	
	BtrRequestStop(ThreadWatch);
	return 0;
}

VOID
BtrTrimWorkingSet(
	VOID
	)
{

}

ULONG
BtrScanRundown(
	VOID
	)
{
	PBTR_THREAD Thread;
	PSLIST_ENTRY ListEntry;
	SLIST_HEADER ActiveListHead;

	InitializeSListHead(&ActiveListHead);

	while (ListEntry = InterlockedPopEntrySList(&BtrThreadRundownQueue)) {

		Thread = CONTAINING_RECORD(ListEntry, BTR_THREAD, SListEntry);

		if (Thread->RundownHandle != NULL) {

			if (BtrIsThreadTerminated(Thread)) {

				BtrCloseMappingPerThread(Thread);
				CloseHandle(Thread->RundownHandle);
				Thread->RundownHandle = NULL;

				if (Thread->Buffer) {
					BtrFree(Thread->Buffer);
					Thread->Buffer = NULL;
				}

				BtrFreeThreadLookaside(Thread);

			} else {
				InterlockedPushEntrySList(&ActiveListHead, &Thread->SListEntry);
			}
		}

		else {
			
			//
			// N.B. do nothing currently, will consider a right solution later
			//
		}

	}

	while (ListEntry = InterlockedPopEntrySList(&ActiveListHead)) {
		InterlockedPushEntrySList(&BtrThreadRundownQueue, ListEntry);
	}

	return S_OK;
}

VOID
BtrScanLookaside(
	VOID
	)
{
	PBTR_LOOKASIDE_STATE State;
	BTR_LOOKASIDE_STATE States[RECORD_LOOKASIDE_NUM + 1];
	ULONG Number;
	
	BtrQueryLookasideState(&States[0], RECORD_LOOKASIDE_NUM + 1);

	for (Number = 0; Number < RECORD_LOOKASIDE_NUM; Number += 1) {
		State = &States[Number];
		DebugTrace("lookaside#%d: totalalloc %d, allocmiss %d totalfree %d, freemiss %d, listdepth %d, curdepth %d",
			Number, State->TotalAllocates, State->AllocateMisses, 
			State->TotalFrees, State->FreeMisses,
			State->ListDepth, State->Depth );
	}

	BtrAdjustLookasideDepth(2);
	return;
}

BOOLEAN
BtrIsThreadTerminated(
	IN PBTR_THREAD Thread 
	)
{
	ULONG Status;

	//
	// N.B. Wait rundown handle to test whether it's terminated,
	// the rundown handle is opened when first build its thread object,
	// so it's guranteed that it's a valid handle. The handle will be
	// closed when thread object is freed, or runtime unloaded.
	//

	Status = WaitForSingleObject(Thread->RundownHandle, 0);
	if (Status == WAIT_OBJECT_0) {
		return TRUE;
	}

	return FALSE;
}

ULONG CALLBACK
BtrWriteProcedure(
	IN PVOID Context
	)
{
	ULONG Status;
	PBTR_NOTIFICATION_PACKET Notification;
	PBTR_CONTROL Packet;
	BTR_CONTROLCODE ControlCode;
	SYSTEMTIME Time;

	BtrInitializeRuntimeThread();

	__try {

		while (TRUE) {

			Status = BtrDeQueueNotification(&BtrSystemQueue[ThreadWrite], 
				                            INFINITE, &Notification);
			if (Status != S_OK) {
				break;
			}

			Packet = (PBTR_CONTROL)Notification->Packet;
			ControlCode = Packet->ControlCode;

			switch (ControlCode) {

			case SCHED_WRITE_RECORD:
			case SCHED_FLUSH_STACKTRACE:
				BtrFlushStackTrace();
				break;

			case SCHED_STOP_RUNTIME:
				GetLocalTime(&Time);
				return 0;
			}

			if (Packet->FreeRequired) {
				BtrFree(Notification);
				BtrFree(Packet);
			}
		}

	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

		//
		// If any exception occurs, we need request to stop runtime.
		//
	}

	BtrRequestStop(ThreadWrite);
	return 0;
}