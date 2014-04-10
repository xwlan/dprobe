//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _HEAP_H_
#define _HEAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

//
// N.B. Don't change the order of heap
//

typedef enum _BTR_HEAP_TYPE {
	SystemHeap,
	FilterHeap,
	RecordHeap,
	MaximumHeap,
	InvalidHeap = -1,
} BTR_HEAP_TYPE, *PBTR_HEAP_TYPE;

typedef PVOID
(*MALLOC_ROUTINE) (
    IN BTR_HEAP_TYPE Type,
    IN SIZE_T NumberOfBytes
    );

typedef VOID
(*PFREE_ROUTINE) (
	IN BTR_HEAP_TYPE Type,
    IN PVOID Buffer
    );

typedef struct DECLSPEC_CACHEALIGN _BTR_LOOKASIDE {

    SLIST_HEADER ListHead;
    USHORT Depth;
    USHORT MaximumDepth;

    ULONG TotalAllocates;
    ULONG TotalFrees;

    ULONG AllocateMisses;
    ULONG AllocateHits;

    ULONG FreeMisses;
    ULONG FreeHits;

	ULONG LastTotalAllocates;
	ULONG LastAllocateMisses;
	ULONG LastFreeMisses;

	BTR_HEAP_TYPE Type;
    MALLOC_ROUTINE Malloc;
    PFREE_ROUTINE Free;

    LIST_ENTRY ListEntry;

} BTR_LOOKASIDE, *PBTR_LOOKASIDE;

typedef struct _BTR_LOOKASIDE_BLOCK {
	SLIST_ENTRY ListEntry;
	USHORT Slot;
	USHORT Type;
	ULONG  Tag;
	CHAR Block[ANYSIZE_ARRAY];
} BTR_LOOKASIDE_BLOCK, *PBTR_LOOKASIDE_BLOCK;

//
// Record heap lookaside increase by 16 bytes, 
// start from 16 * 24 = 384 bytes, maximum 1 KB.
// because the minimum record size is 344 bytes,
// so we get started from an estimated 384 bytes.
//
// The appended lookaside is only used to track
// large block request, we don't actuall allocate
// from it.
//

#define RECORD_MINIMUM_SIZE       384 
#define RECORD_LOOKASIDE_NUM      40 
#define RECORD_LOOKASIDE_TAG      (ULONG)('lrtB')

#define IS_MEMORY_ALLOCATION_ALIGNED(_V)    ())

//
// Lookaside initialization flags 
//

#define LOOKASIDE_FREE_CHECK         0x00000001
#define LOOKASIDE_DEPTH_TUNE         0x00000002
#define LOOKASIDE_SMP_HEAP           0x00000004

#define LOOKASIDE_MINIMUM_DEPTH        4
#define LOOKASIDE_ALLOCATION_THRESHOLD 25

//
// Lookaside performance information
//

typedef struct _BTR_LOOKASIDE_STATE {
	USHORT Size;
	USHORT Type;
	ULONG Depth;
	ULONG MaximumDepth;
	ULONG ListDepth;
	ULONG TotalAllocates;
	ULONG TotalFrees;
	ULONG AllocateMisses;
	ULONG FreeMisses;
	double AllocateMissRatio;
	double FreeMissRatio;
} BTR_LOOKASIDE_STATE, *PBTR_LOOKASIDE_STATE;

PVOID 
BtrMalloc(
	IN ULONG ByteCount
	);

VOID 
BtrFree(
	IN PVOID Address
	);

PVOID
BtrMallocEx(
	IN BTR_HEAP_TYPE Type,
	IN SIZE_T ByteCount
	);

VOID
BtrFreeEx(
	IN BTR_HEAP_TYPE Type,
	IN PVOID Address
	);

PVOID
BtrMallocLookaside(
	IN BTR_HEAP_TYPE Type,
	IN ULONG ByteCount
	);

VOID
BtrFreeLookaside(
	IN BTR_HEAP_TYPE Type,
	IN PVOID Address 
	);

VOID
BtrAdjustLookasideDepth(
	IN ULONG ScanPeriod
	);

VOID
BtrComputeLookasideDepth (
    IN PBTR_LOOKASIDE Lookaside,
    IN ULONG Misses,
    IN ULONG ScanPeriod
    );

BOOLEAN
BtrCheckLookasideBlock(
	IN PBTR_LOOKASIDE_BLOCK Block
	);

BOOLEAN
BtrIsMemoryAllocationAligned(
	IN ULONG_PTR Pointer
	);

VOID
BtrQueryLookasideState(
	OUT PBTR_LOOKASIDE_STATE State,
	IN ULONG NumberOfLookasides
	);

VOID WINAPI
BtrFltFreeRecord(
	PBTR_FILTER_RECORD Record
	);

extern ULONG BtrLookasideFlag;
extern HANDLE BtrHeap[MaximumHeap];
extern BTR_LOOKASIDE BtrLookaside[RECORD_LOOKASIDE_NUM + 1];

extern ULONG BtrFltTotalMallocs;
extern ULONG BtrFltTotalFrees;
extern ULONG BtrFltTotalThreadMallocs;
extern ULONG BtrFltTotalThreadFrees;

#ifdef __cplusplus
}
#endif

#endif // _ALLOC_H_