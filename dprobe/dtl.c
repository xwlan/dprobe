//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2010
//

#include "dtl.h"
#include "queue.h"
#include "pdb.h"
#include "progressdlg.h"
#include "conditiondlg.h"
#include "mspdtl.h"

PSTR DtlCsvRecordColumn = " \"Process\", \"Time\", \"PID\", \"TID\", \"Probe\", \"Duration (ms)\", \"Result\", \"Detail\" \r\n";
PSTR DtlCsvRecordFormat = " \"%S\", \"%02d:%02d:%02d:%03d\", \"%d\", \"%d\", \"%S\", \"%.6lf\", \"0x%I64x\", \"%S\" \r\n";

//
// Dtl Version 
//

USHORT DtlMajorVersion;
USHORT DtlMinorVersion;

LIST_ENTRY DtlObjectList;

DTL_SHARED_DATA DtlSharedData;

ULONG
DtlInitialize(
	VOID
	)
{
	InitializeListHead(&DtlObjectList);
	return S_OK;

}

ULONG
DtlCreateMap(
	IN PDTL_FILE Object
	)
{
	ULONG Status;
	HANDLE Handle;
	PDTL_MAP MapObject;

	ASSERT(!Object->MapObject); 

	Handle = CreateFileMapping(Object->ObjectHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!Handle) {
		Status = GetLastError();
		return Status;
	}

	MapObject = (PDTL_MAP)BspMalloc(sizeof(DTL_MAP));
	MapObject->Offset = 0;
	MapObject->Length = 0;
	MapObject->Address = NULL;
	MapObject->LogicalFirst = 0;
	MapObject->LogicalLast = 0;
	MapObject->PhysicalFirst = 0;
	MapObject->PhysicalLast = 0;
	InitializeListHead(&MapObject->ListEntry);
	
	MapObject->Address = MapViewOfFile(Handle, FILE_MAP_READ | FILE_MAP_WRITE, 
		                               0, 0, DTL_ALIGNMENT_UNIT);
	MapObject->ObjectHandle = Handle;
	Object->MapObject = MapObject;
	MapObject->FileObject = Object;
	return S_OK;
}

PDTL_OBJECT
DtlCreateObject(
	IN PWSTR Path
	)
{
	ULONG Status;
	SYSTEMTIME Time;
	WCHAR Buffer[MAX_PATH];
	PDTL_FILE DataObject;
	PDTL_FILE IndexObject;
	PDTL_OBJECT Object;
	ULONG ThreadId;
	PDTL_COUNTER_OBJECT Counter;
	PDTL_QUEUE_THREAD_CONTEXT ThreadContext;

	GetLocalTime(&Time);

	StringCchPrintf(Buffer, MAX_PATH, L"%ws\\%02d%02d%02d%02d%02d", 
		            Path, Time.wMonth, Time.wDay, Time.wHour, Time.wMinute, Time.wSecond);
	
	Object = (PDTL_OBJECT)BspMalloc(sizeof(DTL_OBJECT));
	DataObject = &Object->DataObject; 
	IndexObject = &Object->IndexObject; 

	StringCchCopy(DataObject->Path, MAX_PATH, Buffer);
	StringCchCat(DataObject->Path, MAX_PATH, L".dtl");

	StringCchCopy(IndexObject->Path, MAX_PATH, Buffer);
	StringCchCat(IndexObject->Path, MAX_PATH, L".dti"); Status = DtlCreateFile(Object, DataObject, DTL_FILE_CREATE, DTL_FILE_DATA); if (Status != S_OK) { return NULL; } Status = DtlCreateFile(Object, IndexObject, DTL_FILE_CREATE, DTL_FILE_INDEX);
	if (Status != S_OK) {
		return NULL;
	}

	InitializeListHead(&Object->ManualGroupList);
	InitializeListHead(&Object->AddOnGroupList);

	BspInitializeLock(&Object->ObjectLock);

	InitializeListHead(&Object->Queue);
	BspInitializeLock(&Object->QueueLock);
	Object->StopEvent = CreateEvent(NULL, TRUE, FALSE, L"D Probe DtlStopEvent");
	InsertTailList(&DtlObjectList, &Object->ListEntry);

	PdbCreateObject(Object);

	//
	// Create counter object
	//

	Counter = (PDTL_COUNTER_OBJECT)BspMalloc(sizeof(DTL_COUNTER_OBJECT));
	Counter->DtlObject = Object;
	Counter->NumberOfCounterSet = 0;
	Counter->LastRecord = -1;
	InitializeListHead(&Counter->CounterSetList);
	Object->CounterObject = Counter;

	//
	// Create filter counter object
	//

	Counter = (PDTL_COUNTER_OBJECT)BspMalloc(sizeof(DTL_COUNTER_OBJECT));
	Counter->DtlObject = Object;
	Counter->NumberOfCounterSet = 0;
	Counter->LastRecord = -1;
	InitializeListHead(&Counter->CounterSetList);
	Object->FilterCounter = Counter;

	ThreadContext = (PDTL_QUEUE_THREAD_CONTEXT)BspMalloc(sizeof(DTL_QUEUE_THREAD_CONTEXT));
	ThreadContext->DtlObject = Object;

	//
	// Initialize shared callback data
	//

	StringCchCopy(DtlSharedData.DtlPath, MAX_PATH, DataObject->Path);

	//
	// N.B. Ensure that queue object is created first
	//

	PspCreateQueueObject();
	ThreadContext->QueueObject = PspGetQueueObject();
	CreateThread(NULL, 0, DtlWorkThread, ThreadContext, 0, &ThreadId);

	return Object;
}

PDTL_OBJECT
DtlOpenObject(
	IN PWSTR Path
	)
{
	ULONG Status;
	PDTL_OBJECT Object;
	PDTL_FILE DataObject;
	PDTL_FILE IndexObject;
	PDTL_MAP DataMapObject;
	PDTL_MAP IndexMapObject;
	ULONG High, Low;
	SIZE_T Length;
	PDTL_FILE_HEADER Header;
	PVOID Address;

	Object = (PDTL_OBJECT)BspMalloc(sizeof(DTL_OBJECT));
	DataObject = &Object->DataObject; 
	IndexObject = &Object->IndexObject; 

	BspInitializeLock(&Object->ObjectLock);
	BspInitializeLock(&Object->QueueLock);
	InitializeListHead(&Object->Queue);

	StringCchCopy(DataObject->Path, MAX_PATH, Path);
	
	Status = DtlCreateFile(Object, DataObject, DTL_FILE_OPEN, DTL_FILE_DATA);
	if (Status != S_OK) {
		return NULL;
	}

	PdbCreateObject(Object);

	//
	// N.B. Under open mode, we only create a single file object,
	// the second (index) is not used, instead we make the map object
	// of file object as index map object, and create a new data map object and
	// chain it to the index map object, this is because we always access index
	// first, and then access data map. 
	//

	IndexMapObject = DataObject->MapObject;
	ASSERT(IndexMapObject->ObjectHandle != NULL);

	Header = Object->Header;
	ASSERT(Header != NULL);

	Low = (ULONG)Header->IndexLocation;
	High = (ULONG)(Header->IndexLocation >> 32);

	ASSERT(Header->IndexLength > sizeof(DTL_INDEX));
	if (Header->IndexLength >= DTL_ALIGNMENT_UNIT) {
		Length = DTL_ALIGNMENT_UNIT;
	} else {
		Length = Header->IndexLength;
	}

	Address = MapViewOfFile(IndexMapObject->ObjectHandle, 
							FILE_MAP_READ | FILE_MAP_WRITE, 
		                    High, Low, Length);
	if (!Address) {
		Status = GetLastError();
		DebugTrace("DtlOpenObject index object %d", Status);
		return NULL;
	}

	IndexMapObject->Offset = Header->IndexLocation;
	IndexMapObject->Length = Length;
	IndexMapObject->Address = Address;
	IndexMapObject->LogicalFirst = 0;
	IndexMapObject->LogicalLast = 1; 
	IndexMapObject->PhysicalFirst = IndexMapObject->LogicalFirst;
	IndexMapObject->PhysicalLast = Length / sizeof(DTL_INDEX) - 1;

	Low = (ULONG)Header->RecordLocation;
	High = 0;

	Length = Header->RecordLength / DTL_ALIGNMENT_UNIT;
	if (Length > 0) {
		Length = DTL_ALIGNMENT_UNIT;
	}

	Address = MapViewOfFile(IndexMapObject->ObjectHandle, 
							FILE_MAP_READ | FILE_MAP_WRITE, 
		                    High, Low, Length);
	
	if (!Address) {
		Status = GetLastError();
		DebugTrace("DtlOpenObject data object %d", Status);
		return NULL;
	}

	DataMapObject = (PDTL_MAP)BspMalloc(sizeof(DTL_MAP));
	DataMapObject->ObjectHandle = IndexMapObject->ObjectHandle;
	DataMapObject->Address = Address;
	DataMapObject->Offset = Header->RecordLocation;
	DataMapObject->LogicalFirst = 0;
	DataMapObject->LogicalLast = 1;
	DataMapObject->PhysicalFirst = -1;
	DataMapObject->PhysicalLast = -1;

	//
	// N.B. data map object is chained at list entry of index map object,
	// they share same kernel handle, another point is, the record offset
	// described by index entry must drift forward size of DTL_FILE_HEADER
	// correctly locate the record. This looks a little bit ugly, we will
	// consider put the file header structure at the bottom of the file,
	// so access the data does not require any adjustment.
	//

	InitializeListHead(&IndexMapObject->ListEntry);
	InsertTailList(&IndexMapObject->ListEntry, &DataMapObject->ListEntry);

	//
	// Load stack trace database
	//

	Low = (ULONG)Header->StackLocation;
	//High = (ULONG)(Header->StackLength >> 32);
	High = (ULONG)(Header->StackLocation >> 32);

	SetFilePointer(DataObject->ObjectHandle, Low, &High, FILE_BEGIN);

	if (Header->StackLength) {
		PdbLoadStackTraceFromDtl(Object->PdbObject, 
			                     DataObject->ObjectHandle);
	}

	//
	// Load probe and add on group
	//

	if (Header->ProbeLength) {
		DtlLoadManualGroup(Object, DataObject->ObjectHandle);
		DtlLoadAddOnGroup(Object, DataObject->ObjectHandle);
	}

	//
	// N.B. Set record count for query information purpose
	//

	DataObject->RecordCount = Header->IndexNumber;
	IndexObject->RecordCount = Header->IndexNumber;

	SetFilePointer(DataObject->ObjectHandle, 0, NULL, FILE_BEGIN);

	InsertHeadList(&DtlObjectList, &Object->ListEntry);
	return Object;
}

ULONG
DtlCreateFile(
	IN PDTL_OBJECT Object,
	IN PDTL_FILE FileObject,
	IN DTL_FILE_MODE Mode,
	IN DTL_FILE_TYPE Type
	)
{
	HANDLE Handle;
	ULONG Status;
	ULONG Flag;

	if (Mode == DTL_FILE_CREATE) {
		Flag = CREATE_NEW;
	} else {
		Flag = OPEN_EXISTING;
	}

	Handle = CreateFile(FileObject->Path,
	                    GENERIC_READ | GENERIC_WRITE,
	                    FILE_SHARE_READ | FILE_SHARE_WRITE, 
	                    NULL,
	                    Flag,
	                    FILE_ATTRIBUTE_NORMAL,
	                    NULL);

	if (Handle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		return Status;
	}
	
	FileObject->Mode = Mode;
	FileObject->Type = Type;
	FileObject->ObjectHandle = Handle;

	if (Mode == DTL_FILE_CREATE) {
		
		SetFilePointer(Handle, DTL_ALIGNMENT_UNIT, 0, FILE_BEGIN);
		SetEndOfFile(Handle);

		FileObject->FileLength = DTL_ALIGNMENT_UNIT;
		FileObject->DataLength = 0;
		FileObject->BeginOffset = 0;
	}

	else {

		PDTL_FILE_HEADER Header;
		ULONG CompleteLength;
		ULONG CheckSum;
		
		ASSERT(Mode == DTL_FILE_OPEN);
		ASSERT(Type == DTL_FILE_DATA);

		Header = (PDTL_FILE_HEADER)BspMalloc(sizeof(DTL_FILE_HEADER));
		SetFilePointer(Handle, (LONG)(0 - sizeof(DTL_FILE_HEADER)), NULL, FILE_END);
		ReadFile(Handle, Header, sizeof(DTL_FILE_HEADER), &CompleteLength, NULL);
		SetFilePointer(Handle, 0, 0, FILE_BEGIN);

		if (Header->Signature != DTL_FILE_SIGNATURE) {
			CloseHandle(Handle);
			return STATUS_DTL_BAD_CHECKSUM;
		}

		CheckSum = DtlComputeCheckSum(Header);
		if (CheckSum != Header->CheckSum) {
			CloseHandle(Handle);
			BspFree(Header);
			return STATUS_DTL_BAD_CHECKSUM;
		}

		FileObject->FileLength = Header->TotalLength;
		Status = DtlAssertFileLength(Handle, FileObject);
		if (Status != TRUE) {
			CloseHandle(Handle);
			BspFree(Header);
			return STATUS_DTL_BAD_CHECKSUM;
		}

		//
		// In read only mode, we don't use DataLength, its information
		// are defined in DTL_FILE_HEADER
		//

		FileObject->DataLength = Header->IndexLocation - Header->RecordLocation;	
		FileObject->BeginOffset = Header->RecordLocation; 

		Object->Header = Header;

	}

	Status = DtlCreateMap(FileObject);
	return Status;
}

ULONG
DtlWriteFile(
	IN PDTL_FILE DataObject,
	IN PDTL_FILE IndexObject,
	IN PDTL_INDEX Index,
	IN PDTL_IO_BUFFER Buffer
	)
{
	ULONG Status;
	ULONG Number;
	ULONG High;
	ULONG Completed;

	Number = Buffer->NumberOfRecords;

	//High = (ULONG)(IndexObject->FileLength >> 32);
	High = (ULONG)(IndexObject->DataLength >> 32);
	SetFilePointer(IndexObject->ObjectHandle, (LONG)IndexObject->DataLength, &High, FILE_BEGIN); 
	Status = WriteFile(IndexObject->ObjectHandle, Index, sizeof(DTL_INDEX) * Number, &Completed, NULL); 
	if (Status != TRUE) {
		Status = GetLastError();
		return Status;
	}
	
	//High = (ULONG)(DataObject->FileLength >> 32);
	High = (ULONG)(DataObject->DataLength >> 32);
	SetFilePointer(DataObject->ObjectHandle, (LONG)DataObject->DataLength, &High, FILE_BEGIN); 
	
	Status = WriteFile(DataObject->ObjectHandle, Buffer->Buffer, Buffer->Length, &Completed, NULL);
	if (Status != TRUE) {
		Status = GetLastError();
		return Status;
	}
	
	DataObject->RecordCount += Number;
	DataObject->DataLength = DataObject->DataLength + Buffer->Length;
	if (DataObject->DataLength > DataObject->FileLength) {
		DataObject->FileLength = DataObject->DataLength;
	}

	IndexObject->RecordCount += Number;
	IndexObject->DataLength = IndexObject->DataLength + Number * sizeof(DTL_INDEX);
	if (IndexObject->DataLength > IndexObject->FileLength) {
		IndexObject->FileLength = IndexObject->DataLength;
	}

	//
	// Update shared data record counter, may not be accurate under
	// some circumstance, but it's tolerable
	//

	DtlSharedData.NumberOfRecords += Number;
	return S_OK;
}

ULONG
DtlDestroyFile(
	IN PDTL_FILE Object
	)
{
	PDTL_MAP MapObject;

	MapObject = Object->MapObject;
	if (MapObject) {
		DtlDestroyMap(Object);	
	}

	CloseHandle(Object->ObjectHandle);

	//
	// Only query object required to be freed
	//

	if (Object->Type == DTL_FILE_QUERY) {
		BspFree(Object);
	}

	return S_OK;
}

ULONG
DtlWriteRecord(
	IN PDTL_OBJECT Object,
	IN PDTL_IO_BUFFER Buffer 
	)
{
	ULONG Status;
	ULONG i, j, Number;
	ULONG64 Offset;
	PDTL_INDEX Index;
    PDTL_INDEX FilterIndex;
	PBTR_RECORD_HEADER Record;
	PDTL_FILE DataObject;
	PDTL_FILE IndexObject;

	BspAcquireLock(&Object->ObjectLock);

	DataObject = &Object->DataObject;
	IndexObject = &Object->IndexObject;

	Number = Buffer->NumberOfRecords;
	Index = (PDTL_INDEX)VirtualAlloc(NULL, sizeof(DTL_INDEX) * Number, MEM_COMMIT, PAGE_READWRITE);

    if (Object->ConditionObject != NULL) {
    	FilterIndex = (PDTL_INDEX)VirtualAlloc(NULL, sizeof(DTL_INDEX) * Number, MEM_COMMIT, PAGE_READWRITE);
    }

    j = 0;
	Record = (PBTR_RECORD_HEADER)Buffer->Buffer;
	Offset = DataObject->DataLength;

	for(i = 0; i < Number; i++) {

		Index[i].Offset = Offset; 
		Index[i].Length = Record->TotalLength;	
		Index[i].Flag = DTL_INDEX_NULL;	
		Offset += Record->TotalLength;

        if (Object->ConditionObject != NULL) {
            if (DtlIsFilteredRecord(Object, Object->ConditionObject, Record)) {
                RtlCopyMemory(&FilterIndex[j], &Index[i], sizeof(DTL_INDEX));
				DtlUpdateFilterCounters(Object, Record);
                j += 1;
            }
        }

		Record = (PBTR_RECORD_HEADER)((PCHAR)Record + Record->TotalLength);
	}

	Status = DtlWriteFile(DataObject, IndexObject, Index, Buffer);	
    if (j > 0) {
        Status = DtlWriteQueryFile(Object, FilterIndex, j);
    }
    
	VirtualFree(Index, 0, MEM_RELEASE);
    if (Object->ConditionObject != NULL) {
    	VirtualFree(FilterIndex, 0, MEM_RELEASE);
    }

	BspReleaseLock(&Object->ObjectLock);

	//
	// Call shared data callback routine
	//

	__try {
		
		if (DtlSharedData.Callback) {
			(*DtlSharedData.Callback)(&DtlSharedData, DtlSharedData.Context);
		}

	} __except(EXCEPTION_EXECUTE_HANDLER) {

	}
	return Status;		
}

