//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "btr.h"
#include "probe.h"
#include "trap.h"
#include "heap.h"
#include "thread.h"
#include "hal.h"
#include "ipc.h"
#include "cache.h"
#include "stktrace.h"
#include "counter.h"
#include "util.h"
#include "call.h"

BTR_PROBE_DATABASE BtrProbeDatabase;

PBTR_PROBE
BtrAllocateProbe(
	IN PVOID Address
	)
{
	ULONG Bucket;
	ULONG ObjectId;
	PBTR_PROBE Probe;
	PBTR_COUNTER_ENTRY Counter;

	ObjectId = BtrAcquireObjectId();
	Counter = BtrAllocateCounter(ObjectId);

	if (!Counter) {
		return NULL;
	}

	Counter->Address = Address;
	Bucket = BtrComputeAddressHash(Address, PROBE_BUCKET_NUMBER);

	Probe = BtrMalloc(sizeof(BTR_PROBE));
	RtlZeroMemory(Probe, sizeof(BTR_PROBE));

	Probe->ObjectId = ObjectId;
	Probe->Counter = Counter;

	InsertHeadList(&BtrProbeDatabase.ListEntry[Bucket], &Probe->ListEntry); 
	return Probe;
}

VOID
BtrFreeProbe(
	IN PBTR_PROBE Probe
	)
{
	RemoveEntryList(&Probe->ListEntry);

	if (Probe->Trap) {
		BtrFreeTrap(Probe->Trap);
	}

	BtrRemoveCounter(Probe->Counter);
	BtrFree(Probe);
}

