//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include <rpc.h>
#include <intrin.h>
#include <dbghelp.h>
#include "list.h"
#include "bsp.h"
#include "progressdlg.h"
#include "mspdtl.h"
#include "msputility.h"
#include "mspprocess.h"

#pragma comment(lib, "rpcrt4.lib")

//
// N.B. This is only a temporary solution to copy msp code into dprobe,
// msp will be fully integrated into dprobe as a dll before rtm, currently
// this is used to evaluate the performance and reliability of msp cache
// design.
//

#define PAGESIZE 4096

HANDLE MspSharedDataHandle;
PBTR_SHARED_DATA MspSharedData;

WCHAR MspSessionPath[MAX_PATH];
WCHAR MspCachePath[MAX_PATH];
HANDLE MspIndexProcedureHandle;

UUID MspSessionUuid;
WCHAR MspSessionUuidString[64];

MSP_DTL_OBJECT MspDtlObject;

ULONG MspRuntimeFeature;

//
// Global dtl object list, note live session object
// MspDtlObject is not inserted into this list,
// this list is for dtl report object.
//

LIST_ENTRY MspDtlReportList;
MSP_CS_LOCK MspDtlReportLock;

ULONG MspInitialized;
HANDLE MspBtrHandle;
HANDLE MspStopEvent;
LIST_ENTRY MspFilterList;

ULONG MspSchedulerId;

//
// CSV Column Defs
//

PSTR MspCsvRecordColumn = " \"Process\", \"Time\", \"PID\", \"TID\", \"Probe\", \"Duration (ms)\", \"Return\", \"Detail\" \r\n";
PSTR MspCsvRecordFormat = " \"%S\", \"%02d:%02d:%02d:%03d\", \"%u\", \"%u\", \"%S\", \"%.3f\", \"0x%I64x\", \"%S\" \r\n";

ULONG
MspValidateCachePath(
	IN PWSTR Path
	)
{
	HANDLE Handle;

	Handle = CreateFile(Path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
						OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_ATTRIBUTE_NORMAL, NULL);
	if (Handle == INVALID_HANDLE_VALUE) {
		return MSP_STATUS_INVALID_PARAMETER;
	}

	CloseHandle(Handle);
	return S_OK;
}

VOID
MspGetCachePath(
	OUT PWCHAR Buffer,
	IN ULONG Length
	)
{
	StringCchCopy(Buffer, Length, MspCachePath);	
}

ULONG
MspInitializeDtl(
	IN PWSTR CachePath	
	)
{
	ULONG Status;
	PMSP_COUNTER_OBJECT Counter;

	//
	// Initialize global dtl object list and lock
	//

	InitializeListHead(&MspDtlReportList);
	MspInitCsLock(&MspDtlReportLock, 100);

	//
	// Initialize live session dtl object
	//

	RtlZeroMemory(&MspDtlObject, sizeof(MSP_DTL_OBJECT));
	InitializeListHead(&MspDtlObject.ProcessListHead);
	MspInitCsLock(&MspDtlObject.ProcessListLock, 100);

	//
	// Initialize counter object
	//

	Counter = &MspDtlObject.CounterObject;
	Counter->Level = CounterRootLevel;
	StringCchCopy(Counter->Name, MAX_PATH, L"All");
	InitializeListHead(&Counter->GroupListHead);
	Counter->DtlObject = &MspDtlObject;

	//
	// Track start time
	//

	GetLocalTime(&MspDtlObject.StartTime);

	//
	// Validate cache path and initialize cache subsystem 
	//

	Status = MspValidateCachePath(CachePath);
	if (Status != MSP_STATUS_OK) {
		return MSP_STATUS_INVALID_PARAMETER;
	}

	StringCchCopy(MspCachePath, MAX_PATH, CachePath);
	StringCchCopy(MspDtlObject.Path, MAX_PATH, MspCachePath);

	Status = MspConfigureCache(&MspDtlObject, MspCachePath);
	if (Status != MSP_STATUS_OK) {
		return Status;
	}

	//
	// Initialize stack trace subsystem
	//

	Status = MspCreateStackTrace(&MspDtlObject.StackTrace);
	if (Status != MSP_STATUS_OK) {
		DebugBreak();
	}
	
	//
	// Initialize global stack trace queue
	//

	InitializeSListHead(&MspStackTraceQueue);

	return MSP_STATUS_OK;
}