ULONG
DtlDecodeStackTrace(
	IN PDTL_OBJECT Object,
	IN PDTL_IO_BUFFER Buffer 
	) 
{
	ULONG i;
	PBTR_RECORD_HEADER Record;
	PPDB_HASH Hash;
	PPDB_OBJECT PdbObject;

	Record = (PBTR_RECORD_HEADER)Buffer->Buffer;

	PdbObject = Object->PdbObject;

	for(i = 0; i < Buffer->NumberOfRecords; i++) {
		Hash = PdbGetStackTrace(Object->PdbObject, Record);
		Record = (PBTR_RECORD_HEADER)((PCHAR)Record + Record->TotalLength);
	}

	return S_OK;
}

ULONG
DtlWriteQueryFile(
    IN PDTL_OBJECT Object,
    IN PDTL_INDEX Index,
    IN ULONG NumberOfIndex
    )
{
    ULONG Status;
	LONG High;
	ULONG Completed;
    PDTL_FILE QueryObject;

    QueryObject = Object->QueryIndexObject;
    High = (LONG)(QueryObject->DataLength >> 32);

	SetFilePointer(QueryObject->ObjectHandle, (LONG)QueryObject->DataLength, &High, FILE_BEGIN);
	Status = WriteFile(QueryObject->ObjectHandle, Index, 
                       sizeof(DTL_INDEX) * NumberOfIndex, &Completed, NULL); 

	if (Status != TRUE) {
		Status = GetLastError();
		return Status;
	}

    QueryObject->DataLength += sizeof(DTL_INDEX) * NumberOfIndex;
    if (QueryObject->DataLength > QueryObject->FileLength) {
        QueryObject->FileLength = QueryObject->DataLength;
    }

    QueryObject->RecordCount += NumberOfIndex;

	//
	// Update filtered record number, note that we can not use add op,
	// because filter can change anytime, we always track its current
	// count, not a accumulated number
	//

	DtlSharedData.NumberOfFilterRecords = (ULONG)QueryObject->RecordCount;
    return S_OK;
}

ULONG
DtlDestroyMap(
	IN PDTL_FILE Object
	)
{
	PDTL_MAP MapObject;

	MapObject = Object->MapObject;

	if (MapObject->Address) {
		UnmapViewOfFile(MapObject->Address);
	}
	
	CloseHandle(MapObject->ObjectHandle);
	BspFree(MapObject);

	return S_OK;
}

BOOLEAN
DtlIsRecordCached(
	IN PDTL_OBJECT Object,
	IN ULONG64 Number
	)
{
	PDTL_FILE IndexObject;
	PDTL_MAP MapObject;

	IndexObject = &Object->IndexObject;
	MapObject = IndexObject->MapObject;

	if (Number >= MapObject->LogicalFirst && Number <= MapObject->LogicalLast) {
		return TRUE;
	}

	return FALSE;
}

ULONG
DtlGetRecordPointer(
	IN PDTL_OBJECT Object,
	IN ULONG64 Number,
	OUT PVOID *Buffer,
	OUT PBOOLEAN NeedFree 
	)
{
	PDTL_FILE DataObject;
	PDTL_FILE IndexObject;
	PDTL_MAP MapObject;
	PDTL_INDEX Index;
	DTL_INDEX Entry;
	DTL_SPAN Span;
	ULONG Complete;
	PVOID Address;
	ULONG High;
	ULONG64 Distance;
	ULONG Status;

	DataObject = &Object->DataObject;
	IndexObject = &Object->IndexObject;

	if (DtlIsRecordCached(Object, Number)) {

		MapObject = IndexObject->MapObject;
		Index = (PDTL_INDEX)MapObject->Address + Number - MapObject->LogicalFirst;

		MapObject = DataObject->MapObject;
		Address = (PCHAR)MapObject->Address + (Index->Offset - MapObject->Offset);

		*Buffer = Address; 
		*NeedFree = FALSE;
		return S_OK;
	} 

	BspAcquireLock(&Object->ObjectLock);
	if (DtlIsPhysicalCached(IndexObject, Number, Number, &Span)) {

		Address = BspMalloc(Span.First->Length);
		Distance = Span.First->Offset;
		High = (ULONG)(Distance & 0xFFFFFFFF00000000);
		SetFilePointer(DataObject->ObjectHandle, (ULONG)Distance, &High, FILE_BEGIN);
		ReadFile(DataObject->ObjectHandle, Address, Span.First->Length, &Complete, NULL);

		*Buffer = Address;
		*NeedFree = TRUE;

		BspReleaseLock(&Object->ObjectLock);
		return S_OK;
	}
	
	Distance = sizeof(DTL_INDEX) * Number;
	High = (ULONG)(Distance >> 32);

	SetFilePointer(IndexObject->ObjectHandle, (ULONG)Distance, &High, FILE_BEGIN);
	Status = ReadFile(IndexObject->ObjectHandle, &Entry, sizeof(Entry), &Complete, NULL);	
	if (Status != TRUE) {
		BspReleaseLock(&Object->ObjectLock);
		return S_FALSE;
	}

	ASSERT(Entry.Offset + Entry.Length < DataObject->DataLength);

	Address = BspMalloc(Entry.Length);

	Distance = Entry.Offset;
	High = (ULONG)(Entry.Offset >> 32);

	SetFilePointer(DataObject->ObjectHandle, (ULONG)Distance, &High, FILE_BEGIN);
	ReadFile(DataObject->ObjectHandle, Address, Entry.Length, &Complete, NULL);

	*Buffer = Address;
	*NeedFree = TRUE;
	BspReleaseLock(&Object->ObjectLock);
	return S_OK;
}

ULONG
DtlCopyRecord(
	IN PDTL_OBJECT Object,
	IN ULONG64 Number,
	IN PVOID Buffer,
	IN ULONG Length
	)
{
	PDTL_FILE DataObject;
	PDTL_FILE IndexObject;
	PDTL_MAP IndexMapObject;
	PDTL_MAP DataMapObject;
	PDTL_INDEX Index;
	PBTR_RECORD_HEADER Record;
	DTL_SPAN Span;
	ULONG64 Offset;
	ULONG RecordLength;
	ULONG CompleteLength;
	ULONG Status;
	LONG High;
	DTL_INDEX RecordIndex;		
	
    BspAcquireLock(&Object->ObjectLock);

	DataObject = &Object->DataObject;
	IndexObject = &Object->IndexObject;
    IndexMapObject = IndexObject->MapObject;
	DataMapObject = DataObject->MapObject;

	//
	// This is the fastest path, that both index and record are memory mapped
	//

	if (DtlIsLogicalCached(IndexObject, Number, Number)) {

		ASSERT(Number >= IndexMapObject->PhysicalFirst);
		Index = (PDTL_INDEX)IndexMapObject->Address + Number - IndexMapObject->PhysicalFirst;

		ASSERT(Index->Offset >= DataMapObject->Offset);
		ASSERT(Index->Offset - DataMapObject->Offset <= DataMapObject->Length);
		Record = (PBTR_RECORD_HEADER)((PCHAR)DataMapObject->Address + Index->Offset - DataMapObject->Offset);

		//__try {
			ASSERT(Record->RecordType == RECORD_EXPRESS || Record->RecordType == RECORD_FILTER);
			//ASSERT(Record->TotalLength < 4096);
			memcpy(Buffer, Record, Record->TotalLength);
		//}
		//__except(EXCEPTION_EXECUTE_HANDLER) {
		//	DebugTrace("DtlCopyRecord cached, exception");
		//}

		BspReleaseLock(&Object->ObjectLock);
		return S_OK;
	}

	//
	// This is the secondary path that only index are memory mapped
	//

	if (DtlIsPhysicalCached(IndexObject, Number, Number, &Span)) {
		
		Offset = Span.First->Offset;
		RecordLength = Span.First->Length;

	} 
	
	//
	// This is the slow path that both index and record are NOT memory mapped,
	// we directly do file I/O here, no memory mapped action taken.
	//

	else {

		Offset = IndexObject->BeginOffset + Number * sizeof(DTL_INDEX);
		High = (LONG)(Offset >> 32);
		High = SetFilePointer(IndexObject->ObjectHandle, (LONG)Offset, &High, FILE_BEGIN);
		if (High == INVALID_SET_FILE_POINTER) {
			Status = GetLastError();
			BspReleaseLock(&Object->ObjectLock);
			return Status;
		}

		Status = ReadFile(IndexObject->ObjectHandle, &RecordIndex, sizeof(DTL_INDEX), &CompleteLength, NULL);
		if (Status != TRUE) {
			Status = GetLastError();
			BspReleaseLock(&Object->ObjectLock);
			return Status;
		}

		Offset = RecordIndex.Offset;
		RecordLength = RecordIndex.Length;
	} 

	//
	// N.B. The following should be rare code path, typically the record to be copied
	// is supposed to be already cached due to cache hint, however, listview can't gurantee
	// this point, slow path do file I/O directly, we don't use memory map.
	//

	High = (LONG)(Offset >> 32);
	High = SetFilePointer(DataObject->ObjectHandle, (LONG)Offset, (PLONG)&High, FILE_BEGIN);		
	if (High == INVALID_SET_FILE_POINTER) {
		Status = GetLastError();
		BspReleaseLock(&Object->ObjectLock);
		return Status;
	}

	if (RecordLength > Length) {
		BspReleaseLock(&Object->ObjectLock);
		return STATUS_DTL_BUFFER_LIMITED;
	}

	Status = ReadFile(DataObject->ObjectHandle, Buffer, RecordLength, &CompleteLength, NULL);
	if (Status != TRUE) {
		Status = GetLastError();
		BspReleaseLock(&Object->ObjectLock);
		return Status;
	}

    BspReleaseLock(&Object->ObjectLock);
	return S_OK;
}

ULONG
DtlCopyFilteredRecord(
	IN PDTL_OBJECT Object,
	IN ULONG64 Number,
	IN PVOID Buffer,
	IN ULONG Length
	)
{
    PDTL_FILE DataObject;
    PDTL_FILE QueryObject;
    ULONG CompleteLength;
	DTL_INDEX Index;
	ULONG64 Offset;
	LONG High;

    BspAcquireLock(&Object->ObjectLock);

	DataObject = &Object->DataObject;
    QueryObject = Object->QueryIndexObject;
	ASSERT(QueryObject != NULL);

	/*
	if (!DtlIsLogicalCached(QueryObject, Number, Number)) {
		Status = DtlAdjustIndexMap(QueryObject, Number, Number + 1023, &Span);
		if (Status != S_OK) {
			BspReleaseLock(&Object->ObjectLock);
			return Status;
		}
	}

	Index = (PDTL_INDEX)MapObject->Address + Number - MapObject->PhysicalFirst;
*/
	ASSERT(Number < QueryObject->RecordCount);

	Offset = sizeof(DTL_INDEX) * Number;
	High = (LONG)(Offset >> 32);

	SetFilePointer(QueryObject->ObjectHandle, (LONG)Offset, &High, FILE_BEGIN);
	ReadFile(QueryObject->ObjectHandle, &Index, sizeof(DTL_INDEX), &CompleteLength, NULL);

	High = (LONG)(Index.Offset >> 32);
    SetFilePointer(DataObject->ObjectHandle, (LONG)Index.Offset, &High, FILE_BEGIN); 
    ReadFile(DataObject->ObjectHandle, Buffer, Index.Length, &CompleteLength, NULL);

    BspReleaseLock(&Object->ObjectLock);
	return S_FALSE;
}

BOOLEAN
DtlIsFileRequireIncrement(
	IN PDTL_FILE Object
	)
{
	ASSERT(Object->FileLength >= Object->DataLength);
	if (Object->FileLength == Object->DataLength) {
		return TRUE;
	}

	return FALSE;
}

BOOLEAN
DtlIsLogicalCached(
	IN PDTL_FILE IndexObject,
	IN ULONG64 First,
	IN ULONG64 Last
	)
{
	PDTL_MAP MapObject;

	ASSERT(IndexObject->Type == DTL_FILE_INDEX || 
		   IndexObject->Type == DTL_FILE_QUERY);

	MapObject = IndexObject->MapObject;
	if (First >= MapObject->LogicalFirst && Last <= MapObject->LogicalLast) {
		return TRUE;
	}
	
	return FALSE;
}

BOOLEAN
DtlIsPhysicalCached(
	IN PDTL_FILE IndexObject,
	IN ULONG64 First,
	IN ULONG64 Last,
	OUT PDTL_SPAN Span
	)
{
	PDTL_MAP MapObject;

	ASSERT(First <= Last);
	ASSERT(IndexObject->Type == DTL_FILE_INDEX || IndexObject->Type == DTL_FILE_QUERY);

	MapObject = IndexObject->MapObject;
	if (First >= MapObject->PhysicalFirst && Last <= MapObject->PhysicalLast) {
		Span->First = (PDTL_INDEX)MapObject->Address + First - MapObject->PhysicalFirst;
		Span->Last = (PDTL_INDEX)MapObject->Address + Last - MapObject->PhysicalFirst;
		return TRUE;
	}

	return FALSE;
}

ULONG
DtlAdjustQueryIndexMap(
	IN PDTL_FILE Object,
	IN ULONG64 First,
	IN ULONG64 Last,
	OUT PDTL_SPAN Span
	)
{
    ULONG Status;
	PDTL_MAP MapObject;
	ULONG64 MappedLength;
	ULONG64 MappedOffset;
	PVOID Address;

	ASSERT(First <= Last); 

	MapObject = Object->MapObject;
	MappedOffset = DtlRoundDownLength(First * sizeof(DTL_INDEX), DTL_ALIGNMENT_UNIT);
	MappedLength = (Last + 1) * sizeof(DTL_INDEX) - MappedOffset;

	if (MappedOffset + MappedLength > Object->FileLength) {
		ASSERT(0);
		return STATUS_DTL_INVALID_INDEX;
	}

	UnmapViewOfFile(MapObject->Address);
	CloseHandle(MapObject->ObjectHandle);

	MapObject->ObjectHandle = CreateFileMapping(Object->ObjectHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!MapObject->ObjectHandle) {
		Status = GetLastError();
		return Status;
	}

	Address = MapViewOfFile(MapObject->ObjectHandle, FILE_MAP_READ | FILE_MAP_WRITE,
						    (ULONG)(MappedOffset >> 32), (ULONG)MappedOffset,  
						    MappedLength);
	if (!Address) {
		ASSERT(0);
		Status = GetLastError();
		return Status;
	}
	
	MapObject->Offset = MappedOffset;
	MapObject->Length = MappedLength;
	MapObject->Address = Address;
	MapObject->LogicalFirst = First;
	MapObject->LogicalLast = Last;

	MapObject->PhysicalFirst = MappedOffset / sizeof(DTL_INDEX);
	MapObject->PhysicalLast = (MappedOffset + MappedLength) / sizeof(DTL_INDEX) - 1;

	Span->First = (PDTL_INDEX)MapObject->Address + First - MapObject->PhysicalFirst;
	Span->Last = (PDTL_INDEX)MapObject->Address + Last - MapObject->PhysicalFirst;
		
	return S_OK;
}

