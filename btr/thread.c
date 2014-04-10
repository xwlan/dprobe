//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "btr.h"
#include "thread.h"
#include "heap.h"
#include "util.h"
#include "probe.h"
#include "hal.h"
#include "ipc.h"
#include "sched.h"
#include "trap.h"
#include <process.h>
#include "cache.h"
#include "call.h"

LIST_ENTRY BtrActiveThreadList;
LIST_ENTRY BtrRundownThreadList;
ULONG BtrActiveThreadCount;
BTR_LOCK BtrThreadLock;

DWORD BtrSystemThreadId[ThreadNumber];
HANDLE BtrSystemThread[ThreadNumber];
BTR_QUEUE BtrSystemQueue[ThreadNumber];
PTHREAD_START_ROUTINE BtrSystemProcedure[ThreadNumber];

PBTR_THREAD BtrNullThread;
DWORD BtrTlsIndex;

SLIST_HEADER BtrThreadLookaside;
SLIST_HEADER BtrThreadRundownQueue;

ULONG 
BtrInitializeThread(
	VOID
	)
{
	ULONG Status;
	ULONG Number;
	ULONG Count;

	//
	// Allocate tls index for thread state management
	//

	BtrTlsIndex = TlsAlloc();
	if (BtrTlsIndex == TLS_OUT_OF_INDEXES) {
		return S_FALSE;
	} 

	BtrInitLock(&BtrThreadLock);
	InitializeListHead(&BtrActiveThreadList);
	InitializeListHead(&BtrRundownThreadList);
	
	//
	// Initialize thread object lookaside
	// and thread rundown queue
	//

	InitializeSListHead(&BtrThreadLookaside);
	InitializeSListHead(&BtrThreadRundownQueue);

	//
	// Initialize file structures.
	//

	BtrInitializeCache();

	//
	// Initialize the shared thread object, it's used for threads without a
	// proper thread object, all threads attached to shared thread object
	// are exempted from probes.
	//
	
	BtrNullThread = BtrAllocateThread(NullThread);
	if (!BtrNullThread) {
		return S_FALSE;
	}

	//
	// Create queues for system threads
	//

	for(Number = 0; Number < ThreadNumber; Number += 1) {
		Status = BtrCreateQueue(&BtrSystemQueue[Number], Number);
		if (Status != S_OK) {
			break;
		}
	}

	if (Status != S_OK) {
		Count = Number;
		for(Number = 0; Number < Count; Number += 1) {
			BtrDestroyQueue(&BtrSystemQueue[Number]);
		}
		return S_FALSE;
	}

	BtrSystemProcedure[ThreadWrite]    = BtrWriteProcedure;
	BtrSystemProcedure[ThreadWatch]    = BtrWatchProcedure;
	BtrSystemProcedure[ThreadSchedule] = BtrScheduleProcedure;
	BtrSystemProcedure[ThreadMessage]  = BtrMessageProcedure;

	for(Number = 0; Number < ThreadNumber; Number += 1) {

		BtrSystemThread[Number] = (HANDLE)CreateThread(NULL, 0, BtrSystemProcedure[Number], 
													   NULL, CREATE_SUSPENDED, 
													   &BtrSystemThreadId[Number]);
		if (!BtrSystemThread[Number]) {
			Status = S_FALSE;
			break;
		}
	}

	if (Status != S_OK) {

		Count = Number;

		for(Number = 0; Number < Count; Number += 1) {
			TerminateThread(BtrSystemThread[Number], 0);
			CloseHandle(BtrSystemThread[Number]);
		}
		
		BtrUninitializeHal();
		BtrUninitializeMm();
		
		if (FlagOn(BtrFeatures, FeatureRemote)) {
			FreeLibraryAndExitThread((HMODULE)BtrDllBase, S_FALSE);
		}

		return S_FALSE;
	}
	
	ResumeThread(BtrSystemThread[ThreadWrite]);
	ResumeThread(BtrSystemThread[ThreadWatch]);
	
	//
	// Only remote mode require messaging service.
	//

	if (!FlagOn(BtrFeatures, FeatureRemote)) {
		TerminateThread(BtrSystemThread[ThreadMessage], 0);
		CloseHandle(BtrSystemThread[ThreadMessage]);
		BtrSystemThread[ThreadMessage] = NULL;
	} else {
		ResumeThread(BtrSystemThread[ThreadMessage]);
	}

	ResumeThread(BtrSystemThread[ThreadSchedule]);
	return S_OK;
}

