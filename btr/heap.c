//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "btr.h"
#include "util.h"
#include "heap.h"
#include "hal.h"
#include "queue.h"

DECLSPEC_CACHEALIGN
HANDLE BtrHeap[MaximumHeap];

DECLSPEC_CACHEALIGN
BTR_LOOKASIDE BtrLookaside[RECORD_LOOKASIDE_NUM + 1];

ULONG BtrLookasideFlag;

PVOID 
BtrMalloc(
	IN ULONG ByteCount
	)
{
	PVOID Pointer;
	SIZE_T Size;

	Size = (SIZE_T)BtrUlongPtrRoundUp((ULONG_PTR)ByteCount, 
		                              (ULONG_PTR)MEMORY_ALLOCATION_ALIGNMENT);

	Pointer = HeapAlloc(BtrHeap[SystemHeap], HEAP_ZERO_MEMORY, Size);
	return Pointer;
}

VOID 
BtrFree(
	IN PVOID Address
	)
{
	__try {
		HeapFree(BtrHeap[SystemHeap], 0, Address);
	} __except(EXCEPTION_EXECUTE_HANDLER) {
	}
}

PVOID
BtrMallocEx(
	IN BTR_HEAP_TYPE Type,
	IN SIZE_T ByteCount 
	)
{
	PVOID Pointer;

	if (Type < SystemHeap || Type > RecordHeap) {
		ASSERT(0);
	}

	Pointer = HeapAlloc(BtrHeap[Type], HEAP_ZERO_MEMORY, ByteCount);
	return Pointer;
}

VOID
BtrFreeEx(
	IN BTR_HEAP_TYPE Type,
	IN PVOID Address
	)
{
	if (Type < SystemHeap || Type > RecordHeap) {
		ASSERT(0);
	}

	__try {
		HeapFree(BtrHeap[Type], 0, Address);
	} __except(EXCEPTION_EXECUTE_HANDLER) {
	}
}

ULONG
BtrInitializeMm(
	ULONG Stage	
	) 
{
	ULONG Status;
	ULONG Number;
	ULONG Count;
	ULONG HeapFragValue = 2;

	//
	// Create various heaps, all requires serilized and align with 16 bytes,
	// enable low fragmentation for all of them.
	//

	Status = S_OK;

	if (Stage == BTR_INIT_STAGE_0) {

		//
		// Library mode only use SystemHeap
		//

		BtrHeap[SystemHeap] = HeapCreate(0, HEAP_CREATE_ALIGN_16, 0);
		if (!BtrHeap[SystemHeap]) {
			return BTR_E_OUTOFMEMORY;
		}

		HeapSetInformation(BtrHeap[SystemHeap], HeapCompatibilityInformation, 
		                   &HeapFragValue, sizeof(HeapFragValue));
		return S_OK;
	}

	//
	// This is BTR_INIT_STAGE_1, for local and remote modes.
	//

	for(Number = SystemHeap; Number < MaximumHeap; Number += 1) {

		BtrHeap[Number] = HeapCreate(0, HEAP_CREATE_ALIGN_16, 0);

		if (!BtrHeap[Number]) {
			Status = BTR_E_OUTOFMEMORY;
			break;
		}
		
		HeapSetInformation(BtrHeap[Number], HeapCompatibilityInformation, 
		                   &HeapFragValue, sizeof(HeapFragValue));
	}

	if (Status != S_OK) {
		Count = Number;
		for(Number = 0; Number < Count; Number += 1) {
			HeapDestroy(BtrHeap[Number]);
		}
		return Status;
	}

	//
	// Initialize lookaside for  record heap, note that we don't create 
	// lookaside for filter heap, it's expected filter heap is used in 
	// low frequency, it's only used for isolation of heap corruption issue.
	//

	for(Number = 0; Number < RECORD_LOOKASIDE_NUM + 1; Number += 1) {

		RtlZeroMemory(&BtrLookaside[Number], sizeof(BTR_LOOKASIDE));
		BtrLookaside[Number].Type = RecordHeap;	
		BtrLookaside[Number].Depth = 0;	
		BtrLookaside[Number].Malloc = BtrMallocEx;
		BtrLookaside[Number].Free = BtrFreeEx;
		if (Number != 0) {
			BtrLookaside[Number].MaximumDepth = 4096;
		} else {
			BtrLookaside[Number].Depth = 4096 * 3;	
			BtrLookaside[Number].MaximumDepth = 4096 * 10;
		}
		InitializeSListHead(&BtrLookaside[Number].ListHead);

	}

	return S_OK;
}

VOID
BtrUninitializeMm(
	VOID
	)
{
	ULONG Number;

	for(Number = 0; Number < (ULONG)MaximumHeap; Number += 1) {
		if (BtrHeap[Number] != NULL) {
			HeapDestroy(BtrHeap[Number]);
		}
	}
}