ULONG
DtlAdjustIndexMap(
	IN PDTL_FILE Object,
	IN ULONG64 First,
	IN ULONG64 Last,
	OUT PDTL_SPAN Span
	)
{
	ULONG Status;
	PDTL_MAP MapObject;
	ULONG64 MappedLength;
	ULONG64 MappedOffset;
	PVOID Address;

	ASSERT(First <= Last); 
	ASSERT(Object->RecordCount > 0);

	if (Last + 1 >= Object->RecordCount) {
		Last = Object->RecordCount - 1;
	}

	//
	// N.B. index part of report file has non-zero BeginOffset, we need add it to
	// relative index offset, because the index part always get started from 64K
	// alignment boundary, so we can simply add it to relative index offset.
	// 

	MapObject = Object->MapObject;
	MappedOffset = DtlRoundDownLength(First * sizeof(DTL_INDEX), DTL_ALIGNMENT_UNIT);
	MappedLength = (Last + 1) * sizeof(DTL_INDEX) - MappedOffset;
	MappedOffset += Object->BeginOffset;

	UnmapViewOfFile(MapObject->Address);
	CloseHandle(MapObject->ObjectHandle);

	MapObject->ObjectHandle = CreateFileMapping(Object->ObjectHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!MapObject->ObjectHandle) {
		Status = GetLastError();
		return Status;
	}

	Address = MapViewOfFile(MapObject->ObjectHandle, FILE_MAP_READ | FILE_MAP_WRITE,
						    (ULONG)(MappedOffset >> 32), (ULONG)MappedOffset,  
						    MappedLength);
	if (!Address) {
		Status = GetLastError();
		return Status;
	}
	
	//
	// N.B. PhysicalLast always equal to LogicalLast since we don't round up mapped length,
	// only round down mapped offset to conform with file mapping requirement
	//

	MapObject->Offset = MappedOffset;
	MapObject->Length = MappedLength;
	MapObject->Address = Address;
	MapObject->LogicalFirst = First;
	MapObject->LogicalLast = Last;

	MapObject->PhysicalFirst = (MappedOffset - Object->BeginOffset) / sizeof(DTL_INDEX);
	MapObject->PhysicalLast = Last;

	Span->First = (PDTL_INDEX)MapObject->Address + First - MapObject->PhysicalFirst;
	Span->Last = (PDTL_INDEX)MapObject->Address + Last - MapObject->PhysicalFirst;
		
	return S_OK;
}

ULONG
DtlUpdateCache(
	IN PDTL_OBJECT Object,
	IN ULONG64 First,
	IN ULONG64 Last
	)
{
	ULONG Status;
	ULONG64 MappedOffset;
	ULONG MappedLength;
	PDTL_FILE DataObject;
	PDTL_FILE IndexObject;
	PDTL_MAP MapObject;
	DTL_SPAN Span;

	BspAcquireLock(&Object->ObjectLock);

	ASSERT(First <= Last);
	DataObject = &Object->DataObject;
	IndexObject = &Object->IndexObject;

	if (DtlIsLogicalCached(IndexObject, First, Last)) {
		BspReleaseLock(&Object->ObjectLock);
		return S_OK;
	}
	
	if (DtlIsPhysicalCached(IndexObject, First, Last, &Span)) {

		MappedOffset = DtlRoundDownLength(Span.First->Offset, DTL_ALIGNMENT_UNIT);
		MappedLength = (ULONG)(Span.Last->Offset + Span.Last->Length - MappedOffset);
		MapObject = IndexObject->MapObject;
		MapObject->LogicalFirst = First;
		MapObject->LogicalLast = Last;

	}
	else {

		Status = DtlAdjustIndexMap(IndexObject, First, Last, &Span);
		if (Status != S_OK) {
			BspReleaseLock(&Object->ObjectLock);
			return Status;
		}
		
		MappedOffset = DtlRoundDownLength(Span.First->Offset, DTL_ALIGNMENT_UNIT);
		MappedLength = (ULONG)(Span.Last->Offset + Span.Last->Length - MappedOffset);
	}

	Status = DtlAdjustDataMap(DataObject, IndexObject, MappedOffset, MappedLength);

	BspReleaseLock(&Object->ObjectLock);
	return Status;
}

ULONG
DtlUpdateQueryCache(
	IN PDTL_OBJECT Object,
	IN ULONG64 First,
	IN ULONG64 Last
	)
{
	ULONG Status;
	ULONG64 MappedOffset;
	ULONG MappedLength;
	DTL_SPAN Span;
	PDTL_FILE DataObject;
	PDTL_FILE QueryObject;
	PDTL_MAP MapObject;

    //
    // N.B. Under filter mode, we only adjust index map,
    // don't map data object, data records are read from
    // file directly, this reserves optimization space for
    // us, we may switch between map and file I/O mode based
    // on width of mapped records, if it's fit into resource
    // constraints, e.g. it does not span 1M boundary, we can
    // directly map them, otherwise, we need read from file.
    //

	BspAcquireLock(&Object->ObjectLock);

    ASSERT(Object->ConditionObject != NULL);
    ASSERT(Object->QueryIndexObject != NULL);

	DataObject = &Object->DataObject;
    QueryObject = Object->QueryIndexObject;

	ASSERT(First <= Last);

	if (DtlIsLogicalCached(QueryObject, First, Last)) {
		BspReleaseLock(&Object->ObjectLock);
		return S_OK;
	}

	if (DtlIsPhysicalCached(QueryObject, First, Last, &Span)) {

		MappedOffset = DtlRoundDownLength(Span.First->Offset, DTL_ALIGNMENT_UNIT);
		MappedLength = (ULONG)(Span.Last->Offset + Span.Last->Length - MappedOffset);
		MapObject = QueryObject->MapObject;
		MapObject->LogicalFirst = First;
		MapObject->LogicalLast = Last;

	}
	else {

		Status = DtlAdjustIndexMap(QueryObject, First, Last, &Span);
		if (Status != S_OK) {
			BspReleaseLock(&Object->ObjectLock);
			return Status;
		}
		
		MappedOffset = DtlRoundDownLength(Span.First->Offset, DTL_ALIGNMENT_UNIT);
		MappedLength = (ULONG)(Span.Last->Offset + Span.Last->Length - MappedOffset);
	}

	DebugTrace("DtlUpdateQueryCache %d -> %d  offset %I64x  length %d", 
			   (ULONG)QueryObject->MapObject->LogicalFirst, 
               (ULONG)QueryObject->MapObject->LogicalLast, 
			   MappedOffset, MappedLength);

	BspReleaseLock(&Object->ObjectLock);
	return S_OK;
}

ULONG
DtlAdjustDataMap(
	IN PDTL_FILE DataObject,
	IN PDTL_FILE IndexObject,
	IN ULONG64 MappedOffset,
	IN ULONG MappedLength
	)
{
	ULONG Status;
	PDTL_MAP MapObject;
	PDTL_MAP IndexMapObject;

	MapObject = DataObject->MapObject;
	IndexMapObject = IndexObject->MapObject;

	UnmapViewOfFile(MapObject->Address);
	CloseHandle(MapObject->ObjectHandle);

	MapObject->ObjectHandle = CreateFileMapping(DataObject->ObjectHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!MapObject->ObjectHandle) {
		Status = GetLastError();
		return Status;
	}

	MapObject->Address = MapViewOfFile(MapObject->ObjectHandle, FILE_MAP_READ | FILE_MAP_WRITE, 
									   (ULONG)(MappedOffset >> 32), (ULONG)MappedOffset, 
									   MappedLength);
	if (!MapObject->Address) {
		ASSERT(0);
		Status = GetLastError();
		return Status;
	}
	
	MapObject->Offset = MappedOffset;
	MapObject->Length = MappedLength;
	MapObject->LogicalFirst = IndexMapObject->LogicalFirst;
	MapObject->LogicalLast = IndexMapObject->LogicalLast;

	return S_OK;
}

LONG CALLBACK 
DtlExceptionFilter(
	IN PEXCEPTION_POINTERS Pointers
	)
{
	DebugTrace("DtlExceptionFilter");
	return 0L;
}

BOOLEAN
DtlVerfiyBufferObject(
	IN PDTL_IO_BUFFER Buffer
	)
{
	PBTR_RECORD_HEADER Record;

	__try {

		Record = (PBTR_RECORD_HEADER)Buffer->Buffer;
		if (Record->TotalLength > Buffer->Length) {
			DebugTrace("DtlVerifyBufferObject bad length");
			return FALSE;
		}

		if (!Buffer->NumberOfRecords) {
			DebugTrace("DtlVerifyBufferObject bad record number");
			return FALSE;
		}
	}
		
	__except(DtlExceptionFilter(GetExceptionInformation())) {
		return FALSE;
	}

	return TRUE;	
}

ULONG
DtlQueryInformation(
	IN PDTL_OBJECT Object,
	IN PDTL_INFORMATION Information
	)
{
	if (Object->DataObject.Mode != DTL_FILE_OPEN) {

		BspAcquireLock(&Object->ObjectLock);

		Information->DataFileLength = Object->DataObject.FileLength;
		Information->IndexFileLength = Object->IndexObject.FileLength; 

        if (!Object->ConditionObject) {
    		Information->NumberOfRecords = Object->IndexObject.RecordCount;
        } else {
            Information->NumberOfRecords = Object->QueryIndexObject->RecordCount;
        }

		BspReleaseLock(&Object->ObjectLock);

	} else {
	
		PDTL_FILE_HEADER Header;

		Header = Object->Header;
		ASSERT(Header != NULL);

		Information->DataFileLength = Header->RecordLength;		
		Information->IndexFileLength = Header->IndexLength;

        if (!Object->ConditionObject) {
    		Information->NumberOfRecords = Header->IndexNumber;
        } else {
            Information->NumberOfRecords = Object->QueryIndexObject->RecordCount;
        }
	}

	return S_OK;
}

ULONG
DtlQueueRecord(
	IN PDTL_OBJECT Object,
	IN PDTL_IO_BUFFER Buffer
	) 
{
	if (DtlVerfiyBufferObject(Buffer) != TRUE) {
		return S_FALSE;
	}

	BspAcquireLock(&Object->QueueLock);
	InsertTailList(&Object->Queue, &Buffer->ListEntry);
	BspReleaseLock(&Object->QueueLock);

	return S_OK;
}

ULONG
DtlFlushQueue(
	IN PDTL_OBJECT Object,
	IN DTL_FLUSH_MODE Mode,
	OUT PLIST_ENTRY QueryListHead
	)
{
	PLIST_ENTRY ListEntry;
	LIST_ENTRY ListHead;
	PDTL_IO_BUFFER Buffer;

	InitializeListHead(&ListHead);

	if (Mode == DTL_FLUSH_QUERY) {
		ASSERT(QueryListHead != NULL);
		InitializeListHead(QueryListHead);
	}

	BspAcquireLock(&Object->QueueLock);

	__try {

		while(IsListEmpty(&Object->Queue) != TRUE) {

			ListEntry = RemoveHeadList(&Object->Queue);
			switch (Mode) {

				case DTL_FLUSH_QUERY:
					InsertTailList(QueryListHead, ListEntry);
					break;

				case DTL_FLUSH_DROP:
					Buffer = (PDTL_IO_BUFFER)CONTAINING_RECORD(ListEntry, DTL_IO_BUFFER, ListEntry);
					BspFree(Buffer->Buffer);
					BspFree(Buffer);
					break;

				case DTL_FLUSH_COMMIT:
					InsertTailList(&ListHead, ListEntry);
					break;

				default:
					ASSERT(0);
			}
		}
	}

	__finally {
		BspReleaseLock(&Object->QueueLock);
	}

	if (Mode != DTL_FLUSH_COMMIT) {
		return S_OK;	
	}

	while (IsListEmpty(&ListHead) != TRUE) {

		ListEntry = RemoveHeadList(&ListHead);
		Buffer = (PDTL_IO_BUFFER)CONTAINING_RECORD(ListEntry, DTL_IO_BUFFER, ListEntry);

		DtlWriteRecord(Object, Buffer);
		DtlDecodeStackTrace(Object, Buffer);
		DtlUpdateCounters(Object, Buffer);

		BspFree(Buffer->Buffer);
		BspFree(Buffer);
	}

	return S_OK;
}

ULONG CALLBACK
DtlWorkThread(
	IN PVOID Context
	)
{
	ULONG Status;
	PDTL_QUEUE_THREAD_CONTEXT ThreadContext;
	PBSP_QUEUE QueueObject;
	PDTL_OBJECT DtlObject;
	PBSP_QUEUE_NOTIFICATION Notification;
	PDTL_IO_BUFFER Buffer;

	ThreadContext = (PDTL_QUEUE_THREAD_CONTEXT)Context;
	QueueObject = (PBSP_QUEUE)ThreadContext->QueueObject;
	DtlObject = (PDTL_OBJECT)ThreadContext->DtlObject;

	BspFree(ThreadContext);

	ASSERT(QueueObject != NULL);
	ASSERT(QueueObject->ObjectHandle != NULL);

	while (TRUE) {

		Status = BspDeQueueNotification(QueueObject, &Notification);
		if (Status != S_OK) {
			break;
		}

		if (Notification->Type == BSP_QUEUE_STOP) {
			SetEvent((HANDLE)Notification->Context);
			BspFree(Notification);
			break;
		}

		if (Notification->Type == BSP_QUEUE_IO) {

			Buffer = (PDTL_IO_BUFFER)Notification->Packet;
			DtlWriteRecord(DtlObject, Buffer);
			DtlDecodeStackTrace(DtlObject, Buffer);
			DtlUpdateCounters(DtlObject, Buffer);

			BspFree(Buffer->Buffer);
			BspFree(Buffer);
			BspFree(Notification);
		}
	}

	return S_OK;
}

ULONG CALLBACK
DtlQueueThread2(
	IN PVOID Context
	)
{
	ULONG Status;
	HANDLE Timer;
	ULONG Period;
	LARGE_INTEGER DueTime;
	PDTL_OBJECT Object;
	HANDLE WaitHandles[2];

	Object = (PDTL_OBJECT)Context;
	ASSERT(Object != NULL);
	
	Period = 1000;
	DueTime.QuadPart = -10000 * Period;
	Timer = CreateWaitableTimer(NULL, FALSE, NULL);

	Status = SetWaitableTimer(Timer, &DueTime, Period, NULL, NULL, FALSE);
	if (Status != TRUE) {
		Status = GetLastError();
		return Status;
	}

	__try {

		WaitHandles[0] = Timer;
		WaitHandles[1] = Object->StopEvent;

		while (TRUE) {

			Status = WaitForMultipleObjects(2, WaitHandles, FALSE, INFINITE);

			if (Status == WAIT_OBJECT_0) {
				DtlFlushQueue(Object, DTL_FLUSH_COMMIT, NULL);
			}
			else if (Status == WAIT_OBJECT_0 + 1) {
				DtlFlushQueue(Object, DTL_FLUSH_DROP, NULL);
				break;
			} 
			else {
				DebugBreak();
			}
		}

	}

	__except(DtlExceptionFilter(GetExceptionInformation())) {
		Status = S_FALSE;	
	}

	CancelWaitableTimer(Timer);
	CloseHandle(Timer);
	return Status;
}

ULONG64 
DtlRoundUpLength(
	IN ULONG64 Length,
	IN ULONG64 Alignment 
	)
{
	return (((ULONG64)(Length) + (ULONG64)(Alignment) - 1) & ~((ULONG64)(Alignment) - 1)); 
}

ULONG64 
DtlRoundDownLength(
	IN ULONG64 Length,
	IN ULONG64 Alignment 
	)
{
	return ((ULONG64)(Length) & ~((ULONG64)(Alignment) - 1));
}

BOOLEAN
DtlAssertFileLength(
	IN HANDLE FileHandle,
	IN PDTL_FILE Object
	)
{
	ULONG Status;
	ULONG High, Low;
	ULONG64 Length;

	High = 0;

	Low = GetFileSize(FileHandle, &High);
	Status = GetLastError();

	if (Low == 0xFFFFFFFF && Status != NO_ERROR) { 
		DebugTrace("DtlAssertFileLength GetFileSize error=%d", Status);
		return FALSE;	
    } 

	Length = (ULONG64)(((ULONG64)High << 32) | (ULONG64)Low);
	if (Length == Object->FileLength) {
		return TRUE;
	}

	ASSERT(0);
	return FALSE;
}