ULONG
MspConfigureCache(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PWSTR Path
	)
{
	ULONG Status;
	UUID Uuid;
	RPC_STATUS RpcStatus;
	PWSTR StringUuid;

	PMSP_MAPPING_OBJECT MappingObject;
	PMSP_FILE_OBJECT IndexObject;
	PMSP_FILE_OBJECT DataObject;

	IndexObject = &DtlObject->IndexObject;
	DataObject = &DtlObject->DataObject;

	MspInitSpinLock(&IndexObject->Lock, 100);
	MspInitSpinLock(&DataObject->Lock, 100);

	//
	// Create session UUID
	//
	
	RpcStatus = UuidCreate(&Uuid);
	if (RpcStatus != RPC_S_OK && RpcStatus != RPC_S_UUID_LOCAL_ONLY) {
		return MSP_STATUS_NO_UUID;
	}

	MspSessionUuid = Uuid;
	UuidToString(&Uuid, &StringUuid);	
	StringCchCopy(MspSessionUuidString, 64, StringUuid);
	RpcStringFree(&StringUuid);

	//
	// Create session cache path based on UUID
	//

	StringCchPrintf(MspSessionPath, MAX_PATH, L"%s\\{%s}", Path, MspSessionUuidString);
	Status = CreateDirectory(MspSessionPath, NULL);
	if (!Status) {
		return MSP_STATUS_ACCESS_DENIED;
	}

	//
	// Create global index file per session
	//

	StringCchPrintf(IndexObject->Path, MAX_PATH, L"%s\\{%s}.i", MspSessionPath, MspSessionUuidString);
	IndexObject->FileObject = CreateFile(IndexObject->Path, GENERIC_READ|GENERIC_WRITE, 
		                                  FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
							              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (IndexObject->FileObject == INVALID_HANDLE_VALUE) {
		return MSP_STATUS_ACCESS_DENIED;	
	}
	IndexObject->Type = FileGlobalIndex;

	//
	// Set initial file size as 4MB
	//

	Status = SetFilePointer(IndexObject->FileObject, MSP_INDEX_FILE_INCREMENT, NULL, FILE_BEGIN);
	if (Status == INVALID_SET_FILE_POINTER) {
		return MSP_STATUS_SETFILEPOINTER;
	}
	Status = SetEndOfFile(IndexObject->FileObject);
	if (!Status) {
		return MSP_STATUS_SETFILEPOINTER;
	}
	IndexObject->FileLength = MSP_INDEX_FILE_INCREMENT;
	IndexObject->ValidLength = 0;

	//
	// Map the index file's first 64KB
	//

	MappingObject = (PMSP_MAPPING_OBJECT)MspMalloc(sizeof(MSP_MAPPING_OBJECT));
	MappingObject->MappedObject = CreateFileMapping(IndexObject->FileObject, NULL, 
													PAGE_READWRITE, 0, 0, NULL);
	if (!MappingObject->MappedObject) {
		return MSP_STATUS_ERROR;
	}

	MappingObject->MappedVa = (PBTR_FILE_INDEX)MapViewOfFile(MappingObject->MappedObject, 
		                                                     PAGE_READWRITE, 0, 0, 
															 MSP_INDEX_MAP_INCREMENT);	
	if (!MappingObject->MappedVa) {
		return MSP_STATUS_OUTOFMEMORY;
	}

	MappingObject->MappedOffset = 0;
	MappingObject->MappedLength = MSP_INDEX_MAP_INCREMENT;
	MappingObject->FirstSequence = 0;
	MappingObject->LastSequence = SEQUENCES_PER_INCREMENT - 1;

	IndexObject->Mapping = MappingObject;
	MappingObject->FileObject = IndexObject;

	//
	// Create global data file per session
	//

	StringCchPrintf(DataObject->Path, MAX_PATH, L"%s\\{%s}.d", MspSessionPath, MspSessionUuidString);
	DataObject->FileObject = CreateFile(DataObject->Path, GENERIC_READ|GENERIC_WRITE, 
		                                      FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
							                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (DataObject->FileObject == INVALID_HANDLE_VALUE) {
		return MSP_STATUS_ACCESS_DENIED;	
	}
	DataObject->Type = FileGlobalData;

	//
	// Set initial file size as 4MB
	//

	Status = SetFilePointer(DataObject->FileObject, MSP_DATA_FILE_INCREMENT, NULL, FILE_BEGIN);
	if (Status == INVALID_SET_FILE_POINTER) {
		return MSP_STATUS_SETFILEPOINTER;
	}
	Status = SetEndOfFile(DataObject->FileObject);
	if (!Status) {
		return MSP_STATUS_SETFILEPOINTER;
	}
	DataObject->FileLength = MSP_DATA_FILE_INCREMENT;
	DataObject->ValidLength = 0;

	//
	// Map the data file's first 64KB
	//

	MappingObject = (PMSP_MAPPING_OBJECT)MspMalloc(sizeof(MSP_MAPPING_OBJECT));
	MappingObject->MappedObject = CreateFileMapping(DataObject->FileObject, NULL, 
													PAGE_READWRITE, 0, 0, NULL);
	if (!MappingObject->MappedObject) {
		return MSP_STATUS_ERROR;
	}

	MappingObject->MappedVa = MapViewOfFile(MappingObject->MappedObject, 
		                                    PAGE_READWRITE, 0, 0, 
											MSP_INDEX_MAP_INCREMENT);	
	if (!MappingObject->MappedVa) {
		return MSP_STATUS_OUTOFMEMORY;
	}
	MappingObject->MappedOffset = 0;
	MappingObject->MappedLength = MSP_DATA_MAP_INCREMENT;

	DataObject->Mapping = MappingObject;
	MappingObject->FileObject = DataObject;

	//
	// Create shared data map
	//

	Status = MspCreateSharedDataMap();
	if (Status != MSP_STATUS_OK) {
		return Status;
	}

	return MSP_STATUS_OK;
}

ULONG
MspInitialize(
	IN MSP_MODE Mode,
	IN ULONG Flag,
	IN PWSTR RuntimeName,
	IN PWSTR RuntimePath,
	IN ULONG RuntimeFeature,
	IN PWSTR FilterPath,
	IN PWSTR CachePath
	)
{
	OSVERSIONINFOEX Information;
	SYSTEM_INFO SystemInfo;
	HMODULE DllHandle;
	HANDLE Handle;
	ULONG Status;

	if (MspInitialized) {
		return MSP_STATUS_OK;
	}

	//
	// Clean cache
	//

	MspCleanCache(CachePath, 0);

	//
	// Test whether it's a valid runtime path
	//

	DllHandle = LoadLibrary(RuntimePath);
	if (!DllHandle) {
		return MSP_STATUS_INVALID_PARAMETER;
	}
	
	StringCchCopy(MspRuntimeName, MAX_PATH, RuntimeName);
	StringCchCopy(MspRuntimePath, MAX_PATH, RuntimePath);

	//
	// Ensure FeatureRemote is enabled
	//

	if (!FlagOn(RuntimeFeature, FeatureRemote)) {
		return MSP_STATUS_INVALID_PARAMETER;
	}
	MspRuntimeFeature = RuntimeFeature;

	//
	// Create normal heap
	//

	MspPrivateHeap = MspCreatePrivateHeap();
	if (!MspPrivateHeap) {
		return MSP_STATUS_INIT_FAILED;
	}
	
	//
	// Get system version information
	//

	Information.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((LPOSVERSIONINFO)&Information);

	MspMajorVersion = Information.dwMajorVersion;
	MspMinorVersion = Information.dwMinorVersion;

	GetSystemInfo(&SystemInfo);
	MspPageSize = SystemInfo.dwPageSize;
	MspProcessorNumber = SystemInfo.dwNumberOfProcessors;

	if (MspIsWindowsXPAbove()) {
		DllHandle = GetModuleHandle(L"ntdll.dll");
		MspRtlCreateUserThread = (RTL_CREATE_USER_THREAD)GetProcAddress(DllHandle, "RtlCreateUserThread");
	}

	MspIsWow64ProcessPtr = (ISWOW64PROCESS)GetProcAddress(GetModuleHandle(L"kernel32"), 
		                                                  "IsWow64Process");
	if (!MspIs64Bits) {
		MspIsWow64 = MspIsWow64Process(GetCurrentProcess());
	}
	
	//
	// Compute performance counter frequency
	//

	MspComputeHardwareFrequency();

	//
	// Create global stop signal event
	//

	MspStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	ASSERT(MspStopEvent != NULL);

	//
	// Initialize log subsystem, default there's no logging.
	//

	MspInitializeLog(Flag);

	//
	// Build filter list
	//

	MspInitializeFilterList(FilterPath);

	//
	// Initialize dtl subsystem
	//

	Status = MspInitializeDtl(CachePath);

	//
	// Create scheduler thread
	//

	Handle = CreateThread(NULL, 0, MspCommandProcedure, NULL, 0, NULL);
	if (!Handle) {
		return MSP_STATUS_ERROR;
	} 

	CloseHandle(Handle);

	//
	// Start timer
	//

	MspStartTimer();

	return MSP_STATUS_OK;
}

ULONG
MspUninitialize(
	VOID
	)
{
	HANDLE DllHandle;

	//
	// Signal stop event to make timer thread, and indexer thread exit
	//

	SetEvent(MspStopEvent);

	WaitForSingleObject(MspIndexProcedureHandle, INFINITE);
	CloseHandle(MspIndexProcedureHandle);

	WaitForSingleObject(MspTimerThreadHandle, INFINITE);
	CloseHandle(MspTimerThreadHandle);

	//
	// Clear stack trace barrier, and wait decode thread exit
	//

	MspClearStackTraceBarrier();

	if (MspDecodeHandle != NULL) {
		WaitForSingleObject(MspDecodeHandle, INFINITE);
		CloseHandle(MspDecodeHandle);
	}

	CloseHandle(MspStopEvent);

	MspStopAllProcess();
	MspCloseCache();

	__try {
		
		MspUnloadFilters();

		while (DllHandle = GetModuleHandle(MspRuntimePath)) {
			FreeLibrary(DllHandle);
		}
	}__except(1) {

	}

	MspUninitializeLog();

	MspDestroyPrivateHeap(MspPrivateHeap);
	MspInitialized = FALSE;

	return MSP_STATUS_OK;
}

ULONG
MspCreateWorldSecurityAttr(
	OUT PMSP_SECURITY_ATTRIBUTE Attr
	)
{
    PACL Acl;
    PSID EveryoneSid;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;

    //
    // Initialize the security descriptor that allow Everyone group to access 
    //

	Acl = (PACL)MspMalloc(1024);
    InitializeAcl(Acl, 1024, ACL_REVISION);

    SecurityDescriptor = (PSECURITY_DESCRIPTOR)MspMalloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    InitializeSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    
	AllocateAndInitializeSid(&WorldAuthority, 1, SECURITY_WORLD_RID,
                             0, 0, 0, 0, 0, 0, 0, &EveryoneSid);

    AddAccessAllowedAce(Acl, ACL_REVISION, GENERIC_READ | GENERIC_WRITE, EveryoneSid);
    SetSecurityDescriptorDacl(SecurityDescriptor, TRUE, Acl, FALSE);

    Attr->Attribute.nLength = sizeof(SECURITY_ATTRIBUTES);
    Attr->Attribute.lpSecurityDescriptor = SecurityDescriptor;
    Attr->Attribute.bInheritHandle = FALSE;

	Attr->Acl = Acl;
	Attr->Sid = EveryoneSid;
	Attr->Descriptor = SecurityDescriptor;

	return MSP_STATUS_OK;
}

ULONG
MspCreateSharedDataMap(
	VOID
	)
{
	ULONG Length;

	//
	// Allocate writer map from page file and map its whole size
	//

	Length = MspUlongRoundUp(BTR_SHARED_DATA_SIZE, PAGESIZE); 
	MspSharedDataHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 
		                                    0, Length, NULL);

	if (!MspSharedDataHandle) {
		return MSP_STATUS_OUTOFMEMORY;
	}

	MspSharedData = (PBTR_SHARED_DATA)MapViewOfFile(MspSharedDataHandle, FILE_MAP_READ|FILE_MAP_WRITE, 
						                            0, 0, 0);
	if (!MspSharedData) {
		CloseHandle(MspSharedDataHandle);
		MspSharedDataHandle = NULL;
		return MSP_STATUS_OUTOFMEMORY;
	}
	
	//
	// Initialize global shared data 
	//

	MspInitSpinLock(&MspSharedData->SpinLock, 10);
	MspSharedData->Sequence = (ULONG)-1;
	MspSharedData->IndexFileLength = MspDtlObject.IndexObject.FileLength;
	MspSharedData->DataFileLength = MspDtlObject.DataObject.FileLength;
	MspSharedData->DataValidLength = 0;
	MspSharedData->SequenceFull = 0;
	MspSharedData->PendingReset = 0;

	return MSP_STATUS_OK;

}

ULONG
MspCopyMemory(
	IN PVOID Destine,
	IN PVOID Source,
	IN ULONG Size
	)
{
	__try {
	
		RtlCopyMemory(Destine, Source, Size);
		return MSP_STATUS_OK;
	}
	__except(1) {
	}

	return MSP_STATUS_EXCEPTION;
}

PBTR_RECORD_HEADER
MspGetRecordPointer(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG Sequence
	)
{
	ULONG Status;
	PMSP_MAPPING_OBJECT MappingObject;
	ULONG64 RecordOffset;
	ULONG64 MappingOffset;
	PBTR_RECORD_HEADER Record;
	BTR_FILE_INDEX Index = {0};

	Status = MspLookupIndexBySequence(DtlObject, Sequence, &Index);
	if (Status != MSP_STATUS_OK) {
		return NULL;
	}

	ASSERT(Index.Committed == 1);

	//
	// Compute the mapping region by record offset
	//

	RecordOffset = Index.Offset;
	MappingOffset = MspUlong64RoundDown(RecordOffset, MSP_INDEX_MAP_INCREMENT);

	//
	// Lookup mapping object by mapping region 
	//

	MappingObject = MspLookupMappingObject(DtlObject, RecordOffset);
	if (!MappingObject) {
		return NULL;
	}

	//
	// Compute record mapped address
	//

	Record = (PBTR_RECORD_HEADER)VA_FROM_OFFSET(MappingObject, RecordOffset);
	return Record;
}

ULONG
MspGetRecordPointerScatter(
	IN ULONG StartSequence,
	IN ULONG MaximumCount,
	OUT MSP_READ_SEGMENT Segment[],
	OUT ULONG CompleteCount
	)
{
	return 0;
}

ULONG WINAPI
MspReadRecord(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG Sequence,
	IN PVOID Buffer,
	IN ULONG Size
	)
{
	PBTR_RECORD_HEADER Record;
	BTR_FILE_INDEX Index;
	LARGE_INTEGER Offset;
	ULONG Length;
	ULONG Status;
	PMSP_FILE_OBJECT DataObject;
	static PVOID AlignedBuffer = 0;

	DataObject = &DtlObject->DataObject;

	//
	// Get index entry by its sequence number
	//

	Status = MspLookupIndexBySequence(DtlObject, Sequence, &Index);
	if (Status != MSP_STATUS_OK) {
		return Status;
	}

	//
	// Get file object by writer id of index
	//

	if (!AlignedBuffer) {
		AlignedBuffer = VirtualAlloc(NULL, PAGESIZE * 2, MEM_COMMIT, PAGE_READWRITE);
	}

	if (Index.Offset >= DataObject->FileLength) {
		MspAcquireSpinLock(&MspSharedData->SpinLock);
		DataObject->FileLength = MspSharedData->DataFileLength;
		MspReleaseSpinLock(&MspSharedData->SpinLock);
	}

	Offset.QuadPart = MspUlong64RoundDown(Index.Offset, 512);
	SetFilePointerEx(DataObject->FileObject, Offset, NULL, FILE_BEGIN);

	if ((ULONG64)Offset.QuadPart + 4096 < DataObject->FileLength) {
		Length = 4096;
	} else {
		Length = (ULONG)((ULONG64)DataObject->FileLength - (ULONG64)Offset.QuadPart);
	}

	ReadFile(DataObject->FileObject, AlignedBuffer, Length, &Length, NULL);
	Record = (PBTR_RECORD_HEADER)((PUCHAR)AlignedBuffer + (ULONG)((ULONG64)Index.Offset - (ULONG64)Offset.QuadPart));

	if (Record->TotalLength > Size) {
		return MSP_STATUS_BUFFER_TOO_SMALL;
	}

	return MspCopyMemory(Buffer, Record, Record->TotalLength);
}

ULONG WINAPI
MspCopyRecord(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG Sequence,
	IN PVOID Buffer,
	IN ULONG Size
	)
{
	PBTR_RECORD_HEADER Record;

	Record = MspGetRecordPointer(DtlObject, Sequence);
	if (!Record) {
		return MSP_STATUS_NO_RECORD;
	}

	if (Record->TotalLength > Size) {
		return MSP_STATUS_BUFFER_TOO_SMALL;
	}

	return MspCopyMemory(Buffer, Record, Record->TotalLength);
} 

ULONG WINAPI
MspGetFilteredRecordCount(
	IN PMSP_FILTER_FILE_OBJECT FileObject
	)
{
	return FileObject->NumberOfRecords;
}

PMSP_FILTER_FILE_OBJECT WINAPI
MspGetFilterFileObject(
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	return DtlObject->FilterFileObject;
}

PCONDITION_OBJECT
MspGetFilterCondition(
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	if (DtlObject->FilterFileObject) {
		return DtlObject->FilterFileObject->FilterObject;
	}

	return NULL;
}

PCONDITION_OBJECT
MspAttachFilterCondition(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PCONDITION_OBJECT Condition
	)
{
	PMSP_FILTER_FILE_OBJECT FileObject;
	PCONDITION_OBJECT Old;

	FileObject = DtlObject->FilterFileObject;

	if (FileObject) {

		//
		// Destroy old filter file object and create new one
		//

		Old = MspDestroyFilterFileObject(FileObject);
		MspCreateFilterFileObject(DtlObject, Condition);

		return Old;
	}
	
	//
	// Create filter file object
	//
	
	MspCreateFilterFileObject(DtlObject, Condition);
	return NULL;
}

PCONDITION_OBJECT
MspDetachFilterCondition(
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	PMSP_FILTER_FILE_OBJECT FileObject;
	PCONDITION_OBJECT Old;

	FileObject = DtlObject->FilterFileObject;

	if (FileObject) {
		Old = MspDestroyFilterFileObject(FileObject);
		return Old;
	}

	return NULL;
}

ULONG
MspComputeFilterScanStep(
	IN PMSP_FILTER_FILE_OBJECT FileObject
	)
{
	ULONG Current;
	ULONG Number;
	PMSP_DTL_OBJECT DtlObject;

	DtlObject = FileObject->DtlObject;
	Current = MspGetRecordCount(DtlObject);

	if ((FileObject->LastScannedSequence + 1) >= Current) {
		ASSERT((FileObject->LastScannedSequence + 1) == Current);
		return 0;
	}

	if (FileObject->LastScannedSequence != -1) {
		Number = Current - FileObject->LastScannedSequence;
	} else {
		Number = Current; 
	}

	return Number;
}

ULONG WINAPI
MspCopyRecordFiltered(
	IN PMSP_FILTER_FILE_OBJECT FileObject,
	IN ULONG FilterSequence,
	IN PVOID Buffer,
	IN ULONG Size
	)
{
	ULONG Status;
	PMSP_MAPPING_OBJECT MappingObject;
	ULONG Number;
	ULONG Sequence;
	ULONG Maximum;
	ULONG64 Offset;
	ULONG64 Start;

	MappingObject = FileObject->MappingObject2;

	if (IS_SEQUENCE_MAPPED(MappingObject, FilterSequence)) {
		Number = FilterSequence - MappingObject->FirstSequence;
		Sequence = MappingObject->FilterIndex[Number];
		Status = MspCopyRecord(FileObject->DtlObject, Sequence, Buffer, Size);
		return Status;
	}

	Offset = FilterSequence * sizeof(MSP_FILTER_INDEX);
	Start = MspUlong64RoundDown(Offset, MSP_INDEX_MAP_INCREMENT);

	Maximum = (ULONG)(MappingObject->FileLength / sizeof(MSP_FILTER_INDEX) - 1);	
	if (FilterSequence < Maximum) {
		
		if (MappingObject->MappedVa) {
			UnmapViewOfFile(MappingObject->MappedVa);
		}

		MappingObject->MappedVa = MapViewOfFile(MappingObject->MappedObject, 
			                                    FILE_MAP_READ|FILE_MAP_WRITE,
												(DWORD)(Start >> 32), (DWORD)Start, 
												MSP_INDEX_MAP_INCREMENT);
		if (!MappingObject->MappedVa) {
			ASSERT(0);
			return MSP_STATUS_OUTOFMEMORY;
		}

		MappingObject->MappedOffset = Start;
		MappingObject->MappedLength = MSP_INDEX_MAP_INCREMENT;
		MappingObject->FirstSequence = FilterSequence;
		MappingObject->LastSequence = FilterSequence + FILTER_SEQUENCES_PER_INCREMENT - 1;

		return MspCopyRecordFiltered(FileObject, FilterSequence, Buffer, Size);
	}

	//
	// File length is changed we need a new mapping object

	MappingObject->FileLength = FileObject->FileLength;
	UnmapViewOfFile(MappingObject->MappedVa);
	MappingObject->MappedVa = NULL;

	CloseHandle(MappingObject->MappedObject);
	MappingObject->MappedObject = CreateFileMapping(FileObject->FileObject, NULL, 
													PAGE_READWRITE, 0, 0, NULL);
	if (!MappingObject->MappedObject) {
		return MSP_STATUS_OUTOFMEMORY;
	}

	return MspCopyRecordFiltered(FileObject, FilterSequence, Buffer, Size);
}

PMSP_MAPPING_OBJECT
MspLookupMappingObject(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG64 Offset
	)
{
	PMSP_MAPPING_OBJECT MappingObject;
	PMSP_FILE_OBJECT DataObject;
	ULONG64 Start;
	ULONG64 End;
	ULONG64 FileLength;

	DataObject = &DtlObject->DataObject;
	MappingObject = DataObject->Mapping;

	Start = MspUlong64RoundDown(Offset, MSP_DATA_MAP_INCREMENT);

	//
	// SharedCache is continous, and it's possible that a record cross
	// allocation boundary, we need enlarge the mapping to ensure it's
	// mapped by a single view.
	//

	End = Start + MSP_DATA_MAP_INCREMENT * 2;

	if (IS_MAPPED_OFFSET(MappingObject, Start, End)) {
		return MappingObject;
	}

	if (MappingObject->MappedVa) {
		UnmapViewOfFile(MappingObject->MappedVa);
		MappingObject->MappedVa = NULL;
		MappingObject->MappedLength = 0;
		MappingObject->MappedOffset = 0;
	}

	if (End <= DataObject->FileLength) {

		MappingObject->MappedVa = MapViewOfFile(MappingObject->MappedObject, 
												FILE_MAP_READ, (DWORD)(Start >> 32), 
												(DWORD)Start, (ULONG)(End - Start));
		if (!MappingObject->MappedVa) {
			return NULL;
		}

		MappingObject->MappedOffset = Start;
		MappingObject->MappedLength = (ULONG)(End - Start);
		return MappingObject;
	}
	
	//
	// N.B. If it's beyond the file length, we must check whether it's
	// a dtl report, if it's a report, it's always safe to map to file end.
	//

	if (DtlObject->Type == DTL_REPORT) {
		
		MappingObject->MappedVa = MapViewOfFile(MappingObject->MappedObject, 
												FILE_MAP_READ, (DWORD)(Start >> 32), 
												(DWORD)Start, (ULONG)(DataObject->FileLength - Start));
		if (!MappingObject->MappedVa) {
			return NULL;
		}

		MappingObject->MappedOffset = Start;
		MappingObject->MappedLength = (ULONG)(DataObject->FileLength - Start);
		return MappingObject;
	}

	//
	// N.B. We must ensure the mapped range is within the file length
	// btr guarantee that if file length is not extended, all committed
	// records are within file length limit, this is important, because
	// we don't know record length, file length is the final firewall of
	// correct mapping.
	//

	MspAcquireSpinLock(&MspSharedData->SpinLock);
	FileLength = ReadForWriteAccess(&MspSharedData->DataFileLength);		
	MspReleaseSpinLock(&MspSharedData->SpinLock);

	if (FileLength == DataObject->FileLength) {

		//
		// N.B. File length is not changed yet, so the offset must be
		// within file length limit, get its minimum to remap again,
		// this guarantee that the record is mapped, and can be accessed
		// safely without page fault.
		//

		End = min(End, DataObject->FileLength);

		MappingObject->MappedVa = MapViewOfFile(MappingObject->MappedObject, 
												FILE_MAP_READ, (DWORD)(Start >> 32), 
												(DWORD)Start, (ULONG)(End - Start));
		if (!MappingObject->MappedVa) {
			return NULL;
		}

		MappingObject->MappedOffset = Start;
		MappingObject->MappedLength = (ULONG)(End - Start);
		return MappingObject;
	}

	MspCloseMappingObject(DataObject);

	DataObject->FileLength = FileLength;
	MappingObject = MspCreateMappingObject(DataObject, Start, (ULONG)(End - Start));
	if (!MappingObject) {
		ASSERT(0);
		return NULL;	
	}

	MappingObject->FileObject = DataObject;
	DataObject->Mapping = MappingObject;

	return MappingObject;
}

VOID
MspCloseMappingObject(
	IN PMSP_FILE_OBJECT FileObject
	)
{
	PMSP_MAPPING_OBJECT MappingObject;

	MappingObject = FileObject->Mapping;
	if (MappingObject) {
		if (MappingObject->MappedVa) {
			UnmapViewOfFile(MappingObject->MappedVa);
		}
		if (MappingObject->MappedObject) {
			CloseHandle(MappingObject->MappedObject);
		}
		MappingObject->MappedVa = NULL;
		MappingObject->MappedOffset = 0;
		MappingObject->MappedLength = 0;
	}
}

PMSP_MAPPING_OBJECT
MspCreateMappingObject(
	IN PMSP_FILE_OBJECT FileObject,
	IN ULONG64 Offset,
	IN ULONG Size
	)
{
	HANDLE Handle;
	PVOID Address;
	PMSP_MAPPING_OBJECT MappingObject;

	if (!MspIsUlong64Aligned(Offset, MSP_DATA_MAP_INCREMENT)) {
		ASSERT(0);
		return NULL;
	}

	if (Offset + Size > FileObject->FileLength) {
		ASSERT(0);
		return NULL;
	}

	Handle = CreateFileMapping(FileObject->FileObject, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!Handle) {
		return NULL;
	}

	Address = MapViewOfFile(Handle, FILE_MAP_READ, (DWORD)(Offset >> 32), 
				            (DWORD)Offset, Size);
	if (!Address) {
		return NULL;
	}

	MappingObject = FileObject->Mapping;

	if (!MappingObject) {
		MappingObject = (PMSP_MAPPING_OBJECT)MspMalloc(sizeof(MSP_MAPPING_OBJECT));
		if (!MappingObject) {
			UnmapViewOfFile(Address);	
			CloseHandle(Handle);
			return NULL;
		}
	}

	MappingObject->MappedObject = Handle;
	MappingObject->MappedVa = Address;
	MappingObject->MappedOffset = Offset;
	MappingObject->MappedLength = Size;

	return MappingObject;
}

ULONG
MspLookupIndexBySequence(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG Sequence,
	OUT PBTR_FILE_INDEX Index
	)
{
	ULONG64 Offset;
	PMSP_MAPPING_OBJECT MappingObject;
	PMSP_FILE_OBJECT IndexObject;
	PBTR_FILE_INDEX Entry;

	IndexObject = &DtlObject->IndexObject;
	MappingObject = IndexObject->Mapping;

	if (Sequence < IndexObject->NumberOfRecords) {

		if (IS_SEQUENCE_MAPPED(MappingObject, Sequence)) {
			Entry = VA_FROM_SEQUENCE(MappingObject, Sequence);		

			if (Entry->Committed) {
				*Index = *Entry;
				return MSP_STATUS_OK;
			} else {
				return MSP_STATUS_NO_SEQUENCE;
			}
		}
		
		if (MappingObject->MappedVa) {
			FlushViewOfFile(MappingObject->MappedVa, 0);
			UnmapViewOfFile(MappingObject->MappedVa);
			MappingObject->MappedVa = NULL;
		}

		Offset = MspUlong64RoundDown(OFFSET_FROM_SEQUENCE(Sequence), MSP_INDEX_MAP_INCREMENT);
		
		if (Offset < IndexObject->FileLength) {
			MappingObject->MappedVa = MapViewOfFile(MappingObject->MappedObject, PAGE_READWRITE, 
				                                    (DWORD)(Offset >> 32), (DWORD)Offset, 
													MSP_INDEX_MAP_INCREMENT );
			if (!MappingObject->MappedVa) {
				return MSP_STATUS_MAPVIEWOFFILE;
			}
			
			MappingObject->MappedOffset = Offset;
			MappingObject->MappedLength = MSP_INDEX_MAP_INCREMENT;
			MappingObject->FirstSequence = (ULONG)(Offset / sizeof(BTR_FILE_INDEX));
			MappingObject->LastSequence = MappingObject->FirstSequence + SEQUENCES_PER_INCREMENT - 1;
			
			return MspLookupIndexBySequence(DtlObject, Sequence, Index);
		}

		//
		// Index file length has been extended, we need reopen the file mapping object
		// to read the extended content.
		//

		MspAcquireSpinLock(&MspSharedData->SpinLock);
		IndexObject->FileLength = MspSharedData->IndexFileLength;
		MspReleaseSpinLock(&MspSharedData->SpinLock);

		ASSERT(IndexObject->FileLength > Offset);

		CloseHandle(MappingObject->MappedObject);
		MappingObject->MappedObject = CreateFileMapping(IndexObject->FileObject, NULL, 
			                                            PAGE_READWRITE, 0, 0, NULL);

		if (!MappingObject->MappedObject) {
			return MSP_STATUS_CREATEFILEMAPPING;
		}

		return MspLookupIndexBySequence(DtlObject, Sequence, Index);
	}

	return MSP_STATUS_NO_SEQUENCE;
}

ULONG MspFilterFileIndex = 0;

PMSP_FILTER_FILE_OBJECT
MspCreateFilterFileObject(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PCONDITION_OBJECT Filter
	)
{
	PMSP_FILTER_FILE_OBJECT FileObject;
	PMSP_MAPPING_OBJECT MappingObject;
	ULONG Status;
	ULONG Number;

	FileObject = (PMSP_FILTER_FILE_OBJECT)MspMalloc(sizeof(MSP_FILTER_FILE_OBJECT));
	if (!FileObject) {
		return NULL;
	}

	MspFilterFileIndex += 1;
	StringCchPrintf(FileObject->Path, MAX_PATH, L"%s\\{%s}.i.f%x", MspSessionPath,
					MspSessionUuidString, MspFilterFileIndex);

	FileObject->FileObject = CreateFile(FileObject->Path, GENERIC_READ|GENERIC_WRITE, 
		                                FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, 
						                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (FileObject->FileObject == INVALID_HANDLE_VALUE) {
		return NULL;
	}

	Status = MspExtendFileLength(FileObject->FileObject, MSP_INDEX_FILE_INCREMENT);
	if (Status != MSP_STATUS_OK) {
		return NULL;
	}
	
	FileObject->FileLength = MSP_INDEX_FILE_INCREMENT;
	FileObject->ValidLength = 0;
	FileObject->NumberOfRecords = 0;
	FileObject->FilterObject = Filter;

	//
	// N.B. LastScannedSequence is initialized as -1 to make a scan
	// start from sequence 0.
	//

	FileObject->LastScannedSequence = -1;

	//
	// Create file mapping object and map its first 64KB segment
	//

	for (Number = 0; Number < 2; Number += 1) {

		MappingObject = (PMSP_MAPPING_OBJECT)MspMalloc(sizeof(MSP_MAPPING_OBJECT));
		if (!MappingObject) {
			return NULL;
		}

		MappingObject->MappedObject = CreateFileMapping(FileObject->FileObject, NULL, PAGE_READWRITE, 0, 
														MSP_INDEX_FILE_INCREMENT, NULL);
		if (!MappingObject->MappedObject) {
			return NULL;
		}

		MappingObject->MappedVa = MapViewOfFile(MappingObject->MappedObject, FILE_MAP_READ|FILE_MAP_WRITE, 
												0, 0, MSP_INDEX_MAP_INCREMENT);
		if (!MappingObject->MappedVa) {
			return NULL;
		}

		MappingObject->MappedOffset = 0;
		MappingObject->MappedLength = MSP_INDEX_MAP_INCREMENT;
		MappingObject->FirstSequence = 0;
		MappingObject->LastSequence = FILTER_SEQUENCES_PER_INCREMENT - 1;
		MappingObject->FilterIndex = (PMSP_FILTER_INDEX)MappingObject->MappedVa;

		if (Number == 0) {
			FileObject->MappingObject = MappingObject;

		} else {

			//
			// N.B. MappingObject2 never change file length, so it must
			// track its mapping object's size to adjust its view appropriately.
			//

			MappingObject->FileLength = FileObject->FileLength;
			FileObject->MappingObject2 = MappingObject;
		}
	}

	DtlObject->FilterFileObject = FileObject;
	FileObject->DtlObject = DtlObject;

	return FileObject;
}

ULONG
MspCloseFileMapping(
	IN PMSP_MAPPING_OBJECT Mapping
	)
{
	ASSERT(Mapping != NULL);

	if (!Mapping) {
		return MSP_STATUS_OK;
	}

	if (Mapping->MappedVa) {
		UnmapViewOfFile(Mapping->MappedVa);
	}

	if (Mapping->MappedObject) {
		CloseHandle(Mapping->MappedObject);
	}

	MspFree(Mapping);
	return MSP_STATUS_OK;
}

PCONDITION_OBJECT
MspDestroyFilterFileObject(
	IN PMSP_FILTER_FILE_OBJECT FileObject
	)
{
	PMSP_DTL_OBJECT DtlObject;
	PCONDITION_OBJECT Condition;

	MspCloseFileMapping(FileObject->MappingObject);
	MspCloseFileMapping(FileObject->MappingObject2);

	CloseHandle(FileObject->FileObject);
	DeleteFile(FileObject->Path);

	Condition = FileObject->FilterObject;
	DtlObject = FileObject->DtlObject;
	DtlObject->FilterFileObject = NULL;

	MspFree(FileObject);
	return Condition;
}

ULONG
MspScanFilterRecord(
	IN PMSP_FILTER_FILE_OBJECT FileObject,
	IN ULONG ScanStep
	)
{
	ULONG Status;
	ULONG Maximum;
	ULONG LastSequence;
	ULONG Step;
	ULONG Start;
	ULONG Number;
	PMSP_FILE_OBJECT IndexObject;
	UCHAR Buffer[PAGESIZE];
	PMSP_DTL_OBJECT DtlObject;

	DtlObject = FileObject->DtlObject;
	if (!MspGetRecordCount(DtlObject)) {
		return 0;
	}

	IndexObject = &DtlObject->IndexObject;
	Maximum = IndexObject->NumberOfRecords - 1;
	LastSequence = FileObject->LastScannedSequence;

	//
	// If no sequence to be scanned, just return last
	// record number of current filter object.
	//

	if ((LastSequence != -1) && LastSequence >= Maximum) {
		return FileObject->NumberOfRecords;
	}

	//
	// Compute legitimate scan start sequence and step
	//

	Start = LastSequence + 1;
	Step = min(ScanStep, Maximum - LastSequence + 1);	
	
	for(Number = Start; Number < Start + Step; Number += 1) {

		Status = MspCopyRecord(FileObject->DtlObject, Number, Buffer, PAGESIZE);
		if (Status != MSP_STATUS_OK) {
			FileObject->LastScannedSequence = Number - 1;
			return FileObject->NumberOfRecords;
		}

		//
		// Check filter condition, if it's filtered record,
		// write its sequence number into filter index file
		//

		if (MspIsFilteredRecord(FileObject->DtlObject, FileObject->FilterObject,
			                    (PBTR_RECORD_HEADER)Buffer)) {

			Status = MspWriteFilterIndex(FileObject, (PBTR_RECORD_HEADER)Buffer, 
										 Number, FileObject->NumberOfRecords);
			if (Status == MSP_STATUS_OK) {
				FileObject->NumberOfRecords += 1;
			}
			else {
				return FileObject->NumberOfRecords;
			}
		}
	}

	FileObject->LastScannedSequence = Number - 1;
	return FileObject->NumberOfRecords;
}

BOOLEAN
MspIsFilteredRecord(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PCONDITION_OBJECT FilterObject,
	IN PBTR_RECORD_HEADER Record
	)
{
	PLIST_ENTRY ListEntry;
    PCONDITION_ENTRY Condition;
    BOOLEAN Filtered;

	ASSERT(DtlObject != NULL && FilterObject != NULL);

	//
	// N.B. if there's no condition in filter object, it's treated
	// as no filter at all, however, this should not happen under
	// normal circumstance.
	//

	if (IsListEmpty(&FilterObject->ConditionListHead)) {
		return TRUE;
	}

    ListEntry = FilterObject->ConditionListHead.Flink;

    while (ListEntry != &FilterObject->ConditionListHead) {
        Condition = CONTAINING_RECORD(ListEntry, CONDITION_ENTRY, ListEntry);
		Filtered = (*ConditionHandlers[Condition->Object])(DtlObject, Condition, Record);
        if (Filtered != TRUE) {
            return FALSE;
        }
        ListEntry = ListEntry->Flink;
    }

    return TRUE;
}

ULONG
MspWriteFilterIndex(
	IN PMSP_FILTER_FILE_OBJECT FileObject,
	IN PBTR_RECORD_HEADER Record,
	IN ULONG Sequence,
	IN ULONG FilterSequence
	)
{
	ULONG Status;
	ULONG Number;
	ULONG Maximum;
	ULONG64 Start;
	ULONG64 Length;
	PMSP_MAPPING_OBJECT MappingObject;

	//
	// Compute whether current view is not yet finished scan.
	//

	MappingObject = FileObject->MappingObject;
	ASSERT(MappingObject != NULL);

	if (IS_SEQUENCE_MAPPED(MappingObject, FilterSequence)) {
		ASSERT(FilterSequence <= MappingObject->LastSequence);
		Number = FilterSequence - MappingObject->FirstSequence;
		MappingObject->FilterIndex[Number] = Sequence;
		return MSP_STATUS_OK;
	}
	
	Maximum = (ULONG)(FileObject->FileLength / sizeof(BTR_FILE_INDEX) - 1);	
	if (FilterSequence < Maximum) {
		
		Start = MappingObject->MappedOffset + MappingObject->MappedLength,
		ASSERT(Start + MSP_INDEX_MAP_INCREMENT <= FileObject->FileLength);

		if (MappingObject->MappedVa) {
			FlushViewOfFile(MappingObject->MappedVa, 0);
			UnmapViewOfFile(MappingObject->MappedVa);
		}

		MappingObject->MappedVa = MapViewOfFile(MappingObject->MappedObject, 
			                                    FILE_MAP_READ|FILE_MAP_WRITE,
												(DWORD)(Start >> 32), (DWORD)Start, 
												MSP_INDEX_MAP_INCREMENT);
		if (!MappingObject->MappedVa) {
			ASSERT(0);
			return MSP_STATUS_OUTOFMEMORY;
		}

		MappingObject->MappedOffset = Start;
		MappingObject->MappedLength = MSP_INDEX_MAP_INCREMENT;
		MappingObject->FirstSequence = FilterSequence;
		MappingObject->LastSequence = FilterSequence + FILTER_SEQUENCES_PER_INCREMENT - 1;

		return MspWriteFilterIndex(FileObject, Record, Sequence, FilterSequence);
	}

	//
	// File length is limited, extend filter index file length
	//

	Length = FileObject->FileLength;
	Length += MSP_INDEX_FILE_INCREMENT;
	Status = MspExtendFileLength(FileObject->FileObject, Length);
	if (Status != MSP_STATUS_OK) {
		ASSERT(0);
		return Status;
	}
	FileObject->FileLength = Length;

	//
	// The index file is already extended, use the new length to create
	// new file mapping object, note that we only unmap/null old view and keep
	// all other members unchanged to support recursive calls.
	//

	FlushViewOfFile(MappingObject->MappedVa, 0);
	UnmapViewOfFile(MappingObject->MappedVa);
	MappingObject->MappedVa = NULL;

	CloseHandle(MappingObject->MappedObject);
	MappingObject->MappedObject = CreateFileMapping(FileObject->FileObject, NULL, 
													PAGE_READWRITE, 0, 0, NULL);
	if (!MappingObject->MappedObject) {
		ASSERT(0);
		return MSP_STATUS_OUTOFMEMORY;
	}

	return MspWriteFilterIndex(FileObject, Record, Sequence, FilterSequence);
}

ULONG
MspQueueFilterWorkItem(
	IN PMSP_FILTER_FILE_OBJECT FileObject
	)
{
	if (!FileObject->WorkItemEvent) {
		FileObject->WorkItemEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	QueueUserWorkItem(MspFilterWorkRoutine, FileObject, 
		              WT_EXECUTEDEFAULT|WT_EXECUTELONGFUNCTION);	
	return 0;
}

ULONG CALLBACK
MspFilterWorkRoutine(
	IN PVOID Context
	)
{
	PMSP_FILTER_FILE_OBJECT FileObject;
	ULONG Status;
	ULONG Number;

	FileObject = (PMSP_FILTER_FILE_OBJECT)Context;
	ASSERT(FileObject != NULL);

	while (1) {
	
		Status = WaitForSingleObject(FileObject->WorkItemEvent, 0);
		if (Status == WAIT_OBJECT_0) {
			return 0;
		}
	
		Number = MspScanFilterRecord(FileObject, 1024);
		Sleep(1000);
	}

	return 0;
}

ULONG CALLBACK
MspIndexProcedure(
	IN PVOID Context
	)
{
	HANDLE TimerHandle;
	ULONG TimerPeriod;
	LARGE_INTEGER DueTime;
	ULONG Status;
	HANDLE Handles[2];
	PMSP_FILE_OBJECT FileObject;
	PMSP_FILE_OBJECT IndexObject;
	PMSP_FILE_OBJECT MasterFileObject;
	PMSP_MAPPING_OBJECT MappingObject;
	ULONG NextSequence = 0;

	//
	// Clone a file object from global index file object 
	// since it's only used in index procedure, keep it private.
	//

	MasterFileObject = (PMSP_FILE_OBJECT)Context;
	FileObject = (PMSP_FILE_OBJECT)MspMalloc(sizeof(MSP_FILE_OBJECT));
	FileObject->Type = MasterFileObject->Type;
	FileObject->FileLength = MasterFileObject->FileLength;

	//
	// Create file object based on master file object
	//

	IndexObject = MspGetIndexObject(MspDtlObject);
	StringCchCopy(FileObject->Path, MAX_PATH, MasterFileObject->Path);
	FileObject->FileObject = CreateFile(IndexObject->Path, GENERIC_READ|GENERIC_WRITE, 
		                                FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
							            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (FileObject->FileObject == INVALID_HANDLE_VALUE) {
		MspFree(FileObject);
		return MSP_STATUS_CREATEFILE;
	}

	MappingObject = (PMSP_MAPPING_OBJECT)MspMalloc(sizeof(MSP_MAPPING_OBJECT));
	MappingObject->MappedObject = CreateFileMapping(FileObject->FileObject, NULL, 
		                                            PAGE_READWRITE, 0, 0, NULL);
	if (!MappingObject->MappedObject) {
		CloseHandle(FileObject->FileObject);
		MspFree(MappingObject);
		MspFree(FileObject);
		return MSP_STATUS_ERROR;
	}

	MappingObject->MappedVa = MapViewOfFile(MappingObject->MappedObject, 
		                                    FILE_MAP_READ|FILE_MAP_WRITE, 
										    0, 0, MSP_INDEX_MAP_INCREMENT);
	if (!MappingObject->MappedVa) {
		CloseHandle(MappingObject->MappedObject);
		CloseHandle(FileObject->FileObject);
		MspFree(MappingObject);
		MspFree(FileObject);
		return MSP_STATUS_ERROR;
	}

	MappingObject->MappedOffset = 0;
	MappingObject->MappedLength = MSP_INDEX_MAP_INCREMENT;
	MappingObject->FirstSequence = 0;
	MappingObject->LastSequence = SEQUENCES_PER_INCREMENT - 1;

	MappingObject->FileObject = FileObject;
	FileObject->Mapping = MappingObject;

	//
	// Create work loop timer, half second expires
	//

	TimerHandle = CreateWaitableTimer(NULL, FALSE, NULL);
	TimerPeriod = 500;

	DueTime.QuadPart = -10000 * TimerPeriod;
	Status = SetWaitableTimer(TimerHandle, &DueTime, TimerPeriod, 
		                      NULL, NULL, FALSE);	
	if (Status != TRUE) {
		Status = GetLastError();
		return Status;
	}

	Handles[0] = TimerHandle;
	Handles[1] = MspStopEvent;

	Status = MSP_STATUS_OK;

	while (Status == MSP_STATUS_OK) {
	
		ULONG Signal;
		Signal = WaitForMultipleObjects(2, Handles, FALSE, INFINITE);

		switch (Signal) {

			case WAIT_OBJECT_0:

				Status = MspUpdateNextSequence(FileObject, MappingObject, &NextSequence);
				if (Status == MSP_STATUS_OK) {

					//
					// Update sequence to master index file object 
					//

					MasterFileObject->NumberOfRecords = NextSequence;
				}
				
				break;

			case WAIT_OBJECT_0 + 1:
				Status = MSP_STATUS_STOPPED;
				break;

			default:
				Status = MSP_STATUS_ERROR;
				break;
		} 
		
	}

	if (MappingObject->MappedVa) {
		UnmapViewOfFile(MappingObject->MappedVa);
	}
	if (MappingObject->MappedObject) {
		CloseHandle(MappingObject->MappedObject);
	}
	if (FileObject->FileObject) {
		CloseHandle(FileObject->FileObject);
	}

	MspFree(MappingObject);
	MspFree(FileObject);

	CancelWaitableTimer(TimerHandle);
	CloseHandle(TimerHandle);
	return Status;
}

ULONG
MspUpdateNextSequence(
	IN PMSP_FILE_OBJECT FileObject,
	IN PMSP_MAPPING_OBJECT MappingObject,
	IN PULONG NextSequence
	)
{
	ULONG Status;
	ULONG Number;
	ULONG Sequence;
	ULONG Maximum;
	ULONG64 Start;
	ULONG64 Length;
	PBTR_FILE_INDEX Index;

	//
	// Compute whether current view is not yet finished scan.
	//

	Sequence = *NextSequence;
	Index = (PBTR_FILE_INDEX)MappingObject->MappedVa;

	if (IS_SEQUENCE_MAPPED(MappingObject, Sequence)) {

		ASSERT(Sequence <= MappingObject->LastSequence);
		Number = Sequence - MappingObject->FirstSequence;
		for ( ; Number < SEQUENCES_PER_INCREMENT; Number += 1) {
			if (Index[Number].Committed == 1) {
				Sequence += 1;
			} else {
				break;
			}
		}
	}
	
	//
	// If scaned current sequence is within current view limits, return
	//

	if (Sequence <= MappingObject->LastSequence) {
		*NextSequence = Sequence;
		return MSP_STATUS_OK;
	}

	//
	// The current view is fully scanned, slide to next region, first check
	// file length limits.
	//

	Maximum = (ULONG)(FileObject->FileLength / sizeof(BTR_FILE_INDEX) - 1);	
	if (Sequence < Maximum) {
		
		Start = MappingObject->MappedOffset + MappingObject->MappedLength,
		ASSERT(Start + MSP_INDEX_MAP_INCREMENT <= FileObject->FileLength);

		if (MappingObject->MappedVa) {
			UnmapViewOfFile(MappingObject->MappedVa);
		}

		MappingObject->MappedVa = MapViewOfFile(MappingObject->MappedObject, 
			                                    FILE_MAP_READ|FILE_MAP_WRITE,
												(DWORD)(Start >> 32), (DWORD)Start, 
												MSP_INDEX_MAP_INCREMENT);
		if (!MappingObject->MappedVa) {
			return MSP_STATUS_OUTOFMEMORY;
		}

		MappingObject->MappedOffset = Start;
		MappingObject->MappedLength = MSP_INDEX_MAP_INCREMENT;
		MappingObject->FirstSequence = Sequence;
		MappingObject->LastSequence = Sequence + SEQUENCES_PER_INCREMENT - 1;

		*NextSequence = Sequence;
		return MspUpdateNextSequence(FileObject, MappingObject, NextSequence);
	}

	//
	// Current file length is scanned over, let's check whether the file length is 
	// already extended.
	//

	MspAcquireSpinLock(&MspSharedData->SpinLock);
	Length = MspSharedData->IndexFileLength;
	MspReleaseSpinLock(&MspSharedData->SpinLock);

	if (Length == FileObject->FileLength) {

		MspAcquireSpinLock(&MspSharedData->SpinLock);

		Length = MspSharedData->IndexFileLength;
		if (Length != FileObject->FileLength) {
			
			//
			// Other thread already extended the file length, do nothing here
			//

		} else {

			Length += MSP_INDEX_FILE_INCREMENT;
			Status = MspExtendFileLength(FileObject->FileObject, Length);
			if (Status == MSP_STATUS_OK) {
				MspSharedData->IndexFileLength = Length;
			}
		}

		MspReleaseSpinLock(&MspSharedData->SpinLock);

		if (Status != MSP_STATUS_OK) {
			return Status;
		}
	}

	//
	// The index file is already extended, use the new length to create
	// new file mapping object, note that we only unmap/null old view and keep
	// all other members unchanged to support recursive calls.
	//

	UnmapViewOfFile(MappingObject->MappedVa);
	MappingObject->MappedVa = NULL;

	CloseHandle(MappingObject->MappedObject);
	MappingObject->MappedObject = CreateFileMapping(FileObject->FileObject, NULL, 
													PAGE_READWRITE, 0, 0, NULL);
	if (!MappingObject->MappedObject) {
		return MSP_STATUS_OUTOFMEMORY;
	}

	//
	// Update current file length
	//

	FileObject->FileLength = Length;
	*NextSequence = Sequence;

	return MspUpdateNextSequence(FileObject, MappingObject, NextSequence);
}

ULONG
MspExtendFileLength(
	IN HANDLE FileObject,
	IN ULONG64 Length
	)
{
	ULONG Status;
	LARGE_INTEGER NewLength;

	NewLength.QuadPart = Length;

	Status = SetFilePointerEx(FileObject, NewLength, NULL, FILE_BEGIN);
	if (!Status) {
		Status = GetLastError();
		return Status;
	}

	Status = SetEndOfFile(FileObject);
	if (!Status) {
		Status = GetLastError();
		return Status;
	}

	return MSP_STATUS_OK;
}

ULONG
MspCloseCache(
	VOID
	)
{
	PMSP_FILE_OBJECT IndexObject, DataObject;

	//
	// N.B. MspCloseCache assume all traces are stopped, so
	// btr side won't have any writer working there.
	//

	IndexObject = MspGetIndexObject(MspDtlObject);
	DataObject = MspGetDataObject(MspDtlObject);

	MspCloseFileObject(IndexObject, FALSE);
	MspFree(IndexObject->Mapping);
	
	MspCloseFileObject(DataObject, FALSE);
	MspFree(DataObject->Mapping);

	DeleteFile(IndexObject->Path);
	DeleteFile(DataObject->Path);

	//
	// Close stack trace object
	//

	MspCloseStackTrace(&MspDtlObject.StackTrace);
	
	//
	// Delete session directory
	//

	RemoveDirectory(MspSessionPath);
	return MSP_STATUS_OK;
}

ULONG
MspCloseFileObject(
	IN PMSP_FILE_OBJECT FileObject,
	IN BOOLEAN FreeRequired
	)
{
	PMSP_MAPPING_OBJECT MappingObject;

	ASSERT(FileObject != NULL);

	MappingObject = FileObject->Mapping;

	if (MappingObject != NULL) {
		if (MappingObject->MappedVa) {
			UnmapViewOfFile(MappingObject->MappedVa);
		}
		if (MappingObject->MappedObject) {
			CloseHandle(MappingObject->MappedObject);
		}
		if (FreeRequired) {
			MspFree(MappingObject);
		}
	}

	CloseHandle(FileObject->FileObject);

	if (FreeRequired) {
		MspFree(FileObject);
	}

	return MSP_STATUS_OK;
}

VOID
MspInitSpinLock(
	IN PMSP_SPINLOCK Lock,
	IN ULONG SpinCount
	)
{
	Lock->Acquired = 0;
	Lock->ThreadId = 0;
	Lock->SpinCount = SpinCount;
}

VOID
MspAcquireSpinLock(
	IN PMSP_SPINLOCK Lock
	)
{
	ULONG Acquired;
	ULONG ThreadId;
	ULONG OwnerId;
	ULONG Count;
	ULONG SpinCount;

	ThreadId = GetCurrentThreadId();
	OwnerId = ReadForWriteAccess(&Lock->ThreadId);
	if (OwnerId == ThreadId) {
	    return;
	}

	SpinCount = Lock->SpinCount;

	while (TRUE) {
		
		Acquired = InterlockedBitTestAndSet((volatile LONG *)&Lock->Acquired, 0);
		if (Acquired != 1) {
			Lock->ThreadId = ThreadId;
			break;
		}

		Count = 0;

		do {

			YieldProcessor();
			Acquired = ReadForWriteAccess(&Lock->Acquired);

			Count += 1;
			if (Count >= SpinCount) { 
				SwitchToThread();
				break;
			}
		} while (Acquired != 0);
	}	
}

VOID
MspReleaseSpinLock(
	IN PMSP_SPINLOCK Lock
	)
{	
	Lock->ThreadId = 0;
	_InterlockedAnd((volatile LONG *)&Lock->Acquired, 0);
}

VOID
MspInitCsLock(
	IN PMSP_CS_LOCK Lock,
	IN ULONG SpinCount
	)
{
	InitializeCriticalSectionAndSpinCount(Lock, SpinCount);
}

VOID
MspAcquireCsLock(
	IN PMSP_CS_LOCK Lock
	)
{
	EnterCriticalSection(Lock);
}

VOID
MspReleaseCsLock(
	IN PMSP_CS_LOCK Lock
	)
{
	LeaveCriticalSection(Lock);
}

VOID
MspDeleteCsLock(
	IN PMSP_CS_LOCK Lock
	)
{
	DeleteCriticalSection(Lock);
}

ULONG
MspStartIndexProcedure(
	IN PMSP_FILE_OBJECT FileObject
	)
{
	MspIndexProcedureHandle = CreateThread(NULL, 0, MspIndexProcedure, FileObject, 0, NULL);
	if (!MspIndexProcedureHandle) {
		return MSP_STATUS_ERROR;
	}

	return MSP_STATUS_OK;
}

HANDLE
MspDuplicateHandle(
	IN HANDLE DestineProcess,
	IN HANDLE SourceHandle
	)
{
	ULONG Status;
	HANDLE DestineHandle;

	Status = DuplicateHandle(GetCurrentProcess(), 
				             SourceHandle,
							 DestineProcess,
							 &DestineHandle,
							 DUPLICATE_SAME_ACCESS,
							 FALSE,
							 DUPLICATE_SAME_ACCESS);
	if (Status) {
		return DestineHandle;
	}

	return NULL;
}

ULONG
MspCreateSharedMemory(
	IN PWSTR ObjectName,
	IN SIZE_T Length,
	IN ULONG ViewOffset,
	IN SIZE_T ViewSize,
	OUT PMSP_MAPPING_OBJECT *Object
	)
{
	HANDLE Handle;
	PVOID Address;
	PMSP_MAPPING_OBJECT Mapping;

	Handle = CreateFileMapping(NULL, NULL, PAGE_READWRITE, ((ULONG64)Length >> 32),
							   (ULONG)Length, ObjectName);
	if (!Handle) {
		return GetLastError();
	}

	Address = MapViewOfFile(Handle, FILE_MAP_READ|FILE_MAP_WRITE, ((ULONG64)ViewOffset >> 32), 
		                    (ULONG)ViewOffset, ViewSize );

	if (!Address) {
		CloseHandle(Handle);
		return GetLastError();
	}

	Mapping = (PMSP_MAPPING_OBJECT)MspMalloc(sizeof(MSP_MAPPING_OBJECT));
	RtlZeroMemory(Mapping, sizeof(*Mapping));

	InitializeListHead(&Mapping->ListEntry);

	Mapping->Type = PageFileBacked;
	Mapping->MappedVa = Address;
	Mapping->MappedLength = (ULONG)ViewSize;
	Mapping->MappedOffset = ViewOffset;
	Mapping->FileLength = Length;

	if (ObjectName != NULL) {
		//StringCchCopy(Mapping->ObjectName, 64, ObjectName);
	}

	*Object = Mapping;
	return MSP_STATUS_OK;
}

ULONG
MspSaveFile(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PWSTR Path,
	IN MSP_SAVE_TYPE Type,
	IN ULONG Count,
	IN BOOLEAN Filtered,
	IN MSP_WRITE_CALLBACK Callback,
	IN PVOID Context
	)
{
	PMSP_SAVE_CONTEXT SaveContext;

	SaveContext = (PMSP_SAVE_CONTEXT)MspMalloc(sizeof(MSP_SAVE_CONTEXT));
	SaveContext->Object = DtlObject;
	SaveContext->Type = Type;
	SaveContext->Count = Count;
	SaveContext->Filtered = Filtered;
	SaveContext->Callback = Callback;
	SaveContext->Context = Context;
	StringCchCopy(SaveContext->Path, MAX_PATH, Path);

	if (Type == MSP_SAVE_DTL) {
		QueueUserWorkItem(MspWriteDtlWorkItem, SaveContext,  
			              WT_EXECUTEDEFAULT|WT_EXECUTELONGFUNCTION);	
	}

	else if (Type == MSP_SAVE_CSV) {
		QueueUserWorkItem(MspWriteCsvWorkItem, SaveContext,  
			              WT_EXECUTEDEFAULT|WT_EXECUTELONGFUNCTION);	
	}

	else {
		ASSERT(0);
	}

	return MSP_STATUS_OK;
}

ULONG CALLBACK
MspWriteCsvWorkItem(
	IN PVOID Context
	)
{
	ULONG Number;
	ULONG Status;
	PVOID Cache;
	ULONG Complete;
	ULONG Round;
	ULONG Remainder;
	WCHAR Path[MAX_PATH];
	ULONG RecordCount;
	HANDLE DataHandle;
	HANDLE IndexHandle;
	HANDLE CsvHandle;
	BOOLEAN Filtered;
	PMSP_DTL_OBJECT Object;
	PMSP_SAVE_CONTEXT SaveContext;
	MSP_WRITE_CALLBACK Callback;
	PVOID CallbackContext;
	BOOLEAN Cancel;
	ULONG Percent;

	SaveContext = (PMSP_SAVE_CONTEXT)Context;
	Object = SaveContext->Object;
	
	Cancel = FALSE;
	Callback = SaveContext->Callback;
	CallbackContext = SaveContext->Context;

	//
	// Filtered write go to another code path
	//

	Filtered = SaveContext->Filtered;
	if (Filtered) {
		Status = MspWriteCsvFiltered(SaveContext);
		return Status;
	} 
	
	//
	// Create CSV file
	//

	StringCchCopy(Path, MAX_PATH, SaveContext->Path);
	CsvHandle = CreateFile(Path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
	                       FILE_ATTRIBUTE_NORMAL, NULL);

	if (CsvHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		return Status;
	}

	//
	// Create data file 
	//

	DataHandle = CreateFile(Object->DataObject.Path, GENERIC_READ | GENERIC_WRITE, 
		                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (DataHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		CloseHandle(CsvHandle);
		DeleteFile(Path);
		return Status;
	}

	//
	// Create index file
	//

	IndexHandle = CreateFile(Object->IndexObject.Path, GENERIC_READ | GENERIC_WRITE, 
			                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (IndexHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		CloseHandle(DataHandle);
		CloseHandle(CsvHandle);
		DeleteFile(Path);
		return Status;
	}

	//
	// Write CSV format columns
	//

	WriteFile(CsvHandle, MspCsvRecordColumn, (ULONG)strlen(MspCsvRecordColumn), &Complete, NULL);

	//
	// N.B. Presume here page size is 4K, we allocate 64K index cache which hold
	// 4K index
	//

	RecordCount = MspGetRecordCount(Object);

	Cache = BspAllocateGuardedChunk(16);
	Round = RecordCount / 4096;
	Remainder = RecordCount % 4096;

	for (Number = 0; Number < Round; Number += 1) { 

		Status = ReadFile(IndexHandle, Cache, sizeof(BTR_FILE_INDEX) * 4096, &Complete, NULL);
		if (Status != TRUE) {
			Status = GetLastError();
			goto CsvExit;
		}

		Status = MspWriteCsvRound(Object, Cache, 4096, DataHandle, CsvHandle);
		if (Status != MSP_STATUS_OK) {
			goto CsvExit;
		}
		
	    Percent = ((Number + 1) * 4096 * 100) / RecordCount;
		Cancel = (*Callback)(CallbackContext, Percent, FALSE, 0);

		if (Cancel) {
			goto CsvExit;
		}

	}

	if (Remainder != 0) {
		
		Status = ReadFile(IndexHandle, Cache, (ULONG)(sizeof(BTR_FILE_INDEX) * Remainder), &Complete, NULL);
		if (Status != TRUE) {
			Status = GetLastError();
			goto CsvExit;
		}

		Status = MspWriteCsvRound(Object, Cache, (ULONG)Remainder, DataHandle, CsvHandle);
		if (Status != MSP_STATUS_OK) {
			goto CsvExit;
		}
		
	}

	(*Callback)(CallbackContext, 100, FALSE, 0);

CsvExit:

	CloseHandle(DataHandle);
	CloseHandle(IndexHandle);
	CloseHandle(CsvHandle);
	BspFreeGuardedChunk(Cache);

	if (Status != MSP_STATUS_OK) {
		(*Callback)(CallbackContext, Percent, TRUE, Status);
		DeleteFile(Path);
	}

	if (Cancel) { 
		DeleteFile(Path);
	}
	
	return MSP_STATUS_OK;
}

ULONG
MspWriteCsvRound(
	IN PMSP_DTL_OBJECT Object,
	IN PBTR_FILE_INDEX IndexCache,
	IN ULONG NumberOfIndex,
	IN HANDLE DataHandle,
	IN HANDLE CsvHandle
	)
{
	ULONG Status;
	ULONG CompleteLength;
	PBTR_RECORD_HEADER Record;
	ULONG Number;
	ULONG Count;
	SYSTEMTIME Time;
	double Duration;
	WCHAR Name[MAX_PATH];
	WCHAR Process[MAX_PATH];
	WCHAR Detail[MAX_PATH];
	CHAR Cache[4096];
	CHAR Buffer[4096];

	for(Number = 0; Number < NumberOfIndex; Number += 1) {

		ASSERT(IndexCache[Number].Length < 4096);

		//
		// Read record
		//

		ReadFile(DataHandle, Cache, (ULONG)IndexCache[Number].Length, &CompleteLength, NULL);
		Record = (PBTR_RECORD_HEADER)Cache;

		//
		// Decode process name
		//

		Process[0] = 0;
		MspDecodeProcessName(Object, Record, Process, MAX_PATH);

		//
		// Decode probe name
		//

		Name[0] = 0;
		MspDecodeProbeName(Object, Record, Name, MAX_PATH);

		//
		// Decode detail
		//

		Detail[0] = 0;
		MspDecodeDetail(Record, Detail, MAX_PATH);

		FileTimeToSystemTime(&Record->FileTime, &Time);
		Duration = BspComputeMilliseconds(Record->Duration);

		Count = _snprintf(Buffer, BspPageSize - 1, MspCsvRecordFormat, 
						  Process, Time.wHour, Time.wMinute, Time.wSecond, Time.wMilliseconds,
						  Record->ProcessId, Record->ThreadId, Name, Duration, Record->Return, Detail);

		if (Count != -1) {
			Status = WriteFile(CsvHandle, Buffer, Count, &CompleteLength, NULL); 
			if (Status != TRUE) {
				Status = GetLastError();
				return Status;
			}
		}
	}

	return MSP_STATUS_OK;
}

ULONG
MspWriteCsvFiltered(
	IN PMSP_SAVE_CONTEXT Context
	)
{
	ULONG Status;
	ULONG Complete;
	ULONG Round;
	WCHAR Path[MAX_PATH];
	ULONG RecordCount;
	HANDLE DataHandle;
	HANDLE IndexHandle;
	HANDLE CsvHandle;
	HANDLE SequenceHandle;
	PMSP_DTL_OBJECT Object;
	PMSP_FILTER_FILE_OBJECT FileObject;
	MSP_WRITE_CALLBACK Callback;
	PVOID CallbackContext;
	BOOLEAN Cancel;
	ULONG Percent;
	ULONG i, j;
	ULONG Sequence;
	LARGE_INTEGER Offset;
	BTR_FILE_INDEX Index;
	ULONG RecordLength;
	PBTR_RECORD_HEADER Record;
	ULONG Count;
	SYSTEMTIME Time;
	double Duration;
	WCHAR Name[MAX_PATH];
	WCHAR Process[MAX_PATH];
	WCHAR Detail[MAX_PATH];
	CHAR Buffer[4096];

	Cancel = FALSE;
	Callback = Context->Callback;
	CallbackContext = Context->Context;

	Object = Context->Object;
	FileObject = MspGetFilterFileObject(Object);
	ASSERT(FileObject != NULL);

	RecordCount = MspGetFilteredRecordCount(FileObject);

	//
	// Create CSV file
	//

	StringCchCopy(Path, MAX_PATH, Context->Path);
	CsvHandle = CreateFile(Path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
	                       FILE_ATTRIBUTE_NORMAL, NULL);

	if (CsvHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		(*Callback)(CallbackContext, 0, TRUE, Status);
		return MSP_STATUS_OK;
	}

	//
	// Create data file 
	//

	DataHandle = CreateFile(Object->DataObject.Path, GENERIC_READ | GENERIC_WRITE, 
		                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (DataHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		(*Callback)(CallbackContext, 0, TRUE, Status);
		CloseHandle(CsvHandle);
		DeleteFile(Path);
		return MSP_STATUS_OK;
	}

	//
	// Create index file
	//

	IndexHandle = CreateFile(Object->IndexObject.Path, GENERIC_READ | GENERIC_WRITE, 
			                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (IndexHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		(*Callback)(CallbackContext, 0, TRUE, Status);
		CloseHandle(DataHandle);
		CloseHandle(CsvHandle);
		DeleteFile(Path);
		return MSP_STATUS_OK;
	}

	//
	// Filter sequence file
	//

	SequenceHandle = CreateFile(FileObject->Path, GENERIC_READ | GENERIC_WRITE, 
							    FILE_SHARE_READ | FILE_SHARE_WRITE, 
							    NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (SequenceHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		(*Callback)(CallbackContext, 0, TRUE, Status);
		CloseHandle(CsvHandle);
		CloseHandle(IndexHandle);
		CloseHandle(DataHandle);
		DeleteFile(Path);

		return MSP_STATUS_OK;
	}

	//
	// Write CSV format columns
	//

	WriteFile(CsvHandle, MspCsvRecordColumn, (ULONG)strlen(MspCsvRecordColumn), &Complete, NULL);

	Round = RecordCount / 9;
	j = 0;
	Cancel = FALSE;
	
	Record = (PBTR_RECORD_HEADER)VirtualAlloc(NULL, 1024 * 64, MEM_COMMIT, PAGE_READWRITE);

	for(i = 0; i < RecordCount; i++) {

		//
		// Read sequence
		//

		Status = ReadFile(SequenceHandle, &Sequence, sizeof(ULONG), &Complete, NULL);
		if (!Status) {
			goto Exit;
		}

		//
		// Read index
		//

		Offset.QuadPart = Sequence * sizeof(BTR_FILE_INDEX);
		Status = SetFilePointerEx(IndexHandle, Offset, NULL, FILE_BEGIN); 
		if (!Status) {
			goto Exit;
		}

		Status = ReadFile(IndexHandle, &Index, sizeof(Index), &Complete, NULL);
		if (!Status) {
			goto Exit;
		}

		//
		// Read record
		//

		Offset.QuadPart = Index.Offset;
		RecordLength = (ULONG)Index.Length;

		Status = SetFilePointerEx(DataHandle, Offset, NULL, FILE_BEGIN);
		if (!Status) {
			goto Exit;
		}

		Status = ReadFile(DataHandle, Record, RecordLength, &Complete, NULL);
		if (!Status) {
			goto Exit;
		}

		//
		// Decode process name
		//

		Process[0] = 0;
		MspDecodeProcessName(Object, Record, Process, MAX_PATH);

		//
		// Decode probe name
		//

		Name[0] = 0;
		MspDecodeProbeName(Object, Record, Name, MAX_PATH);

		//
		// Decode detail
		//

		Detail[0] = 0;
		MspDecodeDetail(Record, Detail, MAX_PATH);

		FileTimeToSystemTime(&Record->FileTime, &Time);
		Duration = BspComputeMilliseconds(Record->Duration);

		Count = _snprintf(Buffer, BspPageSize - 1, MspCsvRecordFormat, 
						  Process, Time.wHour, Time.wMinute, Time.wSecond, Time.wMilliseconds,
						  Record->ProcessId, Record->ThreadId, Name, Duration, Record->Return, Detail);

		if (Count != -1) {
			Status = WriteFile(CsvHandle, Buffer, Count, &Complete, NULL); 
			if (!Status) {
				goto Exit;
			}
		}

		//
		// Call callback every %10 increment
		//

		if (i != 0) {

			if (Round != 0 && (i % Round == 0)) {

				j += 1;
				Percent = j * 10;
				Percent = min(Percent, 90);

				Cancel = (*Callback)(CallbackContext, Percent, FALSE, 0);
				if (Cancel) {
					Status = FALSE;
					goto Exit;
				}
			}
		}
	}

Exit:
	
	CloseHandle(CsvHandle);

	if (!Status) {
		
		if (!Cancel) {
			Status = GetLastError();
			(*Callback)(CallbackContext, 0, TRUE, Status);
		}

		DeleteFile(Path);
	} 

	CloseHandle(IndexHandle);
	CloseHandle(DataHandle);
	CloseHandle(SequenceHandle);
	VirtualFree(Record, 0, MEM_RELEASE);

	return MSP_STATUS_OK;
}

ULONG CALLBACK
MspWriteDtlWorkItem(
	IN PVOID Context
	)
{
	ULONG Status;
	ULONG64 Length;
	HANDLE FileHandle;
	ULONG CompleteLength;
	MSP_DTL_HEADER Header;
	ULONG Low, High;
	ULONG64 IndexLength;
	ULONG64 DataLength;
	ULONG NumberOfRecords;
	PMSP_DTL_OBJECT DtlObject;
	PMSP_FILE_OBJECT IndexObject;
	PMSP_FILE_OBJECT DataObject;
	PMSP_SAVE_CONTEXT SaveContext;
	MSP_WRITE_CALLBACK Callback;
	PVOID CallbackContext;
	BOOLEAN Cancel;

	SaveContext = (PMSP_SAVE_CONTEXT)Context;

	//
	// If it's filtered, branch to MspWriteDtlFiltered()
	//

	if (SaveContext->Filtered) {
		return MspWriteDtlFiltered(SaveContext);
	}

	DtlObject = SaveContext->Object;
	Callback = SaveContext->Callback;
	CallbackContext = SaveContext->Context;
	NumberOfRecords = SaveContext->Count;

	IndexObject = MspGetIndexObject((*DtlObject));
	DataObject = MspGetDataObject((*DtlObject));

	Status = MspComputeReplicaLength(DtlObject, NumberOfRecords, &IndexLength, &DataLength);
	if (Status != MSP_STATUS_OK) {
		(*Callback)(CallbackContext, 0, TRUE, Status);
		return Status;
	}

	//
	// Replicate data snapshot
	//

	Cancel = FALSE;
    //FileHandle = MspReplicateData(IndexObject, DataObject, SaveContext, IndexLength, DataLength, &Cancel);
    FileHandle = MspReplicateRecord(IndexObject, DataObject, SaveContext, 
		                            IndexLength, DataLength, NumberOfRecords, &Cancel);

	if (Cancel) {
		return MSP_STATUS_OK;
	}

	if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		(*Callback)(CallbackContext, 0, TRUE, Status);
		return Status;
	}

	RtlZeroMemory(&Header, sizeof(Header));

	Header.RecordLocation = 0; 
	Header.RecordLength = DataLength;
	Header.IndexLocation = DataLength;
	Header.IndexNumber = NumberOfRecords; 
	Header.IndexLength = IndexLength;

	Length = DataLength + IndexLength;
	Low = (ULONG)Length;
	High = (ULONG)(Length >> 32);

	//
	// Write stack trace database
	//
	
	Header.StackLocation = Length;
	MspWriteStackTraceToDtl(DtlObject, FileHandle, Header.StackLocation, NULL);

	Low = GetFileSize(FileHandle, &High);
	Header.ProbeLocation = (ULONG64)(((ULONG64)High << 32) | (ULONG64)Low);
	Header.StackLength = Header.ProbeLocation - Header.StackLocation;

	//
	// Append probe groups
	//

	MspWriteProbeToDtl(DtlObject, FileHandle, Header.ProbeLocation, NULL);
	Low = GetFileSize(FileHandle, &High);
	Header.SummaryLocation = (ULONG64)(((ULONG64)High << 32) | (ULONG64)Low);
	Header.ProbeLength = Header.SummaryLocation - Header.ProbeLocation;
	
	//
	// Append summary report
	//

	Low = GetFileSize(FileHandle, &High);
	Length = (ULONG64)(((ULONG64)High << 32) | (ULONG64)Low);
	Header.SummaryLength = Length - Header.SummaryLocation;

	//
	// Fill dtl header fields
	//

	Header.TotalLength = Length + sizeof(MSP_DTL_HEADER);
	Header.Signature = MSP_DTL_SIGNATURE;
	Header.MajorVersion = 1;
	Header.MinorVersion = 0;

#if defined (_M_X64)
	Header.Is64Bits = TRUE;
#else
	Header.Is64Bits = FALSE;
#endif

	Header.StartTime = DtlObject->StartTime;
	Header.EndTime = DtlObject->EndTime;
	Header.Reserved[0] = 0;
	Header.Reserved[1] = 0;
	Header.CheckSum = MspComputeDtlCheckSum(&Header);

	//
	// N.B. We put the file 'header' in fact at the end of the file, this
	// obtain benefit that we don't need to adjust the record offset when
	// we read the record in report mode, meanwhile, it's easy to append
	// new segment to the whole dtl file.
	//

	WriteFile(FileHandle, &Header, sizeof(Header), &CompleteLength, NULL);
	SetEndOfFile(FileHandle);
	CloseHandle(FileHandle);

    (*Callback)(CallbackContext, 100, FALSE, 0);
	return MSP_STATUS_OK;
}

ULONG 
MspWriteDtlFiltered(
	IN PMSP_SAVE_CONTEXT Context
	)
{
	ULONG Status;
	ULONG64 Length;
	ULONG RecordLength;
	ULONG Complete;
	MSP_DTL_HEADER Header;
	ULONG Low, High;
	ULONG64 IndexLength;
	ULONG64 DataLength;
	ULONG NumberOfRecords;
	PMSP_DTL_OBJECT DtlObject;
	PMSP_FILE_OBJECT IndexObject;
	PMSP_FILE_OBJECT DataObject;
	MSP_WRITE_CALLBACK Callback;
	PMSP_FILTER_FILE_OBJECT FileObject;
	PVOID CallbackContext;
	BOOLEAN Cancel;
	PCHAR Buffer;
	ULONG i, j;
	ULONG Round;
	ULONG Remain;
	HANDLE FileHandle;
	HANDLE SequenceHandle;
	HANDLE IndexHandle;
	HANDLE DataHandle;
	HANDLE IndexCacheHandle;
	WCHAR Path[MAX_PATH];
	WCHAR IndexCachePath[MAX_PATH];
	ULONG Sequence;
	BTR_FILE_INDEX Index;
	LARGE_INTEGER Offset;
	ULONG Percent;

	DtlObject = Context->Object;
	FileObject = DtlObject->FilterFileObject;
	ASSERT(FileObject != NULL);

	NumberOfRecords = Context->Count;
	ASSERT(NumberOfRecords <= FileObject->NumberOfRecords);

	Callback = Context->Callback;
	CallbackContext = Context->Context;
	StringCchCopy(Path, MAX_PATH, Context->Path);
	StringCchCopy(IndexCachePath, MAX_PATH, Path);
	StringCchCat(IndexCachePath, MAX_PATH, L".tmp");

	IndexObject = &DtlObject->IndexObject;
	DataObject = &DtlObject->DataObject;

	//
	// Destine File handle
	//

	FileHandle = CreateFile(Path, GENERIC_READ | GENERIC_WRITE, 
							FILE_SHARE_READ | FILE_SHARE_WRITE, 
							NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		(*Callback)(CallbackContext, 0, TRUE, Status);
		return MSP_STATUS_OK;
	}

	//
	// Filter sequence file
	//

	SequenceHandle = CreateFile(FileObject->Path, GENERIC_READ | GENERIC_WRITE, 
							    FILE_SHARE_READ | FILE_SHARE_WRITE, 
							    NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	if (SequenceHandle == INVALID_HANDLE_VALUE) {

		Status = GetLastError();
		(*Callback)(CallbackContext, 0, TRUE, Status);

		CloseHandle(FileHandle);
		DeleteFile(Path);

		return MSP_STATUS_OK;
	}

	//
	// Data file handle
	//

    DataHandle = CreateFile(DataObject->Path, GENERIC_READ | GENERIC_WRITE, 
							FILE_SHARE_READ | FILE_SHARE_WRITE, 
							NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (DataHandle == INVALID_HANDLE_VALUE) {

		Status = GetLastError();
		(*Callback)(CallbackContext, 0, TRUE, Status);

		CloseHandle(SequenceHandle);
		CloseHandle(FileHandle);
		DeleteFile(Path);

		return MSP_STATUS_OK;
	}

	//
	// Index file handle
	//

    IndexHandle = CreateFile(IndexObject->Path, GENERIC_READ | GENERIC_WRITE, 
							 FILE_SHARE_READ | FILE_SHARE_WRITE, 
							 NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (IndexHandle == INVALID_HANDLE_VALUE) {

		Status = GetLastError();
		(*Callback)(CallbackContext, 0, TRUE, Status);

		CloseHandle(DataHandle);
		CloseHandle(SequenceHandle);
		CloseHandle(FileHandle);
		DeleteFile(Path);

		return MSP_STATUS_OK;
	}

	//
	// Index cache file handle
	//

    IndexCacheHandle = CreateFile(IndexCachePath, GENERIC_READ | GENERIC_WRITE, 
							 FILE_SHARE_READ | FILE_SHARE_WRITE, 
							 NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (IndexCacheHandle == INVALID_HANDLE_VALUE) {

		Status = GetLastError();
		(*Callback)(CallbackContext, 0, TRUE, Status);

		CloseHandle(IndexHandle);
		CloseHandle(DataHandle);
		CloseHandle(SequenceHandle);
		CloseHandle(FileHandle);
		DeleteFile(Path);

		return MSP_STATUS_OK;
	}

	Status = TRUE;
	DataLength = 0;
	Buffer = VirtualAlloc(NULL, DTL_COPY_UNIT, MEM_COMMIT, PAGE_READWRITE);

	Round = NumberOfRecords / 9;
	j = 0;
	Cancel = FALSE;

	//
	// N.B. Set stack trace barrier to pause decode thread to avoid
	// race with current thread.
	//

	if (DtlObject->Type == DTL_LIVE) {
		MspSetStackTraceBarrier();
	}

	for(i = 0; i < NumberOfRecords; i++) {

		//
		// Read sequence
		//

		Status = ReadFile(SequenceHandle, &Sequence, sizeof(ULONG), &Complete, NULL);
		if (!Status) {
			goto Exit;
		}

		//
		// Read index
		//

		Offset.QuadPart = Sequence * sizeof(BTR_FILE_INDEX);
		Status = SetFilePointerEx(IndexHandle, Offset, NULL, FILE_BEGIN); 
		if (!Status) {
			goto Exit;
		}

		Status = ReadFile(IndexHandle, &Index, sizeof(Index), &Complete, NULL);
		if (!Status) {
			goto Exit;
		}

		//
		// Read record
		//

		Offset.QuadPart = Index.Offset;
		RecordLength = (ULONG)Index.Length;

		Status = SetFilePointerEx(DataHandle, Offset, NULL, FILE_BEGIN);
		if (!Status) {
			goto Exit;
		}

		Status = ReadFile(DataHandle, Buffer, RecordLength, &Complete, NULL);
		if (!Status) {
			goto Exit;
		}

		//
		// Adjust index and write new index and record
		//

		Index.Offset = DataLength;
		Status = WriteFile(IndexCacheHandle, &Index, sizeof(Index), &Complete, NULL);
		if (!Status) {
			goto Exit;
		}

		Status = WriteFile(FileHandle, Buffer, RecordLength, &Complete, NULL);
		if (!Status) {
			goto Exit;
		}
		
		//
		// N.B. Decode if and only if it's current dtl object
		//

		if (DtlObject->Type == DTL_LIVE) {
			MspDecodeStackTraceByRecord((PBTR_RECORD_HEADER)Buffer, &DtlObject->StackTrace);
		}

		DataLength += RecordLength;

		//
		// Call callback every %10 increment
		//

		if (i != 0) {

			if (Round != 0 && (i % Round == 0)) {

				j += 1;
				Percent = j * 10;
				Percent = min(Percent, 90);

				Cancel = (*Callback)(CallbackContext, Percent, FALSE, 0);
				if (Cancel) {
					Status = FALSE;
					goto Exit;
				}
			}
		}
	}

	//
	// Roundup both index and data length to align with map increment
	//
	
	IndexLength = NumberOfRecords * sizeof(BTR_FILE_INDEX);
	IndexLength = MspUlong64RoundUp(IndexLength, MSP_INDEX_MAP_INCREMENT);
	DataLength = MspUlong64RoundUp(DataLength, MSP_INDEX_MAP_INCREMENT);

	Offset.QuadPart = IndexLength;
	SetFilePointerEx(IndexCacheHandle, Offset, NULL, FILE_BEGIN);
	SetEndOfFile(IndexCacheHandle);

	Offset.QuadPart = DataLength;
	SetFilePointerEx(FileHandle, Offset, NULL, FILE_BEGIN);
	SetEndOfFile(FileHandle);

	//
	// Start using non-buffered copy to append index file into end of data file
	//

	Round = (ULONG)(IndexLength / DTL_COPY_UNIT);
	Remain = (ULONG)(IndexLength % DTL_COPY_UNIT);

	//
	// Reset set index cache file pointer to start
	//

	Offset.QuadPart = 0;
	SetFilePointerEx(IndexCacheHandle, Offset, NULL, FILE_BEGIN);	
	
	for(i = 0; i < Round; i++) {
		Status = ReadFile(IndexCacheHandle, Buffer, DTL_COPY_UNIT, &Complete, NULL);
		if (!Status) {
			goto Exit;
		}
		Status = WriteFile(FileHandle, Buffer, DTL_COPY_UNIT, &Complete, NULL);
		if (!Status) {
			goto Exit;
		}
	}

	Status = ReadFile(IndexCacheHandle, Buffer, Remain, &Complete, NULL);
	if (!Status) {
		goto Exit;
	}

	Status = WriteFile(FileHandle, Buffer, Remain, &Complete, NULL);
	if (!Status) {
		goto Exit;
	}


Exit:
	
	if (DtlObject->Type == DTL_LIVE) {
		MspClearStackTraceBarrier();
		MspClearDbghelp();
	}

	if (!Status) {
		
		if (!Cancel) {
			Status = GetLastError();
			(*Callback)(CallbackContext, 0, TRUE, Status);
		}

		CloseHandle(IndexHandle);
		CloseHandle(DataHandle);
		CloseHandle(SequenceHandle);

		CloseHandle(FileHandle);
		DeleteFile(Path);

		CloseHandle(IndexCacheHandle);
		DeleteFile(IndexCachePath);

		VirtualFree(Buffer, 0, MEM_RELEASE);
		return MSP_STATUS_OK;
	}
	else {
		
		Status = MSP_STATUS_OK;
		CloseHandle(IndexHandle);
		CloseHandle(DataHandle);
		CloseHandle(SequenceHandle);
		CloseHandle(IndexCacheHandle);
		DeleteFile(IndexCachePath);
		VirtualFree(Buffer, 0, MEM_RELEASE);
	}

	//
	// Write headers
	//

	RtlZeroMemory(&Header, sizeof(Header));

	Header.RecordLocation = 0; 
	Header.RecordLength = DataLength;
	Header.IndexLocation = DataLength;
	Header.IndexNumber = NumberOfRecords; 
	Header.IndexLength = IndexLength;

	Length = DataLength + IndexLength;
	Low = (ULONG)Length;
	High = (ULONG)(Length >> 32);

	//
	// Write stack trace database
	//
	
	Header.StackLocation = Header.IndexLocation + Header.IndexLength;
	MspWriteStackTraceToDtl(DtlObject, FileHandle, Header.StackLocation, NULL);

	Low = GetFileSize(FileHandle, &High);
	Header.ProbeLocation = (ULONG64)(((ULONG64)High << 32) | (ULONG64)Low);
	Header.StackLength = Header.ProbeLocation - Header.StackLocation;

	//
	// Append probe groups
	//

	MspWriteProbeToDtl(DtlObject, FileHandle, Header.ProbeLocation, NULL);
	Low = GetFileSize(FileHandle, &High);
	Header.SummaryLocation = (ULONG64)(((ULONG64)High << 32) | (ULONG64)Low);
	Header.ProbeLength = Header.SummaryLocation - Header.ProbeLocation;
	
	//
	// Append summary report
	//

	Low = GetFileSize(FileHandle, &High);
	Length = (ULONG64)(((ULONG64)High << 32) | (ULONG64)Low);
	Header.SummaryLength = Length - Header.SummaryLocation;

	//
	// Fill dtl header fields
	//

	Header.TotalLength = Length + sizeof(MSP_DTL_HEADER);
	Header.Signature = MSP_DTL_SIGNATURE;
	Header.MajorVersion = 1;
	Header.MinorVersion = 0;

#if defined (_M_X64)
	Header.Is64Bits = TRUE;
#else
	Header.Is64Bits = FALSE;
#endif

	Header.StartTime = DtlObject->StartTime;
	Header.EndTime = DtlObject->EndTime;
	Header.Reserved[0] = 0;
	Header.Reserved[1] = 0;
	Header.CheckSum = MspComputeDtlCheckSum(&Header);

	//
	// N.B. We put the file 'header' in fact at the end of the file, this
	// obtain benefit that we don't need to adjust the record offset when
	// we read the record in report mode, meanwhile, it's easy to append
	// new segment to the whole dtl file.
	//

	WriteFile(FileHandle, &Header, sizeof(Header), &Complete, NULL);
	CloseHandle(FileHandle);

    (*Callback)(CallbackContext, 100, FALSE, 0);
	return MSP_STATUS_OK;
}

ULONG
MspComputeDtlCheckSum(
	IN PMSP_DTL_HEADER Header
	)
{
	ULONG ReservedOffset;
	ULONG MajorVersionOffset;	
	PULONG Buffer;
	ULONG Result;
	ULONG Seed;
	ULONG Number;

	//
	// N.B. If any new members are appended in MSP_DTL_HEADER
	// structure, we should consider adapt it into checksum computing,
	// this routine ensure that the file header was not damaged to avoid
	// to read into bad data, it include MajorVersion exclude Reserved.
	//

	ReservedOffset = FIELD_OFFSET(MSP_DTL_HEADER, Reserved);
	MajorVersionOffset = FIELD_OFFSET(MSP_DTL_HEADER, MajorVersion);

	ASSERT((ReservedOffset - MajorVersionOffset) % sizeof(ULONG) == 0);

	Seed = 'ltd';
	Result = 0;

	Buffer = (PULONG)&Header->MajorVersion;
	for(Number = 0; Number < (ReservedOffset - MajorVersionOffset) / sizeof(ULONG); Number += 1) {
		Result += Seed ^ Buffer[Number];
	}

	DebugTrace("MspDtlComputeCheckSum  0x%08x", Result);
	return Result;
}

ULONG
MspComputeReplicaLength(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG NumberOfRecords,
	OUT PULONG64 IndexLength,
	OUT PULONG64 DataLength
	)
{
	ULONG Status;
	BTR_FILE_INDEX Index;
	PVOID Buffer;
	ULONG64 Size;
	PMSP_FILE_OBJECT IndexObject;
	PMSP_FILE_OBJECT DataObject;

	ASSERT(NumberOfRecords > 1);

	IndexObject = &DtlObject->IndexObject;
	DataObject = &DtlObject->DataObject;

	Status = MspLookupIndexBySequence(DtlObject, NumberOfRecords - 1, &Index);
	if (Status != MSP_STATUS_OK) {
		return Status;
	}
	
	Buffer = MspMalloc(BTR_MAXIMUM_RECORD_SIZE);
	Status = MspCopyRecord(DtlObject, NumberOfRecords - 1, Buffer, BTR_MAXIMUM_RECORD_SIZE);
	if (Status != MSP_STATUS_OK) {
		return Status;
	}

	//
	// N.B. The index and data length must be aligned with map increment boundary
	// this will make report file easily mapped and accessed, the length must make
	// sense to be within limit of current file length.
	//

	Size = MspUlong64RoundUp(NumberOfRecords * sizeof(BTR_FILE_INDEX), 
		                     MSP_INDEX_MAP_INCREMENT);
	*IndexLength = min(Size, IndexObject->FileLength);

    Size = MspUlong64RoundUp(Index.Offset + ((PBTR_RECORD_HEADER)Buffer)->TotalLength,
		                     MSP_DATA_MAP_INCREMENT);
	*DataLength = min(Size, DataObject->FileLength);

	MspFree(Buffer);
	return MSP_STATUS_OK;
}

#define SHOULD_COPY_NO_BUFFERING(_L) \
	(_L > 1024 * 1024 * 64)

HANDLE
MspReplicateData(
	IN PMSP_FILE_OBJECT IndexObject,
	IN PMSP_FILE_OBJECT DataObject,
	IN PMSP_SAVE_CONTEXT Context,
    IN ULONG64 IndexLength,
    IN ULONG64 DataLength,
	OUT BOOLEAN *Cancel
    )
{
    ULONG Status;
    HANDLE FileHandle;
	HANDLE IndexHandle;
    HANDLE DataHandle;
    PVOID Buffer;
    ULONG Percent;
    ULONG CompleteLength;
    ULONG64 Round, Remain, i;
	ULONG64 TotalLength;
	MSP_WRITE_CALLBACK Callback;
	PVOID CallbackContext;
	WCHAR Path[MAX_PATH];

	*Cancel = FALSE;
	Callback = Context->Callback;
	CallbackContext = Context->Context;
	StringCchCopy(Path, MAX_PATH, Context->Path);

	TotalLength = DataLength + IndexLength;

	//
	// Create target file
	//

    FileHandle = CreateFile(Path, GENERIC_READ | GENERIC_WRITE, 
							FILE_SHARE_READ | FILE_SHARE_WRITE, 
							NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		return INVALID_HANDLE_VALUE;
	}

    DataHandle = CreateFile(DataObject->Path, GENERIC_READ | GENERIC_WRITE, 
							FILE_SHARE_READ | FILE_SHARE_WRITE, 
							NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (DataHandle == INVALID_HANDLE_VALUE) {
		CloseHandle(FileHandle);
		DeleteFile(Path);
		return INVALID_HANDLE_VALUE;
	}

    IndexHandle = CreateFile(IndexObject->Path, GENERIC_READ | GENERIC_WRITE, 
							 FILE_SHARE_READ | FILE_SHARE_WRITE, 
							 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (IndexHandle == INVALID_HANDLE_VALUE) {
		CloseHandle(DataHandle);
		CloseHandle(FileHandle);
		DeleteFile(Path);
		return INVALID_HANDLE_VALUE;
	}

	Buffer = VirtualAlloc(NULL, DTL_COPY_UNIT, MEM_COMMIT, PAGE_READWRITE);

	//
	// Copy data
	//

	Round = DataLength / DTL_COPY_UNIT;
	Remain = DataLength % DTL_COPY_UNIT; 

	for(i = 0; i < Round; i++) {

		ReadFile(DataHandle, Buffer, DTL_COPY_UNIT, &CompleteLength, NULL);
		WriteFile(FileHandle, Buffer, DTL_COPY_UNIT, &CompleteLength, NULL);

		Percent = (ULONG)((DTL_COPY_UNIT * (i + 1)) / (TotalLength / 100));
		*Cancel = (*Callback)(CallbackContext, Percent, FALSE, 0);

		if (*Cancel) {

			CloseHandle(DataHandle);
			CloseHandle(FileHandle);
			DeleteFile(Path);

			VirtualFree(Buffer, 0, MEM_RELEASE);
			return INVALID_HANDLE_VALUE;
		}
	}

	ReadFile(DataHandle, Buffer, (ULONG)Remain, &CompleteLength, NULL);
	WriteFile(FileHandle, Buffer, (ULONG)Remain, &CompleteLength, NULL);
	CloseHandle(DataHandle);

	//
	// Copy index
	//

	Round = IndexLength / DTL_COPY_UNIT;
	Remain = IndexLength % DTL_COPY_UNIT; 

	for(i = 0; i < Round; i++) {

		ReadFile(IndexHandle, Buffer, DTL_COPY_UNIT, &CompleteLength, NULL);
		WriteFile(FileHandle, Buffer, DTL_COPY_UNIT, &CompleteLength, NULL);

		Percent = (ULONG)((DTL_COPY_UNIT * (i + 1) + DataLength) / (TotalLength / 100));
		*Cancel = (*Callback)(CallbackContext, Percent, FALSE, 0);

		if (Percent == 100) {
			*Cancel = FALSE;
		}

		if (*Cancel) {

			CloseHandle(IndexHandle);
			CloseHandle(FileHandle);
			DeleteFile(Path);

			VirtualFree(Buffer, 0, MEM_RELEASE);
			return INVALID_HANDLE_VALUE;
		}
	}

	if (Remain > 0) {
		ReadFile(IndexHandle, Buffer, (ULONG)Remain, &CompleteLength, NULL);
		WriteFile(FileHandle, Buffer, (ULONG)Remain, &CompleteLength, NULL);
		CloseHandle(IndexHandle);
	}

    //
    // N.B. We don't specify it's complete since we still want to copy
    // index, stack trace, probe group etc to the final file.
    //

    *Cancel = (*Callback)(CallbackContext, 99, FALSE, 0);
    VirtualFree(Buffer, 0, MEM_RELEASE);

	if (*Cancel) {

		CloseHandle(FileHandle);
		DeleteFile(Path);

		return INVALID_HANDLE_VALUE;
	}
	
	return FileHandle;
}

HANDLE
MspReplicateRecord(
	IN PMSP_FILE_OBJECT IndexObject,
	IN PMSP_FILE_OBJECT DataObject,
	IN PMSP_SAVE_CONTEXT Context,
	IN ULONG64 IndexLength,
	IN ULONG64 DataLength,
	IN ULONG RecordCount,
	OUT BOOLEAN *Cancel
    )
{
	ULONG Status;
    HANDLE FileHandle;
	HANDLE IndexHandle;
    HANDLE DataHandle;
    PVOID Buffer;
    ULONG CurrentPercent;
    ULONG LastPercent;
    ULONG CompleteLength;
	ULONG i;
	ULONG64 TotalLength;
	ULONG64 WrittenLength;
	MSP_WRITE_CALLBACK Callback;
	PVOID CallbackContext;
	WCHAR Path[MAX_PATH];
	BTR_FILE_INDEX Index;
	LARGE_INTEGER Offset;
	PMSP_DTL_OBJECT DtlObject;
	ULONG Round, Remain;

	*Cancel = FALSE;
	Callback = Context->Callback;
	CallbackContext = Context->Context;
	StringCchCopy(Path, MAX_PATH, Context->Path);

	//
	// Create target file
	//

    FileHandle = CreateFile(Path, GENERIC_READ | GENERIC_WRITE, 
							FILE_SHARE_READ | FILE_SHARE_WRITE, 
							NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		return INVALID_HANDLE_VALUE;
	}

	//
	// Create duplicated data file handle 
	//

    DataHandle = CreateFile(DataObject->Path, GENERIC_READ | GENERIC_WRITE, 
							FILE_SHARE_READ | FILE_SHARE_WRITE, 
							NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (DataHandle == INVALID_HANDLE_VALUE) {
		CloseHandle(FileHandle);
		DeleteFile(Path);
		return INVALID_HANDLE_VALUE;
	}

	//
	// Create duplicated index file handle 
	//

    IndexHandle = CreateFile(IndexObject->Path, GENERIC_READ | GENERIC_WRITE, 
							 FILE_SHARE_READ | FILE_SHARE_WRITE, 
							 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (IndexHandle == INVALID_HANDLE_VALUE) {
		CloseHandle(DataHandle);
		CloseHandle(FileHandle);
		DeleteFile(Path);
		return INVALID_HANDLE_VALUE;
	}

	DtlObject = Context->Object;
	TotalLength = IndexLength + DataLength;
	WrittenLength = 0;

	Buffer = VirtualAlloc(NULL, DTL_COPY_UNIT, MEM_COMMIT, PAGE_READWRITE);
	
	//
	// N.B. Set stack trace barrier to pause decode thread to avoid
	// race with current thread.
	//

	if (DtlObject->Type == DTL_LIVE) {
		MspSetStackTraceBarrier();
	}

	//
	// Copy data
	//

	CurrentPercent = 0;
	LastPercent = 0;

	for(i = 0; i < RecordCount; i++) {

		//
		// Read index
		//

		ReadFile(IndexHandle, &Index, sizeof(BTR_FILE_INDEX), &CompleteLength, NULL);

		//
		// Read record
		//

		Offset.QuadPart = Index.Offset;
		ReadFile(DataHandle, Buffer, (ULONG)Index.Length, &CompleteLength, NULL);

		//
		// Write record
		//

		WriteFile(FileHandle, Buffer, (ULONG)Index.Length, &CompleteLength, NULL);

		//
		// Decode stack trace
		//

		if (DtlObject->Type == DTL_LIVE) {
			MspDecodeStackTraceByRecord((PBTR_RECORD_HEADER)Buffer, &DtlObject->StackTrace);
		}

		WrittenLength += Index.Length;
		CurrentPercent = (ULONG)((WrittenLength * 100) / TotalLength);

		if (CurrentPercent == LastPercent) {
			continue;
		}

		*Cancel = (*Callback)(CallbackContext, CurrentPercent, FALSE, 0);

		if (*Cancel) {

			CloseHandle(DataHandle);
			CloseHandle(FileHandle);
			DeleteFile(Path);

			VirtualFree(Buffer, 0, MEM_RELEASE);
			return INVALID_HANDLE_VALUE;
		}

		LastPercent = CurrentPercent;
	}

	if (DtlObject->Type == DTL_LIVE) {
		MspClearStackTraceBarrier();
		MspClearDbghelp();
	}

	CloseHandle(DataHandle);

	//
	// Set file handle to DataLength which is pre-computed by caller,
	// data length must be 64KB aligned.
	//

	Offset.QuadPart = DataLength;
	SetFilePointerEx(FileHandle, Offset, NULL, FILE_BEGIN);

	//
	// Copy index
	//

	Round = (ULONG)IndexLength / DTL_COPY_UNIT;
	Remain = (ULONG)IndexLength % DTL_COPY_UNIT; 

	//
	// Reset index file handle
	//

	Offset.QuadPart = 0;
	SetFilePointerEx(IndexHandle, Offset, NULL, FILE_BEGIN);

	for(i = 0; i < Round; i++) {

		ReadFile(IndexHandle, Buffer, DTL_COPY_UNIT, &CompleteLength, NULL);
		WriteFile(FileHandle, Buffer, DTL_COPY_UNIT, &CompleteLength, NULL);

		CurrentPercent = (ULONG)((DTL_COPY_UNIT * (i + 1) + DataLength) / (TotalLength / 100));
		*Cancel = (*Callback)(CallbackContext, CurrentPercent, FALSE, 0);

		if (CurrentPercent == 100) {
			*Cancel = FALSE;
		}

		if (*Cancel) {

			CloseHandle(IndexHandle);
			CloseHandle(FileHandle);
			DeleteFile(Path);

			VirtualFree(Buffer, 0, MEM_RELEASE);
			return INVALID_HANDLE_VALUE;
		}
	}

	if (Remain > 0) {
		ReadFile(IndexHandle, Buffer, (ULONG)Remain, &CompleteLength, NULL);
		WriteFile(FileHandle, Buffer, (ULONG)Remain, &CompleteLength, NULL);
		CloseHandle(IndexHandle);
	}

    //
    // N.B. We don't specify it's complete since we still want to copy
    // index, stack trace, probe group etc to the final file.
    //

    *Cancel = (*Callback)(CallbackContext, 99, FALSE, 0);
    VirtualFree(Buffer, 0, MEM_RELEASE);

	if (*Cancel) {

		CloseHandle(FileHandle);
		DeleteFile(Path);

		return INVALID_HANDLE_VALUE;
	}
	
	return FileHandle;
}

ULONG
MspWriteStackTraceToDtl(
	IN PMSP_DTL_OBJECT DtlObject,
	IN HANDLE FileHandle,
	IN ULONG64 Location,
	PULONG64 CompleteLength 
	)
{
	ULONG Status;
	ULONG CompleteBytes;
	HANDLE Handle;
	ULONG Number;
	ULONG Round;
	ULONG Remain;
	LARGE_INTEGER Size;
	LARGE_INTEGER Start;
	LARGE_INTEGER End;
	LARGE_INTEGER EntryLocation;
	LARGE_INTEGER EntrySize;
	LARGE_INTEGER TextLocation;
	LARGE_INTEGER TextSize;
	PMSP_FILE_OBJECT FileObject;
	PMSP_STACKTRACE_OBJECT Object;
	MSP_STACKTRACE_OBJECT Database;
	PVOID Buffer;

	//
	// The *.s file is written in a form that hash lookup still work, chained bucket's
	// next pointer is changed to be bucket number in disk structure, database object
	// itself is first serialized, and then entry buckets, text buckets in linear method,
	// boundary are 64KB aligned to make lookup code work as expected.
	//

	Object = MspGetStackTraceObject(DtlObject);

	Buffer = VirtualAlloc(NULL, MSP_STACKTRACE_FILE_INCREMENT, MEM_COMMIT, PAGE_READWRITE);
	if (!Buffer) {
		return GetLastError();
	}

	Start.QuadPart = MspUlong64RoundUp(Location, MSP_STACKTRACE_MAP_INCREMENT);
	ASSERT((ULONG64)Start.QuadPart == Location);

	if ((ULONG64)Start.QuadPart != Location) {
		VirtualFree(Buffer, 0, MEM_RELEASE);
		return MSP_STATUS_ERROR;
	}
	
	//
	// Pre computing entry and text's location and size and record into database object
	//

	EntryLocation.QuadPart = Location + sizeof(Database);
	EntrySize.QuadPart = Object->EntryObject->FileLength;

	TextLocation.QuadPart = EntryLocation.QuadPart + EntrySize.QuadPart;
	TextSize.QuadPart = Object->TextObject->FileLength;

	//
	// Remember our end location, after write, the file pointer will be positioned to it
	//

	RtlCopyMemory(&Database, Object, sizeof(Database));
	Database.EntryOffset = EntryLocation;
	Database.EntrySize = EntrySize;
	Database.TextOffset = TextLocation;
	Database.TextSize = TextSize;

	Status = WriteFile(FileHandle, &Database, sizeof(Database), &CompleteBytes, NULL);
	if (Status != TRUE) {
		VirtualFree(Buffer, 0, MEM_RELEASE);
		return GetLastError();
	}

	//
	// Write entry buckets
	//

	FileObject = Object->EntryObject;
	Handle = CreateFile(FileObject->Path, GENERIC_READ, FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, 
		                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (Handle == INVALID_HANDLE_VALUE) {
		VirtualFree(Buffer, 0, MEM_RELEASE);
		return GetLastError();
	}

	Number = 0;
	Round = (ULONG)(EntrySize.QuadPart / MSP_STACKTRACE_FILE_INCREMENT); 
	Remain = (ULONG)(EntrySize.QuadPart % MSP_STACKTRACE_FILE_INCREMENT);

	for(Number = 0; Number < Round; Number += 1) {
		ReadFile(Handle, Buffer, MSP_STACKTRACE_FILE_INCREMENT, &CompleteBytes, NULL);
		WriteFile(FileHandle, Buffer, MSP_STACKTRACE_FILE_INCREMENT, &CompleteBytes, NULL);
	}

	if (Remain != 0) {
		ReadFile(Handle, Buffer, Remain, &CompleteBytes, NULL);
		WriteFile(FileHandle, Buffer, Remain, &CompleteBytes, NULL);
	}

	CloseHandle(Handle);

	//
	// Write string text buckets
	//

	FileObject = Object->TextObject;
	Handle = CreateFile(FileObject->Path, GENERIC_READ, FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, 
		                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (Handle == INVALID_HANDLE_VALUE) {
		VirtualFree(Buffer, 0, MEM_RELEASE);
		return GetLastError();
	}

	Number = 0;
	Round = (ULONG)(TextSize.QuadPart / MSP_STACKTRACE_FILE_INCREMENT); 
	Remain = (ULONG)(TextSize.QuadPart % MSP_STACKTRACE_FILE_INCREMENT);

	for(Number = 0; Number < Round; Number += 1) {
		ReadFile(Handle, Buffer, MSP_STACKTRACE_FILE_INCREMENT, &CompleteBytes, NULL);
		WriteFile(FileHandle, Buffer, MSP_STACKTRACE_FILE_INCREMENT, &CompleteBytes, NULL);
	}

	if (Remain != 0) {
		ReadFile(Handle, Buffer, Remain, &CompleteBytes, NULL);
		WriteFile(FileHandle, Buffer, Remain, &CompleteBytes, NULL);
	}

	VirtualFree(Buffer, 0, MEM_RELEASE);
	CloseHandle(Handle);

	Size.QuadPart = EntrySize.QuadPart + TextSize.QuadPart + sizeof(Database);

	if (CompleteLength != NULL) {
		*CompleteLength = (ULONG)Size.QuadPart;
	}

	End.QuadPart = Size.QuadPart + Location;
	End.QuadPart = MspUlong64RoundUp(End.QuadPart, MSP_DATA_MAP_INCREMENT);

	SetFilePointerEx(FileHandle, End, NULL, FILE_BEGIN);
	SetEndOfFile(FileHandle);
	return MSP_STATUS_OK;	
}

ULONG
MspLoadStackTraceFromDtl(
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	ULONG Size;
	ULONG Complete;
	PMSP_MAPPING_OBJECT MappingObject;
	PMSP_FILE_OBJECT FileObject;
	PMSP_STACKTRACE_OBJECT Object;
	PMSP_FILE_OBJECT DataObject;
	HANDLE FileHandle, MappingHandle;
	PVOID Address;
	WCHAR Path[MAX_PATH];
	LARGE_INTEGER Offset;
	PMSP_DTL_HEADER Header;
	ULONG Round;
	ULONG Remain;
	PVOID Buffer;
	HANDLE DataHandle;
	ULONG Number;

	DataObject = &DtlObject->DataObject;
	DataHandle = DataObject->FileObject;

	StringCchCopy(Path, MAX_PATH, DataObject->Path);
	StringCchCat(Path, MAX_PATH, L".s");

	FileHandle = CreateFile(Path, GENERIC_READ|GENERIC_WRITE, 
		                    FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, 
							CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		return MSP_STATUS_CREATEFILE;
	}

	Header = &DtlObject->Header;
	Object = &DtlObject->StackTrace;

	//
	// Read serialized stack trace object
	//

	Offset.QuadPart = Header->StackLocation;
	SetFilePointerEx(DataHandle, Offset, NULL, FILE_BEGIN);
	ReadFile(DataHandle, Object, sizeof(MSP_STACKTRACE_OBJECT), &Complete, NULL);

	//
	// Because dtl report's stack trace is static, no decoding required,
	// we don't use lock and its context object
	//

	InitializeListHead(&Object->ContextListHead);
	MspInitCsLock(&Object->Lock, 4000);
	Object->Context = NULL;

	//
	// Copy stack entry file 
	//

	Offset.QuadPart = Object->EntryOffset.QuadPart;
	SetFilePointerEx(DataHandle, Offset, NULL, FILE_BEGIN);
	Buffer = VirtualAlloc(NULL, DTL_COPY_UNIT, MEM_COMMIT, PAGE_READWRITE);

	Round = (ULONG)(Object->EntrySize.QuadPart / DTL_COPY_UNIT);
	Remain = (ULONG)(Object->EntrySize.QuadPart % DTL_COPY_UNIT);

	for(Number = 0; Number < Round; Number += 1) {
		ReadFile(DataHandle, Buffer, DTL_COPY_UNIT, &Complete, NULL);
		WriteFile(FileHandle, Buffer, DTL_COPY_UNIT, &Complete, NULL);
	}

	ReadFile(DataHandle, Buffer, Remain, &Complete, NULL);
	WriteFile(FileHandle, Buffer, Remain, &Complete, NULL);
	SetEndOfFile(FileHandle);

	MappingHandle = CreateFileMapping(FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!MappingHandle) {
		CloseHandle(FileHandle);
		VirtualFree(Buffer, 0, MEM_RELEASE);
		return MSP_STATUS_CREATEFILEMAPPING;
	}

	Size = MSP_FIXED_BUCKET_NUMBER * sizeof(MSP_STACKTRACE_ENTRY);
	Size = MspUlongRoundUp(Size, 1024 * 64);

	Address = MapViewOfFile(MappingHandle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, Size);
	if (!Address) {
		CloseHandle(MappingHandle);
		CloseHandle(FileHandle);
		VirtualFree(Buffer, 0, MEM_RELEASE);
		return MSP_STATUS_MAPVIEWOFFILE;	
	}

	//
	// Create file object and map its first buckets into address space
	// if chained bucket required, chained mapping should be used, note 
	// that file object's mapping object is always live, and never get
	// closed
	//

	FileObject = (PMSP_FILE_OBJECT)MspMalloc(sizeof(MSP_FILE_OBJECT));
	RtlZeroMemory(FileObject, sizeof(MSP_FILE_OBJECT));
	
	StringCchCopyW(FileObject->Path, MAX_PATH, Path);
	FileObject->Type = FileStackEntry;
	FileObject->FileLength = Object->EntrySize.QuadPart;
	FileObject->FileObject = FileHandle;
	
	MappingObject = (PMSP_MAPPING_OBJECT)MspMalloc(sizeof(MSP_MAPPING_OBJECT));
	MappingObject->MappedObject = MappingHandle;
	MappingObject->FileLength = FileObject->FileLength;
	MappingObject->MappedVa = Address;
	MappingObject->MappedOffset = 0;
	MappingObject->MappedLength = Size;
	MappingObject->Type = DiskFileBacked;

	//
	// N.B. LastSequence is in fact larger than NumberOfBuckets
	//

	MappingObject->FirstSequence = 0;
	MappingObject->LastSequence = Size / sizeof(MSP_STACKTRACE_ENTRY) - 1;

	FileObject->Mapping = MappingObject;
	MappingObject->FileObject = FileObject;

	Object->EntryObject = FileObject;
	Object->u1.Buckets = (PMSP_STACKTRACE_ENTRY)MappingObject->MappedVa;

	//
	// Create extend mapping object for entry
	//

	MappingHandle = CreateFileMapping(FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!MappingHandle) {
		CloseHandle(FileHandle);
		VirtualFree(Buffer, 0, MEM_RELEASE);
		return MSP_STATUS_CREATEFILEMAPPING;
	}

	MappingObject = (PMSP_MAPPING_OBJECT)MspMalloc(sizeof(MSP_MAPPING_OBJECT));
	MappingObject->MappedObject = MappingHandle;
	MappingObject->FileLength = FileObject->FileLength;
	MappingObject->MappedVa = NULL;
	MappingObject->MappedOffset = 0;
	MappingObject->MappedLength = 0;
	MappingObject->Type = DiskFileBacked;

	MappingObject->FileObject = FileObject;
	Object->EntryExtendMapping = MappingObject;

	//
	// Create stack text file
	//

	StringCchCopy(Path, MAX_PATH, DataObject->Path);
	StringCchCat(Path, MAX_PATH, L".t");

	FileHandle = CreateFile(Path, GENERIC_READ|GENERIC_WRITE, 
		                    FILE_SHARE_READ|FILE_SHARE_WRITE, 
		                    NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		VirtualFree(Buffer, 0, MEM_RELEASE);
		return MSP_STATUS_CREATEFILE;
	}

	Offset.QuadPart = Object->TextOffset.QuadPart;
	SetFilePointerEx(DataHandle, Offset, NULL, FILE_BEGIN);

	Round = (ULONG)(Object->TextSize.QuadPart / DTL_COPY_UNIT);
	Remain = (ULONG)(Object->TextSize.QuadPart % DTL_COPY_UNIT);

	for(Number = 0; Number < Round; Number += 1) {
		ReadFile(DataHandle, Buffer, DTL_COPY_UNIT, &Complete, NULL);
		WriteFile(FileHandle, Buffer, DTL_COPY_UNIT, &Complete, NULL);
	}

	ReadFile(DataHandle, Buffer, Remain, &Complete, NULL);
	WriteFile(FileHandle, Buffer, Remain, &Complete, NULL);
	SetEndOfFile(FileHandle);

	VirtualFree(Buffer, 0, MEM_RELEASE);

	//
	// Create initial file length double of buckets, and map only buckets size
	//

	Size = MSP_FIXED_BUCKET_NUMBER * sizeof(MSP_STACKTRACE_TEXT);
	Size = MspUlongRoundUp(Size, 1024 * 64);

	MappingHandle = CreateFileMapping(FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!MappingHandle) {
		CloseHandle(FileHandle);
		return MSP_STATUS_CREATEFILEMAPPING;
	}

	Address = MapViewOfFile(MappingHandle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, Size);

	//
	// Create file object and map its first buckets into address space
	// if chained bucket required, chained mapping should be used, note 
	// that file object's mapping object is always live, and never get
	// closed
	//

	FileObject = (PMSP_FILE_OBJECT)MspMalloc(sizeof(MSP_FILE_OBJECT));
	RtlZeroMemory(FileObject, sizeof(MSP_FILE_OBJECT));
	
	StringCchCopyW(FileObject->Path, MAX_PATH, Path);
	FileObject->Type = FileStackText;
	FileObject->FileLength = Object->TextSize.QuadPart;
	FileObject->FileObject = FileHandle;
	
	MappingObject = (PMSP_MAPPING_OBJECT)MspMalloc(sizeof(MSP_MAPPING_OBJECT));
	MappingObject->MappedObject = MappingHandle;
	MappingObject->FileLength = FileObject->FileLength;
	MappingObject->MappedVa = Address;
	MappingObject->MappedOffset = 0;
	MappingObject->MappedLength = Size;
	MappingObject->Type = DiskFileBacked;
	
	//
	// N.B. LastSequence is in fact larger than NumberOfBuckets
	//

	MappingObject->FirstSequence = 0;
	MappingObject->LastSequence = Size / sizeof(MSP_STACKTRACE_TEXT) - 1;

	FileObject->Mapping = MappingObject;
	MappingObject->FileObject = FileObject;

	Object->TextObject = FileObject;
	Object->u2.Buckets = (PMSP_STACKTRACE_TEXT)MappingObject->MappedVa;

	//
	// Create extend mapping object for text 
	//

	MappingHandle = CreateFileMapping(FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!MappingHandle) {
		CloseHandle(FileHandle);
		return MSP_STATUS_CREATEFILEMAPPING;
	}

	MappingObject = (PMSP_MAPPING_OBJECT)MspMalloc(sizeof(MSP_MAPPING_OBJECT));
	MappingObject->MappedObject = MappingHandle;
	MappingObject->FileLength = FileObject->FileLength;
	MappingObject->MappedVa = NULL;
	MappingObject->MappedOffset = 0;
	MappingObject->MappedLength = 0;
	MappingObject->Type = DiskFileBacked;

	MappingObject->FileObject = FileObject;
	Object->TextExtendMapping = MappingObject;

	//
	// Clear bitmap, we don't use it
	//

	BtrInitializeBitMap(&Object->EntryBitmap, NULL, 0);
	BtrInitializeBitMap(&Object->TextBitmap, NULL, 0);

	return MSP_STATUS_OK;
}

ULONG
MspWriteProbeToDtl(
	IN PMSP_DTL_OBJECT DtlObject,
	IN HANDLE FileHandle,
	IN ULONG64 Location,
	PULONG64 CompleteLength 
	)
{
	ULONG Complete;
	ULONG Number;
	ULONG NumberOfProcess;
	ULONG HeadLength;
	ULONG TotalLength;
	ULONG ProbeCount;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListEntry2;
	PMSP_PROCESS Process;
	PMSP_PROBE_TABLE Table;
	PMSP_DTL_PROCESS DtlProcess;
	PMSP_DTL_PROCESS_ENTRY Entry;
	PMSP_PROBE Probe;
	PBTR_FILTER Filter;
	LARGE_INTEGER Offset;
	SYSTEMTIME Time;

	NumberOfProcess = DtlObject->ProcessCount;
	HeadLength = FIELD_OFFSET(MSP_DTL_PROCESS, Entries[NumberOfProcess]);

	//
	// Skip the MSP_DTL_PROCESS structure, we write it finally
	//

	Offset.QuadPart = Location + HeadLength;
	SetFilePointerEx(FileHandle, Offset, NULL, FILE_BEGIN);

	DtlProcess = (PMSP_DTL_PROCESS)MspMalloc(HeadLength);
	DtlProcess->NumberOfProcess = NumberOfProcess;

	//
	// N.B. Entry's offset fields all use relative offset, it's relative
	// to MSP_DTL_PROCESS structure, not relative to file head
	//

	Number = 0;
	TotalLength = HeadLength;
	ListEntry = DtlObject->ProcessListHead.Flink;

	while (ListEntry != &DtlObject->ProcessListHead) {

		Process = CONTAINING_RECORD(ListEntry, MSP_PROCESS, ListEntry);
		Entry = &DtlProcess->Entries[Number];
		
		Entry->ProcessId = Process->ProcessId;
		Entry->StartTime = Process->StartTime;
		
		if (Process->State != MSP_STATE_STARTED) {
			Entry->EndTime = Process->EndTime;
		} else {
			GetLocalTime(&Time);
			SystemTimeToFileTime(&Time, &Entry->EndTime);
		}

		StringCchCopy(Entry->UserName, MAX_PATH, Process->UserName);
		StringCchCopy(Entry->BaseName, MAX_PATH, Process->BaseName);
		StringCchCopy(Entry->FullName, MAX_PATH, Process->FullName);
		StringCchCopy(Entry->Argument, MAX_PATH, Process->Argument);
		
		Table = Process->ProbeTable;
		Entry->ProbeCount = Table->FastProbeCount + Table->FilterProbeCount;
		Entry->ProbeOffset = TotalLength;

		//
		// Gather probe object for current entry
		//

		ProbeCount = 0;
		ListEntry2 = Table->FastListHead.Flink;

		while (ListEntry2 != &Table->FastListHead) {

			Probe = CONTAINING_RECORD(ListEntry2, MSP_PROBE, TypeListEntry);
			WriteFile(FileHandle, Probe, sizeof(MSP_PROBE), &Complete, NULL);

			ProbeCount += 1;
			ListEntry2 = ListEntry2->Flink;
		}

		//
		// Write filter probes, N.B. This is a patch for dtl report.
		//

		ListEntry2 = Table->FilterListHead.Flink;
		while (ListEntry2 != &Table->FilterListHead) {
			
			Probe = CONTAINING_RECORD(ListEntry2, MSP_PROBE, TypeListEntry);
			WriteFile(FileHandle, Probe, sizeof(MSP_PROBE), &Complete, NULL);

			ProbeCount += 1;
			ListEntry2 = ListEntry2->Flink;
		}

		ASSERT(ProbeCount == Entry->ProbeCount);

		//
		// Adjust next entry's probe offset
		//

		TotalLength = TotalLength + sizeof(MSP_PROBE) * ProbeCount;
		Number += 1;

		ListEntry = ListEntry->Flink;
	}

	ASSERT(NumberOfProcess = Number);

	//
	// Write filters
	//

	DtlProcess->FilterOffset = TotalLength;

	Number = 0;
	ListEntry = MspFilterList.Flink;

	while (ListEntry != &MspFilterList) {

		Filter = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);
		WriteFile(FileHandle, Filter, sizeof(BTR_FILTER), &Complete, NULL);

		Number += 1;
		TotalLength += sizeof(BTR_FILTER);

		ListEntry = ListEntry->Flink;
	}

	DtlProcess->NumberOfFilters = Number;
	DtlProcess->TotalLength = TotalLength;

	//
	// Write the MSP_DTL_PROCESS structure
	//

	Offset.QuadPart = Location;
	SetFilePointerEx(FileHandle, Offset, NULL, FILE_BEGIN);
	WriteFile(FileHandle, DtlProcess, HeadLength, &Complete, NULL);
	MspFree(DtlProcess);

	//
	// Round up this segment to be mapping increment aligned
	//

	Offset.QuadPart = Location + TotalLength;
	Offset.QuadPart = MspUlong64RoundUp(Offset.QuadPart, MSP_DATA_MAP_INCREMENT);
	SetFilePointerEx(FileHandle, Offset, NULL, FILE_BEGIN);
	SetEndOfFile(FileHandle);

	return MSP_STATUS_OK;	
}

ULONG
MspLoadProbeFromDtl(
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	ULONG Complete;
	ULONG Number;
	ULONG NumberOfProcess;
	ULONG HeadLength;
	PLIST_ENTRY ListHead;
	PMSP_PROCESS Process;
	PMSP_PROBE_TABLE Table;
	PMSP_DTL_PROCESS DtlProcess;
	PMSP_DTL_PROCESS_ENTRY Entry;
	PMSP_PROBE Probe;
	LARGE_INTEGER Offset;
	HANDLE FileHandle;
	PMSP_FILE_OBJECT DataObject;
	ULONG64 Value;
	ULONG i;
	ULONG Bucket;
	PMSP_DTL_HEADER Header;

	Header = &DtlObject->Header;
	DataObject = &DtlObject->DataObject;	
	FileHandle = CreateFile(DataObject->Path, GENERIC_READ | GENERIC_WRITE,
		                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
							OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		return GetLastError();
	}

	//
	// Read first 8 bytes of MSP_DTL_PROCESS structure
	//

	Offset.QuadPart = Header->ProbeLocation;
	SetFilePointerEx(FileHandle, Offset, NULL, FILE_BEGIN);

	ReadFile(FileHandle, &Value, sizeof(ULONG64), &Complete, NULL);
	NumberOfProcess = (ULONG)(Value >> 32);
	DtlObject->ProcessCount = NumberOfProcess;

	ASSERT(NumberOfProcess != 0);

	//
	// Read the full MSP_DTL_PROCESS structure
	//

	HeadLength = FIELD_OFFSET(MSP_DTL_PROCESS, Entries[NumberOfProcess]);
	DtlProcess = (PMSP_DTL_PROCESS)MspMalloc(HeadLength);

	Offset.QuadPart = Header->ProbeLocation;
	SetFilePointerEx(FileHandle, Offset, NULL, FILE_BEGIN);
	ReadFile(FileHandle, DtlProcess, HeadLength, &Complete, NULL);

	//
	// Build process list for dtl object
	//

	DtlObject->ProcessCount = 0;
	InitializeListHead(&DtlObject->ProcessListHead);
	MspInitCsLock(&DtlObject->ProcessListLock, 4000);

	for(Number = 0; Number < NumberOfProcess; Number += 1) {

		Entry = &DtlProcess->Entries[Number];
		Process = (PMSP_PROCESS)MspMalloc(sizeof(MSP_PROCESS));
		RtlZeroMemory(Process, sizeof(MSP_PROCESS));

		StringCchCopy(Process->UserName, MAX_PATH, Entry->UserName);
		StringCchCopy(Process->BaseName, MAX_PATH, Entry->BaseName);
		StringCchCopy(Process->Argument, MAX_PATH, Entry->Argument);
		StringCchCopy(Process->FullName, MAX_PATH, Entry->FullName);

		Process->State = MSP_STATE_REPORT;
		Process->ProcessId = Entry->ProcessId;
		Process->StartTime = Entry->StartTime;
		Process->EndTime = Entry->EndTime;

		MspCreateProbeTable(Process, MSP_PROBE_TABLE_SIZE);
		Table = Process->ProbeTable;

		//
		// Read serialized fast probe object, and insert into probe table
		//

		for (i = 0; i < Entry->ProbeCount; i++) {
			
			Probe = (PMSP_PROBE)MspMalloc(sizeof(MSP_PROBE));
			ReadFile(FileHandle, Probe, sizeof(MSP_PROBE), &Complete, NULL);

			//
			// Clean dirty fields read from file
			//

			Probe->Counter = NULL;
			Probe->Callback = NULL;

			Bucket = MspComputeProbeHash(Table, Probe->Address);
			ListHead = &Table->ListHead[Bucket];

			InsertTailList(ListHead, &Probe->ListEntry);

			if (Probe->Type == PROBE_FAST) {
				InsertTailList(&Table->FastListHead, &Probe->TypeListEntry);
				Table->ActiveFastCount += 1;
				Table->FastProbeCount += 1;
			}

			if (Probe->Type == PROBE_FILTER) {
				InsertTailList(&Table->FilterListHead, &Probe->TypeListEntry);
				Table->ActiveFilterCount += 1;
				Table->FilterProbeCount += 1;
			}
			
		}

		InsertTailList(&DtlObject->ProcessListHead, &Process->ListEntry);
		DtlObject->ProcessCount += 1;
	}

	//
	// N.B. we don't read filters, we use current process's
	// decode filters instead.
	//

	MspFree(DtlProcess);
	CloseHandle(FileHandle);
	return MSP_STATUS_OK;	
}

PMSP_DTL_OBJECT
MspCurrentDtlObject(
	VOID
	)
{
	return &MspDtlObject;
}

ULONG
MspGetRecordCount(
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	return DtlObject->IndexObject.NumberOfRecords;
}

ULONG
MspReferenceCounterObject(
	IN PMSP_DTL_OBJECT DtlObject,
	OUT PMSP_COUNTER_OBJECT *Object 
	)
{
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PMSP_PROCESS Process;
	PMSP_COUNTER_OBJECT Counter;
	PMSP_COUNTER_GROUP Group;
	ULONG Number;

	Counter = &DtlObject->CounterObject;
	ASSERT(IsListEmpty(&Counter->GroupListHead));

	ListHead = &DtlObject->ProcessListHead;
	ListEntry = ListHead->Flink;
	Number = 0;

	while (ListEntry != ListHead) {
		Process = CONTAINING_RECORD(ListEntry, MSP_PROCESS, ListEntry);	
		Group = Process->Counter;
		InsertTailList(&Counter->GroupListHead, &Group->ListEntry);
		ListEntry = ListEntry->Flink;
		Number += 1;
	}

	Counter->NumberOfCalls = 0;
	Counter->NumberOfStackTraces = 0;
	Counter->NumberOfExceptions = 0;
	Counter->MinimumDuration = 0;
	Counter->MaximumDuration = 0;
	Counter->NumberOfGroups = Number;

	*Object = Counter;
	return Number;
}

VOID
MspReleaseCounterObject(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PMSP_COUNTER_OBJECT Object 
	)
{
	while (IsListEmpty(&Object->GroupListHead) != TRUE) { 
		RemoveHeadList(&Object->GroupListHead);
	}

	Object->NumberOfCalls = 0;
	Object->NumberOfStackTraces = 0;
	Object->NumberOfExceptions = 0;
	Object->MinimumDuration = 0;
	Object->MaximumDuration = 0;
	Object->NumberOfGroups = 0;
}

ULONG
MspIsValidDtlReport(
	IN PWSTR Path
	)
{
	ULONG Status;
	PMSP_DTL_HEADER Header;
	HANDLE FileHandle;
	ULONG CompleteLength;
	ULONG CheckSum;

	FileHandle = CreateFile(Path,
		                    GENERIC_READ | GENERIC_WRITE,
		                    FILE_SHARE_READ | FILE_SHARE_WRITE, 
		                    NULL,
		                    OPEN_EXISTING,
		                    FILE_ATTRIBUTE_NORMAL,
		                    NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		return Status;
	}
	
	//
	// Ensure its size at least contain a header
	//

	CompleteLength = GetFileSize(FileHandle, NULL);
	if (CompleteLength < sizeof(MSP_DTL_HEADER)) {
		CloseHandle(FileHandle);
		return MSP_STATUS_BAD_CHECKSUM;
	}

	//
	// Read the file header at the end of the report file
	//

	Header = (PMSP_DTL_HEADER)MspMalloc(sizeof(MSP_DTL_HEADER));
	SetFilePointer(FileHandle, (LONG)(0 - sizeof(MSP_DTL_HEADER)), NULL, FILE_END);
	ReadFile(FileHandle, Header, sizeof(MSP_DTL_HEADER), &CompleteLength, NULL);

	//
	// Only check signature and checksum
	//

	if (Header->Signature != MSP_DTL_SIGNATURE) {
		CloseHandle(FileHandle);
		MspFree(Header);
		return MSP_STATUS_BAD_CHECKSUM;
	}

	CheckSum = MspComputeDtlCheckSum(Header);
	if (CheckSum != Header->CheckSum) {
		CloseHandle(FileHandle);
		MspFree(Header);
		return MSP_STATUS_BAD_CHECKSUM;
	}

	CloseHandle(FileHandle);
	MspFree(Header);

	return MSP_STATUS_OK;
}

ULONG
MspOpenDtlReport(
	IN PWSTR Path,
	OUT PMSP_DTL_OBJECT *DtlObject
	)
{
	ULONG Status;
	PMSP_DTL_OBJECT Object;
	PMSP_FILE_OBJECT DataObject;
	PMSP_FILE_OBJECT IndexObject;
	PMSP_MAPPING_OBJECT MappingObject;
	PMSP_DTL_HEADER Header;
	HANDLE FileHandle;
	HANDLE DataHandle;
	ULONG Complete;
	ULONG CheckSum;
	ULONG Round;
	ULONG Remain;
	ULONG Number;
	PVOID Buffer;
	LARGE_INTEGER Offset;
	PMSP_COUNTER_OBJECT Counter;

	*DtlObject = NULL;

	Object = (PMSP_DTL_OBJECT)MspMalloc(sizeof(MSP_DTL_OBJECT));
	RtlZeroMemory(Object, sizeof(MSP_DTL_OBJECT));

	DataObject = &Object->DataObject; 
	IndexObject = &Object->IndexObject; 

	StringCchCopy(DataObject->Path, MAX_PATH, Path);
	FileHandle = CreateFile(DataObject->Path,
		                    GENERIC_READ | GENERIC_WRITE,
		                    FILE_SHARE_READ | FILE_SHARE_WRITE, 
		                    NULL,
		                    OPEN_EXISTING,
		                    FILE_ATTRIBUTE_NORMAL,
		                    NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		MspFree(Object);
		return Status;
	}
	
	//
	// First of all read the file header at the end of the report file
	//

	Header = &Object->Header;

	SetFilePointer(FileHandle, (LONG)(0 - sizeof(MSP_DTL_HEADER)), NULL, FILE_END);
	ReadFile(FileHandle, Header, sizeof(MSP_DTL_HEADER), &Complete, NULL);
	if (Header->Signature != MSP_DTL_SIGNATURE) {
		CloseHandle(FileHandle);
		MspFree(Object);
		return MSP_STATUS_BAD_CHECKSUM;
	}

	if (BspIs64Bits) {

		//
		// 64 bits dprobe
		//

		if (Header->Is64Bits != TRUE) {
			CloseHandle(FileHandle);
			MspFree(Object);
			return MSP_STATUS_REQUIRE_32BITS;
		}
	}
	else {

		//
		// 32 bits dprobe
		//

		if (Header->Is64Bits == TRUE) {
			CloseHandle(FileHandle);
			MspFree(Object);
			return MSP_STATUS_REQUIRE_64BITS;
		}
	}

	//
	// Check dtl report's checksum
	//

	CheckSum = MspComputeDtlCheckSum(Header);
	if (CheckSum != Header->CheckSum) {
		CloseHandle(FileHandle);
		MspFree(Object);
		return MSP_STATUS_BAD_CHECKSUM;
	}

	Object->Type = DTL_REPORT;

	//
	// Initialize data file object
	//

	DataObject->Type = FileGlobalData;
	DataObject->FileObject = FileHandle;
	DataObject->FileLength = Header->RecordLength;	
	DataObject->NumberOfRecords = (ULONG)Header->IndexNumber;

	MappingObject = MspCreateMappingObject(DataObject, 0, MSP_DATA_MAP_INCREMENT);
	if (!MappingObject) {
		MspCloseFileObject(DataObject, FALSE);
		return MSP_STATUS_CREATEFILEMAPPING;
	}

	MappingObject->FileObject = DataObject;
	DataObject->Mapping = MappingObject;
	DataHandle = FileHandle;

	//
	// Create new index file
	//

	StringCchCopy(IndexObject->Path, MAX_PATH, Path);
	StringCchCat(IndexObject->Path, MAX_PATH, L".i");

	FileHandle = CreateFile(IndexObject->Path,
		                    GENERIC_READ | GENERIC_WRITE,
		                    FILE_SHARE_READ | FILE_SHARE_WRITE, 
		                    NULL,
		                    CREATE_ALWAYS,
		                    FILE_ATTRIBUTE_NORMAL,
		                    NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		MspCloseFileObject(DataObject, FALSE);
		MspFree(Object);
		return Status;
	}
	
	//
	// Copy index to index file
	//

	Offset.QuadPart = Header->IndexLocation;
	SetFilePointerEx(DataHandle, Offset, NULL, FILE_BEGIN);

	Round = (ULONG)Header->IndexLength / DTL_COPY_UNIT;
	Remain = Header->IndexLength % DTL_COPY_UNIT;
	Buffer = VirtualAlloc(NULL, DTL_COPY_UNIT, MEM_COMMIT, PAGE_READWRITE);

	for(Number = 0; Number < Round; Number += 1) {
		ReadFile(DataHandle, Buffer, DTL_COPY_UNIT, &Complete, NULL);
		WriteFile(FileHandle, Buffer, DTL_COPY_UNIT, &Complete, NULL);
	}

	ReadFile(DataHandle, Buffer, Remain, &Complete, NULL);
	WriteFile(FileHandle, Buffer, Remain, &Complete, NULL);
	SetEndOfFile(FileHandle);

	VirtualFree(Buffer, 0, MEM_RELEASE);

	//
	// Initialize index object
	//

	IndexObject->Type = FileGlobalIndex;
	IndexObject->FileObject = FileHandle;
	IndexObject->FileLength = Header->IndexLength;	
	IndexObject->NumberOfRecords = (ULONG)Header->IndexNumber;

	MappingObject = MspCreateMappingObject(IndexObject, 0, MSP_INDEX_MAP_INCREMENT);
	if (!MappingObject) {
		MspCloseFileObject(DataObject, FALSE);
		MspCloseFileObject(IndexObject, FALSE);
		return MSP_STATUS_CREATEFILEMAPPING;
	}

	MappingObject->FirstSequence = 0;
	MappingObject->LastSequence = SEQUENCES_PER_INCREMENT - 1;

	MappingObject->FileObject = IndexObject;
	IndexObject->Mapping = MappingObject;

	//
	// Load stack trace database
	//

	MspLoadStackTraceFromDtl(Object);

	//
	// Load process and probes
	//

	MspLoadProbeFromDtl(Object);

	//
	// Initialize counter object
	//

	Counter = &Object->CounterObject;
	Counter->Level = CounterRootLevel;
	StringCchCopy(Counter->Name, MAX_PATH, L"All");
	InitializeListHead(&Counter->GroupListHead);
	Counter->DtlObject = Object;

	//
	// Load summary
	//

	if (Header->SummaryLength != 0) {
		MspLoadSummaryFromDtl(Object);
	}

	*DtlObject = Object;
	return MSP_STATUS_OK;
}

VOID
MspCloseDtlReport(
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	PMSP_FILE_OBJECT IndexObject;
	PMSP_FILE_OBJECT DataObject;
	PMSP_COUNTER_OBJECT Counter;
	ULONG Count;

	//
	// Check whether require to write counters
	//

	if (DtlObject->Header.SummaryLength == 0) {
		Count = MspReferenceCounterObject(DtlObject, &Counter);
		if (Count != 0) {
			MspWriteSummary(DtlObject, Counter);
		}
	}

	//
	// Close and delete index file
	//

	DataObject = &DtlObject->DataObject;
	MspCloseFileObject(DataObject, FALSE);

	IndexObject = &DtlObject->IndexObject;
	MspCloseFileObject(IndexObject, FALSE);
	DeleteFile(IndexObject->Path);

	//
	// Close stack trace and delete *.s, *.t files
	//

	MspCloseStackTrace(&DtlObject->StackTrace);

	//
	// Free process and probe objects
	//

	MspFreeDtlProcess(DtlObject);

	MspDeleteCsLock(&DtlObject->ProcessListLock);
	MspFree(DtlObject);
}

VOID
MspFreeDtlProcess(
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PMSP_PROCESS Process;

	ListHead = &DtlObject->ProcessListHead;

	while (IsListEmpty(ListHead) != TRUE) {

		ListEntry = RemoveHeadList(ListHead);
		Process = CONTAINING_RECORD(ListEntry, MSP_PROCESS, ListEntry);

		MspDestroyProbeTable(Process->ProbeTable);

		if (Process->Counter != NULL) {
			MspFree(Process->Counter);
		}

		MspFree(Process);
	}

}

VOID
MspDumpDtlRecord(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PSTR Path
	)
{
	PMSP_FILE_OBJECT IndexObject;
	PMSP_FILE_OBJECT DataObject;
	HANDLE IndexHandle;
	HANDLE DataHandle;
	ULONG Number;
	BTR_FILE_INDEX Index;
	LARGE_INTEGER Offset;
	ULONG Complete;
	PBTR_RECORD_HEADER Record;
	FILE *fp;

	IndexObject = &DtlObject->IndexObject;
	DataObject = &DtlObject->DataObject;

	IndexHandle = IndexObject->FileObject;
	DataHandle = DataObject->FileObject;

	SetFilePointer(IndexHandle, 0, NULL, FILE_BEGIN);
	SetFilePointer(DataHandle, 0, NULL, FILE_BEGIN);

	Record = (PBTR_RECORD_HEADER)_alloca(MspPageSize);
	fp = fopen(Path, "a");

	for(Number = 0; Number < IndexObject->NumberOfRecords; Number++) {
	
		ReadFile(IndexHandle, &Index, sizeof(BTR_FILE_INDEX), &Complete, NULL);

		Offset.QuadPart = Index.Offset;
		SetFilePointerEx(DataHandle, Offset, NULL, FILE_BEGIN);

		ReadFile(DataHandle, Record, (ULONG)Index.Length, &Complete, NULL);
	
		fprintf(fp, "seq %08u pid %04u tid %04u addr 0x%p\n", 
				Record->Sequence, Record->ProcessId, Record->ThreadId, Record->Address);
	}

	fclose(fp);
	
	SetFilePointer(IndexHandle, 0, NULL, FILE_BEGIN);
	SetFilePointer(DataHandle, 0, NULL, FILE_BEGIN);
}

VOID
MspInsertCounterEntryByRecord(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record
	)
{
	PMSP_COUNTER_GROUP Group;
	PMSP_COUNTER_ENTRY Entry;
	PMSP_PROCESS Process;
	PMSP_PROBE Probe;
	PBTR_FILTER Filter;
	BOOLEAN NewEntry = FALSE;

	//
	// Lookup probe by object id
	//

	Probe = MspLookupProbeByObjectId(DtlObject, Record);
	if (!Probe) {
		return;
	}

	Entry = Probe->Counter;
	if (!Entry) {

		Entry = (PMSP_COUNTER_ENTRY)MspMalloc(sizeof(MSP_COUNTER_ENTRY));
		RtlZeroMemory(Entry, sizeof(MSP_COUNTER_ENTRY));
		Entry->Retire.ObjectId = Probe->ObjectId;
		Entry->Retire.Type = Probe->Type;
		Entry->Retire.Address = Record->Address;

		if (Probe->Type == PROBE_FAST) {
			StringCchPrintf(Entry->Retire.Name, MAX_PATH, L"%s!%s", Probe->DllName, Probe->ApiName);
		}

		if (Probe->Type == PROBE_FILTER) {

			Filter = MspLookupDecodeFilter((PBTR_FILTER_RECORD)Record);
			ASSERT(Filter != NULL);

			if (!Filter) {
				StringCchPrintf(Entry->Retire.Name, MAX_PATH, L"UnkFlt_%x", Record->Address);
			}
			else {

				StringCchPrintf(Entry->Retire.Name, MAX_PATH, L"%s!%s", 
								Filter->Probes[Probe->FilterBit].DllName, 
								Filter->Probes[Probe->FilterBit].ApiName); 
			}
		}

		DebugTrace("counter: %S", Entry->Retire.Name);

		//
		// Attach counter entry to probe object
		//

		Probe->Counter = Entry;
		NewEntry = TRUE;

	}

	//
	// Lookup counter group by process id
	//

	Process = MspLookupProcess(DtlObject, Record->ProcessId);
	ASSERT(Process != NULL);

	if (!Process->Counter) {

		Group = (PMSP_COUNTER_GROUP)MspMalloc(sizeof(MSP_COUNTER_GROUP));
		RtlZeroMemory(Group, sizeof(MSP_COUNTER_GROUP));

		Group->Level = CounterGroupLevel;
		Group->Process = Process;
		Group->Object = &DtlObject->CounterObject;

		InitializeListHead(&Group->ActiveListHead);
		InitializeListHead(&Group->RetireListHead);
		StringCchCopy(Group->Name, MAX_PATH, Process->BaseName);

		//
		// Attach counter group to process object
		//

		Process->Counter = Group;
	}

	//
	// Insert counter to counter group's retire list, for dtl report,
	// all counters are considered retired.
	//

	Group = Process->Counter;

	if (NewEntry) {
		InsertHeadList(&Group->RetireListHead, &Entry->ListEntry);
		Group->NumberOfEntries += 1;
		Entry->Group = Group;
	}

	Group->Retire.NumberOfCalls += 1;

	//
	// Update counter values
	//

	Entry->Retire.NumberOfCalls += 1;
			
	if (Entry->Retire.MaximumDuration < Record->Duration) {
		Entry->Retire.MaximumDuration = Record->Duration;
	}

	if (Entry->Retire.MinimumDuration > Record->Duration) {
		Entry->Retire.MinimumDuration = Record->Duration;
	}
}

PMSP_COUNTER_ENTRY
MspLookupCounterByProbe(
	IN PMSP_COUNTER_GROUP Group,
	IN PMSP_PROBE Probe
	)
{
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PMSP_COUNTER_ENTRY Entry;

	//
	// N.B. This routine is only called by dtl report,
	// all counters are considered retired.
	//

	ListHead = &Group->RetireListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {

		Entry = CONTAINING_RECORD(ListEntry, MSP_COUNTER_ENTRY, ListEntry);
		if (Entry->Retire.ObjectId == Probe->ObjectId) {
			return Entry;
		}

		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

ULONG
MspWriteSummary(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PMSP_COUNTER_OBJECT Counter
	)
{
	PMSP_FILE_OBJECT DataObject;
	HANDLE FileHandle;
	LARGE_INTEGER Offset;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead2;
	PLIST_ENTRY ListEntry2;
	PMSP_COUNTER_GROUP Group;
	PMSP_DTL_COUNTER_GROUP DtlGroup;
	PMSP_COUNTER_ENTRY Entry;
	SIZE_T Size;
	SIZE_T TotalLength;
	ULONG Complete;
	ULONG Number;
	ULONG CheckSum;
	
	//
	// Sanity check whether it's required to write summary
	//

	ASSERT(DtlObject->Header.SummaryLength == 0);

	DataObject = &DtlObject->DataObject;
	FileHandle = DataObject->FileObject;
	ASSERT(FileHandle != NULL);

	Offset.QuadPart = DtlObject->Header.SummaryLocation;
	SetFilePointerEx(FileHandle, Offset, NULL, FILE_BEGIN);
	TotalLength = 0;
	
	//
	// Write group count
	//

	ASSERT(Counter->NumberOfGroups != 0);
	WriteFile(FileHandle, &Counter->NumberOfGroups, sizeof(ULONG), &Complete, NULL);
	TotalLength += sizeof(ULONG);

	ListHead = &Counter->GroupListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {

		Group = CONTAINING_RECORD(ListEntry, MSP_COUNTER_GROUP, ListEntry);
		Size = FIELD_OFFSET(MSP_DTL_COUNTER_GROUP, Entry[Group->NumberOfEntries]);
		DtlGroup = (PMSP_DTL_COUNTER_GROUP)MspMalloc(Size);

		DtlGroup->NumberOfEntries = Group->NumberOfEntries;
		DtlGroup->ProcessId = Group->Process->ProcessId;
		DtlGroup->NumberOfCalls = Group->Retire.NumberOfCalls;
		DtlGroup->NumberOfStackTraces = Group->Retire.NumberOfStackTraces;
		DtlGroup->NumberOfExceptions = Group->Retire.NumberOfExceptions;
		DtlGroup->MaximumDuration = Group->Retire.MaximumDuration;
		DtlGroup->MinimumDuration = Group->Retire.MinimumDuration;

		ListHead2 = &Group->RetireListHead;
		ListEntry2 = ListHead2->Flink;
		Number = 0;

		while (ListEntry2 != ListHead2) {

			Entry = CONTAINING_RECORD(ListEntry2, MSP_COUNTER_ENTRY, ListEntry);
			RtlCopyMemory(&DtlGroup->Entry[Number], Entry, sizeof(MSP_COUNTER_ENTRY));
			Number += 1;

			ListEntry2 = ListEntry2->Flink;
		}

		ASSERT(DtlGroup->NumberOfEntries == Number);
		WriteFile(FileHandle, DtlGroup, (ULONG)Size, &Complete, NULL);
		MspFree(DtlGroup);

		TotalLength += Size;
		ListEntry = ListEntry->Flink;
	}

	//
	// Fix header fields, and re-compute checksum
	//

	DtlObject->Header.SummaryLength = (ULONG64)TotalLength;
	DtlObject->Header.TotalLength += (ULONG64)TotalLength;

	CheckSum = MspComputeDtlCheckSum(&DtlObject->Header);
	DtlObject->Header.CheckSum = CheckSum;

	WriteFile(FileHandle, &DtlObject->Header, sizeof(MSP_DTL_HEADER), &Complete, NULL);
	SetEndOfFile(FileHandle);

	return MSP_STATUS_OK;
}

ULONG
MspLoadSummaryFromDtl(
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	PMSP_DTL_HEADER Header;
	HANDLE FileHandle;
	LARGE_INTEGER Offset;
	ULONG64 SummaryLength;
	ULONG GroupCount;
	ULONG EntryCount;
	ULONG ProcessId;
	ULONG Complete;
	PMSP_COUNTER_ENTRY Entry;
	PMSP_COUNTER_GROUP Group;
	ULONG i, j;

	Header = &DtlObject->Header;
	FileHandle = DtlObject->DataObject.FileObject;

	Offset.QuadPart = Header->SummaryLocation;
	SummaryLength = Header->SummaryLength;

	SetFilePointerEx(FileHandle, Offset, NULL, FILE_BEGIN);

	//
	// Read group count
	//

	ReadFile(FileHandle, &GroupCount, sizeof(ULONG), &Complete, NULL);
	ASSERT(GroupCount != 0);

	for(i = 0; i < GroupCount; i += 1) {

		PMSP_PROCESS Process;

		//
		// Create counter group by its pid
		//

		ReadFile(FileHandle, &ProcessId, sizeof(ULONG), &Complete, NULL); 
		Process = MspLookupProcess(DtlObject, ProcessId);
		ASSERT(Process != NULL);

		Group = (PMSP_COUNTER_GROUP)MspMalloc(sizeof(MSP_COUNTER_GROUP));
		RtlZeroMemory(Group, sizeof(MSP_COUNTER_GROUP));
		Group->Level = CounterGroupLevel;
		Group->Process = Process;
		Group->Object = &DtlObject->CounterObject;

		InitializeListHead(&Group->ActiveListHead);
		InitializeListHead(&Group->RetireListHead);
		StringCchCopy(Group->Name, MAX_PATH, Process->BaseName);

		//
		// Read entry and insert into probe table by its address and object id
		//

		ReadFile(FileHandle, &EntryCount, sizeof(ULONG), &Complete, NULL); 
		ASSERT(EntryCount != 0);

		ReadFile(FileHandle, &Group->Retire.NumberOfCalls, sizeof(ULONG), &Complete, NULL);
		ReadFile(FileHandle, &Group->Retire.NumberOfStackTraces, sizeof(ULONG), &Complete, NULL);
		ReadFile(FileHandle, &Group->Retire.NumberOfExceptions, sizeof(ULONG), &Complete, NULL);
		ReadFile(FileHandle, &Group->Retire.MaximumDuration, sizeof(ULONG), &Complete, NULL);
		ReadFile(FileHandle, &Group->Retire.MinimumDuration, sizeof(ULONG), &Complete, NULL);
		
		Process->Counter = Group;

		for(j = 0; j < EntryCount; j += 1) {
			
			ULONG Status;

			Entry = (PMSP_COUNTER_ENTRY)MspMalloc(sizeof(MSP_COUNTER_ENTRY));
			ReadFile(FileHandle, Entry, sizeof(MSP_COUNTER_ENTRY), &Complete, NULL); 

			Entry->Group = Group;
			Entry->Active = NULL;

			Status = MspInsertCounterEntryByAddress(Process, Entry);

			if (Status == MSP_STATUS_OK) {

				InsertHeadList(&Group->RetireListHead, &Entry->ListEntry);
				Group->NumberOfEntries += 1;

			} else {
				MspFree(Entry);
			}
		}

	}

	return MSP_STATUS_OK;
}

ULONG
MspInsertCounterEntryByAddress(
	IN PMSP_PROCESS Process, 
	IN PMSP_COUNTER_ENTRY Counter
	)
{
	ULONG Bucket;
	PMSP_PROBE Probe;
	PMSP_PROBE_TABLE Table;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;

	Table = Process->ProbeTable;
	Bucket = MspComputeProbeHash(Table, Counter->Retire.Address);
	ListHead = &Table->ListHead[Bucket];
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Probe = CONTAINING_RECORD(ListEntry, MSP_PROBE, ListEntry);
		if (Probe->ObjectId == Counter->Retire.ObjectId) {
			Probe->Counter = Counter;
			return MSP_STATUS_OK;
		}
		ListEntry = ListEntry->Flink;
	}

	return MSP_STATUS_NOT_FOUND;
}