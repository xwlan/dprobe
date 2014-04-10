#include "mspdtl.h"
#include "mspprobe.h"
#include "msputility.h"
#include "mspprocess.h"

#define PAGESIZE 4096

ULONG 
MspComputeProbeHash(
	IN PMSP_PROBE_TABLE Table,
	IN PVOID Address
	)
{
	ULONG Hash;

	Hash = (ULONG)((ULONG_PTR)Address ^ (ULONG_PTR)HASH_GOLDEN_RATIO);
	Hash = (Hash >> 24) ^ (Hash >> 16) ^ (Hash >> 8) ^ (Hash);

	return Hash % Table->TotalBuckets;
}

ULONG
MspCreateProbeTable(
	IN PMSP_PROCESS Process,
	IN ULONG NumberOfBuckets
	)
{
	ULONG Size;
	HANDLE Handle;
	PVOID Address;
	ULONG Number;
	PMSP_PROBE_TABLE Table;

	ASSERT(Process != NULL);

	Size = MspUlongRoundUp(sizeof(MSP_PROBE_TABLE), PAGESIZE);

	Handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, Size, NULL);
	if (!Handle) {
		return MSP_STATUS_CREATEFILEMAPPING;
	}

	Address = MapViewOfFile(Handle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
	if (!Address) {
		CloseHandle(Handle);
		return MSP_STATUS_MAPVIEWOFFILE;	
	}

	Table = (PMSP_PROBE_TABLE)Address;
	MspInitCsLock(&Table->Lock, 100);

	Table->TotalBuckets = NumberOfBuckets;
	Table->FilledBuckets = 0;
	Table->Process = Process;
	Table->Object = Handle;
	Table->FastProbeCount = 0;
	Table->FilterProbeCount = 0;
	Table->ActiveFastCount = 0;
	Table->ActiveFilterCount = 0;

	InitializeListHead(&Table->FastListHead);
	InitializeListHead(&Table->FilterListHead);

	for (Number = 0; Number < Table->TotalBuckets; Number += 1) {
		InitializeListHead(&Table->ListHead[Number]);
	}

	Process->ProbeTable = Table;
	return MSP_STATUS_OK;
}

VOID
MspDestroyProbeTable(
	IN PMSP_PROBE_TABLE Table
	)
{
	ULONG Number;
	PMSP_PROBE Probe;
	HANDLE Object;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;

	for (Number = 0; Number < Table->TotalBuckets; Number += 1) {

		ListHead = &Table->ListHead[Number];

		while (IsListEmpty(ListHead) != TRUE) {

			ListEntry = RemoveHeadList(ListHead);
			Probe = CONTAINING_RECORD(ListEntry, MSP_PROBE, ListEntry);

			if (Probe->Counter != NULL) {
				MspFree(Probe->Counter);
			}

			MspFree(Probe);
		}
	}

	MspDeleteCsLock(&Table->Lock);

	Object = Table->Object;
	UnmapViewOfFile(Table);
	CloseHandle(Object);
}

PMSP_PROBE_TABLE
MspQueryProbeTable(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG ProcessId
	)
{
	PLIST_ENTRY ListEntry;
	PMSP_PROCESS Process;

	MspAcquireCsLock(&DtlObject->ProcessListLock);

	ListEntry = DtlObject->ProcessListHead.Flink;
	while (ListEntry != &DtlObject->ProcessListHead) {
	
		Process = CONTAINING_RECORD(ListEntry, MSP_PROCESS, ListEntry);
		if (Process->ProcessId == ProcessId) {
			MspReleaseCsLock(&DtlObject->ProcessListLock);
			return Process->ProbeTable;
		}
	
		ListEntry = ListEntry->Flink;
	}

	MspReleaseCsLock(&DtlObject->ProcessListLock);
	return NULL;
}