HANDLE
DtlCopyDataFile(
    IN PDTL_OBJECT Object,
    IN ULONG64 DataLength,
    IN PWSTR Path,
    IN HWND hWnd
    )
{
    ULONG Status;
    HANDLE FileHandle;
    HANDLE DataHandle;
    PVOID Buffer;
    ULONG Percent;
    ULONG CompleteLength;
    ULONG64 Round, Remain, i;
    PWCHAR PercentSign;
	BOOLEAN Cancel = FALSE;

    DeleteFile(Path);
    FileHandle = CreateFile(Path, GENERIC_READ | GENERIC_WRITE, 
							FILE_SHARE_READ | FILE_SHARE_WRITE, 
							NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
        DebugTrace("DtlCopyDataFile FileHandle %d", Status);
		return INVALID_HANDLE_VALUE;
	}

    //
    // Create a new file stream to avoid file pointer race with existing data object
    //

    DataHandle = CreateFile(Object->DataObject.Path, GENERIC_READ | GENERIC_WRITE, 
							FILE_SHARE_READ | FILE_SHARE_WRITE, 
							NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (DataHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
        DebugTrace("DtlCopyDataFile DataHandle %d", Status);
		return INVALID_HANDLE_VALUE;
	}

    Round = DataLength / DTL_COPY_UNIT;
    Remain = DataLength % DTL_COPY_UNIT; 

    Buffer = VirtualAlloc(NULL, DTL_COPY_UNIT, MEM_COMMIT, PAGE_READWRITE);

    for(i = 0; i < Round; i++) {

        ReadFile(DataHandle, Buffer, DTL_COPY_UNIT, &CompleteLength, NULL);
        WriteFile(FileHandle, Buffer, DTL_COPY_UNIT, &CompleteLength, NULL);

        Percent = (ULONG)(DTL_COPY_UNIT * (i + 1) / (DataLength / 100));
        StringCchPrintf(Buffer, MAX_PATH - 1, 
                        L"Saved %u# of total %I64u KB data records ...", 
                        Percent, DataLength / (ULONG64)1024);

        PercentSign = wcschr(Buffer, '#'); 
        *PercentSign = L'%';

        Cancel = ProgressDialogCallback(hWnd, Percent, Buffer, FALSE, FALSE); 

		if (Cancel) {
			CloseHandle(FileHandle);
			CloseHandle(DataHandle);
			DeleteFile(Path);
			VirtualFree(Buffer, 0, MEM_RELEASE);
			return INVALID_HANDLE_VALUE;
		}
    }

    ReadFile(DataHandle, Buffer, (ULONG)Remain, &CompleteLength, NULL);
    WriteFile(FileHandle, Buffer, (ULONG)Remain, &CompleteLength, NULL);
    CloseHandle(DataHandle);

    //
    // N.B. We don't specify it's complete since we still want to copy
    // index, stack trace, probe group etc to the final file.
    //

    StringCchPrintf(Buffer, MAX_PATH - 1, 
                    L"Saved %u# of total %I64u KB data records ...", 
                    100, DataLength / 1024);

    PercentSign = wcschr(Buffer, '#'); 
    *PercentSign = L'%';
    Cancel = ProgressDialogCallback(hWnd, 99, Buffer, FALSE, FALSE); 
    VirtualFree(Buffer, 0, MEM_RELEASE);

	if (Cancel) {
		CloseHandle(FileHandle);
		DeleteFile(Path);
		return INVALID_HANDLE_VALUE;
	}
	
	return FileHandle;
}

ULONG
DtlWriteDtl(
	IN PDTL_OBJECT Object,
	IN PWSTR Path,
	IN HWND hWnd
	)
{
	PDTL_FILE DataObject;
	PDTL_FILE IndexObject;
	ULONG Number;
	ULONG Status;
	ULONG64 Length;
    ULONG64 RecordCount;
	ULONG64 Round;
	ULONG64 Remainder;
	PVOID Buffer;
	HANDLE FileHandle;
    HANDLE IndexHandle;
	ULONG CompleteLength;
	DTL_FILE_HEADER Header;
	ULONG Low, High;

	DataObject = &Object->DataObject;
	IndexObject = &Object->IndexObject;

	RtlZeroMemory(&Header, sizeof(Header));

    //
    // N.B. DtlObject's objectlock can not be held
    // during the routine, otherwise logview will
    // be blocked, we just take a snapshot of current
    // data length and record count, re-create new
    // file handle to do I/O.
    //

	BspAcquireLock(&Object->ObjectLock);
	RecordCount = IndexObject->RecordCount;
	Length = DataObject->DataLength;
	BspReleaseLock(&Object->ObjectLock);
    
    FileHandle = DtlCopyDataFile(Object, Length, Path, hWnd);
	if (FileHandle == INVALID_HANDLE_VALUE) {
		ProgressDialogCallback(hWnd, 100, L"Problem occurred when writing file ...", TRUE, TRUE);
		Status = GetLastError();
		return Status;
	}

	//
	// Seek to positon of alignment of DataObject->DataLength, we need
	// align the index part with DTL_ALIGNMENT_UNIT to map it from the
	// alignment boundary.
	//

	Length = DtlRoundUpLength(Length, DTL_ALIGNMENT_UNIT);
	
	Header.RecordLocation = 0; 
	Header.RecordLength = Length;
	Header.IndexLocation = Length;
	Header.IndexNumber = RecordCount; 
	Header.IndexLength = RecordCount * sizeof(DTL_INDEX);

	Low = (ULONG)Length;
	High = (ULONG)(Length >> 32);

	Status = SetFilePointer(FileHandle, Low, &High, FILE_BEGIN);
	if (Status == INVALID_SET_FILE_POINTER) {
		Status = GetLastError();
        CloseHandle(FileHandle);
		ProgressDialogCallback(hWnd, 100, L"Problem occurred when writing file ...", TRUE, TRUE);
		return Status;
	}

    //
    // Create a new file stream to avoid file pointer race with existing data object
    //

    IndexHandle = CreateFile(Object->IndexObject.Path, GENERIC_READ | GENERIC_WRITE, 
							 FILE_SHARE_READ | FILE_SHARE_WRITE, 
							 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (IndexHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
        CloseHandle(FileHandle);
		ProgressDialogCallback(hWnd, 100, L"Problem occurred when writing file ...", TRUE, TRUE);
		return Status;
	}

	//
	// Append index file into dtl data file, copy unit is currently 4MB
	//

    Round = (ULONG)(Header.IndexLength / DTL_COPY_UNIT);
    Remainder = Header.IndexLength % DTL_COPY_UNIT;

	Status = TRUE;
	Buffer = VirtualAlloc(NULL, DTL_COPY_UNIT, MEM_COMMIT, PAGE_READWRITE);

	for (Number = 0; Number < Round; Number += 1) {
		Status = ReadFile(IndexHandle, Buffer, DTL_COPY_UNIT, &CompleteLength, NULL);
		if (Status != TRUE) {
			break;
		}
		Status = WriteFile(FileHandle, Buffer, DTL_COPY_UNIT, &CompleteLength, NULL);
		if (Status != TRUE) {
			break;
		}
	}
	
	if (Status != TRUE) {
		goto FSERROR;
	}

	if (Remainder != 0) {

		Status = ReadFile(IndexHandle, Buffer, (ULONG)Remainder, &CompleteLength, NULL);
		if (Status != TRUE) {
			goto FSERROR;
		}

		Status = WriteFile(FileHandle, Buffer, (ULONG)Remainder, &CompleteLength, NULL);
		if (Status != TRUE) {
			goto FSERROR;
		}
	}

FSERROR:
	if (Status != TRUE) {

		Status = GetLastError();
		CloseHandle(FileHandle);
	    CloseHandle(IndexHandle);
		VirtualFree(Buffer, 0, MEM_RELEASE);
		ProgressDialogCallback(hWnd, 100, L"Problem occurred when writing file ...", TRUE, TRUE);
		return Status;

	} else {

	    CloseHandle(IndexHandle);
		VirtualFree(Buffer, 0, MEM_RELEASE);

	}

	//
	// Append stack trace database
	//
	
	Header.StackLocation = Header.IndexLocation + Header.IndexLength;
	PdbWriteStackTraceToDtl(Object->PdbObject, FileHandle);
	
	Low = GetFileSize(FileHandle, &High);
	Header.ProbeLocation = (ULONG64)(((ULONG64)High << 32) | (ULONG64)Low);
	Header.StackLength = Header.ProbeLocation - Header.StackLocation;
	
	//
	// Append probe groups
	//

	DtlWriteManualGroup(Object, FileHandle);
	DtlWriteAddOnGroup(Object, FileHandle);

	Low = GetFileSize(FileHandle, &High);
	Header.SummaryLocation = (ULONG64)(((ULONG64)High << 32) | (ULONG64)Low);
	Header.ProbeLength = Header.SummaryLocation - Header.ProbeLocation;
	
	//
	// Append summary report
	//

	DtlWriteSummaryReport(Object, FileHandle);
	Low = GetFileSize(FileHandle, &High);
	Length = (ULONG64)(((ULONG64)High << 32) | (ULONG64)Low);
	Header.SummaryLength = Length - Header.SummaryLocation;

	//
	// Fill dtl header fields
	//

	Header.TotalLength = Length + sizeof(DTL_FILE_HEADER);
	Header.Signature = DTL_FILE_SIGNATURE;
	Header.MajorVersion = DtlMajorVersion;
	Header.MinorVersion = DtlMinorVersion;
	Header.FeatureFlag = 0;
	Header.StartTime = Object->StartTime;
	Header.EndTime = Object->EndTime;
	Header.Reserved[0] = 0;
	Header.Reserved[1] = 0;
	Header.CheckSum = DtlComputeCheckSum(&Header);

	//
	// N.B. We put the file 'header' in fact at the end of the file, this
	// obtain benefit that we don't need to adjust the record offset when
	// we read the record in report mode, meanwhile, it's easy to append
	// new segment to the whole dtl file.
	//

	WriteFile(FileHandle, &Header, sizeof(Header), &CompleteLength, NULL);
	CloseHandle(FileHandle);
	
    ProgressDialogCallback(hWnd, 100, L"Successfully saved file ...", FALSE, TRUE); 
	return S_OK;
}

ULONG
DtlWriteDtlFiltered(
	IN PDTL_OBJECT Object,
	IN PWSTR Path,
	IN HWND hWnd
	)
{
	PDTL_FILE DataObject;
	PDTL_FILE IndexObject;
	ULONG Number;
	ULONG Status;
	ULONG64 Length;
    ULONG64 RecordCount;
	ULONG64 Round;
	ULONG64 Remainder;
	PVOID Buffer;
	HANDLE DataHandle;
    HANDLE IndexHandle;
	HANDLE AdjustedIndexHandle;
	HANDLE FileHandle;
	ULONG CompleteLength;
	DTL_FILE_HEADER Header;
	DTL_INDEX Index;
	ULONG Low, High;
	ULONG i;
	PBTR_RECORD_HEADER Cache;
	LARGE_INTEGER FileSize;
	WCHAR AdjustedIndexPath[MAX_PATH];
	WCHAR Text[MAX_PATH];
	ULONG64 CurrentDataOffset;
	ULONG Percent;
	BOOL Cancel;

	//
	// N.B. Filtered saving use QueryIndexObject as index object
	//

	DataObject = &Object->DataObject;
	IndexObject = Object->QueryIndexObject;

	ASSERT(Object->ConditionObject != NULL);
	ASSERT(IndexObject != NULL);

	RtlZeroMemory(&Header, sizeof(Header));

    //
    // N.B. DtlObject's objectlock can not be held
    // during the routine, otherwise logview will
    // be blocked, we just take a snapshot of current
    // data length and record count, re-create new
    // file handle to do I/O.
    //

	RecordCount = IndexObject->RecordCount;
    
	//
	// Create the new target file
	//

	DeleteFile(Path);
	FileHandle = CreateFile(Path, GENERIC_READ | GENERIC_WRITE, 
							FILE_SHARE_READ | FILE_SHARE_WRITE, 
							NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
        DebugTrace("Failed to create data file, %d", Status);
		return Status;
	}

	//
	// Create adjusted index file
	//

	StringCchPrintf(AdjustedIndexPath, MAX_PATH, L"%ws.dti", Path);
	AdjustedIndexHandle = CreateFile(AdjustedIndexPath, GENERIC_READ | GENERIC_WRITE, 
									 FILE_SHARE_READ | FILE_SHARE_WRITE, 
									 NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (AdjustedIndexHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
        DebugTrace("Failed to create adjusted index file, %d", Status);
		CloseHandle(FileHandle);
		DeleteFile(Path);
		return Status;
	}

	//
	// Create new data object and new index object stream to avoid file pointer collision
	//

	DataHandle = CreateFile(DataObject->Path, GENERIC_READ | GENERIC_WRITE, 
							FILE_SHARE_READ | FILE_SHARE_WRITE, 
							NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (DataHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
        DebugTrace("Failed to create data file, %d", Status);
		return Status;
	}

	IndexHandle = CreateFile(IndexObject->Path, GENERIC_READ | GENERIC_WRITE, 
							 FILE_SHARE_READ | FILE_SHARE_WRITE, 
							 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (IndexHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
        DebugTrace("Failed to create index file, %d", Status);
		CloseHandle(DataHandle);
		return Status;
	}

	CurrentDataOffset = 0;
	Percent = 0;

	Cache = (PBTR_RECORD_HEADER)BspMalloc(4096 * 64);

	for (i = 0; i < RecordCount; i++) {

		//
		// Read filtered index
		//

		ReadFile(IndexHandle, &Index, sizeof(DTL_INDEX), &CompleteLength, NULL);

		//
		// Position file pointer to indexed record and read out 
		//

		Low = (LONG)Index.Offset;
		High = (LONG)(Index.Offset >> 32);
		Status = SetFilePointer(DataHandle, Low, &High, FILE_BEGIN);
		ReadFile(DataHandle, Cache, Index.Length, &CompleteLength, NULL);

		//
		// Write to new target file
		//

		WriteFile(FileHandle, Cache, Index.Length, &CompleteLength, NULL);

		//
		// Write adjusted index file
		//

		Index.Offset = CurrentDataOffset;
		WriteFile(AdjustedIndexHandle, &Index, sizeof(DTL_INDEX), &CompleteLength, NULL);
		
		CurrentDataOffset += Index.Length;

		//
		// Post progress notification
		//

		if ((ULONG)((i * 100) / RecordCount) > Percent) {

			Percent = (ULONG)((i * 100) / RecordCount);
	        StringCchPrintf(Text, MAX_PATH - 1, 
	                        L"Saved %%%u of total %I64u data records ...", 
	                        Percent, RecordCount);

	        Cancel = ProgressDialogCallback(hWnd, Percent, Text, FALSE, FALSE); 

			if (Cancel) {

				CloseHandle(FileHandle);
				DeleteFile(Path);

				CloseHandle(DataHandle);
				CloseHandle(IndexHandle);

				CloseHandle(AdjustedIndexHandle);
				DeleteFile(AdjustedIndexPath);

				BspFree(Cache);
				return S_FALSE;
			}
		}
	}

	BspFree(Cache);

	//
	// Seek to positon of alignment of DataObject->DataLength, we need
	// align the index part with DTL_ALIGNMENT_UNIT to map it from the
	// alignment boundary.
	//

	GetFileSizeEx(FileHandle, &FileSize);
	FileSize.QuadPart = DtlRoundUpLength(FileSize.QuadPart, DTL_ALIGNMENT_UNIT);
	SetFilePointerEx(FileHandle, FileSize, NULL, FILE_BEGIN);
	
	Header.RecordLocation = 0; 
	Header.RecordLength = FileSize.QuadPart;
	Header.IndexLocation = FileSize.QuadPart;
	Header.IndexNumber = RecordCount; 
	Header.IndexLength = RecordCount * sizeof(DTL_INDEX);

	//
	// Append index file into dtl data file, copy unit is currently 4MB
	//

    Round = (ULONG)(Header.IndexLength / DTL_COPY_UNIT);
    Remainder = Header.IndexLength % DTL_COPY_UNIT;

	Status = TRUE;
	Buffer = VirtualAlloc(NULL, DTL_COPY_UNIT, MEM_COMMIT, PAGE_READWRITE);

	//
	// Rewind adjusted index file pointer to its start position 
	//

	SetFilePointer(AdjustedIndexHandle, 0, NULL, FILE_BEGIN);

	for (Number = 0; Number < Round; Number += 1) {
		Status = ReadFile(AdjustedIndexHandle, Buffer, DTL_COPY_UNIT, &CompleteLength, NULL);
		if (Status != TRUE) {
			break;
		}
		Status = WriteFile(FileHandle, Buffer, DTL_COPY_UNIT, &CompleteLength, NULL);
		if (Status != TRUE) {
			break;
		}
	}
	
	if (Status != TRUE) {
		goto FSERROR;
	}

	if (Remainder != 0) {

		Status = ReadFile(AdjustedIndexHandle, Buffer, (ULONG)Remainder, &CompleteLength, NULL);
		if (Status != TRUE) {
			goto FSERROR;
		}

		Status = WriteFile(FileHandle, Buffer, (ULONG)Remainder, &CompleteLength, NULL);
		if (Status != TRUE) {
			goto FSERROR;
		}
	}

FSERROR:
	if (Status != TRUE) {

		Status = GetLastError();

		CloseHandle(FileHandle);
		DeleteFile(Path);

	    CloseHandle(IndexHandle);
		CloseHandle(DataHandle);

	    CloseHandle(AdjustedIndexHandle);
		DeleteFile(AdjustedIndexPath);

		VirtualFree(Buffer, 0, MEM_RELEASE);
		ProgressDialogCallback(hWnd, 100, L"Problem occurred when writing file ...", TRUE, TRUE);
		return Status;

	} else {

		CloseHandle(DataHandle);
	    CloseHandle(IndexHandle);
	    CloseHandle(AdjustedIndexHandle);
		DeleteFile(AdjustedIndexPath);

		VirtualFree(Buffer, 0, MEM_RELEASE);
	}

	//
	// Append stack trace database
	//
	
	Header.StackLocation = Header.IndexLocation + Header.IndexLength;
	GetFileSizeEx(FileHandle, &FileSize);
	ASSERT((ULONG64)FileSize.QuadPart == Header.StackLocation);

	PdbWriteStackTraceToDtl(Object->PdbObject, FileHandle);
	
	Low = GetFileSize(FileHandle, &High);
	Header.ProbeLocation = (ULONG64)(((ULONG64)High << 32) | (ULONG64)Low);
	Header.StackLength = Header.ProbeLocation - Header.StackLocation;
	
	//
	// Append probe groups
	//

	DtlWriteManualGroup(Object, FileHandle);
	DtlWriteAddOnGroup(Object, FileHandle);

	Low = GetFileSize(FileHandle, &High);
	Header.SummaryLocation = (ULONG64)(((ULONG64)High << 32) | (ULONG64)Low);
	Header.ProbeLength = Header.SummaryLocation - Header.ProbeLocation;
	
	//
	// Append summary report
	//

	DtlWriteSummaryReport(Object, FileHandle);
	Low = GetFileSize(FileHandle, &High);
	Length = (ULONG64)(((ULONG64)High << 32) | (ULONG64)Low);
	Header.SummaryLength = Length - Header.SummaryLocation;

	//
	// Fill dtl header fields
	//

	Header.TotalLength = Length + sizeof(DTL_FILE_HEADER);
	Header.Signature = DTL_FILE_SIGNATURE;
	Header.MajorVersion = DtlMajorVersion;
	Header.MinorVersion = DtlMinorVersion;
	Header.FeatureFlag = 0;
	Header.StartTime = Object->StartTime;
	Header.EndTime = Object->EndTime;
	Header.Reserved[0] = 0;
	Header.Reserved[1] = 0;
	Header.CheckSum = DtlComputeCheckSum(&Header);

	//
	// N.B. We put the file 'header' in fact at the end of the file, this
	// obtain benefit that we don't need to adjust the record offset when
	// we read the record in report mode, meanwhile, it's easy to append
	// new segment to the whole dtl file.
	//

	WriteFile(FileHandle, &Header, sizeof(Header), &CompleteLength, NULL);
	CloseHandle(FileHandle);
	
    ProgressDialogCallback(hWnd, 100, L"Successfully saved file ...", FALSE, TRUE); 
	return S_OK;
}

ULONG
DtlComputeCheckSum(
	IN PDTL_FILE_HEADER Header
	)
{
	ULONG ReservedOffset;
	ULONG MajorVersionOffset;	
	PULONG Buffer;
	ULONG Result;
	ULONG Seed;
	ULONG Number;

	//
	// N.B. If any new members are appended in DTL_FILE_HEADER
	// structure, we should consider adapt it into checksum computing,
	// this routine ensure that the file header was not damaged to avoid
	// to read into bad data, it include MajorVersion exclude Reserved.
	//

	ReservedOffset = FIELD_OFFSET(DTL_FILE_HEADER, Reserved);
	MajorVersionOffset = FIELD_OFFSET(DTL_FILE_HEADER, MajorVersion);

	ASSERT((ReservedOffset - MajorVersionOffset) % sizeof(ULONG) == 0);

	Seed = 'ltd';
	Result = 0;

	Buffer = (PULONG)&Header->MajorVersion;
	for(Number = 0; Number < (ReservedOffset - MajorVersionOffset) / sizeof(ULONG); Number += 1) {
		Result += Seed ^ Buffer[Number];
	}

	DebugTrace("DtlComputeCheckSum  0x%08x", Result);
	return Result;
}
	
ULONG
DtlWriteSummaryReport(
	IN PDTL_OBJECT Object, 
	IN HANDLE FileHandle
	)
{
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY CounterListEntry;
	PDTL_COUNTER_OBJECT Counter;
	PDTL_PROBE_COUNTER CounterEntry;
	PDTL_PROBE_COUNTER Buffer;
	PDTL_COUNTER_SET Set;
	ULONG CompleteBytes;
	ULONG i, j;
	
	//
	// N.B. We need use FilterCounter to store only filtered 
	// summary report if current dtl object has a filter attached.
	//

	if (Object->ConditionObject != NULL) {
		Counter = Object->FilterCounter;
	}
	else {
		Counter = Object->CounterObject;
	}

	//
	// N.B. Counter objects can never be NULL
	//

	ASSERT(Counter != NULL);

	if (IsListEmpty(&Counter->CounterSetList)) {
		return S_OK;
	}

	WriteFile(FileHandle, &Counter->NumberOfCounterSet, sizeof(ULONG), &CompleteBytes, NULL);

	ListEntry = Counter->CounterSetList.Flink;
	for(i = 0; i < Counter->NumberOfCounterSet; i += 1) {

		Set = CONTAINING_RECORD(ListEntry, DTL_COUNTER_SET, ListEntry);
		WriteFile(FileHandle, Set, sizeof(DTL_COUNTER_SET), &CompleteBytes, NULL);

		Buffer = (PDTL_PROBE_COUNTER)BspMalloc(sizeof(DTL_PROBE_COUNTER) * Set->NumberOfCounter);
		CounterListEntry = Set->CounterList.Flink;
		for (j = 0; j < Set->NumberOfCounter; j += 1) {
			CounterEntry = CONTAINING_RECORD(CounterListEntry, DTL_PROBE_COUNTER, ListEntry);
			RtlCopyMemory(Buffer + j, CounterEntry, sizeof(DTL_PROBE_COUNTER));
			CounterListEntry = CounterListEntry->Flink;
		}

		WriteFile(FileHandle, Buffer, sizeof(DTL_PROBE_COUNTER) * Set->NumberOfCounter, &CompleteBytes, NULL);
		BspFree(Buffer);

		ListEntry = ListEntry->Flink;
	}

	return S_OK;
}

ULONG
DtlLoadSummaryReport(
	IN PDTL_OBJECT Object,
	IN HANDLE FileHandle
	)
{
	PDTL_COUNTER_OBJECT Counter;
	ULONG CompleteBytes;
	ULONG i, j;
	PDTL_COUNTER_SET Set;
	PDTL_PROBE_COUNTER CounterEntry;
	ULONG64 TotalRecords = 0;

	Counter = (PDTL_COUNTER_OBJECT)BspMalloc(sizeof(DTL_COUNTER_OBJECT));
	Counter->DtlObject = Object;
	Counter->NumberOfCounterSet = 0;
	Counter->LastRecord = -1;
	InitializeListHead(&Counter->CounterSetList);
	Object->CounterObject = Counter;

	ReadFile(FileHandle, &Counter->NumberOfCounterSet, sizeof(ULONG), &CompleteBytes, NULL);

	for(i = 0; i < Counter->NumberOfCounterSet; i += 1) {

		Set = (PDTL_COUNTER_SET)BspMalloc(sizeof(DTL_COUNTER_SET));
		ReadFile(FileHandle, Set, sizeof(DTL_COUNTER_SET), &CompleteBytes, NULL);	

		TotalRecords += Set->RecordCount;
		InsertTailList(&Counter->CounterSetList, &Set->ListEntry);

		InitializeListHead(&Set->CounterList);

		for(j = 0; j < Set->NumberOfCounter; j += 1) {
			CounterEntry = (PDTL_PROBE_COUNTER)BspMalloc(sizeof(DTL_PROBE_COUNTER));
			ReadFile(FileHandle, CounterEntry, sizeof(DTL_PROBE_COUNTER), &CompleteBytes, NULL);	
			InsertTailList(&Set->CounterList, &CounterEntry->ListEntry);
		}
	}

	Counter->LastRecord = TotalRecords - 1;
	return S_OK;
}

ULONG
DtlGetDiskFreeSpace(
	IN PWSTR Path,
	OUT PULARGE_INTEGER FreeBytes
	)
{
	ULONG Status;
	WCHAR Drive[16];

	_wsplitpath(Path, Drive, NULL, NULL, NULL);

	Status = GetDiskFreeSpaceEx(Drive, FreeBytes, NULL, NULL);
	if (Status != TRUE) {
		Status = GetLastError();
		return Status;
	}

	return S_OK;
}

//
// N.B. The probe information is written to disk in the following layout:
// 1, count of probe group
// 2, probe group array  (each probe group records its probe object count)
// 3, probe object array (all groups' probe objects are in same array)
// 4, addon group array (all available addon full path name)
//

ULONG
DtlWriteManualProbe(
	IN PDTL_MANUAL_GROUP Group,
	IN HANDLE FileHandle
	)
{
	ULONG Complete;
	PLIST_ENTRY ListEntry;
	PDTL_PROBE Probe;

	ListEntry = Group->ProbeListHead.Flink;
	while (ListEntry != &Group->ProbeListHead) {
		Probe = CONTAINING_RECORD(ListEntry, DTL_PROBE, Entry);
		WriteFile(FileHandle, Probe, sizeof(DTL_PROBE), &Complete, NULL);
		ListEntry = ListEntry->Flink;
	}
	
	return S_OK;
}

ULONG 
DtlWriteManualGroup(
	IN PDTL_OBJECT Object,
	IN HANDLE FileHandle
	)
{
	ULONG Length;
	ULONG Complete;
	PDTL_MANUAL_GROUP Group;
	PLIST_ENTRY ListEntry;

	if (!Object->NumberOfManualGroup) {
		return S_OK;
	}

	WriteFile(FileHandle, &Object->NumberOfManualGroup, sizeof(ULONG), &Complete, NULL);
	
	ListEntry = Object->ManualGroupList.Flink;
	Length = FIELD_OFFSET(DTL_MANUAL_GROUP, ListEntry);	

	while (ListEntry != &Object->ManualGroupList) {
		Group = CONTAINING_RECORD(ListEntry, DTL_MANUAL_GROUP, ListEntry);
		WriteFile(FileHandle, Group, Length, &Complete, NULL);
		ListEntry = ListEntry->Flink;
	}
	
	ListEntry = Object->ManualGroupList.Flink;
	while (ListEntry != &Object->ManualGroupList) {
		Group = CONTAINING_RECORD(ListEntry, DTL_MANUAL_GROUP, ListEntry);
		DtlWriteManualProbe(Group, FileHandle);
		ListEntry = ListEntry->Flink;
	}

	return 0;	
}

PDTL_PROBE
DtlQueryProbeByAddress(
	IN PDTL_OBJECT Object,
	IN ULONG ProcessId,
	IN PVOID Address,
	IN ULONG TypeFlag 
	)
{
	PDTL_PROBE Probe;
	PDTL_MANUAL_GROUP Group;
	PLIST_ENTRY ListEntry;

	Group = DtlQueryManualGroup(Object, ProcessId);

	if (Group != NULL) {
		ListEntry = Group->ProbeListHead.Flink;
		while (ListEntry != &Group->ProbeListHead) {
			Probe = CONTAINING_RECORD(ListEntry, DTL_PROBE, Entry);
			if (Probe->Address == Address && Probe->Flag == TypeFlag) {
				return Probe;
			}
			ListEntry = ListEntry->Flink;
		}
	}

	return NULL;
}

PDTL_MANUAL_GROUP
DtlQueryManualGroup(
	IN PDTL_OBJECT Object,
	IN ULONG ProcessId
	)
{
	PLIST_ENTRY ListEntry;
	PDTL_MANUAL_GROUP Group;

	ListEntry = Object->ManualGroupList.Flink;

	while (ListEntry != &Object->ManualGroupList) {
		
		Group = CONTAINING_RECORD(ListEntry, DTL_MANUAL_GROUP, ListEntry);
		if (Group->ProcessId == ProcessId) {
			return Group;
		}

		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

PDTL_MANUAL_GROUP 
DtlCreateManualGroup(
	IN PDTL_OBJECT Object,
	IN PPSP_PROCESS Process
	)
{
	PDTL_MANUAL_GROUP Group;

	ASSERT(Process->ProcessId != 0);

	Group = DtlQueryManualGroup(Object, Process->ProcessId);
	if (Group != NULL) {
		return Group;
	}

	Group = (PDTL_MANUAL_GROUP)BspMalloc(sizeof(DTL_MANUAL_GROUP));
	Group->ProcessId = Process->ProcessId;
	Group->NumberOfProbes = 0;

	StringCchCopy(Group->ProcessName, MAX_PATH, Process->Name);
	StringCchCopy(Group->FullPathName, MAX_PATH, Process->FullPathName);
	
	InitializeListHead(&Group->ProbeListHead);
	InsertHeadList(&Object->ManualGroupList, &Group->ListEntry);
	Object->NumberOfManualGroup += 1;

	//
	// Increase shared data process counter 
	//

	_InterlockedIncrement(&DtlSharedData.NumberOfProcesses);
	return Group;
}

ULONG
DtlCommitProbe(
	IN PDTL_OBJECT Object,
	IN PDTL_MANUAL_GROUP Group,
	IN PPSP_PROBE Probe 
	)
{
	PDTL_PROBE DtlProbe;

	if (Probe->Result != ProbeCommit) {
		DebugBreak();
	}

	DtlProbe = DtlCreateProbe(Probe);
	InsertTailList(&Group->ProbeListHead, &DtlProbe->Entry);
		
	Group->NumberOfProbes += 1;	

	//
	// Increase shared data probe counter 
	//

	_InterlockedIncrement(&DtlSharedData.NumberOfProbes);
	return S_OK;
}

ULONG
DtlInsertVariantProbe(
	IN PDTL_PROBE RootObject,
	IN PDTL_PROBE VariantObject 
	)
{
	return 0;
}
	
PDTL_PROBE
DtlCreateProbe(
	IN PPSP_PROBE Probe
	)
{
	PDTL_PROBE DtlProbe;

	DtlProbe = (PDTL_PROBE)BspMalloc(sizeof(DTL_PROBE));

	DtlProbe->Address = (PVOID)Probe->Address;
	DtlProbe->Flag = PROBE_EXPRESS;
	StringCchCopy(DtlProbe->ModuleName, 64, Probe->ModuleName);
	StringCchCopy(DtlProbe->ProcedureName, 64, Probe->MethodName);

	return DtlProbe;
}

PDTL_PROBE
DtlQueryManualProbe(
	IN PDTL_OBJECT Object,
	IN ULONG ProcessId,
	IN PVOID Address
	)
{
	PDTL_PROBE Probe;

	Probe = DtlQueryProbeByAddress(Object, ProcessId, Address, PROBE_EXPRESS);
	return Probe;
}

ULONG
DtlLookupProcessName(
	IN PDTL_OBJECT Object,
	IN ULONG ProcessId,
	IN PWCHAR Buffer,
	IN ULONG BufferLength
	)
{
	PDTL_MANUAL_GROUP Group;
	PDTL_ADDON_GROUP AddOnGroup;

	Group = DtlQueryManualGroup(Object, ProcessId);
	
	if (Group != NULL) {
		StringCchCopy(Buffer, BufferLength, Group->ProcessName);
		return S_OK; 
	}

	AddOnGroup = DtlLookupAddOnGroup(Object, ProcessId);
	if (AddOnGroup != NULL) {
		StringCchCopy(Buffer, BufferLength, AddOnGroup->ProcessName);
		return S_OK;
	}

	StringCchCopy(Buffer, BufferLength, L"N/A");
	return S_OK;
}

ULONG
DtlLookupProbeByAddress(
	IN PDTL_OBJECT Object,
	IN ULONG ProcessId,
	IN PVOID Address,
	IN PWCHAR Buffer, 
	IN ULONG BufferLength
	)
{
	PDTL_PROBE Probe;
	
	Probe = DtlQueryProbeByAddress(Object, ProcessId, Address, PROBE_EXPRESS);
	if (Probe != NULL) {
		StringCchPrintf(Buffer, BufferLength, L"%ws", Probe->ProcedureName);
		return S_OK;
	}

	Probe = DtlQueryProbeByAddress(Object, ProcessId, Address, PROBE_FILTER);
	if (Probe != NULL) {
		StringCchPrintf(Buffer, BufferLength, L"%ws", Probe->ProcedureName);
		return S_OK;
	}

	StringCchPrintf(Buffer, BufferLength, L"0x%p", Address);
	return S_OK;
}

PDTL_ADDON_GROUP
DtlLookupAddOnGroup(
	IN PDTL_OBJECT Object,
	IN ULONG ProcessId
	)
{
	PLIST_ENTRY ListEntry;
	PDTL_ADDON_GROUP Group;

	ListEntry = Object->AddOnGroupList.Flink;
	
	while (ListEntry != &Object->AddOnGroupList) {
		Group = CONTAINING_RECORD(ListEntry, DTL_ADDON_GROUP, ListEntry);
		if (Group->ProcessId == ProcessId) {
			return Group;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

ULONG
DtlWriteAddOnGroup(
	IN PDTL_OBJECT Object,
	IN HANDLE FileHandle
	)
{
	PCHAR Buffer;
	ULONG Number;
	ULONG Complete;
	PLIST_ENTRY ListEntry;
	PDTL_ADDON_GROUP AddOn;

	ListEntry = Object->AddOnGroupList.Flink;

	Buffer = BspMalloc(sizeof(ULONG) + sizeof(DTL_ADDON_GROUP) * Object->NumberOfAddOnGroup);
	*(PULONG)Buffer = Object->NumberOfAddOnGroup;

	Number = 0;
	while (ListEntry != &Object->AddOnGroupList) {
		
		AddOn = CONTAINING_RECORD(ListEntry, DTL_ADDON_GROUP, ListEntry);
		RtlCopyMemory(Buffer + sizeof(ULONG) + Number * sizeof(DTL_ADDON_GROUP), 
			          AddOn, sizeof(DTL_ADDON_GROUP));

		ListEntry = ListEntry->Flink;
		Number += 1;
	}

	WriteFile(FileHandle, Buffer, sizeof(ULONG) + Number * sizeof(DTL_ADDON_GROUP), &Complete, NULL);
	BspFree(Buffer);

	return 0;
}

ULONG
DtlLoadAddOnGroup(
	IN PDTL_OBJECT Object,
	IN HANDLE FileHandle
	)
{
	ULONG Status;
	ULONG Number;
	ULONG Complete;
	PDTL_ADDON_GROUP AddOn;

	InitializeListHead(&Object->AddOnGroupList);
	Status = ReadFile(FileHandle, &Object->NumberOfAddOnGroup, sizeof(ULONG), &Complete, NULL);
	if (Status != TRUE) {
		Status = GetLastError();
		DebugTrace("DtlLoadAddOnGroup %d", Status);
		return Status;
	}

	for (Number = 0; Number < Object->NumberOfAddOnGroup; Number += 1) {

		AddOn = (PDTL_ADDON_GROUP)BspMalloc(sizeof(DTL_ADDON_GROUP));

		Status = ReadFile(FileHandle, AddOn, sizeof(DTL_ADDON_GROUP), &Complete, NULL);
		if (Status != TRUE) {
			Status = GetLastError();
			DebugTrace("DtlLoadAddOnGroup %d #%d", Status, Number);
			return Status;
		}

		InsertTailList(&Object->AddOnGroupList, &AddOn->ListEntry);
	}

	return S_OK;
}

ULONG
DtlCommitAddOnGroup(
	IN PDTL_OBJECT Object,
	IN PPSP_COMMAND Command
	)
{
	PLIST_ENTRY ListEntry;
	PDTL_ADDON_GROUP Group;
	PPSP_PROCESS Process;

	Process = Command->Process;
	ListEntry = Object->AddOnGroupList.Flink;
	
	while (ListEntry != &Object->AddOnGroupList) {
		Group = CONTAINING_RECORD(ListEntry, DTL_ADDON_GROUP, ListEntry);
		if (Group->ProcessId == Process->ProcessId && 
			!wcsicmp(Group->FullPathName, Process->FullPathName)) {
			return S_OK;
		}
		ListEntry = ListEntry->Flink;
	}

	Group = (PDTL_ADDON_GROUP)BspMalloc(sizeof(DTL_ADDON_GROUP));

	Group->ProcessId = Process->ProcessId;
	StringCchCopy(Group->ProcessName,  MAX_PATH, Process->Name);
	StringCchCopy(Group->FullPathName, MAX_PATH, Process->FullPathName);

	InsertTailList(&Object->AddOnGroupList, &Group->ListEntry);
	Object->NumberOfAddOnGroup += 1;

	return S_OK;
}

ULONG
DtlDecodeAddOnParameters(
	IN PBTR_FILTER_RECORD Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	BTR_DECODE_CALLBACK Decode;
	PBTR_FILTER AddOn;

	ASSERT(Record != NULL);
	
	AddOn = PspLookupAddOnByTag(Record);

	if (AddOn != NULL) {
		Decode = AddOn->Probes[Record->ProbeOrdinal].DecodeCallback;
		if (Decode != NULL) {
			__try {
				(*Decode)(Record, Length, Buffer);
			}
			__except(EXCEPTION_EXECUTE_HANDLER) {
				StringCchCopy(Buffer, Length, L"Decode Exception");
			}
		}
		else {
			StringCchCopy(Buffer, Length, L"No Decode Procedure");
		}
	} else {
		StringCchCopy(Buffer, Length, L"No AddOn Object");
	}

	return S_OK;
}

ULONG
DtlDecodeParameters(
	IN PBTR_EXPRESS_RECORD Record,
	IN PDTL_PROBE Probe,
	IN PWCHAR Buffer,
	IN ULONG Length,
	OUT PULONG ActualLength
	)
{
#if defined (_M_IX86)

	StringCchPrintfW(Buffer, Length, L"reg: ecx(0x%08x) edx(0x%08x) stack: (0x%08x 0x%08x 0x%08x 0x%08x)",
					Record->Argument.Ecx, Record->Argument.Edx, Record->Argument.Stack[0],
					Record->Argument.Stack[1], Record->Argument.Stack[2], Record->Argument.Stack[3]);
	
#elif defined (_M_X64)
	StringCchPrintfW(Buffer, Length, L"reg: rcx(0x%I64x) rdx(0x%I64x) xmm0: %f xmm1: %f xmm2: %f xmm3: %f"
					L"stack: 0x%I64x 0x%I64x",
					Record->Argument.Rcx, Record->Argument.Rdx, 
					Record->Argument.Xmm0.m128d_f64[0], 
					Record->Argument.Xmm1.m128d_f64[0],
					Record->Argument.Xmm2.m128d_f64[0], 
					Record->Argument.Xmm3.m128d_f64[0], 
					Record->Argument.Stack[0], 
					Record->Argument.Stack[1]);
#endif

	return S_OK;
}

PDTL_PROBE
DtlQueryProbeByName(
	IN PDTL_OBJECT Object,
	IN ULONG ProcessId,
	IN PWSTR ModuleName,
	IN PWSTR ProcedureName,
	IN ULONG TypeFlag
	)
{
	PLIST_ENTRY Entry;
	PDTL_PROBE Probe;
	PDTL_MANUAL_GROUP Group;

	Group = DtlQueryManualGroup(Object, ProcessId);

	if (Group != NULL) {
	
		Entry = Group->ProbeListHead.Flink;
		while (Entry != &Group->ProbeListHead) {
			Probe = CONTAINING_RECORD(Entry, DTL_PROBE, Entry);
			if (wcscmp(ProcedureName, Probe->ProcedureName) == 0 &&
				wcscmp(ModuleName, Probe->ModuleName) == 0) {
				return Probe;
			}
			Entry = Entry->Flink;
		}
		
	}

	return NULL;
}

PDTL_PROBE
DtlQueryProbeByRecord(
	IN PDTL_OBJECT Object,
	IN PBTR_RECORD_HEADER Record
	)
{
	PDTL_PROBE Probe;

	Probe = DtlQueryProbeByAddress(Object,
								   Record->ProcessId,  
		                           (PVOID)Record->Address, 
								   PROBE_EXPRESS);
	return Probe;
}

ULONG
DtlDecodeReturn(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	switch (Record->ReturnType) {

		case ReturnVoid:
			StringCchCopy(Buffer, Length, L"N/A");
			break;

		case ReturnInt8:
		case ReturnUInt8:
			StringCchPrintf(Buffer, Length, L"0x%02x", (UCHAR)Record->Return);
			break;

		case ReturnUInt16:
		case ReturnInt16:
			StringCchPrintf(Buffer, Length, L"0x%04x", (USHORT)Record->Return);
			break;

		case ReturnUInt32:
		case ReturnInt32:

#if defined(_M_IX86)
		case ReturnPtr:
		case ReturnIntPtr:
		case ReturnUIntPtr:
#endif
			StringCchPrintf(Buffer, Length, L"0x%08x", (ULONG)Record->Return);
			break;

		case ReturnUInt64:
		case ReturnInt64:

#if defined(_M_X64)
		case ReturnPtr:
		case ReturnIntPtr:
		case ReturnUIntPtr:
#endif
			StringCchPrintf(Buffer, Length, L"0x%I64x", (ULONG64)Record->Return);
			break;

			//
			// N.B. x86 and x64 deal with floating point data differently.
			// for x86 floating point data is extended automatically by FPU,
			// so it's in fact 8 bytes for both float/double in memory or FPU registers.
			// for x64, CPU treat float as 4 bytes, double as 8 bytes. To correctly decode
			// float point data, we need explicitly dereference appropriate data size.
			//

#if defined(_M_IX86)

		case ReturnFloat:
		case ReturnDouble:
			StringCchPrintf(Buffer, Length, L"%f", *(double *)&Record->Return);
			break;

#elif defined(_M_X64)

		case ReturnFloat:
			StringCchPrintf(Buffer, Length, L"%f", *(float *)&Record->Return);
			break;

		case ReturnDouble:
			StringCchPrintf(Buffer, Length, L"%f", *(double *)&Record->Return);
			break;
#endif

		default:

			//
			// N.B. If there's no ReturnType specified, we just return its pointer sized value.
			//

			StringCchPrintf(Buffer, Length, L"0x%p", (ULONG_PTR)Record->Return);
			break;

	}

	return 0;
}

ULONG
DtlLookupAddOnProbeByOrdinal(
	IN PDTL_OBJECT Object,
	IN PBTR_FILTER_RECORD Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	ULONG Status;
	PBTR_FILTER AddOn;

	ASSERT(Record != NULL);

	Status = S_FALSE;
	AddOn = PspLookupAddOnByTag(Record);

	if (AddOn != NULL) {
		wcsncpy(Buffer, AddOn->Probes[Record->ProbeOrdinal].ApiName, Length);
		Status = S_OK;
	} else {
		StringCchCopy(Buffer, Length, L"No AddOn Object");
	}
	
	return Status;
}

ULONG
DtlDecodeCaller(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	return 0;
}

ULONG
DtlLookupAddOnName(
	IN PDTL_OBJECT Object,
	IN PBTR_FILTER_RECORD Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	ULONG Status;
	PBTR_FILTER AddOn;

	ASSERT(Record != NULL);

	AddOn = PspLookupAddOnByTag(Record);

	if (AddOn != NULL) {
		StringCchCopy(Buffer, Length, AddOn->FilterName);
		Status = S_OK;
	} else {
		StringCchCopy(Buffer, Length, L"Unknown AddOn");
		Status = S_FALSE;
	}
	
	return Status;
}

BOOLEAN
DtlIsCachedFile(
	IN PDTL_OBJECT Object,
	IN PWSTR Path
	)
{
	PDTL_FILE DataObject;

	DataObject = &Object->DataObject;

	if (_wcsicmp(DataObject->Path, Path)) {
		return FALSE;
	}

	return TRUE;
}

ULONG CALLBACK
DtlQueryFilterCallback(
	IN HWND hWnd,
	IN PVOID Context
	)
{
    ULONG Status;
    PDTL_FILTER_CONTEXT FilterContext;

    FilterContext = (PDTL_FILTER_CONTEXT)Context;
    Status = DtlApplyQueryFilter(FilterContext->DtlObject, 
                                 FilterContext->FilterObject, 
                                 hWnd);
    ProgressDialogCallback(hWnd, 100, L"Finished filter captured records.", FALSE, TRUE);
    EndDialog(hWnd, IDCANCEL);

    return Status;
}

ULONG CALLBACK
DtlWriteDtlCallback(
	IN HWND hWnd,
	IN PVOID Context
	)
{
	ULONG Status;
	BOOL Cancel = FALSE;
	PDTL_OBJECT Object;
	WCHAR Path[MAX_PATH];
	DTL_INFORMATION Information;
	ULONG64 RecordCount;
	PDTL_SAVE_CONTEXT SaveContext;

	SaveContext = (PDTL_SAVE_CONTEXT)Context;
	Object = SaveContext->Object;

	StringCchCopy(Path, MAX_PATH, ((PDTL_SAVE_CONTEXT)Context)->Path);

	DtlQueryInformation(Object, &Information);
	RecordCount = Information.NumberOfRecords;

	if (!SaveContext->Filtered) {
		Status = DtlWriteDtl(Object, Path, hWnd);
	} else {
		Status = DtlWriteDtlFiltered(Object, Path, hWnd);
	}

    if (Status != S_OK) {
        DebugTrace("DtlWriteDtl %d", Status);
    }

	EndDialog(hWnd, IDCANCEL);
	return Status;
}

ULONG CALLBACK
DtlWriteCsvCallback(
	IN HWND hWnd,
	IN PVOID Context
	)
{
	PDTL_OBJECT Object;
	ULONG64 Number;
	ULONG Status;
	PVOID Cache;
	ULONG Complete;
	ULONG64 Divider;
	ULONG64 Remainder;
	ULONG64 RecordNumber;
	WCHAR Path[MAX_PATH];
	ULONG64 RecordCount;
	DTL_INFORMATION Information;
	HANDLE DataHandle;
	HANDLE IndexHandle;
	HANDLE CsvHandle;
	BOOL Cancel;
    WCHAR Buffer[MAX_PATH];
	BOOLEAN FilteredOperation;

	Cancel = FALSE;
	Object = ((PDTL_SAVE_CONTEXT)Context)->Object;
	FilteredOperation = ((PDTL_SAVE_CONTEXT)Context)->Filtered;

	StringCchCopy(Path, MAX_PATH, ((PDTL_SAVE_CONTEXT)Context)->Path);

	if (FilteredOperation) {
		RecordCount = Object->QueryIndexObject->RecordCount;
	} else {
		DtlQueryInformation(Object, &Information);
		RecordCount = Information.NumberOfRecords;
	}

	CsvHandle = CreateFile(Path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
	                       FILE_ATTRIBUTE_NORMAL, NULL);

	if (CsvHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		return Status;
	}

	DataHandle = CreateFile(Object->DataObject.Path, GENERIC_READ | GENERIC_WRITE, 
		                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if (!FilteredOperation) {
		IndexHandle = CreateFile(Object->IndexObject.Path, GENERIC_READ | GENERIC_WRITE, 
			                     FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	} else {

		//
		// N.B. Use query index file 
		//

		ASSERT(Object->ConditionObject != NULL);
		ASSERT(Object->QueryIndexObject != NULL);
		IndexHandle = CreateFile(Object->QueryIndexObject->Path, GENERIC_READ | GENERIC_WRITE, 
			                     FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	}

	if (DataHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		return Status;
	}

	WriteFile(CsvHandle, DtlCsvRecordColumn, (ULONG)strlen(DtlCsvRecordColumn), &Complete, NULL);

	//
	// N.B. Presume here page size is 4K, we allocate 64K index cache which hold
	// 4K index
	//

	Cache = BspAllocateGuardedChunk(16);
	Divider = DtlRoundDownLength(RecordCount, 4096) / 4096;
	Remainder = RecordCount % 4096;

	for (Number = 0; Number < Divider; Number += 1) { 

		ReadFile(IndexHandle, Cache, sizeof(DTL_INDEX) * 4096, &Complete, NULL);

		RecordNumber = Number * 4096;

		if (!FilteredOperation) {
			Status = DtlWriteCsvRound(hWnd, RecordNumber, RecordCount, 
				                      Object, Cache, 4096, DataHandle, CsvHandle);
		} else {
			Status = DtlWriteCsvRoundFiltered(hWnd, RecordNumber, RecordCount, 
						                      Object, Cache, 4096, DataHandle, CsvHandle);
		}

		if (Status != S_OK) {
			break;
		}
	}

	if (Remainder != 0) {
		ReadFile(IndexHandle, Cache, (ULONG)(sizeof(DTL_INDEX) * Remainder), &Complete, NULL);
		if (!FilteredOperation) {
			Status = DtlWriteCsvRound(hWnd, RecordNumber + 4096, RecordCount, 
				                      Object, Cache, (ULONG)Remainder, DataHandle, CsvHandle);
		} else {
			Status = DtlWriteCsvRoundFiltered(hWnd, RecordNumber + 4096, RecordCount, 
						                      Object, Cache, (ULONG)Remainder, DataHandle, CsvHandle);
		}
	}

	CloseHandle(DataHandle);
	CloseHandle(IndexHandle);
	CloseHandle(CsvHandle);

	if (Status != S_OK && (Object->DataObject.Mode != DTL_FILE_OPEN)) {
		DeleteFile(Path);
	} else {
        StringCchPrintf(Buffer, MAX_PATH - 1, 
                        L"Saved total %I64d records, total size is %I64d KB.",
                        RecordCount, Information.DataFileLength / 1024);
		ProgressDialogCallback(hWnd, 100, Buffer, FALSE, TRUE);
	}

	BspFreeGuardedChunk(Cache);
	return S_OK;
}

ULONG
DtlWriteCsvRound(
	IN HWND hWnd,
	IN ULONG64 RecordNumber,
	IN ULONG64 TotalNumber,
	IN PDTL_OBJECT Object,
	IN PDTL_INDEX IndexCache,
	IN ULONG NumberOfIndex,
	IN HANDLE DataHandle,
	IN HANDLE CsvHandle
	)
{
	BOOL Cancel;
	ULONG Status;
	ULONG CompleteLength;
	PBTR_RECORD_HEADER Record;
	PBTR_FILTER_RECORD Filter;
	ULONG Number;
	ULONG Count;
	SYSTEMTIME Time;
	PDTL_PROBE Probe;
	double Duration;
	WCHAR Name[MAX_PATH];
	WCHAR Process[MAX_PATH];
	WCHAR Detail[MAX_PATH];
	CHAR Cache[4096];
	CHAR Buffer[4096 * 2];
	ULONG64 StepLength;
    ULONG Percent;

	StepLength = TotalNumber / 100;

	for(Number = 0; Number < NumberOfIndex; Number += 1) {

		ASSERT(IndexCache[Number].Length < 4096);
		ReadFile(DataHandle, Cache, IndexCache[Number].Length, &CompleteLength, NULL);

		Record = (PBTR_RECORD_HEADER)Cache;
		if (Record->RecordType == RECORD_EXPRESS) {
			Probe = DtlQueryProbeByRecord(Object, Record);
			if (!Probe) {
				continue;
			}
		} else {
			Filter = (PBTR_FILTER_RECORD)Record;
		}

		DtlLookupProcessName(Object, Record->ProcessId, Process, MAX_PATH);

		if (Record->RecordType == RECORD_EXPRESS) {

			DtlLookupProbeByAddress(Object, 
				                    Record->ProcessId, 
									(PVOID)Record->Address, 
									Name, MAX_PATH - 1);
		} else {

			DtlLookupAddOnProbeByOrdinal(Object, 
				                        (PBTR_FILTER_RECORD)Record, 
										Name, MAX_PATH - 1);
		}

		if (Record->RecordType == RECORD_EXPRESS) {
			DtlDecodeParameters((PBTR_EXPRESS_RECORD)Record, Probe, Detail, MAX_PATH - 1, &CompleteLength);
		}
		else {
			DtlDecodeAddOnParameters(Filter, Detail, MAX_PATH - 1);
		}

		FileTimeToSystemTime(&Record->FileTime, &Time);
		Duration = BspComputeMilliseconds(Record->Duration);

		Count = _snprintf_s(Buffer, BspPageSize - 1, _TRUNCATE, DtlCsvRecordFormat, 
			Process, Time.wHour, Time.wMinute, Time.wSecond, Time.wMilliseconds,
			Record->ProcessId, Record->ThreadId, Name, Duration, Record->Return, Detail);

		Status = WriteFile(CsvHandle, Buffer, Count, &CompleteLength, NULL); 
		if (Status != TRUE) {
			Status = GetLastError();
			return Status;
		}

        StringCchPrintf((PWCHAR)Buffer, MAX_PATH - 1, L"Saved %I64d of total %I64d records ...", 
                        RecordNumber + Number, TotalNumber);

        Percent = (ULONG)((RecordNumber + Number) / (TotalNumber / 100));
		Cancel = ProgressDialogCallback(hWnd, Percent, (PWCHAR)Buffer, FALSE, FALSE);
		if (Cancel) {
			EndDialog(hWnd, IDCANCEL);
			return S_FALSE;
		}
	}

	return S_OK;
}

ULONG
DtlWriteCsvRoundFiltered(
	IN HWND hWnd,
	IN ULONG64 RecordNumber,
	IN ULONG64 TotalNumber,
	IN PDTL_OBJECT Object,
	IN PDTL_INDEX IndexCache,
	IN ULONG NumberOfIndex,
	IN HANDLE DataHandle,
	IN HANDLE CsvHandle
	)
{
	BOOL Cancel;
	ULONG Status;
	ULONG CompleteLength;
	PBTR_RECORD_HEADER Record;
	PBTR_FILTER_RECORD Filter;
	ULONG Number;
	ULONG Count;
	SYSTEMTIME Time;
	PDTL_PROBE Probe;
	double Duration;
	WCHAR Name[MAX_PATH];
	WCHAR Process[MAX_PATH];
	WCHAR Detail[MAX_PATH];
	CHAR Cache[4096 * 64];
	CHAR Buffer[4096];
	ULONG64 StepLength;
    ULONG Percent;
	LONG High;

	StepLength = TotalNumber / 100;

	for(Number = 0; Number < NumberOfIndex; Number += 1) {

		ASSERT(IndexCache[Number].Length < 4096 * 64);
		
		//
		// N.B. For filtered saveing, because the record are not continous,
		/// we need explicitly adjust file pointer to appropriate position
		//

		High = (LONG)(IndexCache[Number].Offset >> 32);
		SetFilePointer(DataHandle, (LONG)IndexCache[Number].Offset, &High, FILE_BEGIN);

		ReadFile(DataHandle, Cache, IndexCache[Number].Length, &CompleteLength, NULL);

		Record = (PBTR_RECORD_HEADER)Cache;
		if (Record->RecordType == RECORD_EXPRESS) {
			Probe = DtlQueryProbeByRecord(Object, Record);
			if (!Probe) {
				continue;
			}
		} else {
			Filter = (PBTR_FILTER_RECORD)Record;
		}

		DtlLookupProcessName(Object, Record->ProcessId, Process, MAX_PATH);

		if (Record->RecordType == RECORD_EXPRESS) {

			DtlLookupProbeByAddress(Object, 
				                    Record->ProcessId, 
									(PVOID)Record->Address, 
									Name, MAX_PATH - 1);
		} else {

			DtlLookupAddOnProbeByOrdinal(Object, 
				                        (PBTR_FILTER_RECORD)Record, 
										Name, MAX_PATH - 1);
		}

		if (Record->RecordType == RECORD_EXPRESS) {
			DtlDecodeParameters((PBTR_EXPRESS_RECORD)Record, Probe, Detail, MAX_PATH - 1, &CompleteLength);
		}
		else {
			DtlDecodeAddOnParameters(Filter, Detail, MAX_PATH - 1);
		}

		FileTimeToSystemTime(&Record->FileTime, &Time);
		Duration = BspComputeMilliseconds(Record->Duration);
		
		Count = _snprintf_s(Buffer, BspPageSize - 1, _TRUNCATE, DtlCsvRecordFormat, 
			Process, Time.wHour, Time.wMinute, Time.wSecond, Time.wMilliseconds,
			Record->ProcessId, Record->ThreadId, Name, Duration, Record->Return, Detail);

		Status = WriteFile(CsvHandle, Buffer, Count, &CompleteLength, NULL); 
		if (Status != TRUE) {
			Status = GetLastError();
			return Status;
		}

        StringCchPrintf((PWCHAR)Buffer, MAX_PATH - 1, L"Saved %I64d of total %I64d records ...", 
                        RecordNumber + Number, TotalNumber);

        Percent = (ULONG)((RecordNumber + Number) / (TotalNumber / 100));
		Cancel = ProgressDialogCallback(hWnd, Percent, (PWCHAR)Buffer, FALSE, FALSE);
		if (Cancel) {
			EndDialog(hWnd, IDCANCEL);
			return S_FALSE;
		}
	}

	return S_OK;
}

ULONG
DtlLoadDtl(
	IN PWSTR Path,
	IN PDTL_OBJECT Object
	)
{
	return S_OK;
}

ULONG 
DtlLoadManualGroup(
	IN PDTL_OBJECT Object,
	IN HANDLE FileHandle
	)
{
	ULONG CompleteBytes;
	ULONG Number;
	ULONG Length;
	PDTL_MANUAL_GROUP Group;
	PLIST_ENTRY ListEntry;
	PDTL_PROBE Probe;

	InitializeListHead(&Object->ManualGroupList);
	Length = FIELD_OFFSET(DTL_MANUAL_GROUP, ListEntry);	

	ReadFile(FileHandle, &Object->NumberOfManualGroup, sizeof(ULONG), &CompleteBytes, NULL);

	for(Number = 0; Number < Object->NumberOfManualGroup; Number += 1) {
		Group = (PDTL_MANUAL_GROUP)BspMalloc(sizeof(DTL_MANUAL_GROUP));
		ReadFile(FileHandle, Group, Length, &CompleteBytes, NULL);
		InsertTailList(&Object->ManualGroupList, &Group->ListEntry);
	}

	ListEntry = Object->ManualGroupList.Flink;

	while (ListEntry != &Object->ManualGroupList) {

		Group = CONTAINING_RECORD(ListEntry, DTL_MANUAL_GROUP, ListEntry);
		InitializeListHead(&Group->ProbeListHead);

		Probe = (PDTL_PROBE)VirtualAlloc(NULL, sizeof(DTL_PROBE) * Group->NumberOfProbes, 
										 MEM_COMMIT, PAGE_READWRITE);

		ReadFile(FileHandle, Probe, sizeof(DTL_PROBE) * Group->NumberOfProbes, &CompleteBytes, NULL);
		
		for(Number = 0; Number < Group->NumberOfProbes; Number += 1) {
			InsertTailList(&Group->ProbeListHead, &Probe[Number].Entry);
		}
		
		ListEntry = ListEntry->Flink;
	}
	
	return 0;	
}

ULONG
DtlDropCache(
	IN PDTL_OBJECT Object
	)
{
	return S_OK;
}

PDTL_FILE
DtlCreateQueryFile(
    IN PDTL_OBJECT Object 
    )
{
    HANDLE Handle;
    ULONG Status;
    PDTL_FILE FileObject;
	SYSTEMTIME Time;
	WCHAR Buffer[MAX_PATH];

	//
	// Keep the routine as simple as possible, decouple the destroy and
	// attach operation from this routine.
	//

    //DtlDestroyQueryFile(Object);

    FileObject = (PDTL_FILE)BspMalloc(sizeof(DTL_FILE));
    
	GetSystemTime(&Time);
    StringCchCopy(FileObject->Path, MAX_PATH, Object->DataObject.Path);

	StringCchPrintf(Buffer, MAX_PATH, L".%02d-%02d-%02d.filter", 
		            Time.wHour, Time.wMinute, Time.wSecond);

    StringCchCat(FileObject->Path, MAX_PATH, Buffer);

    Handle = CreateFile(FileObject->Path,
	                    GENERIC_READ | GENERIC_WRITE,
	                    FILE_SHARE_READ | FILE_SHARE_WRITE, 
	                    NULL,
	                    CREATE_NEW,
	                    FILE_ATTRIBUTE_NORMAL,
	                    NULL);

	if (Handle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
        BspFree(FileObject);
		return NULL;
	}

    FileObject->ObjectHandle = Handle;
    FileObject->Type = DTL_FILE_QUERY;
    FileObject->Mode = DTL_FILE_CREATE;
	FileObject->FileLength = DTL_ALIGNMENT_UNIT;
	FileObject->DataLength = 0;
	FileObject->BeginOffset = 0;

    SetFilePointer(FileObject->ObjectHandle, DTL_ALIGNMENT_UNIT, NULL, FILE_BEGIN);
    SetEndOfFile(FileObject->ObjectHandle);
    SetFilePointer(FileObject->ObjectHandle, 0, 0, FILE_BEGIN);

    Status = DtlCreateMap(FileObject);
    if (Status != S_OK) {

        CloseHandle(FileObject->ObjectHandle);
        if (FileObject->MapObject) {
            BspFree(FileObject->MapObject);
        }

        BspFree(FileObject);
        return NULL;
    }

    //Object->QueryIndexObject = FileObject;
    return FileObject;
}

ULONG
DtlDestroyQueryFile(
	IN PDTL_OBJECT Object
	)
{
    PDTL_MAP MapObject;
    PDTL_FILE FileObject;

	if (Object->QueryIndexObject != NULL) {

        FileObject = Object->QueryIndexObject;
        MapObject = FileObject->MapObject;

        if (MapObject != NULL) {
            UnmapViewOfFile(MapObject->Address);
            CloseHandle(MapObject->ObjectHandle);
        }

        CloseHandle(FileObject->ObjectHandle);
        DeleteFile(FileObject->Path);

        BspFree(MapObject);
        BspFree(FileObject);

		Object->QueryIndexObject = NULL;
    }

	return S_OK;
}

//
// N.B. ConditionObject should only include the additional condition list
// and merged into current filter object.
//

ULONG
DtlApplyQueryFilterLite(
	IN PDTL_OBJECT DtlObject,
	IN PCONDITION_OBJECT ConditionObject 
	)
{
	PDTL_FILE CurrentQuery;
	PDTL_FILE NewQuery;
	PDTL_FILE DataObject;
	LONG High;
    ULONG64 i;
    CHAR Buffer[4096 * 64];
	DTL_INDEX Index;
	ULONG CompleteLength;
	PLIST_ENTRY ListEntry;

	ASSERT(ConditionObject != NULL);
	
	CurrentQuery = DtlObject->QueryIndexObject;
	ASSERT(CurrentQuery != NULL);

    if (!CurrentQuery) {
        return S_FALSE;
    }

	//
	// Seek the current query file pointer to offset 0 
	//

	SetFilePointer(CurrentQuery->ObjectHandle, 0, 0, FILE_BEGIN);

	//
	// Create new query file
	//

	NewQuery = DtlCreateQueryFile(DtlObject);
	if (!NewQuery) {
		return S_FALSE;
	}

	SetFilePointer(NewQuery->ObjectHandle, 0, 0, FILE_BEGIN);

	//
	// Destroy old filter counters
	//

	DtlDestroyFilterCounters(DtlObject);
	DataObject = &DtlObject->DataObject;

	for(i = 0; i < CurrentQuery->RecordCount; i++) {

		//
		// Read current query index slot
		//

		ReadFile(CurrentQuery->ObjectHandle, &Index, sizeof(DTL_INDEX), &CompleteLength, NULL);
		
		//
		// Read indexed record
		//

		High = Index.Offset >> 32;
		SetFilePointer(DataObject->ObjectHandle, (LONG)Index.Offset, &High, FILE_BEGIN);
		ReadFile(DataObject->ObjectHandle, Buffer, Index.Length, &CompleteLength, NULL);

		//
		// If current record match the filter condition, write to new query file and
		// update filter counters
		//

		if (DtlIsFilteredRecord(DtlObject, ConditionObject, (PBTR_RECORD_HEADER)Buffer)) {

			WriteFile(NewQuery->ObjectHandle, &Index, sizeof(Index), &CompleteLength, NULL);	

			NewQuery->DataLength += sizeof(DTL_INDEX);
		    if (NewQuery->DataLength > NewQuery->FileLength) {
		        NewQuery->FileLength = NewQuery->DataLength;
		    }
			
			NewQuery->RecordCount += 1;
			DtlUpdateFilterCounters(DtlObject, (PBTR_RECORD_HEADER)Buffer);
		} 
	}

	DebugTrace("DtlApplyQueryFilterLite: record count #%I64u, filtered #%I64u", 
		        DtlObject->IndexObject.RecordCount, NewQuery->RecordCount);

	//
	// Append the extra condition entries to current filter object
	//

	while (IsListEmpty(&ConditionObject->ConditionListHead) != TRUE) {
		ListEntry = RemoveHeadList(&ConditionObject->ConditionListHead);
		InsertTailList(&DtlObject->ConditionObject->ConditionListHead, ListEntry);
		DtlObject->ConditionObject->NumberOfConditions += 1;
	}

	//
	// Destroy old query and attach new query to dtl object
	//

	DtlDestroyQueryFile(DtlObject);
	DtlObject->QueryIndexObject = NewQuery;

    return S_OK;
}

ULONG
DtlApplyQueryFilter(
    IN PDTL_OBJECT DtlObject,
    IN PCONDITION_OBJECT ConditionObject,
    IN HWND hWnd
    )
{
    PDTL_FILE QueryObject;
	PMSP_FILTER_FILE_OBJECT FileObject;

	DtlDestroyQueryFile(DtlObject);
	DtlDestroyFilterCounters(DtlObject);

	if (!ConditionObject) {

		FileObject = DtlObject->FileObject;
		if (FileObject != NULL) {
			if (FileObject->FilterObject) {
				FileObject->FilterObject = NULL;
			}
			MspDestroyFilterFileObject(FileObject);
		}

		DtlObject->ConditionObject = NULL;
		DtlObject->FileObject = NULL;

		return 0;
	}
	
    QueryObject = DtlCreateQueryFile(DtlObject);
    if (!QueryObject) {
        return S_FALSE;
    }

	//
	// Attach the new query object onto dtl object
	//

	DtlObject->QueryIndexObject = QueryObject;

	if (DtlObject->FileObject) {
		MspDestroyFilterFileObject(DtlObject->FileObject);
	}
	MspCreateFilterFileObject(ConditionObject);

    /*for(i = 0; i < DtlObject->IndexObject.RecordCount; i++) {
        DtlCopyRecord(DtlObject, i, Buffer, 4096 * 64); 
        if (DtlIsFilteredRecord(DtlObject, ConditionObject, (PBTR_RECORD_HEADER)Buffer)) {
            DtlCopyFilterIndex(DtlObject, i, QueryObject);
			DtlUpdateFilterCounters(DtlObject, Buffer);
        }
    }
    
	DebugTrace("DtlApplyQueryFilter record count #%I64u, filtered #%I64u", 
		        DtlObject->IndexObject.RecordCount, DtlObject->QueryIndexObject->RecordCount);*/

    return 0;
}

ULONG
DtlFreeFilterObject(
	IN PCONDITION_OBJECT Object
	)
{
	PCONDITION_ENTRY Condition;
	PLIST_ENTRY ListEntry;

	if (!Object) {
		return S_OK;
	}

	ListEntry = Object->ConditionListHead.Flink;
	while (IsListEmpty(&Object->ConditionListHead) != TRUE) {
		ListEntry = RemoveHeadList(&Object->ConditionListHead);
		Condition = CONTAINING_RECORD(ListEntry, CONDITION_ENTRY, ListEntry);
		BspFree(Condition);
	}

	BspFree(Object);
	return S_OK;
}

BOOLEAN
DtlIsFilteredRecord(
	IN PDTL_OBJECT DtlObject,
    IN PCONDITION_OBJECT ConditionObject,
    IN PBTR_RECORD_HEADER Record
    )
{
    PLIST_ENTRY ListEntry;
    PCONDITION_ENTRY Condition;
    BOOLEAN Filtered;

    if (ConditionObject == NULL) {
        ASSERT(0);
    }

	//
	// N.B. if there's no condition in filter object, it's treated
	// as no filter at all, however, this should not happen under
	// normal circumstance.
	//

	if (IsListEmpty(&ConditionObject->ConditionListHead)) {
		return TRUE;
	}

    ListEntry = ConditionObject->ConditionListHead.Flink;

    while (ListEntry != &ConditionObject->ConditionListHead) {
        Condition = CONTAINING_RECORD(ListEntry, CONDITION_ENTRY, ListEntry);
		Filtered = (*ConditionHandlers[Condition->Object])(DtlObject, Condition, Record);
        if (Filtered != TRUE) {
            return FALSE;
        }
        ListEntry = ListEntry->Flink;
    }

    return TRUE;
}

ULONG64
DtlCopyFilterIndex(
    IN PDTL_OBJECT DtlObject, 
    IN ULONG64 Number,
    IN PDTL_FILE QueryObject
    )
{
    PDTL_FILE IndexObject;
    ULONG CompleteLength;
	DTL_INDEX Index;
	ULONG64 Offset;
	LONG High;

    BspAcquireLock(&DtlObject->ObjectLock);

	IndexObject = &DtlObject->IndexObject;
    QueryObject = DtlObject->QueryIndexObject;

	ASSERT(Number < IndexObject->RecordCount);

	//
	// N.B. Take care to compute position of index, for dtl open mode,
	// the index location is not from 0 as is in create mode, to make
	// unify access of index, we need re-design to strip the index from
	// dtl file, this can ensure open/create has unified access method.
	//

	if (DtlObject->DataObject.Mode == DTL_FILE_CREATE) {
		Offset = sizeof(DTL_INDEX) * Number;
	} else {
		Offset = DtlObject->Header->IndexLocation + sizeof(DTL_INDEX) * Number;
	}

	High = (LONG)(Offset >> 32);

	SetFilePointer(IndexObject->ObjectHandle, (LONG)Offset, &High, FILE_BEGIN);
	ReadFile(IndexObject->ObjectHandle, &Index, sizeof(DTL_INDEX), &CompleteLength, NULL);
    WriteFile(QueryObject->ObjectHandle, &Index, sizeof(DTL_INDEX),&CompleteLength, NULL);

    QueryObject->DataLength += sizeof(DTL_INDEX);
    if (QueryObject->DataLength > QueryObject->FileLength) {
        QueryObject->FileLength = QueryObject->DataLength;
    }

    QueryObject->RecordCount += 1;
    BspReleaseLock(&DtlObject->ObjectLock);

    return QueryObject->RecordCount;
}

BOOLEAN
DtlIsAllowEnterDtl(
    PDTL_OBJECT Object
    )
{
    return TRUE;
}

ULONG
DtlDestroyObject(
	IN PDTL_OBJECT Object
	)
{
	__try {

		if (Object->PdbObject) {
			PdbFreeResource(Object->PdbObject);
		}

		if (Object->ConditionObject) {
			DtlFreeFilterObject(Object->ConditionObject);
		}

		if (Object->QueryIndexObject) {
			DtlDestroyFile(Object->QueryIndexObject);
			Object->QueryIndexObject = NULL;
		}

		if (Object->Header) {
			BspFree(Object->Header);
		}
		
		DtlFreeManualGroup(&Object->ManualGroupList);
		DtlFreeAddOnGroup(&Object->AddOnGroupList);

		DtlDestroyFile(&Object->DataObject);
		DtlDestroyFile(&Object->IndexObject);

		RemoveEntryList(&Object->ListEntry);
		BspDestroyLock(&Object->ObjectLock);
		BspDestroyLock(&Object->QueueLock);
		BspFree(Object);

	}

	__except(EXCEPTION_EXECUTE_HANDLER) {
		DebugTrace("DtlDestroyObject exception occurred");
	}

	return S_OK;
}

VOID
DtlFreeManualGroup(
	IN PLIST_ENTRY ListHead
	)
{
	PLIST_ENTRY ListEntry; 
	PDTL_PROBE Probe;
	PDTL_MANUAL_GROUP Group;

	while (IsListEmpty(ListHead) != TRUE) {

		ListEntry = RemoveHeadList(ListHead);
		Group = CONTAINING_RECORD(ListEntry, DTL_MANUAL_GROUP, ListEntry);
		Probe = CONTAINING_RECORD(Group->ProbeListHead.Flink, DTL_PROBE, Entry);
		VirtualFree(Probe, 0, MEM_RELEASE);

		/*
		while (IsListEmpty(&Group->ProbeListHead) != TRUE) {

			ListEntry2 = RemoveHeadList(&Group->ProbeListHead);
			Probe = CONTAINING_RECORD(ListEntry2, DTL_PROBE, Entry);

			while (IsListEmpty(&Probe->VariantList) != TRUE) {
				ListEntry3 = RemoveHeadList(&Probe->VariantList);
				Variant = CONTAINING_RECORD(ListEntry3, DTL_PROBE, Entry);
				BspFree(Variant);
			}

			BspFree(Probe);
		}*/

		BspFree(Group);
	}
}

VOID
DtlFreeAddOnGroup(
	IN PLIST_ENTRY ListHead
	)
{
	
	PLIST_ENTRY ListEntry; 
	PDTL_ADDON_GROUP Group;

	while (IsListEmpty(ListHead) != TRUE) {
		ListEntry = RemoveHeadList(ListHead);
		Group = CONTAINING_RECORD(ListEntry, DTL_ADDON_GROUP, ListEntry);
		BspFree(Group);
	}
}

ULONG
DtlUpdateCounters(
	IN PDTL_OBJECT DtlObject,
	IN PDTL_IO_BUFFER Buffer
	)
{
	ULONG i;
	PBTR_RECORD_HEADER Record;
	PDTL_COUNTER_SET Set;
	PDTL_PROBE_COUNTER Counter;

	Record = (PBTR_RECORD_HEADER)Buffer->Buffer;

	for(i = 0; i < Buffer->NumberOfRecords; i++) {

		Set = DtlLookupCounterSet(DtlObject, Record, FALSE);
		ASSERT(Set != NULL);

		Set->RecordCount += 1;

		Counter = DtlLookupProbeCounter(DtlObject, Set, Record);
		ASSERT(Counter != NULL);

		Counter->RecordCount += 1;

		if (Record->Duration > Counter->MaximumDuration) {
			Counter->MaximumDuration = Record->Duration;
		}

		Counter->AverageDuration = 
			(ULONG)((Counter->AverageDuration * (Counter->RecordCount - 1) + Record->Duration) / Counter->RecordCount);
		
		Record = (PBTR_RECORD_HEADER)((PCHAR)Record + Record->TotalLength);
	}

	return S_OK;
}

ULONG
DtlUpdateCounter(
	IN PDTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record
	)
{
	PDTL_COUNTER_SET Set;
	PDTL_PROBE_COUNTER Counter;

	Set = DtlLookupCounterSet(DtlObject, Record, FALSE);
	ASSERT(Set != NULL);

	Set->RecordCount += 1;

	Counter = DtlLookupProbeCounter(DtlObject, Set, Record);
	ASSERT(Counter != NULL);

	Counter->RecordCount += 1;

	if (Record->Duration > Counter->MaximumDuration) {
		Counter->MaximumDuration = Record->Duration;
	}

	Counter->AverageDuration = 
		(ULONG)((Counter->AverageDuration * (Counter->RecordCount - 1) + Record->Duration) / Counter->RecordCount);

	return S_OK;
}

ULONG
DtlUpdateFilterCounters(
	IN PDTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record 
	)
{
	PDTL_COUNTER_SET Set;
	PDTL_PROBE_COUNTER Counter;

	Set = DtlLookupCounterSet(DtlObject, Record, TRUE);
	ASSERT(Set != NULL);

	Set->RecordCount += 1;

	Counter = DtlLookupProbeCounter(DtlObject, Set, Record);
	ASSERT(Counter != NULL);

	Counter->RecordCount += 1;

	if (Record->Duration > Counter->MaximumDuration) {
		Counter->MaximumDuration = Record->Duration;
	}

	Counter->AverageDuration = 
		(ULONG)((Counter->AverageDuration * (Counter->RecordCount - 1) + Record->Duration) / Counter->RecordCount);

	return S_OK;
}

ULONG
DtlDestroyFilterCounters(
	IN PDTL_OBJECT Object
	)
{
	PDTL_COUNTER_SET Set;
	PDTL_PROBE_COUNTER Counter;
	PDTL_COUNTER_OBJECT CounterObject;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY CounterListEntry;

	//
	// N.B. Under any condition, filter counter object can not be NULL,
	// it's at least an empty object
	//

	ASSERT(Object->FilterCounter != NULL);

	CounterObject = Object->FilterCounter;

	while (IsListEmpty(&CounterObject->CounterSetList) != TRUE) {

		ListEntry = RemoveHeadList(&CounterObject->CounterSetList);	
		Set = CONTAINING_RECORD(ListEntry, DTL_COUNTER_SET, ListEntry);
		
		while (IsListEmpty(&Set->CounterList) != TRUE) {
			CounterListEntry = RemoveHeadList(&Set->CounterList);	
			Counter = CONTAINING_RECORD(CounterListEntry, DTL_PROBE_COUNTER, ListEntry);
			BspFree(Counter);
		}

		BspFree(Set);
	}

	//
	// Reset its fields
	//

	CounterObject->LastRecord = -1;
	CounterObject->NumberOfCounterSet = 0;

	return S_OK;
}

PDTL_PROBE_COUNTER
DtlLookupProbeCounter(
	IN PDTL_OBJECT DtlObject,
	IN PDTL_COUNTER_SET Set,
	IN PBTR_RECORD_HEADER Record
	)
{
	PLIST_ENTRY ListEntry;
	PDTL_PROBE_COUNTER Counter;
	WCHAR Buffer[MAX_PATH];

	if (Record->RecordType == RECORD_EXPRESS) {
		StringCchCopy(Buffer, MAX_PATH - 1, L"Manual Probe");
	} else {
		DtlLookupAddOnName(DtlObject, (PBTR_FILTER_RECORD)Record, Buffer, MAX_PATH - 1);
	}

	ListEntry = Set->CounterList.Flink;
	while (ListEntry != &Set->CounterList) {
		Counter = CONTAINING_RECORD(ListEntry, DTL_PROBE_COUNTER, ListEntry);
		if (Counter->Address == (PVOID)Record->Address) {
			if (!wcscmp(Counter->Provider, Buffer)) {
				return Counter;
			}
		}
		ListEntry = ListEntry->Flink;
	}

	Counter = (PDTL_PROBE_COUNTER)BspMalloc(sizeof(DTL_PROBE_COUNTER));
	Counter->ProcessId = Record->ProcessId;
	Counter->Address = (PVOID)Record->Address;
	Counter->MaximumDuration = Record->Duration;
	Counter->AverageDuration = Record->Duration;
	Counter->RecordCount = 0;
	Counter->RecordPercent = 0;
	Counter->StackSampleCount = 0;
	StringCchCopy(Counter->Provider, MAX_PATH - 1, Buffer);

	if (Record->RecordType == RECORD_FILTER) {
		DtlLookupAddOnProbeByOrdinal(DtlObject, (PBTR_FILTER_RECORD)Record, Buffer, MAX_PATH);
	} else {
		DtlLookupProbeByAddress(DtlObject, Record->ProcessId, (PVOID)Record->Address, Buffer, MAX_PATH);
	}
	StringCchCopy(Counter->Name, MAX_PATH - 1, Buffer);

	InsertHeadList(&Set->CounterList, &Counter->ListEntry);
	Set->NumberOfCounter += 1;
	return Counter;
}

PDTL_COUNTER_SET
DtlLookupCounterSet(
	IN PDTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record,
	IN BOOLEAN FilterCounter
	)
{
	PLIST_ENTRY ListEntry;
	PDTL_COUNTER_OBJECT Counter;
	PDTL_COUNTER_SET Set;
	PDTL_MANUAL_GROUP Group;

	if (FilterCounter != TRUE) {
		Counter = DtlObject->CounterObject;
	} else {
		Counter = DtlObject->FilterCounter;
	}

	ASSERT(Counter != NULL);

	ListEntry = Counter->CounterSetList.Flink;
	while (ListEntry != &Counter->CounterSetList) {
		Set = CONTAINING_RECORD(ListEntry, DTL_COUNTER_SET, ListEntry);
		if (Set->ProcessId == Record->ProcessId) {
			return Set;
		}
		ListEntry = ListEntry->Flink;
	}

	Set = (PDTL_COUNTER_SET)BspMalloc(sizeof(DTL_COUNTER_SET));
	Set->ProcessId = Record->ProcessId;
	Set->RecordCount = 0;
	Set->RecordPercent = 0;

	Group = DtlQueryManualGroup(DtlObject, Record->ProcessId);
	ASSERT(Group != NULL);
	StringCchCopy(Set->ProcessName, MAX_PATH - 1, Group->ProcessName);

	InitializeListHead(&Set->CounterList);
	InsertHeadList(&Counter->CounterSetList, &Set->ListEntry);

	Counter->NumberOfCounterSet += 1;
	return Set;
}

PDTL_COUNTER_OBJECT
DtlQueryCounters(
	IN PDTL_OBJECT DtlObject
	)
{
	return 0;
}

ULONG
DtlCleanCache(
	IN PDTL_OBJECT Object,
	IN BOOLEAN Save
	)
{
	ULONG Status;
	PDTL_FILE DataObject;
	PDTL_FILE IndexObject;
	PDTL_MAP MapObject;

	ASSERT(Object != NULL);

	Status = S_OK;
	BspAcquireLock(&Object->ObjectLock);

	DataObject = &Object->DataObject;
	IndexObject = &Object->IndexObject;

	if (DataObject->ObjectHandle != NULL) {

		MapObject = DataObject->MapObject; 
		if (MapObject && MapObject->ObjectHandle) {
			UnmapViewOfFile(MapObject->Address);
			CloseHandle(MapObject->ObjectHandle);
		}
		
		CloseHandle(DataObject->ObjectHandle);

		if (!Save) {
			Status = DeleteFile(DataObject->Path); 
			if (!Status) {
				Status = GetLastError();
			} else {
				Status = S_OK;
			}
		}
	}

	if (IndexObject->ObjectHandle != NULL) {

		MapObject = IndexObject->MapObject; 
		if (MapObject && MapObject->ObjectHandle) {
			UnmapViewOfFile(MapObject->Address);
			CloseHandle(MapObject->ObjectHandle);
		}
		
		CloseHandle(IndexObject->ObjectHandle);

		if (!Save) {
			Status = DeleteFile(IndexObject->Path); 
			if (!Status) {
				Status = GetLastError();
			} else {
				Status = S_OK;
			}
		}
	}

	BspReleaseLock(&Object->ObjectLock);
	return S_OK;
}

VOID
DtlRegisterSharedDataCallback(
	IN PDTL_OBJECT Object,
	IN DTL_SHARED_DATA_CALLBACK Callback,
	IN PVOID Context
	)
{
	DtlSharedData.Context = Context;
	DtlSharedData.Callback = Callback;
}

BOOLEAN
DtlIsReportFile(
	IN PDTL_OBJECT Object
	)
{
	PDTL_FILE DataObject;

	DataObject = &Object->DataObject;
	if (DataObject->Mode != DTL_FILE_CREATE) {
		return TRUE;
	}

	return FALSE;
}

VOID
DtlDeleteGarbageFiles(
	IN PWSTR Path
	)
{
	ULONG Status;
	ULONG DtlStatus;
	WIN32_FIND_DATA Data;
	HANDLE FindHandle;
	WCHAR Buffer[MAX_PATH];
	WCHAR FullPathName[MAX_PATH];
	
	//
	// Delete *.dti index files
	//

	Status = FALSE;
	wcsncpy(Buffer, Path, MAX_PATH);
	wcscat(Buffer, L"\\*.dti");

	FindHandle = FindFirstFile(Buffer, &Data);
	if (FindHandle != INVALID_HANDLE_VALUE) {
		Status = TRUE;
	}

	while (Status) {
		
		wcsncpy(FullPathName, Path, MAX_PATH);
		wcsncat(FullPathName, L"\\", MAX_PATH);
		wcsncat(FullPathName, Data.cFileName, MAX_PATH);
		DeleteFile(FullPathName);

		Status = (ULONG)FindNextFile(FindHandle, &Data);
	}

	//
	// Delete *.filter filter files
	//

	Status = FALSE;
	wcsncpy(Buffer, Path, MAX_PATH);
	wcscat(Buffer, L"\\*.filter");

	FindHandle = FindFirstFile(Buffer, &Data);
	if (FindHandle != INVALID_HANDLE_VALUE) {
		Status = TRUE;
	}

	while (Status) {
		
		wcsncpy(FullPathName, Path, MAX_PATH);
		wcsncat(FullPathName, L"\\", MAX_PATH);
		wcsncat(FullPathName, Data.cFileName, MAX_PATH);
		DeleteFile(FullPathName);

		Status = (ULONG)FindNextFile(FindHandle, &Data);
	}

	//
	// Delete uncompleted *.dtl files. 
	//

	Status = FALSE;
	wcsncpy(Buffer, Path, MAX_PATH);
	wcscat(Buffer, L"\\*.dtl");

	FindHandle = FindFirstFile(Buffer, &Data);
	if (FindHandle != INVALID_HANDLE_VALUE) {
		Status = TRUE;
	}

	while (Status) {
		
		wcsncpy(FullPathName, Path, MAX_PATH);
		wcsncat(FullPathName, L"\\", MAX_PATH);
		wcsncat(FullPathName, Data.cFileName, MAX_PATH);

		DtlStatus = DtlIsValidReport(FullPathName);
		if (DtlStatus == STATUS_DTL_BAD_CHECKSUM) { 
			DeleteFile(FullPathName);
		}

		Status = (ULONG)FindNextFile(FindHandle, &Data);
	}
}