PVOID
BtrMallocLookaside(
	IN BTR_HEAP_TYPE Type,
	IN ULONG ByteCount
	)
{
	ULONG Size;
	USHORT Slot;
	PVOID Address;
	PSLIST_ENTRY ListEntry;
	PBTR_LOOKASIDE_BLOCK Block;
	PBTR_LOOKASIDE Lookaside;
	
	Size = FIELD_OFFSET(BTR_LOOKASIDE_BLOCK, Block[ByteCount]);
	
	//
	// Compute the lookaside object slot
	//
	
	if (Size >= 1024) {
		Slot = RECORD_LOOKASIDE_NUM;
	}
	else if (Size <= RECORD_MINIMUM_SIZE) {
		Slot = 0;
	} 
	else {
		Slot = (USHORT)((Size - RECORD_MINIMUM_SIZE) / 16 + 1);
	}

	if (Slot >= RECORD_LOOKASIDE_NUM) {

		//
		// It's a large block request, directly allocate from heap
		//

		Address = (*BtrLookaside[RECORD_LOOKASIDE_NUM].Malloc)(RecordHeap, Size);	
		if (!Address) {
			return NULL;
		}

		Block = (PBTR_LOOKASIDE_BLOCK)Address;
		Block->Slot = RECORD_LOOKASIDE_NUM;
		Block->Type = Type;
		Block->Tag = RECORD_LOOKASIDE_TAG;
		BtrLookaside[RECORD_LOOKASIDE_NUM].TotalAllocates += 1;
		return (PVOID)((ULONG_PTR)Block + FIELD_OFFSET(BTR_LOOKASIDE_BLOCK, Block));
	}

	Lookaside = &BtrLookaside[Slot];
	ListEntry = InterlockedPopEntrySList(&Lookaside->ListHead);
	if (ListEntry != NULL) {

		Lookaside->TotalAllocates += 1;
		Lookaside->AllocateHits += 1;

		Block = (PBTR_LOOKASIDE_BLOCK)ListEntry;
		Block->Slot = Slot;
		Block->Type = Type;
		Block->Tag = RECORD_LOOKASIDE_TAG;
		return (PVOID)((ULONG_PTR)Block + FIELD_OFFSET(BTR_LOOKASIDE_BLOCK, Block));
	}

	Address = (*Lookaside->Malloc)(RecordHeap, RECORD_MINIMUM_SIZE + Slot * 16);
	if (!Address) {
		return NULL;
	}

	Lookaside->TotalAllocates += 1;
	Lookaside->AllocateMisses += 1;

	Block = (PBTR_LOOKASIDE_BLOCK)Address;
	Block->Slot = Slot;
	Block->Type = Type;
	Block->Tag = RECORD_LOOKASIDE_TAG;
	return (PVOID)((ULONG_PTR)Block + FIELD_OFFSET(BTR_LOOKASIDE_BLOCK, Block));
}

VOID
BtrFreeLookaside(
	IN BTR_HEAP_TYPE Type,
	IN PVOID Address 
	)
{
	PBTR_LOOKASIDE_BLOCK Block;
	PBTR_LOOKASIDE Lookaside;
	ULONG Offset;

	Offset = FIELD_OFFSET(BTR_LOOKASIDE_BLOCK, Block);
	Block = (PBTR_LOOKASIDE_BLOCK)((PUCHAR)Address - Offset);

	if (FlagOn(BtrLookasideFlag, LOOKASIDE_FREE_CHECK)) {
		if (!BtrCheckLookasideBlock(Block)) {
			__debugbreak();
		}
	}

	if (Block->Slot >= RECORD_LOOKASIDE_NUM) {
		(*BtrLookaside[RECORD_LOOKASIDE_NUM].Free)(Type, Block);
		BtrLookaside[RECORD_LOOKASIDE_NUM].TotalFrees += 1;
		return;
	}

	Lookaside = &BtrLookaside[Block->Slot];

	if (QueryDepthSList(&Lookaside->ListHead) > Lookaside->Depth) {
		(*Lookaside->Free)(Type, Block);
		Lookaside->TotalFrees += 1;
		Lookaside->FreeMisses += 1;
	} else {
		InterlockedPushEntrySList(&Lookaside->ListHead, &Block->ListEntry);
		Lookaside->TotalFrees += 1;
		Lookaside->FreeHits += 1;
	}
}

BOOLEAN
BtrIsMemoryAllocationAligned(
	IN ULONG_PTR Pointer
	)
{
	return !(Pointer & (ULONG_PTR)(MEMORY_ALLOCATION_ALIGNMENT - 1));
}

BOOLEAN
BtrCheckLookasideBlock(
	IN PBTR_LOOKASIDE_BLOCK Block
	)
{
	//
	// If slot beyond the limits of lookaside list,
	// or pointer is not memory allocation aligned,
	// or tag is corrupted, it's considered a bad block.
	//

	if (Block->Slot > RECORD_LOOKASIDE_NUM || 
		!BtrIsMemoryAllocationAligned((ULONG_PTR)Block) ||
		Block->Tag != RECORD_LOOKASIDE_TAG) {

		return FALSE;
	}

	return TRUE;
}

