//
// Apsaras Labs
// lan.john@gmail.com
// Copyright(C) 2009 - 2010
//

#include "sdk.h"
#include "bsp.h"
#include "rsp.h"
#include "ssp.h"
#include "btr.h"

RSP_STORAGE RspStorage;
RSP_STORAGE RspIndexStorage;
RSP_INDEX RspIndex;
RSP_VIEW RspView;

LIST_ENTRY RspRecordList;
LIST_ENTRY RspCallbackList;

LIST_ENTRY RspProbeGroupList;
LIST_ENTRY RspAddOnGroupList;

ULONG RspProbeGroupCount;

ULONG RspHostThreadId;
WCHAR RspStoragePath[MAX_PATH];
WCHAR RspBaseFileName[MAX_PATH];

#define RSP_STORAGE_CAPACITY    ((ULONG)0x80000000)
#define RSP_STORAGE_UNIT        ((ULONG)0x00100000)

#define RSP_INDEX_CAPACITY	    ((ULONG)0x00100000)
#define RSP_INDEX_MAXIMUM       ((ULONG)0x000FFFFF)
#define RSP_INVALID_INDEX       ((ULONG)0xFFFFFFFF)
#define RSP_INVALID_RANGE       ((ULONG)0xFFFFFFFF)
#define RSP_RANGE_NUMBER        ((ULONG)2048)


typedef struct _RSP_FILE_HEADER {
	ULONG Signature;
	ULONG CheckSum;
	USHORT MajorVersion;
	USHORT MinorVersion;
	ULONG  BuildNumber;
	ULONG  FeatureFlag;
	SYSTEMTIME StartTime;
	SYSTEMTIME EndTime;
	ULONG64 TotalLength;
	ULONG64 RecordLocation;
	ULONG64 RecordNumber;
	ULONG64 RecordLength;
	ULONG64 IndexLocation;
	ULONG64 IndexLength;
	ULONG64 StackLocation;
	ULONG64 StackLength;
	ULONG64 PerformaceLocation;
	ULONG64 PerformanceLength;
} RSP_FILE_HEADER, *PRSP_FILE_HEADER;

//
//	Ensure RSP_INDEX_UNIT_SIZE is page aligned
//


ULONG64 FORCEINLINE
RspRoundUpLength(
	IN ULONG64 Length,
	IN ULONG64 Unit 
	)
{
	return (((ULONG64)(Length) + (ULONG64)(Unit) - 1) & ~((ULONG64)(Unit) - 1)); 
}

ULONG64 FORCEINLINE
RspRoundDownLength(
	IN ULONG64 Length,
	IN ULONG64 Unit
	)
{
	return ((ULONG64)(Length) & ~((ULONG64)(Unit) - 1));
}

ULONG
RspInitialize(
	IN PWSTR StoragePath	
	)
{
	ULONG Status;
	ULONG Length;
	SYSTEMTIME Time;
	WCHAR Buffer[MAX_PATH];

	Status = S_OK;
	RtlZeroMemory(&RspStorage, sizeof(RSP_STORAGE));
	RtlZeroMemory(&RspIndex, sizeof(RSP_INDEX));
	RtlZeroMemory(&RspView, sizeof(RSP_VIEW));

	InitializeListHead(&RspRecordList);
	InitializeListHead(&RspCallbackList);
	InitializeListHead(&RspProbeGroupList);
	InitializeListHead(&RspAddOnGroupList);

	RspHostThreadId = GetCurrentThreadId();
	
	if (WorkMode == CaptureMode) {

		PspGetFilePath(RspStoragePath, MAX_PATH - 1);
		StringCchCat(RspStoragePath, MAX_PATH - 1, L"\\log");

		if (CreateDirectory(RspStoragePath, NULL) != TRUE) {
			if (GetLastError() != ERROR_ALREADY_EXISTS) {
				return S_FALSE;
			}															
		}

		StringCchCopy(Buffer, MAX_PATH, RspStoragePath);
		Length = (ULONG)wcsnlen_s(Buffer, MAX_PATH);

		GetLocalTime(&Time);
		StringCchPrintf(Buffer + Length, MAX_PATH - Length, L"\\dprobe-%02d%02d%02d%02d%02d.dtl", 
						Time.wMonth, Time.wDay, Time.wHour, Time.wMinute, Time.wSecond);

		Status = RspCreateCache(Buffer);
		SspInitialize();

	} else {

		Status = RspLoadCache(StoragePath);
		SspInitialize();
		SspLoadStackTraceDatabase();
	}

	return Status;
}

ULONG
RspGetStorageBaseName(
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	StringCchCopy(Buffer, Length, RspStorage.FileName);
	return 0;
}