PMSP_PROBE
MspLookupProbe(
	IN struct _MSP_PROCESS *Process,
	IN BTR_PROBE_TYPE Type,
	IN PVOID Address,
	IN PWSTR DllName,
	IN PWSTR ApiName,
	IN PBTR_FILTER Filter
	)
{
	PMSP_PROBE_TABLE Table;
	PMSP_PROBE Probe;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	ULONG Bucket;

	Table = Process->ProbeTable;
	if (!Table) {
		return NULL;
	}

	Bucket = MspComputeProbeHash(Table, Address);
	ListHead = &Table->ListHead[Bucket];

	if (IsListEmpty(ListHead)) {
		return NULL;
	}

	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {

		Probe = CONTAINING_RECORD(ListEntry, MSP_PROBE, ListEntry);

		if (Probe->Type == Type && Probe->Address == Address) {

			if (Probe->Type == PROBE_FAST) {

				ASSERT(DllName != NULL);
				ASSERT(ApiName != NULL);

				if (!wcscmp(Probe->DllName, DllName) && !wcscmp(Probe->ApiName, ApiName)) {
					return Probe;
				}
			}

			if (Probe->Type == PROBE_FILTER) {
				if (IsEqualGUID(&Filter->FilterGuid, &Probe->Filter->FilterGuid)) {
					return Probe;
				}
			}
		}

		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

PMSP_PROBE
MspLookupProbeEx(
	IN struct _MSP_DTL_OBJECT *DtlObject,
	IN ULONG ProcessId,
	IN BTR_PROBE_TYPE Type,
	IN PVOID Address,
	IN PWSTR DllName,
	IN PWSTR ApiName,
	IN PBTR_FILTER Filter,
	OUT PMSP_PROBE_TABLE *Return
	)
{
	PMSP_PROBE Probe;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PMSP_PROBE_TABLE Table;
	ULONG Bucket;

	Table = MspQueryProbeTable(DtlObject, ProcessId);
	if (!Table) {
		*Return = NULL;
		return NULL;
	}

	Bucket = MspComputeProbeHash(Table, Address);
	ListHead = &Table->ListHead[Bucket];

	if (IsListEmpty(ListHead)) {
		*Return = Table;
		return NULL;
	}

	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {

		Probe = CONTAINING_RECORD(ListEntry, MSP_PROBE, ListEntry);

		if (Probe->Type == Type && Probe->Address == Address) {

			if (Probe->Type == PROBE_FAST) {

				ASSERT(DllName != NULL);
				ASSERT(ApiName != NULL);

				if (!wcscmp(Probe->DllName, DllName) && !wcscmp(Probe->ApiName, ApiName)) {
					*Return = Table;
					return Probe;
				}
			}

			if (Probe->Type == PROBE_FILTER) {
				if (IsEqualGUID(&Filter->FilterGuid, &Probe->Filter->FilterGuid)) {
					*Return = Table;
					return Probe;
				}
			}
		}

		ListEntry = ListEntry->Flink;
	}

	*Return = Table;
	return NULL;
}

ULONG
MspInsertProbe(
	IN struct _MSP_PROCESS *Process,
	IN BTR_PROBE_TYPE Type,
	IN BTR_RETURN_TYPE Return,
	IN PVOID Address,
	IN ULONG ObjectId,
	IN ULONG Counter,
	IN PWSTR DllName,
	IN PWSTR ApiName,
	IN PBTR_FILTER Filter,
	IN ULONG FilterBit
	)
{
	PMSP_PROBE Probe;
	PMSP_PROBE_TABLE Table;

	Table = Process->ProbeTable;

	//
	// Filter probe only set its bitmap
	//

	Probe = MspAllocateProbe(Table, Address);
	if (!Probe) {
		return MSP_STATUS_ERROR;
	}

	Probe->Address = Address;
	Probe->Type = Type;
	Probe->Return = Return;
	Probe->State = ActivatedProbe;
	Probe->ObjectId = ObjectId;

	if (ApiName) {
		wcscpy_s(Probe->ApiName, MAX_PATH, ApiName);
	}
	if (DllName) {
		wcscpy_s(Probe->DllName, MAX_PATH, DllName);
	}

	if (Type == PROBE_FAST) {

		Table->ActiveFastCount += 1;
		Table->FastProbeCount += 1;
		InsertTailList(&Table->FastListHead, &Probe->TypeListEntry);

	}
	else if (Type == PROBE_FILTER) {

		ASSERT(Filter != NULL);

		Probe->Filter = Filter;
		Probe->FilterBit = FilterBit;
		BtrSetBit(&Filter->BitMap, FilterBit);

		wcscpy_s(Probe->DllName, MAX_PATH, Filter->Probes[FilterBit].DllName);
		wcscpy_s(Probe->ApiName, MAX_PATH, Filter->Probes[FilterBit].ApiName);

		Table->ActiveFilterCount += 1;
		Table->FilterProbeCount += 1;
		InsertTailList(&Table->FilterListHead, &Probe->TypeListEntry);

	} else {

		ASSERT(0);
	}

	MspInsertCounterEntry(Process, Probe, Counter);
	return MSP_STATUS_OK;
}

PMSP_PROBE
MspAllocateProbe(
	IN PMSP_PROBE_TABLE Table,
	IN PVOID Address
	)
{
	ULONG Bucket;
	PMSP_PROBE Probe;
	PLIST_ENTRY ListHead;

	Bucket = MspComputeProbeHash(Table, Address);
	ListHead = &Table->ListHead[Bucket];

	Probe = (PMSP_PROBE)MspMalloc(sizeof(MSP_PROBE));
	RtlZeroMemory(Probe, sizeof(MSP_PROBE));
	Probe->Address = Address;

	//
	// N.B. New allocate probe object is always inserted in list head
	//

	InsertHeadList(ListHead, &Probe->ListEntry);
	return Probe;
}

BOOLEAN
MspRemoveProbe(
	IN struct _MSP_PROCESS *Process,
	IN BTR_PROBE_TYPE Type,
	IN PVOID Address,
	IN PWSTR DllName,
	IN PWSTR ApiName,
	IN PBTR_FILTER Filter,
	IN ULONG FilterBit,
	OUT PMSP_PROBE Clone
	)
{
	PMSP_PROBE_TABLE Table;
	PMSP_PROBE Probe;
	PLIST_ENTRY ListHead;
	ULONG Bucket;

	Table = Process->ProbeTable;
	if (!Table) {
		return FALSE;
	}

	Probe = MspLookupProbe(Process, Type, 
		                   Address, DllName, ApiName, Filter);
	if (!Probe) {
		return FALSE;
	}

	MspRemoveCounterEntry(Process, Probe);

	Probe->State = DeactivatedProbe;
	RemoveEntryList(&Probe->ListEntry);

	//
	// N.B. We never free probe object, instead we put in in list tail
	//

	Bucket = MspComputeProbeHash(Table, Address);
	ListHead = &Table->ListHead[Bucket];
	InsertTailList(ListHead, &Probe->ListEntry);

	if (Type == PROBE_FAST) {

		// 
		// N.B. Since we never really free probe, we don't decrease
		// FastProbeCount
		//

		Table->ActiveFastCount -= 1;
	}
	else if (Type == PROBE_FILTER) {

		ASSERT(Filter != NULL);
		BtrClearBit(&Filter->BitMap, FilterBit);

		Table->ActiveFilterCount -= 1;

	} else {

		ASSERT(0);
	}

	return TRUE;
}

ULONG
MspRemoveAllProbes(
	IN struct _MSP_PROCESS *Process
	)
{
	ULONG Number;
	PMSP_PROBE_TABLE Table;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PMSP_PROBE Probe;

	Table = Process->ProbeTable;

	for(Number = 0; Number < Table->TotalBuckets; Number += 1) {

		ListHead = &Table->ListHead[Number];
		ListEntry = ListHead->Flink;

		while (ListEntry != ListHead) {

			Probe = CONTAINING_RECORD(ListEntry, MSP_PROBE, ListEntry);

			if (Probe->State != DeactivatedProbe) {
				Probe->State = DeactivatedProbe;
				if (Probe->Type == PROBE_FILTER) {
					BtrClearBit(&Probe->Filter->BitMap, Probe->FilterBit);
				}
				MspRemoveCounterEntry(Process, Probe);
			}

			ListEntry = ListEntry->Flink;
		}
	}

	return MSP_STATUS_OK;
}

PBTR_FILTER
MspLookupFilter(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG ProcessId,
	IN GUID *FilterGuid
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_FILTER Filter;
	PMSP_PROCESS Process;

	if (DtlObject->Type == DTL_REPORT) {
		return MspQueryDecodeFilterByGuid(FilterGuid);
	}

	Process = MspLookupProcess(DtlObject, ProcessId);

	ListEntry = Process->FilterList.Flink;
	while (ListEntry != &Process->FilterList) {
	
		Filter = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);	
		if (IsEqualGUID(&Filter->FilterGuid, FilterGuid)) {
			return Filter;
		}

		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

struct _MSP_PROCESS*
MspQueryProcess(
	IN ULONG ProcessId,
	IN PWSTR ImageName,
	IN PWSTR FullPath
	)
{
	return 0;
}

PMSP_PROBE
MspLookupFastProbe(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record
	)
{
	PMSP_PROBE Probe;

	if (Record->RecordType == RECORD_FAST) {
		Probe = MspLookupProbeForDecode(DtlObject, Record->ProcessId, 
									    PROBE_FAST, Record->Address);
		return Probe;
	}

	return NULL;
}

PBTR_FILTER_PROBE
MspLookupFilterProbe(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record
	)
{
	PBTR_FILTER Filter;
	PBTR_FILTER_RECORD FilterRecord;

	ASSERT(Record->RecordType == RECORD_FILTER);

	FilterRecord = (PBTR_FILTER_RECORD)Record;
	Filter = MspQueryDecodeFilterByGuid(&FilterRecord->FilterGuid);

	if (!Filter) {
		return NULL;
	}

	return &Filter->Probes[FilterRecord->ProbeOrdinal];
}

PMSP_PROBE
MspLookupProbeForDecode(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG ProcessId,
	IN BTR_PROBE_TYPE Type,
	IN PVOID Address 
	)
{
	PMSP_PROBE_TABLE Table;
	PMSP_PROBE Probe;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	ULONG Bucket;

	Table = MspQueryProbeTable(DtlObject, ProcessId);
	if (!Table) {
		return NULL;
	}

	Bucket = MspComputeProbeHash(Table, Address);
	ListHead = &Table->ListHead[Bucket];

	ListEntry = ListHead->Flink;
	while (ListEntry != ListHead) {

		Probe = CONTAINING_RECORD(ListEntry, MSP_PROBE, ListEntry);
		if (Probe->Type == Type && Probe->Address == Address) {
			return Probe;
		}

		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

PMSP_PROBE
MspLookupProbeByObjectId(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record
	)
{
	PMSP_PROBE_TABLE Table;
	PMSP_PROBE Probe;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	ULONG Bucket;
	PVOID Address;

	Table = MspQueryProbeTable(DtlObject, Record->ProcessId);
	if (!Table) {
		return NULL;
	}

	Address = Record->Address;
	Bucket = MspComputeProbeHash(Table, Address);
	ListHead = &Table->ListHead[Bucket];

	ListEntry = ListHead->Flink;
	while (ListEntry != ListHead) {

		Probe = CONTAINING_RECORD(ListEntry, MSP_PROBE, ListEntry);
		if (Probe->ObjectId == Record->ObjectId) {
			return Probe;
		}

		ListEntry = ListEntry->Flink;
	}

	return NULL;
	
}