//
// N.B. The lookaside adjustment algorithm is from WRK 2.0
//

VOID
BtrComputeLookasideDepth (
    IN PBTR_LOOKASIDE Lookaside,
    IN ULONG Misses,
    IN ULONG ScanPeriod
    )
{
	ULONG Allocates;
    ULONG Delta;
    USHORT MaximumDepth;
    ULONG Ratio;
    LONG Target;

	//
	// Compute the total number of allocations and misses per second for
	// this scan period.
	//

	Allocates = Lookaside->TotalAllocates - Lookaside->LastTotalAllocates;
	Lookaside->LastTotalAllocates = Lookaside->TotalAllocates;

	//
	// If the allocate rate is less than the minimum threshold, then lower
	// the maximum depth of the lookaside list. Otherwise, if the miss rate
	// is less than 0.5%, then lower the maximum depth. Otherwise, raise the
	// maximum depth based on the miss rate.
	//

	MaximumDepth = Lookaside->MaximumDepth;
	Target = Lookaside->Depth;
	if (Allocates < (ScanPeriod * LOOKASIDE_ALLOCATION_THRESHOLD)) {
		if ((Target -= 10) < LOOKASIDE_MINIMUM_DEPTH) {
			Target = LOOKASIDE_MINIMUM_DEPTH;
		}

	} else {

		//
		// N.B. The number of allocates is guaranteed to be greater than
		//      zero because of the above test. 
		//
		// N.B. It is possible that the number of misses are greater than the
		//      number of allocates, but this won't cause the an incorrect
		//      computation of the depth adjustment.
		//      

		Ratio = (Misses * 1000) / Allocates;
		if (Ratio < 5) {
			if ((Target -= 1) < LOOKASIDE_MINIMUM_DEPTH) {
				Target = LOOKASIDE_MINIMUM_DEPTH;
			}

		} else {
			if ((Delta = ((Ratio * (MaximumDepth - Target)) / (1000 * 2)) + 5) > 30) {
				Delta = 30;
			} 

			else if (Delta < 10) {
				Delta = 10;
			}

			if ((Target += Delta) > MaximumDepth) {
				Target = MaximumDepth;
			}

			DebugTrace("BtrComputeLookasideDepth: delta=%d, target=%d", Delta, Target);
		}
	}

    Lookaside->Depth = (USHORT)Target;
    return;
}

VOID
BtrAdjustLookasideDepth(
	IN ULONG ScanPeriod
	)
{
	ULONG Number;
	ULONG Misses;
	PBTR_LOOKASIDE Lookaside;

	for (Number = 0; Number < RECORD_LOOKASIDE_NUM; Number += 1) {
		Lookaside = &BtrLookaside[Number];
		Misses = Lookaside->AllocateMisses - Lookaside->LastAllocateMisses;
		Lookaside->LastAllocateMisses = Lookaside->AllocateMisses;
		BtrComputeLookasideDepth(Lookaside, Misses, ScanPeriod);
	}
}

VOID
BtrQueryLookasideState(
	OUT PBTR_LOOKASIDE_STATE State,
	IN ULONG NumberOfLookasides
	)
{
	ULONG Number;
	PBTR_LOOKASIDE Lookaside;

	ASSERT(NumberOfLookasides <= RECORD_LOOKASIDE_NUM + 1);

	for(Number = 0; Number < NumberOfLookasides; Number += 1) {

		Lookaside = &BtrLookaside[Number];

		if (Number != RECORD_LOOKASIDE_NUM) {
			State->Size = (USHORT)(RECORD_MINIMUM_SIZE + Number * 16);
		} else {
			State->Size = (USHORT)0xffff;
		}

		State->Type = Lookaside->Type;
		State->TotalAllocates = Lookaside->TotalAllocates;
		State->TotalFrees = Lookaside->TotalFrees;
		State->AllocateMisses = Lookaside->AllocateMisses;
		State->FreeMisses = Lookaside->FreeMisses;
		State->Depth = Lookaside->Depth;
		State->MaximumDepth = Lookaside->MaximumDepth;
		State->ListDepth = QueryDepthSList(&Lookaside->ListHead);

		if (State->TotalAllocates != 0) {
			State->AllocateMissRatio = (double)(State->AllocateMisses * 1.0) / (double)(State->TotalAllocates * 1.0);
		} else {
			State->AllocateMissRatio = 0.0;
		}

		if (State->TotalFrees != 0) {
			State->FreeMissRatio = (double)(State->FreeMisses * 1.0) / (double)(State->TotalFrees * 1.0);
		} else {
			State->FreeMissRatio = 0.0;
		}

		State += 1;
	}
}
