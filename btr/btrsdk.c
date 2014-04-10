//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "btr.h"
#include "btrsdk.h"
#include "thread.h"
#include "heap.h"
#include "util.h"
#include "probe.h"
#include "trap.h"
#include "ipc.h"
#include "call.h"
#include "queue.h"

ULONG BtrFltTotalMallocs;
ULONG BtrFltTotalFrees;
ULONG BtrFltTotalThreadMallocs;
ULONG BtrFltTotalThreadFrees;

PVOID WINAPI
BtrFltMalloc(
	IN ULONG Length
	)
{
	PBTR_THREAD Thread;
	BOOLEAN ClearFlag;
	PVOID Address;

	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		return BtrMallocEx(SystemHeap, Length);
	}

	ClearFlag = FALSE;
	Thread = BtrGetCurrentThread();

	if (!FlagOn(Thread->ThreadFlag, BTR_FLAG_EXEMPTION)){
		SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		ClearFlag = TRUE;
	}
	 
	Address = BtrMallocEx(FilterHeap, Length);

	if (ClearFlag) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	}

	return Address;
}

VOID WINAPI
BtrFltFree(
	IN PVOID Address
	)
{
	PBTR_THREAD Thread;
	BOOLEAN ClearFlag;

	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		BtrFreeEx(SystemHeap, Address);
		return;
	}

	ClearFlag = FALSE;
	Thread = BtrGetCurrentThread();

	if (!FlagOn(Thread->ThreadFlag, BTR_FLAG_EXEMPTION)) {
		SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		ClearFlag = TRUE;
	}

	BtrFreeEx(FilterHeap, Address);

	if (ClearFlag) { 
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	}
}

VOID WINAPI
BtrFltGetContext(
	OUT PBTR_PROBE_CONTEXT Context 
	)
{
	PBTR_THREAD Thread;
	PBTR_PROBE Probe;
	PBTR_TRAP Trap;
	PBTR_CALLFRAME CallFrame;
	PLIST_ENTRY ListEntry;
	BOOLEAN ClearFlag;

	ASSERT(Context != NULL);

	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		return;
	}

	if (!Context) {
		return;
	}

	Thread = BtrGetCurrentThread();
	if (!FlagOn(Thread->ThreadFlag, BTR_FLAG_EXEMPTION)) {
		SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		ClearFlag = TRUE;
	}

	ListEntry = Thread->FrameList.Flink;

	CallFrame = CONTAINING_RECORD(ListEntry, BTR_CALLFRAME, ListEntry);
	Probe = (PBTR_PROBE)CallFrame->Probe;
	Trap = Probe->Trap;
	
	Context->Destine = (PVOID)Trap->HijackedCode;
	Context->Frame = (PVOID)CallFrame;

	if (ClearFlag) { 
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	}
}

VOID WINAPI
BtrFltSetContext(
	IN PBTR_PROBE_CONTEXT Context 
	)
{
	PBTR_CALLFRAME CallFrame;
	PBTR_THREAD Thread;
	BOOLEAN ClearFlag;

	ASSERT(Context != NULL);

	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		return;
	}

	if (Context != NULL) {

		Thread = BtrGetCurrentThread();
		if (!FlagOn(Thread->ThreadFlag, BTR_FLAG_EXEMPTION)) {
			SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
			ClearFlag = TRUE;
		}

		CallFrame = (PBTR_CALLFRAME)Context->Frame;
		CallFrame->LastError = GetLastError();

		if (ClearFlag) { 
			ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		}
	}
}

DWORD WINAPI
BtrFltGetLastError(
	IN PBTR_PROBE_CONTEXT Context	
	)
{
	PBTR_CALLFRAME CallFrame;

	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		return 0;
	}

	CallFrame = (PBTR_CALLFRAME)Context->Frame;
	return CallFrame->LastError;
}

VOID WINAPI
BtrFltQueueRecord(
	IN PVOID Record
	)
{
	PBTR_THREAD Thread;
	PBTR_CALLFRAME CallFrame;
	PLIST_ENTRY ListEntry;

	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		return;
	}

	Thread = BtrGetCurrentThread();
	ListEntry = Thread->FrameList.Flink;

	CallFrame = CONTAINING_RECORD(ListEntry, BTR_CALLFRAME, ListEntry);
	CallFrame->Record = Record;
	return;
}

PBTR_FILTER_RECORD WINAPI
BtrFltAllocateRecord(
	IN ULONG DataLength, 
	IN GUID FilterGuid,
	IN ULONG Ordinal
	)
{
	ULONG TotalLength;
	BOOLEAN ClearFlag = FALSE;
	PBTR_FILTER_RECORD Record;
	PBTR_THREAD Thread;

	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		return NULL;
	}

	TotalLength = FIELD_OFFSET(BTR_FILTER_RECORD, Data[DataLength]);
	Thread = BtrGetCurrentThread();

	if (TotalLength < 4096) {

		if (Thread->BufferBusy != TRUE) {

			Record = (PBTR_FILTER_RECORD)Thread->Buffer;
			RtlZeroMemory(Record, sizeof(BTR_RECORD_HEADER));

			Record->Base.RecordType = RECORD_FILTER;
			Record->Base.TotalLength = TotalLength;
			Record->Base.LastError = 0;
			Record->ProbeOrdinal = Ordinal;
			Record->FilterGuid = FilterGuid;

			InterlockedIncrement(&BtrFltTotalThreadMallocs);
			InterlockedIncrement(&BtrFltTotalMallocs);

			return Record;
		}
	}

	if (!FlagOn(Thread->ThreadFlag, BTR_FLAG_EXEMPTION)){
		SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		ClearFlag = TRUE;
	}

	Record = (PBTR_FILTER_RECORD)BtrMallocLookaside(RecordHeap, TotalLength);
	
	if (ClearFlag) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	}
	
	if (!Record){
		return NULL;
	}

	Record->Base.RecordType = RECORD_FILTER;
	Record->Base.TotalLength = TotalLength;
	Record->Base.LastError = 0;
	Record->ProbeOrdinal = Ordinal;
	Record->FilterGuid = FilterGuid;

	InterlockedIncrement(&BtrFltTotalMallocs);
	return Record;
}