VOID
BtrUninitializeThread(
	VOID
	)
{
	BtrCleanThreadLookaside();
}

PBTR_TEB 
BtrGetCurrentTeb(
	VOID
	)
{
	return (PBTR_TEB)NtCurrentTeb();
}

PBTR_THREAD
BtrAllocateThreadLookaside(
	VOID
	)
{
	PSLIST_ENTRY ListEntry;
	PBTR_THREAD Thread;

	ListEntry = InterlockedPopEntrySList(&BtrThreadLookaside);
	if (ListEntry != NULL) {
		Thread = CONTAINING_RECORD(ListEntry, BTR_THREAD, SListEntry);
		RtlZeroMemory(Thread, sizeof(BTR_THREAD));
		return Thread;
	}

	Thread = (PBTR_THREAD)_aligned_malloc(sizeof(BTR_THREAD), MEMORY_ALLOCATION_ALIGNMENT);
	RtlZeroMemory(Thread, sizeof(BTR_THREAD));
	return Thread;
}

VOID
BtrFreeThreadLookaside(
	IN PBTR_THREAD Thread
	)
{
	//
	// N.B. We don't use threshold to measure whether to free it to crt heap,
	// it's supposed that thread number is limited typically.
	//

	InterlockedPushEntrySList(&BtrThreadLookaside, &Thread->SListEntry);
}

VOID
BtrCleanThreadLookaside(
	VOID
	)
{
	PSLIST_ENTRY ListEntry;
	PBTR_THREAD Thread;

	__try {

		while (ListEntry = InterlockedPopEntrySList(&BtrThreadLookaside)) {
			Thread = CONTAINING_RECORD(ListEntry, BTR_THREAD, SListEntry);
			_aligned_free(Thread);
		}

	}__except(EXCEPTION_EXECUTE_HANDLER) {

		//
		// do nothing
		//
	}
}

VOID
BtrQueueRundownThread(
	IN PBTR_THREAD Thread
	)
{
	ASSERT(FlagOn(Thread->ThreadFlag, BTR_FLAG_RUNDOWN));
	InterlockedPushEntrySList(&BtrThreadRundownQueue, &Thread->SListEntry);
}

PBTR_THREAD
BtrAllocateThread(
	IN BTR_THREAD_TYPE Type	
	)
{
	ULONG Status;
	PBTR_THREAD Thread;

	Thread = (PBTR_THREAD)BtrAllocateThreadLookaside();
	if (!Thread) {
		return NULL;
	}

	RtlZeroMemory(Thread, sizeof(BTR_THREAD));
	Thread->Type = Type;
	Thread->ThreadTag = TRAP_EXEMPTION_COOKIE;
	Thread->ThreadId = GetCurrentThreadId();
	Thread->StackBase = HalGetCurrentStackBase();
	InitializeListHead(&Thread->FrameList);
	
	if (Type == NullThread || Type == SystemThread) {
		Thread->ThreadFlag = BTR_FLAG_EXEMPTION;
		return Thread;
	}

	//
	// N.B. The following is for normal thread can be probed as usual
	//

	Thread->RundownHandle = OpenThread(THREAD_QUERY_INFORMATION|SYNCHRONIZE, 
		                              FALSE, Thread->ThreadId);
	if (!Thread->RundownHandle) {

		Status = S_FALSE;

	} else {

		//
		// Create per thread file and mapping objects
		//

		Status = BtrCreateMappingPerThread(Thread);

		if (Status != S_OK) {
			ASSERT(0);
		}
	}

	//
	// Even it failed, we still make a live thread object
	// attach to current thread and set its exemption flag
	//

	if (Status != S_OK) {

		if (Thread->RundownHandle != NULL) {
			CloseHandle(Thread->RundownHandle);
			Thread->RundownHandle = NULL;
		}

		Thread->ThreadFlag = BTR_FLAG_EXEMPTION;
	} 

	else {
	
		Thread->Buffer = BtrMalloc(4096);
		Thread->BufferBusy = FALSE;
	}

	//
	// Insert thread object into global thread list.
	//

	BtrAcquireLock(&BtrThreadLock);
	InsertHeadList(&BtrActiveThreadList, &Thread->ListEntry);
	BtrActiveThreadCount += 1;
	BtrReleaseLock(&BtrThreadLock);

	return Thread;
}