PBTR_PROBE 
BtrLookupProbe(
	IN PVOID Address
	)
{
	ULONG Bucket;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_PROBE Probe;
	
	Bucket = BtrComputeAddressHash(Address, PROBE_BUCKET_NUMBER);

	ListHead = &BtrProbeDatabase.ListEntry[Bucket];
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Probe = CONTAINING_RECORD(ListEntry, BTR_PROBE, ListEntry);
		if (Probe->Address == Address) {
			return Probe;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

ULONG
BtrRegisterFilterProbe(
	IN PBTR_FILTER Filter,
	IN ULONG Number,
	IN BOOLEAN Activate,
	OUT PVOID *AddressOfProbe,
	OUT PULONG ObjectId,
	OUT PULONG Counter
	)
{
	PBTR_FILTER_PROBE FilterProbe;
	PBTR_PROBE Object;
	ULONG Status;
	PBTR_BITMAP BitMap;
	PBTR_TRAP Trap;
	BTR_DECODE_CONTEXT Decode;
	PVOID Address;
	LONG_PTR Destine;
	BTR_DECODE_FLAG Type;
	ULONG Id;
	LONG_PTR PatchAddress;

	ASSERT(Number < Filter->ProbesCount);

	*AddressOfProbe = NULL;
	*ObjectId = 0;
	*Counter = -1;

	BitMap = (PBTR_BITMAP)&Filter->BitMap;
	FilterProbe = &Filter->Probes[Number];
	Address = FilterProbe->Address;

	Object = BtrLookupProbe((PVOID)Address);
	if (Object != NULL) {

		Id = Object->ObjectId;

		if (Object->Type == PROBE_FAST) {
			return BTR_E_PROBE_COLLISION;
		}

		if (memcmp(&Object->FilterGuid, &Filter->FilterGuid, sizeof(GUID)) != 0) {
			return BTR_E_PROBE_COLLISION;
		}

		if (Activate && (FlagOn(Object->Flags, BTR_FLAG_ON))) {

			//
			// It's already registered and running currently,
			// this should not happen though, it means there's
			// a problem in client side state management.
			//

			*AddressOfProbe = Address;
			*ObjectId = Id;
			*Counter = BtrGetCounterBucket(Object->Counter);
			return S_OK;
		}

		//
		// Enable can not be TRUE here, because if the probe object's
		// BTR_FLAG_ON flag is clear, it won't be in hash table, so
		// Enable must be FALSE here, we can safely do unregister.
		//

		ASSERT(Activate != TRUE);
		ASSERT(FlagOn(Object->Flags, BTR_FLAG_ON));

		Status = BtrUnregisterProbe(Object);
		if (Status == S_OK) {
			BtrClearBit(BitMap, Number);
		}

		*AddressOfProbe = Address;
		*ObjectId = Id;
		*Counter = BtrGetCounterBucket(Object->Counter);
		return Status;

	}

	ASSERT(Activate != FALSE);
	
	//
	// Ensure it's executable address
	//

	if (BtrIsExecutableAddress(Address) != TRUE) {
		return BTR_E_CANNOT_PROBE;
	}

	//
	// Check whether the first instruction is a jump opcode,
	// return it's destine absolute address. If Destine != NULL,
	// we can simply patch the offset to redirect it to BTR,
	// no instruction copy involved.
	//

	Type = BtrComputeDestineAddress(Address, &Destine, &PatchAddress);

	if ((LONG_PTR)Address != PatchAddress) {

		PUCHAR Code;
		Code = (PUCHAR)Address;
		ASSERT(Code[0] == 0xeb);

	}

	if (Type == BTR_FLAG_JMP_UNSUPPORTED) { 

		//
		// Decode target routine to check whether it's probable
		//

		RtlZeroMemory(&Decode, sizeof(Decode));
		Decode.RequiredLength = 5;

		//
		// Decode use patch address if it's jmp rel8
		//

		if (PatchAddress != (LONG_PTR)Address) {
			Status = BtrDecodeRoutine((PVOID)PatchAddress, &Decode);
		} else {
			Status = BtrDecodeRoutine(Address, &Decode);
		}

		if (Status != S_OK || Decode.ResultLength < Decode.RequiredLength ||
			FlagOn(Decode.DecodeFlag, BTR_FLAG_BRANCH)) {
			return BTR_E_CANNOT_PROBE;
		}
	}

	//
	// Allocate trap object to fuse target and runtime
	//

	Trap = BtrAllocateTrap((ULONG_PTR)Address); 
	if (!Trap) {
		return BTR_E_CANNOT_PROBE;
	}

	RtlCopyMemory(Trap, &BtrTrapTemplate, sizeof(BTR_TRAP));
	Trap->TrapFlag = Type;

	if (Type == BTR_FLAG_JMP_UNSUPPORTED) {

		if (PatchAddress != (LONG_PTR)Address) {
			BtrFuseTrap((PVOID)PatchAddress, &Decode, Trap);
		} else {
			BtrFuseTrap(Address, &Decode, Trap);
		}

	} else {

		if (PatchAddress != (LONG_PTR)Address) {
			BtrFuseTrapLite((PVOID)PatchAddress, (PVOID)Destine, Trap);
		} else {
			BtrFuseTrapLite(Address, (PVOID)Destine, Trap);
		}
	}

	//
	// Allocate probe object and its attached trampoline
	//

	Object = BtrAllocateProbe(Address);		
	if (!Object) {
		BtrFreeTrap(Trap);
		return BTR_E_OUTOF_PROBE;
	}

	Object->Type = PROBE_FILTER;
	Object->Address = Address;
	Object->PatchAddress = (PVOID)PatchAddress;
	Object->Trap = Trap;

	//
	// Set FUSELITE flag if any
	//

	if (Type != BTR_FLAG_JMP_UNSUPPORTED) {
		SetFlag(Object->Flags, BTR_FLAG_FUSELITE);
	}

	//
	// N.B. Filter probe don't care call type, it's filter author's responsibility
	// to declare correct call type, here default set as __cdecl
	//

	Object->FilterGuid = Filter->FilterGuid;
	Object->FilterCallback = FilterProbe->FilterCallback;
	Object->FilterBit = Number;

	//
	// Integrated filter requires ReturnType to decode return value.
	//

	Object->ReturnType = FilterProbe->ReturnType;

	Status = BtrApplyProbe(Object);
	if (Status != S_OK) {
		BtrFreeProbe(Object);
	}
	
	*AddressOfProbe = Address;
	*ObjectId = Object->ObjectId;
	*Counter = BtrGetCounterBucket(Object->Counter);
	return Status;
}

ULONG
BtrApplyProbe(
	IN PBTR_PROBE Probe
	)
{
	ULONG Status;
	ULONG Protect;
	PBTR_TRAP Trap;
	LIST_ENTRY ListHead;

	if (!VirtualProtect(Probe->PatchAddress, BtrPageSize, PAGE_EXECUTE_READWRITE, &Protect)) {
		return BTR_E_ACCESSDENIED;
	}

	Trap = Probe->Trap;
	ASSERT(Trap != NULL);

	while (TRUE) {

		InitializeListHead(&ListHead);

		Status = BtrSuspendProcess(&ListHead);
		if (Status != S_OK) {
			VirtualProtect(Probe->PatchAddress, BtrPageSize, Protect, &Protect);
			BtrResumeProcess(&ListHead);
			return Status;
		}

		//
		// Whether there's any thread execute code range, address to hijacked length
		//

		if (BtrIsExecutingRange(&ListHead, Probe->PatchAddress, 
			                    Trap->HijackedLength) != TRUE) {
			break;
		}
		
		BtrResumeProcess(&ListHead);
		SwitchToThread();
	}

	Trap->Probe = Probe;

	//
	// Connect to trap object via a jmp rel32
	//

	*(PUCHAR)Probe->PatchAddress = 0xe9;
	
#if defined (_M_IX86)

	*(PLONG)((PUCHAR)Probe->PatchAddress + 1) = (LONG)((LONG_PTR)&Trap->MovEsp4- ((LONG_PTR)Probe->PatchAddress + 5));
	Trap->Object = Probe;

#endif

#if defined (_M_X64)

	//
	// N.B. AMD64 has no mov r/m64, imm64, we have to use 
	// a pair of mov r/m32, imm32 to assign probe object address.
	//

	Trap->ObjectLowPart = PtrToUlong(Probe);
	Trap->ObjectHighPart = (ULONG)((ULONG64)Probe >> 32);

	*(PLONG)((PUCHAR)Probe->PatchAddress + 1) = (LONG)((LONG_PTR)&Trap->MovRsp8 - ((LONG_PTR)Probe->PatchAddress + 5));

#endif

	if (Trap->HijackedLength > 5) {
		RtlFillMemory((PUCHAR)Probe->PatchAddress + 5, Trap->HijackedLength - 5, 0xcc);
	}

	SetFlag(Probe->Flags, BTR_FLAG_ON);

	VirtualProtect(Probe->PatchAddress, BtrPageSize, Protect, &Protect);
	BtrResumeProcess(&ListHead);

	return S_OK;
}

BOOLEAN
BtrIsExecutingRange(
	IN PLIST_ENTRY ListHead,
	IN PVOID Address,
	IN ULONG Length
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_CONTEXT Context;

	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Context = CONTAINING_RECORD(ListEntry, BTR_CONTEXT, ListEntry);

#if defined (_M_IX86)

		if ((ULONG_PTR)Context->Registers.Eip >= (ULONG_PTR) Address && 
			(ULONG_PTR)Context->Registers.Eip < (ULONG_PTR) Address + (ULONG_PTR)Length) {
			return TRUE;
		}

#elif defined (_M_X64)

		if ((ULONG_PTR)Context->Registers.Rip >= (ULONG_PTR) Address && 
			(ULONG_PTR)Context->Registers.Rip < (ULONG_PTR) Address + (ULONG_PTR)Length) {
			return TRUE;
		}

#endif
		ListEntry = ListEntry->Flink;
	}

	return FALSE;
}

VOID
BtrScanPendingThreads(
	OUT PLIST_ENTRY PendingListHead,
	OUT PLIST_ENTRY CompleteListHead
	)
{
	PBTR_THREAD Thread;
	PLIST_ENTRY ListEntry;
	ULONG Terminated = 0;
	ULONG Complete = 0;
	ULONG Pending = 0;

	//
	// N.B. This routine assume that all probes have been unregistered.
	//

	InitializeListHead(PendingListHead);
	InitializeListHead(CompleteListHead);

	while (IsListEmpty(&BtrActiveThreadList) != TRUE) {

		ListEntry = RemoveHeadList(&BtrActiveThreadList);
		Thread = CONTAINING_RECORD(ListEntry, BTR_THREAD, ListEntry);

		if (Thread->RundownHandle != NULL) {

			if (BtrIsThreadTerminated(Thread)) {
				BtrCloseMappingPerThread(Thread);
				CloseHandle(Thread->RundownHandle);
				Terminated += 1;
				continue;
			}
		}

		if (BtrIsPendingFrameExist(Thread)) {
			InsertHeadList(PendingListHead, &Thread->ListEntry);
			Pending += 1;
		} else {
			InsertHeadList(CompleteListHead, &Thread->ListEntry);
			Complete += 1;
		}
	}
}

ULONG
BtrIsUnloading(
	VOID
	)
{
	ULONG Unloading;

	Unloading = ReadForWriteAccess(&BtrUnloading);
	return Unloading;
}

ULONG
BtrUnloadRuntime(
	VOID
	)
{
	PBTR_THREAD Thread;
	LIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	LIST_ENTRY RangeListHead;
	LIST_ENTRY PendingListHead;
	LIST_ENTRY CompleteListHead;
	PBTR_ADDRESS_RANGE Range;

	//
	// Scan pending thread list, note that we only need scan once,
	// because all probes are unregistered, new thread don't get
	// a chance to enter runtime.
	//
	
	BtrAcquireLock(&BtrThreadLock);
	BtrScanPendingThreads(&PendingListHead, &CompleteListHead);
	BtrReleaseLock(&BtrThreadLock);

	//
	// Repeatly query until all pending frames are completed, in some
	// special cases, this will run forever, e.g. a thread is probing
	// a top level routine, which is always pending before the thread exit, 
	// we have no chance to immediately unload runtime in this case,
	// technically we have method to walk the stack frame and hijack
	// its return address, but it's very complicated to do so, it's after
	// all a rare case.
	//

	while (IsListEmpty(&PendingListHead) != TRUE) {

		ListEntry = RemoveHeadList(&PendingListHead);
		Thread = CONTAINING_RECORD(ListEntry, BTR_THREAD, ListEntry);

		if (BtrIsPendingFrameExist(Thread)) {
			InsertTailList(&PendingListHead, &Thread->ListEntry);
			Sleep(1000);

		} else {
			InsertTailList(&CompleteListHead, &Thread->ListEntry);
		}
	}

	//
	// Close left threads' file and mapping objects.
	//

	while (IsListEmpty(&CompleteListHead) != TRUE) {

		ListEntry = RemoveHeadList(&CompleteListHead);
		Thread = CONTAINING_RECORD(ListEntry, BTR_THREAD, ListEntry);

		BtrCloseMappingPerThread(Thread);

		if (Thread->RundownHandle != NULL) {

			if (BtrIsThreadTerminated(Thread)) {
				//
				// do nothing, don't worry dirty stack since its deallocated.
				//
			} else {
				BtrClearThreadStack(Thread);
			}
			CloseHandle(Thread->RundownHandle);

		} else {

			//
			// Without rundown handle, we in fact don't know thread state,
			// we have to clear its stack cookie, this is a rare rare case,
			// I never see it till now.
			//

			BtrClearThreadStack(Thread);
		}
	}

	//
	// Ensure all threads are not executing trap code, not executing
	// runtime code
	//

	InitializeListHead(&RangeListHead);

	ListEntry = BtrTrapPageList.Flink;
	while (ListEntry != &BtrTrapPageList) {

		PBTR_TRAP_PAGE Page;
		Page = CONTAINING_RECORD(ListEntry, BTR_TRAP_PAGE, ListEntry);	

		Range = BtrMalloc(sizeof(BTR_ADDRESS_RANGE));
		Range->Address = (ULONG_PTR)Page->StartVa;
		Range->Size = (ULONG_PTR)Page->EndVa - (ULONG_PTR)Page->StartVa;
		InsertTailList(&RangeListHead, &Range->ListEntry);

		ListEntry = ListEntry->Flink;
	}

	//
	// Append runtime address range, note that we don't need check filter
	// address, because there's no pending frame, so the thread pc can not
	// be possible in address range of filters.
	//

	Range = BtrMalloc(sizeof(BTR_ADDRESS_RANGE));
	Range->Address = BtrDllBase;
	Range->Size = BtrDllSize; 
	InsertHeadList(&RangeListHead, &Range->ListEntry);

	InitializeListHead(&ListHead);
	
	while (BtrSuspendProcess(&ListHead) != S_OK) {
		SwitchToThread();
	}

	while (BtrIsExecutingAddress(&ListHead, &RangeListHead)) {
		BtrResumeProcess(&ListHead);
		Sleep(5000);
		BtrSuspendProcess(&ListHead);
	}

	BtrResumeProcess(&ListHead);

	BtrDeleteLock(&BtrThreadLock);

	//
	// Unload all registered filters
	//

	BtrUnregisterFilters();
	
	//
	// Free trap page list, note that this must be done before
	// heap destroy because trap page's bitmap object is allocated
	// from normal heap.
	//

	BtrUninitializeTrap();
	BtrUninitializeProbe();
	BtrUninitializeCounter();
	BtrUninitializeStackTrace();
	BtrUninitializeCache();
	BtrUninitializeHal();
	BtrUninitializeMm();
	
	if (FlagOn(BtrFeatures, FeatureRemote)) {
		BtrExecuteUnloadRoutine();
		ExitThread(0);

		//
		// never reach here
		//
	}

	//
	// local mode, return here
	//

	return S_OK;
}

ULONG
BtrUnregisterProbe(
	IN PBTR_PROBE Probe 
	)
{
	ULONG Status;
	ULONG Protect;
	LIST_ENTRY ListHead;
	PBTR_TRAP Trap;
	PVOID CopyFrom;

	InitializeListHead(&ListHead);
	Status = BtrSuspendProcess(&ListHead);
	if (Status != S_OK) {
		return Status;
	}

	//
	// Wait until all the following three condition are met:
	// 1, probe object's reference count is 0, this ensure
	//    that thread's pc is in controllable area
	// 2, thread is not executing btr dll code
	// 3, thread is not executing trap code
	//
	// if all three conditions are met, it's safe to decommit
	//

	while ( Probe->References != 0           ||
		    BtrIsExecutingRuntime(&ListHead) || 
		    BtrIsExecutingTrap(&ListHead) ) {
	
		BtrResumeProcess(&ListHead);
		SwitchToThread();

		Status = BtrSuspendProcess(&ListHead);
		if (Status != S_OK) {
			return Status;
		}
	}

	Status = VirtualProtect(Probe->PatchAddress, BtrPageSize, PAGE_EXECUTE_READWRITE, &Protect);
	if (Status != TRUE) {
		Status = GetLastError();
		BtrResumeProcess(&ListHead);
		return Status;
	}

	//
	// Rollback hijacked code to target routine
	//

	Trap = Probe->Trap;

	if (FlagOn(Trap->TrapFlag, BTR_FLAG_JMP_UNSUPPORTED)) {
		CopyFrom = Trap->OriginalCopy;
	} else {
		CopyFrom = Trap->OriginalCopy;
	}

	RtlCopyMemory(Probe->PatchAddress, CopyFrom, Trap->HijackedLength);
	VirtualProtect(Probe->PatchAddress, BtrPageSize, Protect, &Protect);

	BtrFreeProbe(Probe);

	BtrResumeProcess(&ListHead);
	return S_OK;
}

VOID
BtrUnregisterProbes(
	VOID
	)
{
	PBTR_PROBE Probe;
	PBTR_TRAP Trap;
	ULONG Protect;
	ULONG Number;
	HANDLE ProcessHandle;
	PUCHAR CopyFrom;
	LIST_ENTRY ProcessList;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;

	InitializeListHead(&ProcessList);
	BtrSuspendProcess(&ProcessList);

	ProcessHandle = GetCurrentProcess();

	for (Number = 0; Number < BtrProbeDatabase.NumberOfBuckets; Number += 1) {

		ListHead = &BtrProbeDatabase.ListEntry[Number];

		while (IsListEmpty(ListHead) != TRUE) {

			ListEntry = RemoveHeadList(ListHead);
			Probe = CONTAINING_RECORD(ListEntry, BTR_PROBE, ListEntry);

			Trap = Probe->Trap;
			ASSERT(Trap != NULL);

			VirtualProtect(Probe->PatchAddress, BtrPageSize, PAGE_EXECUTE_READWRITE, &Protect);

			__try {

				if (!FlagOn(Probe->Flags, BTR_FLAG_FUSELITE)) {
					CopyFrom = Trap->OriginalCopy;
				}
				else {
					CopyFrom = Trap->OriginalCopy;
				}

				RtlCopyMemory(Probe->PatchAddress, CopyFrom, Trap->HijackedLength);
				FlushInstructionCache(ProcessHandle, Probe->PatchAddress, Trap->HijackedLength);

			} __except(EXCEPTION_EXECUTE_HANDLER) {

			}

			VirtualProtect(Probe->PatchAddress, BtrPageSize, PAGE_EXECUTE_READWRITE, &Protect);
		}
	}

	BtrResumeProcess(&ProcessList);
}

ULONG
BtrRegisterFastProbe(
	IN PVOID Address,
	IN BOOLEAN Activate,
	OUT PULONG ObjectId,
	OUT PULONG Counter
	)
{
	PBTR_PROBE Object;
	ULONG Status;
	PBTR_TRAP Trap;
	BTR_DECODE_CONTEXT Decode;
	BTR_DECODE_FLAG Type;
	LONG_PTR Destine;
	ULONG Id;
	LONG_PTR PatchAddress;

	*ObjectId = 0;
	*Counter = -1;

	Object = BtrLookupProbe(Address);
	if (Object != NULL) {

		Id = Object->ObjectId;

		if (Object->Type != PROBE_FAST) {
			return BTR_E_PROBE_COLLISION;
		}

		if (!Activate) {
			*Counter = BtrGetCounterBucket(Object->Counter);
			Status = BtrUnregisterProbe(Object);
			*ObjectId = Id;
			return Status;
		}

		*ObjectId = Id;
		*Counter = BtrGetCounterBucket(Object->Counter);
		return S_OK;
	}

	//
	// Ensure it's executable address
	//

	if (BtrIsExecutableAddress(Address) != TRUE) {
		return BTR_E_CANNOT_PROBE;
	}

	//
	// Check whether the first instruction is a jump opcode,
	// return it's destine absolute address. If Destine != NULL,
	// we can simply patch the offset to redirect it to BTR,
	// no instruction copy involved.
	//

	Type = BtrComputeDestineAddress(Address, &Destine, &PatchAddress);

	if (Type == BTR_FLAG_JMP_UNSUPPORTED) { 

		//
		// Decode target routine to check whether it's probable
		//

		RtlZeroMemory(&Decode, sizeof(Decode));
		Decode.RequiredLength = 5;

		//
		// Decode use patch address if it's jmp rel8
		//

		if (PatchAddress != (LONG_PTR)Address) {
			Status = BtrDecodeRoutine((PVOID)PatchAddress, &Decode);
		} else {
			Status = BtrDecodeRoutine(Address, &Decode);
		}

		if (Status != S_OK || Decode.ResultLength < Decode.RequiredLength ||
			FlagOn(Decode.DecodeFlag, BTR_FLAG_BRANCH)) {
			return BTR_E_CANNOT_PROBE;
		}

	}

	//
	// Allocate trap object to fuse target and runtime
	//

	Trap = BtrAllocateTrap((ULONG_PTR)Address);
	if (!Trap) {
		return BTR_E_CANNOT_PROBE;
	}

	RtlCopyMemory(Trap, &BtrTrapTemplate, sizeof(BTR_TRAP));
	Trap->TrapFlag = Type;

	if (Type == BTR_FLAG_JMP_UNSUPPORTED) {

		if (PatchAddress != (LONG_PTR)Address) {
			BtrFuseTrap((PVOID)PatchAddress, &Decode, Trap);
		} else {
			BtrFuseTrap(Address, &Decode, Trap);
		}

	} else {

		if (PatchAddress != (LONG_PTR)Address) {
			BtrFuseTrapLite((PVOID)PatchAddress, (PVOID)Destine, Trap);
		} else {
			BtrFuseTrapLite(Address, (PVOID)Destine, Trap);
		}
	}

	//
	// Allocate probe object
	//

	Object = BtrAllocateProbe(Address);		
	if (!Object) {
		BtrFreeTrap(Trap);
		return BTR_E_OUTOF_PROBE;
	}

	Object->Type = PROBE_FAST;
	Object->Flags = 0;
	Object->References = 0;
	Object->Address = Address;
	Object->PatchAddress = (PVOID)PatchAddress;
	Object->ReturnType = ReturnUIntPtr;
	Object->Trap = Trap;
	Object->FilterCallback = NULL;
	Object->FilterBit = 0;
	RtlZeroMemory(&Object->FilterGuid, sizeof(GUID));

	//
	// Set FUSELITE flag if any
	//

	if (Type != BTR_FLAG_JMP_UNSUPPORTED) {
		SetFlag(Object->Flags, BTR_FLAG_FUSELITE);
	}

	Status = BtrApplyProbe(Object);
	if (Status != S_OK) {
		BtrFreeProbe(Object);
		return Status;
	}

	*ObjectId = Object->ObjectId;
	*Counter = BtrGetCounterBucket(Object->Counter);
	return S_OK;
}

ULONG
BtrInitializeProbe(
	VOID
	)
{
	ULONG Number;

	BtrProbeDatabase.NumberOfBuckets = PROBE_BUCKET_NUMBER;
	for(Number = 0; Number < PROBE_BUCKET_NUMBER; Number += 1) {
		InitializeListHead(&BtrProbeDatabase.ListEntry[Number]);
	}

	return S_OK;
}

VOID
BtrUninitializeProbe(
	VOID
	)
{
	return;	
}

