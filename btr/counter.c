//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "btr.h"
#include "counter.h"
#include "util.h"

PBTR_COUNTER_TABLE BtrCounterTable;
HANDLE BtrCounterHandle;

#define COUNTER_PTR_TO_BUCKET(_P, _F)  \

ULONG
BtrInitializeCounter(
	IN HANDLE PerfDataHandle	
	)
{
	BtrCounterHandle = PerfDataHandle;
	BtrCounterTable = MapViewOfFile(BtrCounterHandle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
	if (!BtrCounterTable) {
		return BTR_E_MAPVIEWOFFILE;
	}

	return S_OK;
}

ULONG
BtrUninitializeCounter(
	VOID
	)
{
	if (BtrCounterTable) {
		UnmapViewOfFile(BtrCounterTable);
		CloseHandle(BtrCounterHandle);
	}
	return S_OK;
}

PBTR_COUNTER_ENTRY
BtrAllocateCounter(
	IN ULONG ObjectId
	)
{
	ULONG Number;
	PBTR_COUNTER_ENTRY Entry;
	ULONG Bucket;
	ULONG Allocated;
	SYSTEMTIME SystemTime;

	Bucket = BtrCounterBucket(ObjectId);
	Entry = &BtrCounterTable->Entries[Bucket];	

	if (Entry->ObjectId == 0) {
		RtlZeroMemory(Entry, sizeof(BTR_COUNTER_ENTRY));
		GetLocalTime(&SystemTime);
		SystemTimeToFileTime(&SystemTime, &Entry->StartTime);
		return Entry;
	}

	Allocated = -1;

	for(Number = Bucket + 1; Number < BtrCounterTable->NumberOfBuckets; Number += 1) {
		Entry = &BtrCounterTable->Entries[Number];
		if (!Entry->ObjectId) {
			Allocated = Number;
			break;
		}
	}

	if (Allocated != -1) {
		RtlZeroMemory(Entry, sizeof(BTR_COUNTER_ENTRY));
		GetLocalTime(&SystemTime);
		SystemTimeToFileTime(&SystemTime, &Entry->StartTime);
		return Entry;
	}

	if (Bucket == 0) {
		return NULL;
	}

	for(Number = Bucket - 1; Number != -1; Number -= 1) {
		Entry = &BtrCounterTable->Entries[Number];
		if (!Entry->ObjectId) {
			Allocated = Number;
			break;
		}
	}

	if (Allocated != -1) {
		RtlZeroMemory(Entry, sizeof(BTR_COUNTER_ENTRY));
		GetLocalTime(&SystemTime);
		SystemTimeToFileTime(&SystemTime, &Entry->StartTime);
		return Entry;
	}

	return NULL;
}

VOID
BtrRemoveCounter(
	IN PBTR_COUNTER_ENTRY Entry 
	)
{
	Entry->ObjectId = 0;
}

VOID
BtrUpdateCounter(
	IN PBTR_COUNTER_ENTRY Entry,
	IN PBTR_RECORD_HEADER Header
	)
{
	ULONG Minimum;
	ULONG Maximum;
	ULONG Current;

	InterlockedIncrement(&Entry->NumberOfCalls);
	InterlockedIncrement(&BtrCounterTable->NumberOfCalls);

	if (FlagOn(Header->RecordFlag, BTR_FLAG_EXCEPTION)) {
		InterlockedIncrement(&Entry->NumberOfExceptions);
		InterlockedIncrement(&BtrCounterTable->NumberOfExceptions);
	}	

	//
	// Update minimum duration
	//

	do {
		Minimum = ReadForWriteAccess(&Entry->MinimumDuration);
		if (Header->Duration < Minimum) {
			Current = InterlockedCompareExchange(&Entry->MinimumDuration, 
												 Header->Duration, Minimum);
		} else {
			break;
		}
	}
	while (Current != Minimum);

	//
	// Update maximum duration
	//

	do {
		Maximum = ReadForWriteAccess(&Entry->MaximumDuration);
		if (Header->Duration > Maximum) {
			Current = InterlockedCompareExchange(&Entry->MaximumDuration, 
												 Header->Duration, Maximum);
		} else {
			break;
		}
	}
	while (Current != Maximum);

}

ULONG
BtrGetCounterBucket(
	IN PBTR_COUNTER_ENTRY Entry
	)
{
	ULONG_PTR Bucket;
	PBTR_COUNTER_ENTRY Base = &BtrCounterTable->Entries[0];

	Bucket = Entry - Base;

	ASSERT(Bucket != (ULONG)-1);
	return (ULONG)Bucket;
}