PBTR_THREAD
BtrGetCurrentThread(
	VOID
	)
{
	PBTR_THREAD Thread;

	Thread = BtrGetTlsValue();
	if (Thread != NULL) {
		return Thread;
	}

	//
	// Temporarily set null thread as current thread object
	// to protect thread allocation process.
	//

	BtrSetTlsValue(BtrNullThread);

	Thread = BtrAllocateThread(NormalThread);
	if (Thread == NULL) {
		return BtrNullThread;
	}

	BtrSetTlsValue(Thread);
	return Thread;
}

ULONG
BtrSuspendProcess(
	OUT PLIST_ENTRY ListHead	
	)
{
	ULONG Status;
	HANDLE Toolhelp;
	THREADENTRY32 ThreadEntry;
	ULONG ProcessId;
	ULONG ThreadId;
	PBTR_CONTEXT Context;
	
	ProcessId = GetCurrentProcessId();
	ThreadId = GetCurrentThreadId();

    Toolhelp = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, ProcessId);
	if (Toolhelp == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		return Status;
	}

	ThreadEntry.dwSize = sizeof(ThreadEntry);
    Status = Thread32First(Toolhelp, &ThreadEntry);
	if (Status != TRUE) {
		Status = GetLastError();
		CloseHandle(Toolhelp);
		return Status;
	}

	Status = S_OK;

	do {

		//
		// Suspend current process's threads except current thread
		//

		if (ThreadEntry.th32OwnerProcessID == ProcessId && ThreadEntry.th32ThreadID != ThreadId) {

			HANDLE ThreadHandle;
			ThreadHandle = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, 
				                      FALSE, ThreadEntry.th32ThreadID);

			if (ThreadHandle != NULL) {

				ULONG Count;

				Count = SuspendThread(ThreadHandle);
				if (Count != (ULONG)-1) {

					CONTEXT Registers;

					Registers.ContextFlags = CONTEXT_FULL;
					Status = GetThreadContext(ThreadHandle, &Registers);

					if (Status != TRUE) {
						Status = GetLastError();
						ResumeThread(ThreadHandle);
						CloseHandle(ThreadHandle);
						break;
					}

					Context = (PBTR_CONTEXT)BtrMalloc(sizeof(BTR_CONTEXT));
					Context->ThreadId = ThreadEntry.th32ThreadID;
					Context->ThreadHandle = ThreadHandle;
					Context->Registers = Registers;

					InsertTailList(ListHead, &Context->ListEntry);
					Status = S_OK;
					
				}

				else {
					Status = GetLastError();
					CloseHandle(ThreadHandle);
					break;
				}
			} 

			else {
				Status = GetLastError();
				break;
			}
		} 

    } while (Thread32Next(Toolhelp, &ThreadEntry));
 
	//
	// If it's not completed success, release all suspended
	// threads
	//

	if (Status != S_OK) {
		BtrResumeProcess(ListHead);
	}

	CloseHandle(Toolhelp);
	return Status;
}

VOID
BtrResumeProcess(
	IN PLIST_ENTRY ListHead
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_CONTEXT Context;

	while (IsListEmpty(ListHead) != TRUE) {
		ListEntry = RemoveHeadList(ListHead);
		Context = CONTAINING_RECORD(ListEntry, BTR_CONTEXT, ListEntry);
		ResumeThread(Context->ThreadHandle);
		CloseHandle(Context->ThreadHandle);
		BtrFree(Context);
	}
}

BOOLEAN
BtrIsExecutingAddress(
	IN PLIST_ENTRY ContextList,
	IN PLIST_ENTRY AddressList
	)
{
	PLIST_ENTRY ContextEntry;
	PLIST_ENTRY AddressEntry;
	PBTR_CONTEXT Context;
	PBTR_ADDRESS_RANGE Range;
	ULONG_PTR ThreadPc;

	//
	// N.B. This routine presume all threads except current thread
	//      are already suspended.
	//

	ASSERT(IsListEmpty(ContextList) != TRUE);
	ASSERT(IsListEmpty(AddressList) != TRUE);

	ContextEntry = ContextList->Flink;
	while (ContextEntry != ContextList) {
		
		Context = CONTAINING_RECORD(ContextEntry, BTR_CONTEXT, ListEntry);

#if defined (_M_IX86)
		ThreadPc = (ULONG_PTR)Context->Registers.Eip;
#elif defined (_M_X64)
		ThreadPc = (ULONG_PTR)Context->Registers.Rip;
#endif

		//
		// Check ThreadPc whether fall into specified address range
		//

		AddressEntry = AddressList->Flink;	
		while (AddressEntry != AddressList) {
			Range = CONTAINING_RECORD(AddressEntry, BTR_ADDRESS_RANGE, ListEntry);
			if (ThreadPc >= Range->Address && ThreadPc < (Range->Address + Range->Size)) {
				return TRUE;
			}
			AddressEntry = AddressEntry->Flink;
		}

		ContextEntry = ContextEntry->Flink;
	}

	return FALSE;
}