VOID WINAPI
BtrFltFreeRecord(
	IN PBTR_FILTER_RECORD Record
	)
{
	PBTR_THREAD Thread;
	BOOLEAN ClearFlag;

	if (FlagOn(BtrFeatures, FeatureLibrary)) {

		//
		// N.B. We can free a record in library mode, do nothing here.
		//

		return;
	}

	ClearFlag = FALSE;
	Thread = BtrGetCurrentThread();

	if (Record == (PBTR_FILTER_RECORD)Thread->Buffer) {
		Thread->BufferBusy = FALSE;
		InterlockedIncrement(&BtrFltTotalFrees);
		InterlockedIncrement(&BtrFltTotalThreadFrees);
		return;
	}

	if (!FlagOn(Thread->ThreadFlag, BTR_FLAG_EXEMPTION)) {
		SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		ClearFlag = TRUE;
	}

	BtrFreeLookaside(RecordHeap, Record);

	if (ClearFlag) { 
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	}
	
	InterlockedIncrement(&BtrFltTotalFrees);
}

ULONG
BtrFltDeQueueRecord(
	IN PVOID Buffer,
	IN ULONG Length,
	OUT PULONG RecordLength,
	OUT PULONG RecordCount 
	)
{
	PBTR_THREAD Thread;
	BOOLEAN ClearFlag;
	ULONG Status;

	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		return 0;
	}

	ClearFlag = FALSE;
	Thread = BtrGetCurrentThread();

	if (!FlagOn(Thread->ThreadFlag, BTR_FLAG_EXEMPTION)) {
		SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		ClearFlag = TRUE;
	}

	Status = BtrDeQueueRecord(Buffer, Length, RecordLength, RecordCount);

	if (ClearFlag) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	}

	return Status;
}

VOID
BtrFltSetExemption(
	VOID
	)
{
	PBTR_THREAD Thread;

	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		return;
	}

	Thread = BtrGetCurrentThread();
	ASSERT(Thread != NULL);
	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

VOID
BtrFltClearExemption(
	VOID
	)
{
	PBTR_THREAD Thread;

	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		return;
	}

	Thread = BtrGetCurrentThread();
	ASSERT(Thread != NULL);
	ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
}

SIZE_T WINAPI
BtrFltConvertUnicodeToAnsi(
	IN PWSTR Source, 
	OUT PCHAR *Destine
	)
{
	BOOLEAN ClearFlag;
	PBTR_THREAD Thread;
	SIZE_T Size;

	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		return BtrConvertUnicodeToUTF8(Source, Destine);
	}

	ClearFlag = FALSE;
	Thread = BtrGetCurrentThread();

	if (!FlagOn(Thread->ThreadFlag, BTR_FLAG_EXEMPTION)){
		SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		ClearFlag = TRUE;
	}
	
	Size = BtrConvertUnicodeToUTF8(Source, Destine);

	if (ClearFlag) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	}

	return Size;
}

//
// Free the Destine via BtrFltFree
//

VOID WINAPI
BtrFltConvertAnsiToUnicode(
	IN PCHAR Source,
	OUT PWCHAR *Destine
	)
{
	BOOLEAN ClearFlag;
	PBTR_THREAD Thread;

	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		BtrConvertUTF8ToUnicode(Source, Destine);
		return;
	}

	ClearFlag = FALSE;
	Thread = BtrGetCurrentThread();

	if (!FlagOn(Thread->ThreadFlag, BTR_FLAG_EXEMPTION)){
		SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
		ClearFlag = TRUE;
	}
	
	BtrConvertUTF8ToUnicode(Source, Destine);

	if (ClearFlag) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	}
}

VOID WINAPI
BtrFltEnterExemptionRegion(
	VOID
	)
{
	PBTR_THREAD Thread;
	
	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		return;
	}

	Thread = BtrGetCurrentThread();
	ASSERT(Thread != NULL);

	if (!FlagOn(Thread->ThreadFlag, BTR_FLAG_EXEMPTION)) {
		SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	}
}

VOID WINAPI
BtrFltLeaveExemptionRegion(
	VOID
	)
{
	PBTR_THREAD Thread;

	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		return;
	}

	Thread = BtrGetCurrentThread();
	ASSERT(Thread != NULL);
	
	if (FlagOn(Thread->ThreadFlag, BTR_FLAG_EXEMPTION)) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	}
}