ULONG
RspCreateCache(
	IN PWSTR Path 
	)
{
	HANDLE FileHandle;
	HANDLE ViewHandle;
	PVOID BaseVa;
	ULONG i;

	FileHandle = CreateFile(Path,
	                        GENERIC_READ | GENERIC_WRITE,
	                        0, 
	                        NULL,
	                        CREATE_NEW,
	                        FILE_ATTRIBUTE_NORMAL,
	                        NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		return STATUS_RSP_CREATEFILE_FAILED;
	}
	
	SetFilePointer(FileHandle, RSP_STORAGE_UNIT, 0, FILE_BEGIN);
	SetEndOfFile(FileHandle);

	RspStorage.FileHandle = FileHandle;
	RspStorage.FileLength = RSP_STORAGE_UNIT;
	RspStorage.ValidLength = 0;
	StringCchCopy(RspStorage.FileName, MAX_PATH, Path);

	//
	// Create index object and its index cluster
	//

	BaseVa = VirtualAlloc(NULL, 
		                  RSP_INDEX_CAPACITY * sizeof(RSP_INDEX_ENTRY), 
						  MEM_COMMIT, 
						  PAGE_READWRITE);

	ASSERT(BaseVa != NULL);
	RspIndex.Entry = (PRSP_INDEX_ENTRY)BaseVa;
	RspIndex.LastValidIndex = RSP_INVALID_INDEX;

	//
	// Create view object
	//

	ViewHandle = CreateFileMapping(RspStorage.FileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
	ASSERT(ViewHandle != NULL);
	RspView.ViewHandle = ViewHandle;
	
	//
	// Create view object and its range cluster
	//

	BaseVa = VirtualAlloc(NULL, 
		                  sizeof(RSP_VIEW_RANGE) * RSP_RANGE_NUMBER, 
						  MEM_COMMIT, 
						  PAGE_READWRITE);
	ASSERT(BaseVa != NULL);

	RspView.MappedVa = NULL;
	RspView.ViewHandle = ViewHandle;
	RspView.Range = (PRSP_VIEW_RANGE)BaseVa;
	RspView.RangeCount = RSP_RANGE_NUMBER;
	RspView.CurrentRange = RSP_INVALID_RANGE;
	
	for(i = 0; i < RSP_RANGE_NUMBER; i++) {
		RspView.Range[i].FirstIndex = RSP_INVALID_INDEX;
		RspView.Range[i].LastIndex = RSP_INVALID_INDEX;
	}

	RspView.Range[0].FirstIndex = 0;
	RspView.ValidCount = 1;
	RspStorage.Mode = RspCreateMode;

	return ERROR_SUCCESS;
}

ULONG
RspLoadCache(
	IN PWSTR Path
	)
{
	HANDLE FileHandle;
	WCHAR Buffer[MAX_PATH];
	ULONG IndexCount;
	ULONG CompleteLength;
	PRSP_VIEW_RANGE BaseVa;
	PWCHAR Dot;
	ULONG FileLength;
	PRSP_INDEX_ENTRY LastEntry;

	wcscpy(Buffer, Path);
	Dot = wcsrchr(Buffer, L'.');
	ASSERT(Dot != NULL);

	if (Dot == NULL) {
		return STATUS_RSP_INVALID_PATH;
	}

	wcscpy(Dot + 1, L"dli"); 

	//
	// Open index of log file (*.dli), index file's layout is as followings:
	// 1, SIGNATURE	     (struct RSP_SIGNATURE)
	// 2, Index Count    (ULONG)
	// 3, Index Cluster  (Number of RSP_INDEX_ENTRY) 
	// 4, Range Count    (ULONG)
	// 5, Range Cluster  (Number of RSP_VIEW_RANGE)
	// 6, Probe Group Count
	// 7, Probe Group Table
	// 8, Probe Object Cluster
	//

	FileHandle = CreateFile(Buffer,
	                        GENERIC_READ,
	                        0, 
	                        NULL,
	                        OPEN_EXISTING,
	                        FILE_ATTRIBUTE_NORMAL,
	                        NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		return STATUS_RSP_CREATEFILE_FAILED;
	}

	SetFilePointer(FileHandle, sizeof(RSP_SIGNATURE), NULL, FILE_BEGIN);

	ReadFile(FileHandle, &IndexCount, sizeof(ULONG), &CompleteLength, NULL);
	RspIndex.Entry = (PRSP_INDEX_ENTRY)VirtualAlloc(NULL, 
		                                            sizeof(RSP_INDEX_ENTRY) * IndexCount, 
													MEM_COMMIT, 
													PAGE_READWRITE);

	RspIndex.LastValidIndex = IndexCount - 1;
	ReadFile(FileHandle, RspIndex.Entry, sizeof(RSP_INDEX_ENTRY) * IndexCount, &CompleteLength, NULL);

	ReadFile(FileHandle, &RspView.RangeCount, sizeof(ULONG), &CompleteLength, NULL);
	RspView.ValidCount = RspView.RangeCount;

	BaseVa = (PRSP_VIEW_RANGE)VirtualAlloc(NULL, 
										   sizeof(RSP_VIEW_RANGE) * RspView.ValidCount, 
										   MEM_COMMIT, 
										   PAGE_READWRITE);
	RspView.Range = BaseVa;
	ReadFile(FileHandle, RspView.Range, sizeof(RSP_VIEW_RANGE) * RspView.ValidCount, &CompleteLength, NULL);
	
	//
	// Finally load probe object information
	//

	RspLoadProbeGroup(FileHandle);
	RspLoadAddOnGroup(FileHandle);
	CloseHandle(FileHandle);


	//
	// Open log record file (*.dlr)
	//

	wcscpy(RspStorage.FileName, Path);
	FileHandle = CreateFile(RspStorage.FileName,
	                        GENERIC_READ,
	                        0, 
	                        NULL,
	                        OPEN_EXISTING,
	                        FILE_ATTRIBUTE_NORMAL,
	                        NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		return STATUS_RSP_CREATEFILE_FAILED;
	}

	LastEntry = &RspIndex.Entry[RspIndex.LastValidIndex];

	FileLength = GetFileSize(FileHandle, NULL);
	RspStorage.Mode = RspOpenMode;
	RspStorage.FileHandle = FileHandle;
	RspStorage.FileLength = FileLength;
	RspStorage.ValidLength = LastEntry->FileOffset + LastEntry->Length;

	//
	// Map the first range of view object
	//

	RspMapView(0);

	return ERROR_SUCCESS;
}

ULONG
RspFreeCache(
	VOID
	)
{
	ULONG Number;

	VirtualFree(RspIndex.Entry, 0, MEM_RELEASE);
	RspIndex.LastValidIndex = RSP_INVALID_INDEX;

	RspUnmapView(&Number);
	VirtualFree(RspView.Range, 0, MEM_RELEASE);
	RspView.Range = NULL;
	RspView.RangeCount = 0;

	CloseHandle(RspStorage.FileHandle);

	if (RspStorage.Mode == RspCreateMode) {
		DeleteFile(RspStorage.FileName);
	}
	
	RspStorage.Mode = RspNullMode;
	RspStorage.FileHandle = NULL;
	RspStorage.FileLength = 0;
	RspStorage.ValidLength = 0;
	RspStorage.FileName[0] = L'\0';

	return S_OK;
}

ULONG 
RspCommitCache(
	IN PWSTR Path
	)
{
	HANDLE FileHandle;
	WCHAR Buffer[MAX_PATH];
	PWCHAR Dot;
	ULONG i;
	ULONG Status;
	RSP_SIGNATURE Signature;
	ULONG CompleteLength;
	ULONG IndexCount;
	ULONG Number;
	PRSP_VIEW_RANGE BaseVa;

	ASSERT(RspStorage.Mode == RspCreateMode);

	//
	// Create index file by in memory index object
	//

	wcscpy(Buffer, RspStorage.FileName);
	Dot = wcsrchr(Buffer, L'.');
	ASSERT(Dot != NULL);

	if (Dot == NULL) {
		return STATUS_RSP_INVALID_PATH;
	}

	wcscpy(Dot + 1, L"dli"); 
	FileHandle = CreateFile(Buffer,
	                        GENERIC_READ | GENERIC_WRITE,
	                        0, 
	                        NULL,
	                        CREATE_NEW,
	                        FILE_ATTRIBUTE_NORMAL,
	                        NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		return STATUS_RSP_CREATEFILE_FAILED;
	}

	memset(&Signature, 0, sizeof(Signature));
	wcscpy(Signature.Magic, L"dprobe");
	Signature.Version = 1;

	//
	// First, write signature structure
	//

	Status = WriteFile(FileHandle, 
		               &Signature, 
					   sizeof(RSP_SIGNATURE), 
					   &CompleteLength, 
					   NULL);

	if (Status != TRUE) {
		CloseHandle(FileHandle);
		return STATUS_RSP_WRITEFILE_FAILED;
	}

	//
	// Second, write the index entries
	//

	IndexCount = RspIndex.LastValidIndex + 1;
	WriteFile(FileHandle, &IndexCount, sizeof(ULONG), &CompleteLength, NULL); 

	Status = WriteFile(FileHandle, 
		               RspIndex.Entry, 
					   sizeof(RSP_INDEX_ENTRY) * IndexCount, 
					   &CompleteLength, 
					   NULL);

	VirtualFree(RspIndex.Entry, 0, MEM_RELEASE);
	RspIndex.LastValidIndex = RSP_INVALID_INDEX;

	if (Status != TRUE) {
		CloseHandle(FileHandle);
		return STATUS_RSP_WRITEFILE_FAILED;
	}

	//
	// Third, write view object ranges
	//

	BaseVa = (PRSP_VIEW_RANGE)VirtualAlloc(NULL, 
										   sizeof(RSP_VIEW_RANGE) * RspView.ValidCount, 
										   MEM_COMMIT, 
										   PAGE_READWRITE);
	if (BaseVa == NULL) {
		CloseHandle(FileHandle);
		return STATUS_RSP_OUTOFMEMORY;
	}
	
	for(i = 0; i < RspView.ValidCount; i++) {
		BaseVa[i].FirstIndex = RspView.Range[i].FirstIndex;
		BaseVa[i].LastIndex = RspView.Range[i].LastIndex;
	}

	WriteFile(FileHandle, &RspView.ValidCount, sizeof(ULONG), &CompleteLength, NULL);
	Status = WriteFile(FileHandle, 
		               BaseVa, 
					   sizeof(RSP_VIEW_RANGE) * RspView.ValidCount, 
					   &CompleteLength, 
					   NULL);

	VirtualFree(BaseVa, 0, MEM_RELEASE);

	RspUnmapView(&Number);
	RspView.RangeCount = 0;

	//
	// Finally, write probe group into end of the index file.
	//

	RspWriteProbeGroup(FileHandle);
	RspWriteAddOnGroup(FileHandle);

	CloseHandle(FileHandle);
	CloseHandle(RspStorage.FileHandle);

	RspStorage.FileHandle = NULL;
	RspStorage.FileLength = 0;
	RspStorage.ValidLength = 0;
	RspStorage.FileName[0] = 0;
	RspStorage.Mode = RspNullMode;

	return ERROR_SUCCESS;
}

ULONG
RspCommitFreeCache(
	IN RSP_COMMIT_MODE Mode,
	IN PWSTR Path
	)
{
	HANDLE FileHandle;
	WCHAR Buffer[MAX_PATH];
	PWCHAR Dot;
	ULONG Number;
	ULONG i;
	ULONG Status;

	if (Mode == RspFreeOnly) {
		return RspFreeCache();
	}	

	if (RspStorage.Mode == RspCreateMode) {

		RSP_SIGNATURE Signature;
		ULONG CompleteLength;
		ULONG IndexCount;
		ULONG Number;
		PRSP_VIEW_RANGE BaseVa;

		//
		// Create index file by in memory index object
		//

		wcscpy(Buffer, RspStorage.FileName);
		Dot = wcsrchr(Buffer, L'.');
		ASSERT(Dot != NULL);

		if (Dot == NULL) {
			return STATUS_RSP_INVALID_PATH;
		}

		wcscpy(Dot + 1, L"dli"); 
		FileHandle = CreateFile(Buffer,
		                        GENERIC_READ | GENERIC_WRITE,
		                        0, 
		                        NULL,
		                        CREATE_NEW,
		                        FILE_ATTRIBUTE_NORMAL,
		                        NULL);

		if (FileHandle == INVALID_HANDLE_VALUE) {
			return STATUS_RSP_CREATEFILE_FAILED;
		}

		memset(&Signature, 0, sizeof(Signature));
		wcscpy(Signature.Magic, L"dprobe");
		Signature.Version = 1;

		//
		// First, write signature structure
		//

		Status = WriteFile(FileHandle, 
			               &Signature, 
						   sizeof(RSP_SIGNATURE), 
						   &CompleteLength, 
						   NULL);

		if (Status != TRUE) {
			CloseHandle(FileHandle);
			return STATUS_RSP_WRITEFILE_FAILED;
		}

		//
		// Second, write the index entries
		//

		IndexCount = RspIndex.LastValidIndex + 1;
		WriteFile(FileHandle, &IndexCount, sizeof(ULONG), &CompleteLength, NULL); 

		Status = WriteFile(FileHandle, 
			               RspIndex.Entry, 
						   sizeof(RSP_INDEX_ENTRY) * IndexCount, 
						   &CompleteLength, 
						   NULL);

		VirtualFree(RspIndex.Entry, 0, MEM_RELEASE);
		RspIndex.LastValidIndex = RSP_INVALID_INDEX;

		if (Status != TRUE) {
			CloseHandle(FileHandle);
			return STATUS_RSP_WRITEFILE_FAILED;
		}

		//
		// Third, write view object ranges
		//

		BaseVa = (PRSP_VIEW_RANGE)VirtualAlloc(NULL, 
											   sizeof(RSP_VIEW_RANGE) * RspView.ValidCount, 
											   MEM_COMMIT, 
											   PAGE_READWRITE);
		if (BaseVa == NULL) {
			CloseHandle(FileHandle);
			return STATUS_RSP_OUTOFMEMORY;
		}
		
		for(i = 0; i < RspView.ValidCount; i++) {
			BaseVa[i].FirstIndex = RspView.Range[i].FirstIndex;
			BaseVa[i].LastIndex = RspView.Range[i].LastIndex;
		}

		WriteFile(FileHandle, &RspView.ValidCount, sizeof(ULONG), &CompleteLength, NULL);
		Status = WriteFile(FileHandle, 
			               BaseVa, 
						   sizeof(RSP_VIEW_RANGE) * RspView.ValidCount, 
						   &CompleteLength, 
						   NULL);

		VirtualFree(BaseVa, 0, MEM_RELEASE);

		RspUnmapView(&Number);
		RspView.RangeCount = 0;

		//
		// Finally, write probe group into end of the index file.
		//

		RspWriteProbeGroup(FileHandle);
		RspWriteAddOnGroup(FileHandle);

		CloseHandle(FileHandle);
		CloseHandle(RspStorage.FileHandle);

		RspStorage.FileHandle = NULL;
		RspStorage.FileLength = 0;
		RspStorage.ValidLength = 0;
		RspStorage.FileName[0] = 0;
		RspStorage.Mode = RspNullMode;

		return ERROR_SUCCESS;
	}

	//
	// The file is opened in RspOpenMode mode, 
	// simply clean up all allocated resources.
	//
	
	VirtualFree(RspIndex.Entry, 0, MEM_RELEASE);
	RspIndex.LastValidIndex = RSP_INVALID_INDEX;

	RspUnmapView(&Number);
	VirtualFree(RspView.Range, 0, MEM_RELEASE);
	RspView.Range = NULL;
	RspView.RangeCount = 0;

	CloseHandle(RspStorage.FileHandle);

	RspStorage.Mode = RspNullMode;
	RspStorage.FileHandle = NULL;
	RspStorage.FileLength = 0;
	RspStorage.ValidLength = 0;
	RspStorage.FileName[0] = L'\0';

	return ERROR_SUCCESS;
}

BOOLEAN
RspIsCachedIndex(
	IN ULONG Index
	)
{
	PRSP_VIEW_RANGE Current;

	if (RspView.CurrentRange != RSP_INVALID_RANGE) {
		
		Current = &RspView.Range[RspView.CurrentRange];
		ASSERT(Current != NULL);

		if (Current->FirstIndex <= Index && Index <= Current->LastIndex) {
			return TRUE;
		}
	}

	return FALSE;
}

BOOLEAN
RspIsValidIndex(
	IN ULONG Index
	)
{
	if (Index <= RspIndex.LastValidIndex) {
		return TRUE;
	}

	return FALSE;
}

ULONG
RspComputeRange(
	IN ULONG Index
	)
{
	ULONG Range;
	ULONG FileOffset;

	if (RspIsValidIndex(Index) != TRUE) {
		return STATUS_RSP_INVALID_INDEX;
	}

	FileOffset = (ULONG)RspIndex.Entry[Index].FileOffset;
	Range = FileOffset / RSP_STORAGE_UNIT;

	return Range;
}

ULONG
RspCacheRange(
	IN ULONG FirstIndex,
	IN ULONG LastIndex
	)
{
	ULONG CacheRange;
	ULONG CurrentRange;

	if (RspIsValidIndex(FirstIndex) != TRUE) {
		return STATUS_RSP_INVALID_INDEX;
	}

	//
	// N.B. Compute range of view object which contains the specified
	// index, we only care about the first index, ignore the last index, 
	// typically, the last index falls into the range to be cached.
	//

	CurrentRange = RspView.CurrentRange;
	CacheRange = RspComputeRange(FirstIndex);

	if (CacheRange == CurrentRange) {
		return ERROR_SUCCESS;
	}

	RspUnmapView(&CurrentRange);
	RspMapView(CacheRange);

	return ERROR_SUCCESS;
}

ULONG
RspReadRecord(
	IN ULONG Index,
	IN PVOID Buffer,
	IN ULONG BufferLength
	)
{
	ULONG Status;
	ULONG Number;
	ULONG OldNumber;
	ULONG64 MappedOffset;
	
	Status = RspIsValidIndex(Index);
	if (Status != TRUE) {
		return STATUS_RSP_INVALID_INDEX;
	}

	if (RspIsCachedIndex(Index)) {
		Status = RspCopyRecord(Index, Buffer, BufferLength);
		return Status;
	}

	//
	// Compute new view range which contains the requested index 
	//

	MappedOffset = RspIndex.Entry[Index].FileOffset;
	Number = (ULONG)(MappedOffset / RSP_STORAGE_UNIT);

	//
	// Adjust view object to map the storage unit
	//

	Status = RspAdjustView(&RspView, &RspIndex, Number, &OldNumber);
	if (Status != ERROR_SUCCESS) {
		return Status;
	}

	Status = RspCopyRecord(Index, Buffer, BufferLength);
	return Status;	
}

PVOID
RspGetRecordPointer(
	IN ULONG Index
	)
{
	return NULL;	
}

ULONG
RspCopyRecord(
	IN ULONG Index,
	IN PVOID Buffer,
	IN ULONG BufferLength
	)
{
	ULONG Status;
	ULONG64 MappedOffset;
	ULONG AddressOffset;
	ULONG RecordLength;
	PVOID RecordAddress;
	PRSP_VIEW_RANGE Range;

	__try {

		ASSERT(RspView.CurrentRange != RSP_INVALID_RANGE);

		Range = &RspView.Range[RspView.CurrentRange];
		ASSERT(Range->FirstIndex != RSP_INVALID_INDEX);

		MappedOffset = RspIndex.Entry[Range->FirstIndex].FileOffset;
		ASSERT(MappedOffset % RSP_STORAGE_UNIT == 0);

		AddressOffset = (ULONG)(RspIndex.Entry[Index].FileOffset - MappedOffset);
		RecordLength = RspIndex.Entry[Index].Length;
		RecordAddress = (PCHAR)RspView.MappedVa + (ULONG)AddressOffset;
		
		if (RecordLength < BufferLength) {
			memcpy(Buffer, RecordAddress, RecordLength);
			Status = ERROR_SUCCESS;
		} else {
			Status = STATUS_RSP_BUFFER_LIMIT;
		}

	} 
	
	__except(EXCEPTION_EXECUTE_HANDLER) {
		DebugTrace("RspCopyRecord() exception occurs");
		Status = STATUS_RSP_EXCEPTION;
	}

	return Status;
}

ULONG
RspAdjustView(
	IN PRSP_VIEW ViewObject,
	IN PRSP_INDEX IndexObject,
	IN ULONG RangeNumber,
	OUT PULONG OldRangeNumber
	)
{
	ULONG Status;
	ULONG FileOffset;
	PVOID MappedVa;
	PRSP_VIEW_RANGE Range;

	ASSERT(ViewObject != NULL);
	ASSERT(ViewObject->RangeCount != 0);

	*OldRangeNumber = ViewObject->CurrentRange;

	Range = &ViewObject->Range[RangeNumber];
	FileOffset = (ULONG)IndexObject->Entry[Range->FirstIndex].FileOffset;

	if (ViewObject->CurrentRange != RSP_INVALID_RANGE) {
		Status = UnmapViewOfFile(ViewObject->MappedVa);
		if (Status != TRUE) {
			DebugTrace("STATUS_RSP_VIEW_FAILED, UnmapViewOfFile(), 0x%08x", GetLastError());
			return STATUS_RSP_VIEW_FAILED;
		}
	}

	MappedVa = MapViewOfFile(ViewObject->ViewHandle, 
		                     FILE_MAP_READ, 
							 0, 
							 FileOffset, 
							 RSP_STORAGE_UNIT);
	if (MappedVa != NULL) {
		ViewObject->CurrentRange = RangeNumber;
		ViewObject->MappedVa = MappedVa;
		return S_OK;
	}

	DebugTrace("STATUS_RSP_VIEW_FAILED, MapViewOfFile, 0x%08x", GetLastError());
	return STATUS_RSP_VIEW_FAILED;
}

ULONG
RspExpandStorage(
	VOID	
	)
{
	ULONG Status;
	ULONG64 Length;
	ULONG Number;

	Length = RspStorage.FileLength + RSP_STORAGE_UNIT;
	if (Length > RSP_STORAGE_CAPACITY) {
		return STATUS_RSP_STORAGE_FULL;
	}

	//
	// N.B. SetEndOfFile requires that all opened mapping view
	// are unmapped and closed.
	//
	
	Number = RSP_INVALID_RANGE;
	if (RspView.CurrentRange != RSP_INVALID_RANGE) {
		RspUnmapView(&Number);
	}

	Status = SetFilePointer(RspStorage.FileHandle, 
				            (ULONG)Length, 
						    NULL, 
						    FILE_BEGIN);	
	
	if (Status == INVALID_SET_FILE_POINTER) {
		return STATUS_RSP_STORAGE_FULL;
	}

	Status = SetEndOfFile(RspStorage.FileHandle);
	if (Status != TRUE) {
		return STATUS_RSP_STORAGE_FULL;
	}
	
	RspStorage.FileLength = Length;
	
	if (Number != RSP_INVALID_RANGE) {
		Status = RspMapView(Number);
		if (Status != ERROR_SUCCESS) {
			return Status;
		}
	}

	Number = (ULONG)(RspStorage.FileLength / RSP_STORAGE_UNIT);
	Number = Number - 1;

	RspView.Range[Number].FirstIndex = RspIndex.LastValidIndex + 1;
	RspView.Range[Number].LastIndex = RSP_INVALID_INDEX;
	RspView.ValidCount += 1;

	return ERROR_SUCCESS;
}

ULONG
RspMapView(
	IN ULONG RangeNumber
	)
{
	HANDLE Handle;
	PVOID BaseVa;
	ULONG64 FileOffset;
	PRSP_VIEW_RANGE Range;

	Handle = CreateFileMapping(RspStorage.FileHandle, 
		                       NULL, 
							   PAGE_READONLY, 
							   0, 
							   0, 
							   NULL);

	if (Handle == NULL) {
		return STATUS_RSP_VIEW_FAILED;
	}
	
	Range = &RspView.Range[RangeNumber];

	ASSERT(Range != NULL);
	ASSERT(RspIsValidIndex(Range->FirstIndex));
	ASSERT(RspIsValidIndex(Range->LastIndex));

	FileOffset = RspIndex.Entry[Range->FirstIndex].FileOffset;
	
	BaseVa = MapViewOfFile(Handle, 
		                   FILE_MAP_READ, 
						   0, 
						   (ULONG)FileOffset, 
						   RSP_STORAGE_UNIT);

	if (BaseVa == NULL) {
		return STATUS_RSP_VIEW_FAILED;
	}
	
	RspView.MappedVa = BaseVa;
	RspView.CurrentRange = RangeNumber;
	RspView.ViewHandle = Handle;

	return ERROR_SUCCESS;
}

ULONG
RspUnmapView(
	OUT PULONG RangeNumber 
	)
{
	ULONG Status;

	*RangeNumber = RspView.CurrentRange;

	Status = UnmapViewOfFile(RspView.MappedVa);
	if (Status != TRUE) {
		return STATUS_RSP_VIEW_FAILED;
	}

	CloseHandle(RspView.ViewHandle);

	RspView.MappedVa = NULL;
	RspView.ViewHandle = NULL;
	RspView.CurrentRange = RSP_INVALID_RANGE;

	return ERROR_SUCCESS;
}

ULONG
RspWriteRecord(
	IN PVOID Record,
	IN ULONG Length
	)
{
	ULONG Status;
	ULONG Index;
	ULONG CompleteLength;
	ULONG RequiredLength;
	ULONG RoundUpLength;
	ULONG Number;

	Index = RspIndex.LastValidIndex + 1;
	if (Index > RSP_INDEX_MAXIMUM) {
		return STATUS_RSP_INDEX_FULL;
	}

	RequiredLength = (ULONG)(RspStorage.ValidLength + Length);
	
	if (RequiredLength > RspStorage.FileLength) {

		if (RequiredLength > RSP_STORAGE_CAPACITY) {
			return STATUS_RSP_STORAGE_FULL;
		}	

		Status = RspExpandStorage();
		if (Status != ERROR_SUCCESS) {
			return STATUS_RSP_STORAGE_FULL;
		}

		//
		// N.B. Ensure the record to be written does not span storage unit	
		//

		RoundUpLength = (ULONG)RspRoundUpLength(RspStorage.ValidLength, RSP_STORAGE_UNIT);
		RspStorage.ValidLength = RoundUpLength;

	}

	
	//
	// Adjust file pointer to ValidLength, note that file offset is zero based,
	// so seek file pointer to current ValidLength is to set current file pointer
	// to next write location.
	//

	SetFilePointer(RspStorage.FileHandle, (ULONG)RspStorage.ValidLength, NULL, FILE_BEGIN);
	Status = WriteFile(RspStorage.FileHandle, Record, Length, &CompleteLength, NULL);
	if (Status != TRUE) {
		return STATUS_RSP_WRITE_FAILED;		
	}
			
	//
	// Build the new record's index entry
	//

	RspIndex.LastValidIndex += 1;
	RspIndex.Entry[RspIndex.LastValidIndex].FileOffset = RspStorage.ValidLength;
	RspIndex.Entry[RspIndex.LastValidIndex].Length = Length;
	RspIndex.Entry[RspIndex.LastValidIndex].FlagMask = RSP_INDEX_ALLOCATED;
	
	Number = (ULONG)(RspStorage.ValidLength / RSP_STORAGE_UNIT);
	ASSERT(Number <= RspView.RangeCount);
	ASSERT(RspView.Range[Number].FirstIndex != RSP_INVALID_INDEX);

	RspView.Range[Number].LastIndex = RspIndex.LastValidIndex;
	RspStorage.ValidLength += Length;

	//
	// N.B. The following decodes captured stack trace
	//

	SspGetStackTrace(Record);
	
	return ERROR_SUCCESS;		
}

ULONG
RspGetRecordCount(
	VOID
	)
{
	return (RspIndex.LastValidIndex + 1);	
}

BOOLEAN
RspIsCurrentStorage(
	IN PWSTR Path
	)
{
	ASSERT(Path != NULL);
	
	if (wcsicmp(RspStorage.FileName, Path) != 0) {
		return FALSE;
	}
	
	return TRUE;
}

BOOLEAN
RspRegisterCallback(
	IN RSP_DECODE_CALLBACK Callback
	)
{
	PLIST_ENTRY Entry;
	PBSP_CALLBACK Object;

	ASSERT(Callback != NULL);

	if (Callback == NULL) {
		return FALSE;
	}

	Entry = RspCallbackList.Flink;

	while (Entry != &RspCallbackList) {

		Object = CONTAINING_RECORD(Entry, BSP_CALLBACK, Entry);
		if (Object->Callback == (PVOID)Callback) {
			return TRUE;
		}

		Entry = Entry->Flink;
	}

	Object = (PBSP_CALLBACK)BspMalloc(sizeof(BSP_CALLBACK));
	Object->Callback = Callback;
	InsertTailList(&RspCallbackList, &Object->Entry);

	return TRUE;
}

BOOLEAN
RspRemoveCallback(
	IN RSP_DECODE_CALLBACK Callback
	)
{
	PLIST_ENTRY Entry;
	PBSP_CALLBACK Object;

	ASSERT(Callback != NULL);

	if (Callback == NULL) {
		return FALSE;
	}
	
	Entry = RspCallbackList.Flink;

	while (Entry != &RspCallbackList) {

		Object = CONTAINING_RECORD(Entry, BSP_CALLBACK, Entry);

		if (Object->Callback == (PVOID)Callback) {
			RemoveEntryList(&Object->Entry);
			BspFree(Object);
			return TRUE;
		}

		Entry = Entry->Flink;
	}

	return FALSE;
}

RSP_DECODE_CALLBACK
RspQueryDecode(
	IN PVOID Record
	)
{
	PLIST_ENTRY Entry;
	PVOID Status;
	PBSP_CALLBACK Object;
	RSP_DECODE_CALLBACK Routine;

	ASSERT(Record != NULL);

	Entry = RspCallbackList.Flink;
	while (Entry != &RspCallbackList) {

		Object = CONTAINING_RECORD(Entry, BSP_CALLBACK, Entry);
		ASSERT(Object->Callback != NULL);

		__try {
			Routine = (RSP_DECODE_CALLBACK)Object->Callback;
			Status = (*Routine)(Record, DecodeQuery);	
			if (Status != NULL) {
				return Object->Callback;
			}
		}
		__except(RspExceptionFilter(GetExceptionInformation())) {
			return NULL;
		}

		Entry = Entry->Flink;
	}

	return NULL;
}

LONG CALLBACK 
RspExceptionFilter(
	IN EXCEPTION_POINTERS *Pointers
	)
{
	DebugTrace("RspExceptionFilter()");
	return EXCEPTION_EXECUTE_HANDLER;
}


PRSP_PROBE
RspQueryProbeByName(
	IN ULONG ProcessId,
	IN PWSTR ModuleName,
	IN PWSTR ProcedureName,
	IN ULONG TypeFlag
	)
{
	PLIST_ENTRY Entry;
	PRSP_PROBE Object;
	PRSP_PROBE_GROUP Group;

	Group = RspQueryProbeGroup(ProcessId);

	if (Group != NULL) {
	
		Entry = Group->ProbeListHead.Flink;
		while (Entry != &Group->ProbeListHead) {
			Object = CONTAINING_RECORD(Entry, RSP_PROBE, Entry);
			if (wcscmp(ProcedureName, Object->ProcedureName) == 0 &&
				wcscmp(ModuleName, Object->ModuleName) == 0) {
				return Object;
			}
			Entry = Entry->Flink;
		}
		
	}

	return NULL;
}

PRSP_PROBE
RspQueryProbeByAddress(
	IN ULONG ProcessId,
	IN PVOID Address,
	IN ULONG TypeFlag 
	)
{
	PLIST_ENTRY Entry;
	PRSP_PROBE Object;
	PRSP_PROBE_GROUP Group;

	Group = RspQueryProbeGroup(ProcessId);

	if (Group != NULL) {
		Entry = Group->ProbeListHead.Flink;
		while (Entry != &Group->ProbeListHead) {
			Object = CONTAINING_RECORD(Entry, RSP_PROBE, Entry);
			if (Object->Address == Address && FlagOn(Object->Flag, TypeFlag)) {
				return Object;
			}
			Entry = Entry->Flink;
		}
	}

	return NULL;
}

PRSP_PROBE_GROUP
RspQueryProbeGroup(
	IN ULONG ProcessId
	)
{
	PLIST_ENTRY ListEntry;
	PRSP_PROBE_GROUP Group;

	ListEntry = RspProbeGroupList.Flink;

	while (ListEntry != &RspProbeGroupList) {
		
		Group = CONTAINING_RECORD(ListEntry, RSP_PROBE_GROUP, ListEntry);
		if (Group->ProcessId == ProcessId) {
			return Group;
		}

		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

PRSP_PROBE_GROUP 
RspCreateProbeGroup(
	IN PPSP_PROCESS PspProcess
	)
{
	PRSP_PROBE_GROUP Group;

	ASSERT(PspProcess->ProcessId != 0);

	Group = RspQueryProbeGroup(PspProcess->ProcessId);
	if (Group != NULL) {
		return Group;
	}

	Group = (PRSP_PROBE_GROUP)BspMalloc(sizeof(RSP_PROBE_GROUP));
	Group->ProcessId = PspProcess->ProcessId;
	Group->NumberOfProbes = 0;

	wcscpy(Group->ProcessName, PspProcess->Name);
	wcscpy(Group->FullPathName, PspProcess->FullPathName);
	
	InitializeListHead(&Group->ProbeListHead);

	InsertHeadList(&RspProbeGroupList, &Group->ListEntry);
	RspProbeGroupCount += 1;

	return Group;
}

ULONG
RspCommitProbe(
	IN PRSP_PROBE_GROUP Group,
	IN PPSP_PROBE Probe 
	)
{
	PRSP_PROBE RootProbe;
	PRSP_PROBE NextProbe;

	if (Probe->Result != ProbeCommit) {
		DebugBreak();
	}

	NextProbe = RspCreateProbe(Probe);
	RootProbe = RspQueryManualProbe(Group->ProcessId, 
		                            UlongToPtr(Probe->Address));
	if (RootProbe != NULL) {
		SetFlag(RootProbe->Flag, BTR_FLAG_MULTIVERSION);
		RspChainMultiVersionProbe(RootProbe, NextProbe);
	}
	else {
		InsertTailList(&Group->ProbeListHead, &NextProbe->Entry);
	}
		
	Probe->Result = ProbeNull;
	Group->NumberOfProbes += 1;	

	return S_OK;
}

ULONG
RspChainMultiVersionProbe(
	IN PRSP_PROBE RootProbe,
	IN PRSP_PROBE Subversion
	)
{
	PRSP_PROBE Last;
	PLIST_ENTRY ListEntry;

	ListEntry = RootProbe->VariantList.Flink;
	Last = CONTAINING_RECORD(ListEntry, RSP_PROBE, VariantList);

	SetFlag(Subversion->Flag, BTR_FLAG_VARIANT);
	Subversion->Version = Last->Version + 1;
	
	//
	// N.B. The latest version is always chained at head of
	// the VariantList field.
	//

	InsertHeadList(&RootProbe->VariantList, 
		           &Subversion->VariantList);

	return Subversion->Version;
}
	
PRSP_PROBE
RspCreateProbe(
	IN PPSP_PROBE PspProbe
	)
{
	PRSP_PROBE Probe;
	ULONG Flag, i;

	Probe = (PRSP_PROBE)BspMalloc(sizeof(RSP_PROBE));

	Probe->Address = (PVOID)PspProbe->Address;
	wcsncpy(Probe->ModuleName, PspProbe->ModuleName, 64);
	wcsncpy(Probe->ProcedureName, PspProbe->MethodName, 64);

	Flag = BTR_FLAG_MANUAL;

	if (PspProbe->SetReturn) {
		SetFlag(Flag, BTR_FLAG_SETRETURN);
		Probe->ReturnValue = PspProbe->ReturnValue;
	}

	if (PspProbe->SkipOriginalCall) {
		SetFlag(Flag, BTR_FLAG_SKIPCALL);
	}			

	Probe->Flag = Flag;
	Probe->ArgumentCount = PspProbe->ArgumentCount;
	
	for(i = 0; i < Probe->ArgumentCount; i++) {
		Probe->Parameters[i] = PspProbe->Parameters[i];
		wcsncpy(&Probe->ParameterName[i][0], &PspProbe->ParameterName[i][0], 15);
	}

	InitializeListHead(&Probe->VariantList);
	return Probe;
}

PRSP_PROBE
RspQueryManualProbe(
	IN ULONG ProcessId,
	IN PVOID Address
	)
{
	PRSP_PROBE Probe;

	Probe = RspQueryProbeByAddress(ProcessId, Address, BTR_FLAG_MANUAL);
	return Probe;
}

ULONG
RspLookupProcessName(
	IN ULONG ProcessId,
	IN PWCHAR Buffer,
	IN ULONG BufferLength
	)
{
	PRSP_PROBE_GROUP Group;
	PRSP_ADDON_GROUP AddOnGroup;

	Group = RspQueryProbeGroup(ProcessId);
	
	if (Group != NULL) {
		wcsncpy(Buffer, Group->ProcessName, BufferLength - 1);
		return (ULONG)wcslen(Buffer);
	}

	AddOnGroup = RspLookupAddOnGroup(ProcessId);
	if (AddOnGroup != NULL) {
		wcsncpy(Buffer, AddOnGroup->ProcessName, BufferLength - 1);
		return (ULONG)wcslen(Buffer);
	}

	wcsncpy(Buffer, L"N/A", BufferLength);
	return ERROR_SUCCESS;
}

ULONG
RspLookupProbeByAddress(
	IN ULONG ProcessId,
	IN PVOID Address,
	IN PWCHAR Buffer, 
	IN ULONG BufferLength
	)
{
	PRSP_PROBE Probe;
	
	Probe = RspQueryProbeByAddress(ProcessId, Address, BTR_FLAG_MANUAL);
	if (Probe != NULL) {
		wsprintf(Buffer, L"%ws (%ws)", Probe->ProcedureName, Probe->ModuleName);
		return 0;
	}

	Probe = RspQueryProbeByAddress(ProcessId, Address, BTR_FLAG_FILTER);
	if (Probe != NULL) {
		wsprintf(Buffer, L"%ws (%ws)", Probe->ProcedureName, Probe->ModuleName);
		return 0;
	}

	Probe = RspQueryProbeByAddress(ProcessId, Address, BTR_FLAG_FILTER);
	if (Probe != NULL) {
		wsprintf(Buffer, L"%ws (%ws)", Probe->ProcedureName, Probe->ModuleName);
		return 0;
	}

	wsprintf(Buffer, L"0x%08x", Address);
	return 0;
}

//
// N.B. The probe information is written to disk in the following layout:
// 1, count of probe group
// 2, probe group array  (each probe group records its probe object count)
// 3, probe object array (all groups' probe objects are in same array)
// 4, addon group array (all available addon full path name)
//

ULONG 
RspWriteProbeGroup(
	IN HANDLE FileHandle
	)
{
	ULONG CompleteBytes;
	ULONG OnDiskLength;
	PRSP_PROBE_GROUP Group;
	PRSP_PROBE Probe;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY Entry;

	if (!RspProbeGroupCount) {
		return 0;
	}

	WriteFile(FileHandle, &RspProbeGroupCount, sizeof(ULONG), &CompleteBytes, NULL);
	
	ListEntry = RspProbeGroupList.Flink;
	OnDiskLength = FIELD_OFFSET(RSP_PROBE_GROUP, ListEntry);	

	while (ListEntry != &RspProbeGroupList) {
		Group = CONTAINING_RECORD(ListEntry, RSP_PROBE_GROUP, ListEntry);
		WriteFile(FileHandle, Group, OnDiskLength, &CompleteBytes, NULL);
		ListEntry = ListEntry->Flink;
	}
	
	ListEntry = RspProbeGroupList.Flink;
	while (ListEntry != &RspProbeGroupList) {

		Group = CONTAINING_RECORD(ListEntry, RSP_PROBE_GROUP, ListEntry);

		Entry = Group->ProbeListHead.Flink;
		while (Entry != &Group->ProbeListHead) {

			Probe = CONTAINING_RECORD(Entry, RSP_PROBE, Entry);
			WriteFile(FileHandle, Probe, sizeof(RSP_PROBE), &CompleteBytes, NULL);

			if (FlagOn(Probe->Flag, BTR_FLAG_MULTIVERSION)) {

				//
				// N.B. For multi-version probe, it's written to disk in order of number
				//

				while (IsListEmpty(&Probe->VariantList) != TRUE) {
					PRSP_PROBE Subversion;
					PLIST_ENTRY Entry2;
					Entry2 = RemoveTailList(&Probe->VariantList);
					Subversion = CONTAINING_RECORD(Entry2, RSP_PROBE, VariantList);
					ASSERT(FlagOn(Subversion->Flag, BTR_FLAG_VARIANT));
					WriteFile(FileHandle, Subversion, sizeof(RSP_PROBE), &CompleteBytes, NULL);
				}

			}

			Entry = Entry->Flink;
		}

		ListEntry = ListEntry->Flink;
	}

	return 0;	
}

ULONG 
RspLoadProbeGroup(
	IN HANDLE FileHandle
	)
{
	ULONG CompleteBytes;
	ULONG i;
	ULONG OnDiskLength;
	PRSP_PROBE_GROUP Group;
	PRSP_PROBE Probe;
	PLIST_ENTRY ListEntry;

	InitializeListHead(&RspProbeGroupList);
	OnDiskLength = FIELD_OFFSET(RSP_PROBE_GROUP, ListEntry);	

	ReadFile(FileHandle, &RspProbeGroupCount, sizeof(ULONG), &CompleteBytes, NULL);

	for(i = 0; i < RspProbeGroupCount; i++) {
		Group = (PRSP_PROBE_GROUP)BspMalloc(sizeof(RSP_PROBE_GROUP));
		ReadFile(FileHandle, Group, OnDiskLength, &CompleteBytes, NULL);
		InsertTailList(&RspProbeGroupList, &Group->ListEntry);
	}

	ListEntry = RspProbeGroupList.Flink;

	while (ListEntry != &RspProbeGroupList) {

		Group = CONTAINING_RECORD(ListEntry, RSP_PROBE_GROUP, ListEntry);
		InitializeListHead(&Group->ProbeListHead);

		Probe = (PRSP_PROBE)VirtualAlloc(NULL, 
			                             sizeof(RSP_PROBE) * Group->NumberOfProbes, 
										 MEM_COMMIT, 
										 PAGE_READWRITE);

		ReadFile(FileHandle, Probe, sizeof(RSP_PROBE) * Group->NumberOfProbes, &CompleteBytes, NULL);
		
		for(i = 0; i < Group->NumberOfProbes; ) {
			
			InitializeListHead(&Probe[i].VariantList);

			if (FlagOn(Probe[i].Flag, BTR_FLAG_MULTIVERSION)) {
				
				ULONG Root;

				Root = i;
				InsertTailList(&Group->ProbeListHead, &Probe[i].Entry);
				
				i += 1;

				while (FlagOn(Probe[i].Flag, BTR_FLAG_VARIANT)) {
					InsertHeadList(&Probe[Root].VariantList, &Probe[i].VariantList);
					i += 1;
				}
			} 
			
			else {
				ASSERT(!FlagOn(Probe[i].Flag, BTR_FLAG_VARIANT));
				InsertTailList(&Group->ProbeListHead, &Probe[i].Entry);
				i += 1;
			}
			
		}
		
		ListEntry = ListEntry->Flink;
	}
	
	return 0;	
}

PRSP_PROBE
RspQueryProbeByRecord(
	IN PBTR_RECORD_BASE Record
	)
{
	PRSP_PROBE Probe;
	PLIST_ENTRY Entry;

	Probe = RspQueryProbeByAddress(Record->ProcessId,  
		                           Record->Address, 
								   BTR_FLAG_MANUAL);

	ASSERT(Probe != NULL);

	if (!FlagOn(Probe->Flag, BTR_FLAG_MULTIVERSION)) {
		return Probe;
	}

	Entry = Probe->VariantList.Flink;

	while (Entry != &Probe->VariantList) {
		Probe = CONTAINING_RECORD(Entry, RSP_PROBE, VariantList);
		if (Probe->Version == ((PBTR_MANUAL_RECORD)Record)->Version) {
			return Probe;				
		}
		Entry = Entry->Flink;
	}

	return NULL;
}

ULONG
RspDecodeRecord(
	IN PBTR_RECORD_BASE Record,
	IN RSP_DECODE_FIELD Field,
	IN PWCHAR Buffer,
	IN ULONG BufferLength
	)
{
	ULONG Status;

	Status = STATUS_SUCCESS;

	switch (Field) {

		case DecodeStartTime:

			//
			// N.B. Time need be computed based on EnterTime, ExitTime
			//

			break;

		case DecodeProcessId:
			wsprintf(Buffer, L"%d", Record->ProcessId);
			break;

		case DecodeProcessName:
			RspLookupProcessName(Record->ProcessId, Buffer, BufferLength);
			break;

		case DecodeThreadId:
			wsprintf(Buffer, L"%d", Record->ThreadId);
			break;

		case DecodeProbeId:
			RspLookupProbeByAddress(Record->ProcessId, 
				                    Record->Address, 
									Buffer, BufferLength);
			break;

		case DecodeReturn:
			break;

		case DecodeParameters:
			break;

		case DecodeOthers:
		case DecodeQuery:
			Buffer[0] = 0;
			Status = STATUS_RSP_INVALID_PARAMETER;
			break;

	}

	return Status;	
}

ULONG
RspDecodeReturn(
	IN PBTR_RECORD_BASE Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	StringCchPrintf(Buffer, Length, L"0x%p", Record->Return);
	return 0;
}

ULONG
RspDecodeParameters(
	IN PBTR_MANUAL_RECORD Record,
	IN PRSP_PROBE Probe,
	IN PWCHAR Buffer,
	IN ULONG Length,
	OUT PULONG ActualLength
	)
{
	WCHAR DecodeBuffer[2048];
	PWCHAR Pointer;
	ULONG i;
	ULONG UsedLength;
	ULONG TotalLength;
	PCHAR StringPointer;
	ULONG StringCount;
	ULONG StringLength;
	PBTR_ENCODED_ARGUMENT Value;

	if (Probe->ArgumentCount == 0) {
		wcscpy(Buffer, L"N/A");
		return 0;
	}

	Value = Record->Argument;
	Pointer = DecodeBuffer;
	Pointer[0] = 0;
	UsedLength = 0;
	StringCount = 0;

	StringPointer = (PCHAR)Record + FIELD_OFFSET(BTR_MANUAL_RECORD, Argument[Record->ArgumentCount]);
	
	for(i = 0; i < Probe->ArgumentCount; i++) {
		
		switch (Probe->Parameters[i]) {
			
			case ArgumentUInt8:
			case ArgumentUInt16:
			case ArgumentUInt32:
				wsprintf(Pointer, L"%ws=0x%x, ", Probe->ParameterName[i], (ULONG)Value[i].Value);
				UsedLength = (ULONG)wcslen(Pointer);
				Pointer += UsedLength;
				break;

			case ArgumentInt8:
			case ArgumentInt16:
			case ArgumentInt32:
				wsprintf(Pointer, L"%ws=0x%x, ", Probe->ParameterName[i], (LONG)Value[i].Value);
				UsedLength = (ULONG)wcslen(Pointer);
				Pointer += UsedLength;
				break;

			case ArgumentUInt64:
				wsprintf(Pointer, L"%ws=%I64u, ", Probe->ParameterName[i], (ULONG64)Value[i].Value);
				UsedLength = (ULONG)wcslen(Pointer);
				Pointer += UsedLength;
				break;

			case ArgumentInt64:
				wsprintf(Pointer, L"%ws=%I64d, ", Probe->ParameterName[i], (LONG64)Value[i].Value);
				UsedLength = (ULONG)wcslen(Pointer);
				Pointer += UsedLength;
				break;

			case ArgumentAnsi:
				wsprintf(Pointer, L"%ws=\"%S\", ", Probe->ParameterName[i], StringPointer);
				UsedLength = (ULONG)wcslen(Pointer);
				Pointer += UsedLength;
				StringLength = (ULONG)strlen(StringPointer) + 1;
				StringPointer += StringLength;
				break;

			case ArgumentUnicode:
				wsprintf(Pointer, L"%ws=\"%ws\", ", Probe->ParameterName[i], StringPointer);
				UsedLength = (ULONG)wcslen(Pointer);
				Pointer += UsedLength;
				StringLength = (ULONG)wcslen((PWCHAR)StringPointer) + 1;
				(PWCHAR)StringPointer += StringLength;
				break;

			case ArgumentPointer32:
				wsprintf(Pointer, L"%ws=%p, ", Probe->ParameterName[i], (LONG)Value[i].Value);
				UsedLength = (ULONG)wcslen(Pointer);
				Pointer += UsedLength;
				break;

			case ArgumentPointer64:
				wsprintf(Pointer, L"%ws=%I64p, ", Probe->ParameterName[i], (LONG64)Value[i].Value);
				UsedLength = (ULONG)wcslen(Pointer);
				Pointer += UsedLength;
				break;

			default:
				ASSERT(0);
		}
	}

	TotalLength = (ULONG)wcslen(DecodeBuffer);
	if (TotalLength > Length) {
		return STATUS_RSP_BUFFER_LIMIT;
	}

	//
	// N.B. The following zero out the last ', ' of the parameter list
	//

	if (TotalLength > 2) {
		DecodeBuffer[TotalLength - 2] = 0;
		StringCchCopy(Buffer, Length - 1, DecodeBuffer);
		Buffer[Length - 1] = 0;
	} else {
		Buffer[0] = 0;
	}
	
	return STATUS_SUCCESS;
}

ULONG
RspLookupAddOnProbeByOrdinal(
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
		wcsncpy(Buffer, AddOn->Probes[Record->ProbeOrdinal].ProbeProcedure, Length);
		Status = S_OK;
	} else {
		StringCchCopy(Buffer, Length, L"No AddOn Object");
	}
	
	return Status;
}

ULONG
RspDecodeAddOnParameters(
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
		Decode = AddOn->Probes[Record->ProbeOrdinal].DecodeProcedure;
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
RspCommitAddOnGroup(
	IN PPSP_COMMAND Command
	)
{
	PLIST_ENTRY ListEntry;
	PRSP_ADDON_GROUP Group;
	PPSP_PROCESS Process;

	Process = Command->Process;
	ListEntry = RspAddOnGroupList.Flink;
	
	while (ListEntry != &RspAddOnGroupList) {
		Group = CONTAINING_RECORD(ListEntry, RSP_ADDON_GROUP, ListEntry);
		if (Group->ProcessId == Process->ProcessId) {
			return S_OK;
		}
		ListEntry = ListEntry->Flink;
	}

	Group = (PRSP_ADDON_GROUP)BspMalloc(sizeof(RSP_ADDON_GROUP));

	Group->ProcessId = Process->ProcessId;
	wcscpy(Group->ProcessName, Process->Name);
	wcscpy(Group->FullPathName, Process->FullPathName);

	InsertTailList(&RspAddOnGroupList, &Group->ListEntry);

	return S_OK;
}

PRSP_ADDON_GROUP
RspLookupAddOnGroup(
	IN ULONG ProcessId
	)
{
	PLIST_ENTRY ListEntry;
	PRSP_ADDON_GROUP Group;

	ListEntry = RspAddOnGroupList.Flink;
	
	while (ListEntry != &RspAddOnGroupList) {
		Group = CONTAINING_RECORD(ListEntry, RSP_ADDON_GROUP, ListEntry);
		if (Group->ProcessId == ProcessId) {
			return Group;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

//
// Write both manual probe group and add on group
//

ULONG
PspWriteProbeGroup(
	IN HANDLE FileHandle
	)
{
	return S_OK;
}

ULONG
RspWriteAddOnGroup(
	IN HANDLE FileHandle
	)
{
	PLIST_ENTRY ListEntry;
	PRSP_ADDON_GROUP AddOn;
	ULONG Number;
	ULONG CompleteBytes;
	LONG Distance;

	Number = 0;
	SetFilePointer(FileHandle, sizeof(ULONG), NULL, FILE_CURRENT);

	ListEntry = RspAddOnGroupList.Flink;

	while (ListEntry != &RspAddOnGroupList) {
		
		AddOn = CONTAINING_RECORD(ListEntry, RSP_ADDON_GROUP, ListEntry);
		WriteFile(FileHandle, AddOn, sizeof(RSP_ADDON_GROUP), &CompleteBytes, NULL);	

		Number += 1;
		ListEntry = ListEntry->Flink;

	}

	//
	// Move file pointer back to Number position
	//
	Distance = (LONG)(sizeof(RSP_ADDON_GROUP) * Number + sizeof(ULONG));
	SetFilePointer(FileHandle, -Distance, NULL, FILE_CURRENT);
	WriteFile(FileHandle, &Number, sizeof(ULONG), &CompleteBytes, NULL);

	SetFilePointer(FileHandle, sizeof(RSP_ADDON_GROUP) * Number, NULL, FILE_CURRENT);
	return 0;
}

ULONG
RspLoadAddOnGroup(
	IN HANDLE FileHandle
	)
{
	PRSP_ADDON_GROUP AddOn;
	ULONG i, Number;
	ULONG CompleteBytes;

	InitializeListHead(&RspAddOnGroupList);
	ReadFile(FileHandle, &Number, sizeof(ULONG), &CompleteBytes, NULL);
	
	for (i = 0; i < Number; i++) {
		AddOn = (PRSP_ADDON_GROUP)BspMalloc(sizeof(RSP_ADDON_GROUP));
		ReadFile(FileHandle, AddOn, sizeof(RSP_ADDON_GROUP), &CompleteBytes, NULL);
		InsertTailList(&RspAddOnGroupList, &AddOn->ListEntry);
	}

	return 0;
}

ULONG
RspCreateIndexCache(
	IN PWSTR Path,
	IN PRSP_VIEW ViewObject,
	IN PRSP_INDEX IndexObject
	)
{
	WCHAR Buffer[MAX_PATH];
	WCHAR Drive[MAX_PATH];
	WCHAR Folder[MAX_PATH];
	WCHAR File[MAX_PATH];
	WCHAR Type[MAX_PATH];
	HANDLE FileHandle;
	HANDLE ViewHandle;
	ULONG Status;
	PVOID MappedVa;

	_wsplitpath(Path, Drive, Folder, File, Type);
	_wmakepath(Buffer, Drive, Folder, File, L".dti");

	Status = S_OK;
	
	FileHandle = CreateFile(Buffer,
	                        GENERIC_READ | GENERIC_WRITE,
	                        0, 
	                        NULL,
	                        CREATE_NEW,
	                        FILE_ATTRIBUTE_NORMAL,
	                        NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		DebugTrace("STATUS_RSP_CREATEFILE_FAILED, CreateFile(%ws), 0x%08x", 
			       Buffer, GetLastError());
		return STATUS_RSP_CREATEFILE_FAILED;
	}
	
	SetFilePointer(FileHandle, RSP_STORAGE_UNIT, 0, FILE_BEGIN);
	SetEndOfFile(FileHandle);

	ViewHandle = CreateFileMapping(FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (ViewHandle == NULL) {
		DebugTrace("STATUS_RSP_CREATEMAPPING_FAILED, CreateFileMapping(%ws), 0x%08x", 
			        Path, GetLastError());
		return STATUS_RSP_CREATEMAPPING_FAILED;
	}

	ViewObject->ViewHandle = ViewHandle;
	ViewObject->MappedVa = MappedVa;
	ViewObject->RangeCount = 1;
	ViewObject->ValidCount = 0;
	ViewObject->CurrentRange = -1;
	return Status;	
}