BOOLEAN
BtrIsExecutingRuntime(
	IN PLIST_ENTRY ContextList 
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_CONTEXT Context;
	ULONG_PTR ThreadPc;

	ListEntry = ContextList->Flink;

	while (ListEntry != ContextList) {
		Context = CONTAINING_RECORD(ListEntry, BTR_CONTEXT, ListEntry);

#if defined (_M_IX86)
		ThreadPc = Context->Registers.Eip;
#elif defined (_M_X64)
		ThreadPc = Context->Registers.Rip;
#endif

		if (ThreadPc >= BtrDllBase && ThreadPc < (BtrDllBase + BtrDllSize)) {
			return TRUE;
		}

		ListEntry = ListEntry->Flink;
	}

	return FALSE;
}

BOOLEAN
BtrIsExecutingTrap(
	IN PLIST_ENTRY ContextList
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_CONTEXT Context;
	ULONG_PTR ThreadPc;

	ListEntry = ContextList->Flink;

	while (ListEntry != ContextList) {
		Context = CONTAINING_RECORD(ListEntry, BTR_CONTEXT, ListEntry);

#if defined (_M_IX86)
		ThreadPc = Context->Registers.Eip;
#elif defined (_M_X64)
		ThreadPc = Context->Registers.Rip;
#endif

		if (BtrIsTrapPage((PVOID)ThreadPc)) {
			return TRUE;
		}

		ListEntry = ListEntry->Flink;
	}

	return FALSE;
}

BOOLEAN
BtrIsExecutingFilter(
	IN PLIST_ENTRY ContextList 
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_ADDRESS_RANGE Range;
	LIST_ENTRY RangeListHead;
	PBTR_FILTER Filter;
	BOOLEAN Status;

	//
	// If no filter registered, return FALSE
	//

	if (IsListEmpty(&BtrFilterList)) {
		return FALSE;
	}

	//
	// Build address range list of filter dll
	//

	InitializeListHead(&RangeListHead);

	ListEntry = BtrFilterList.Flink;
	while (ListEntry != &BtrFilterList) {

		Filter = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);
		Range = (PBTR_ADDRESS_RANGE)BtrMalloc(sizeof(BTR_ADDRESS_RANGE));
		Range->Address = (ULONG_PTR)Filter->BaseOfDll;
		Range->Size = (ULONG_PTR)Filter->SizeOfImage;

		InsertTailList(&RangeListHead, &Range->ListEntry);
		ListEntry = ListEntry->Flink;
	}

	Status = BtrIsExecutingAddress(ContextList, &RangeListHead);
	return Status;
}

BOOLEAN
BtrIsPendingFrameExist(
	IN PBTR_THREAD Thread
	)
{
	HANDLE ThreadHandle;
	CONTEXT Context;
	ULONG_PTR ThreadPc;
	BOOLEAN Pending = FALSE;

	//
	// N.B. This routine assume target thread is not runtime thread, and it's
	// thread object from global active thread list.
	//

	ThreadHandle = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, 
							  FALSE, Thread->ThreadId);
	//
	// N.B. Not sure it's because security issue, or thread terminated,
	// anyway, we can not access this thread, return FALSE
	//

	if (!ThreadHandle) {
		return FALSE;
	}

	while (TRUE) {

		if (SuspendThread(ThreadHandle) == (ULONG)-1) {

			//
			// Thread is probably terminated
			//

			CloseHandle(ThreadHandle);
			return FALSE;
		}

		Context.ContextFlags = CONTEXT_FULL;
		if (!GetThreadContext(ThreadHandle, &Context)) {

			//
			// Thread is probably terminated, whatever we need
			// add a ResumeThread, it's safe to do so.
			//
				
			ResumeThread(ThreadHandle);
			CloseHandle(ThreadHandle);
			return FALSE;
		}

#if defined (_M_IX86)
		ThreadPc = Context.Eip;
#endif

#if defined (_M_X64)
		ThreadPc = Context.Rip;
#endif

		if (ThreadPc >= BtrDllBase && ThreadPc < (BtrDllBase + BtrDllSize)) {

			//
			// Executing inside runtime, may be operating on frame list, 
			// repeat again to avoid corrupt the thread's frame list
			//

			ResumeThread(ThreadHandle);
			SwitchToThread();

		} else {

			if (IsListEmpty(&Thread->FrameList) != TRUE) {
				Pending = TRUE;
			}

			break;
		}
	}

	ResumeThread(ThreadHandle);
	CloseHandle(ThreadHandle);
	return Pending;
}

BOOLEAN
BtrIsRuntimeThread(
	IN ULONG ThreadId
	)
{
	ULONG Number;

	for(Number = 0; Number < ThreadNumber; Number += 1) {
		if (BtrSystemThreadId[Number] == ThreadId) {
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN
BtrIsExemptedAddress(
	IN ULONG_PTR Address 
	)
{
	if ((ULONG_PTR)Address >= BtrDllBase && (ULONG_PTR)Address < BtrDllBase + BtrDllSize) {
		return TRUE;
	}

	return FALSE;
}

VOID
BtrInitializeRuntimeThread(
	VOID
	)
{
	PBTR_THREAD Thread;

	Thread = BtrAllocateThread(SystemThread);
	if (!Thread) {
		Thread = BtrNullThread;
	}
	BtrSetTlsValue(Thread);
}

ULONG
BtrScanThreadFinal(
	VOID
	)
{
	PBTR_THREAD Thread;
	PLIST_ENTRY ListEntry;

	BtrAcquireLock(&BtrThreadLock);

	while (IsListEmpty(&BtrActiveThreadList) != TRUE) {

		ListEntry = RemoveHeadList(&BtrActiveThreadList);
		Thread = CONTAINING_RECORD(ListEntry, BTR_THREAD, ListEntry);

		BtrCloseMappingPerThread(Thread);
		if (Thread->RundownHandle) {
			CloseHandle(Thread->RundownHandle);
		}

		//
		// Clear its stack base, otherwise we will read bad value 
		// when user probe the active process again.
		//

		BtrSetTlsValue(NULL);
	}

	BtrReleaseLock(&BtrThreadLock);

	DebugTrace("BtrScanThreadFinal: active thread count: %d", BtrActiveThreadCount);
	return S_OK;
}

VOID
BtrClearThreadStack(
	IN PBTR_THREAD Thread
	)
{
	ULONG_PTR Value;

	__try {
	
		Value = *Thread->StackBase;
		if (Value == (ULONG_PTR)TRAP_EXEMPTION_COOKIE) {
			*Thread->StackBase = 0;
		}

		if (((PBTR_THREAD)Value)->ThreadTag == (ULONG_PTR)TRAP_EXEMPTION_COOKIE) {
			*Thread->StackBase = 0;
		}
		
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
	
		//
		// ignore
		//
	}
}

VOID
BtrSetTlsValue(
	IN PBTR_THREAD Thread
	)
{
	PULONG_PTR StackBase;

	StackBase = HalGetCurrentStackBase();
	*(PBTR_THREAD *)StackBase = Thread;
}

PBTR_THREAD
BtrGetTlsValue(
	VOID
	)
{
	PULONG_PTR StackBase;
	PBTR_THREAD Thread;

	StackBase = HalGetCurrentStackBase();
	Thread = *(PBTR_THREAD *)StackBase;
	return Thread;
}

PBTR_THREAD
BtrEncodeHazardPointer(
	IN PBTR_THREAD Thread
	)
{
#if defined(_M_IX86)
	return (PBTR_THREAD)((ULONG_PTR)Thread | 0x80000000);
#elif defined(_M_X64) 
	return (PBTR_THREAD)((ULONG_PTR)Thread | 0x8000000000000000);
#endif
}

BOOLEAN
BtrIsHazardPointer(
	IN PBTR_THREAD Thread
	)
{
#if defined(_M_IX86)
	return (BOOLEAN)((ULONG_PTR)Thread & 0x80000000);
#elif defined(_M_X64) 
	return (BOOLEAN)((ULONG_PTR)Thread & 0x8000000000000000);
#endif
}

PBTR_THREAD
BtrDecodeHazardPointer(
	IN PBTR_THREAD Thread
	)
{
#if defined(_M_IX86)
	return (PBTR_THREAD)((ULONG_PTR)Thread & ~0x80000000);
#elif defined(_M_X64) 
	return (PBTR_THREAD)((ULONG_PTR)Thread & ~0x8000000000000000);
#endif
}