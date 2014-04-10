//
// Apsara Labs
// lan.john@gmail.com
// Copyrigth(C) 2009-2012
//

#include "bsp.h"
#include "mspdtl.h"
#include "msputility.h"
#include "mdb.h"

HANDLE MspDecodeHandle = NULL;

DECLSPEC_CACHEALIGN 
ULONG MspStackTraceBarrier = 0;

PMSP_STACKTRACE_CONTEXT
MspLookupStackTraceContext(
	IN PMSP_STACKTRACE_OBJECT Object,
	IN ULONG ProcessId,
	IN BOOLEAN CreateIfNo
	)
{
	PLIST_ENTRY ListEntry;
	PMSP_STACKTRACE_CONTEXT Context;
	HANDLE Handle;

	ListEntry = Object->ContextListHead.Flink;
	while (ListEntry != &Object->ContextListHead) {
	
		Context = CONTAINING_RECORD(ListEntry, MSP_STACKTRACE_CONTEXT, ListEntry);	
		if (Context->ProcessId == ProcessId) {
			return Context;
		}

		ListEntry = ListEntry->Flink;
	}

	if (CreateIfNo) {
	
		Handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
		if (!Handle) {
			return NULL;
		}

		Context = (PMSP_STACKTRACE_CONTEXT)MspMalloc(sizeof(MSP_STACKTRACE_CONTEXT));
		Context->ProcessId = ProcessId;
		Context->ProcessHandle = Handle;

		InsertTailList(&Object->ContextListHead, &Context->ListEntry);
		return Context;
	}

	return NULL;
}

BOOLEAN
MspAcquireContext(
	IN PMSP_STACKTRACE_OBJECT Object,
	IN ULONG ProcessId
	)
{
	PMSP_STACKTRACE_CONTEXT Old;
	PMSP_STACKTRACE_CONTEXT New;
	WCHAR SymbolPath[MAX_PATH * 2];

	Old = Object->Context;
	if (Old != NULL) {
		if (Old->ProcessId == ProcessId) {
			return TRUE;
		}
		else {
			SymCleanup(Old->ProcessHandle);
		}
	}
	
	New = MspLookupStackTraceContext(Object, ProcessId, TRUE);
	if (!New) {
		__debugbreak();
		return FALSE;
	}
	
	ASSERT(New->ProcessHandle != NULL);

	MdbGetSymbolPath(SymbolPath, MAX_PATH * 2);
	SymInitializeW(New->ProcessHandle, SymbolPath, TRUE);
	SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | 
		          SYMOPT_INCLUDE_32BIT_MODULES | SYMOPT_LOAD_LINES | 
				  SYMOPT_OMAP_FIND_NEAREST);

	Object->Context = New;
	return TRUE;
}

VOID
MspReleaseContext(
	IN PMSP_STACKTRACE_OBJECT Object,
	IN BOOLEAN Clear	
	)
{
	PMSP_STACKTRACE_CONTEXT Current;

	if (!Clear) {
		return;
	}

	Current = Object->Context;
	if (Current != NULL) {
		SymCleanup(Current->ProcessHandle);
		Object->Context = NULL;
	}
}

ULONG
MspCreateStackTrace(
	IN PMSP_STACKTRACE_OBJECT Object
	)
{
	ULONG Size;
	PMSP_MAPPING_OBJECT MappingObject;
	PMSP_FILE_OBJECT FileObject;
	HANDLE FileHandle, MappingHandle;
	PVOID Address;
	WCHAR Path[MAX_PATH];
	PVOID Buffer;

	InitializeListHead(&Object->ContextListHead);
	MspInitCsLock(&Object->Lock, 4000);
	Object->Context = NULL;

	//
	// Create strack trace file
	//

	StringCchPrintf(Path, MAX_PATH, L"%s\\{%s}.s", MspSessionPath, MspSessionUuidString);
	FileHandle = CreateFile(Path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, 
							CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		return MSP_STATUS_CREATEFILE;
	}

	Size = MSP_FIXED_BUCKET_NUMBER * sizeof(MSP_STACKTRACE_ENTRY);
	Size = MspUlongRoundUp(Size, 1024 * 64);

	SetFilePointer(FileHandle, Size, NULL, FILE_BEGIN);
	SetEndOfFile(FileHandle);

	MappingHandle = CreateFileMapping(FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!MappingHandle) {
		CloseHandle(FileHandle);
		DeleteFile(Path);
		return MSP_STATUS_CREATEFILEMAPPING;
	}

	Address = MapViewOfFile(MappingHandle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, Size);
	if (!Address) {
		CloseHandle(MappingHandle);
		CloseHandle(FileHandle);
		DeleteFile(Path);
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
	FileObject->FileLength = Size;
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
	Object->EntryTotalBuckets = MSP_FIXED_BUCKET_NUMBER;
	Object->EntryFixedBuckets = MSP_FIXED_BUCKET_NUMBER;
	Object->u1.Buckets = (PMSP_STACKTRACE_ENTRY)MappingObject->MappedVa;

	//
	// Create extend mapping object for entry
	//

	MappingHandle = CreateFileMapping(FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!MappingHandle) {
		CloseHandle(FileHandle);
		DeleteFile(Path);
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

	StringCchPrintf(Path, MAX_PATH, L"%s\\{%s}.t", MspSessionPath, MspSessionUuidString);
	FileHandle = CreateFile(Path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, 
							CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		return MSP_STATUS_CREATEFILE;
	}

	Size = MSP_FIXED_BUCKET_NUMBER * sizeof(MSP_STACKTRACE_TEXT);
	Size = MspUlongRoundUp(Size, 1024 * 64);

	SetFilePointer(FileHandle, Size, NULL, FILE_BEGIN);
	SetEndOfFile(FileHandle);

	MappingHandle = CreateFileMapping(FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!MappingHandle) {
		CloseHandle(FileHandle);
		DeleteFile(Path);
		return MSP_STATUS_CREATEFILEMAPPING;
	}

	Address = MapViewOfFile(MappingHandle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, Size);
	if (!Address) {
		CloseHandle(MappingHandle);
		CloseHandle(FileHandle);
		DeleteFile(Path);
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
	FileObject->Type = FileStackText;
	FileObject->FileLength = Size;
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
	Object->TextTotalBuckets = MSP_FIXED_BUCKET_NUMBER;
	Object->TextFixedBuckets = MSP_FIXED_BUCKET_NUMBER;
	Object->u2.Buckets = (PMSP_STACKTRACE_TEXT)MappingObject->MappedVa;

	//
	// Create extend mapping object for text 
	//

	MappingHandle = CreateFileMapping(FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!MappingHandle) {
		CloseHandle(FileHandle);
		DeleteFile(Path);
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
	// Initialize bitmap to scan free first level buckets, note that after
	// all first level buckets are filled, we don't use bitmap anymore,
	// new bucket is allocated by plus pointer one by one.
	//

	Size = MspUlongRoundUp(MSP_FIXED_BUCKET_NUMBER, 8);
	Buffer = MspMalloc(Size);
	RtlZeroMemory(Buffer, Size);

	BtrInitializeBitMap(&Object->EntryBitmap, Buffer, MSP_FIXED_BUCKET_NUMBER);

	Buffer = MspMalloc(Size);
	RtlZeroMemory(Buffer, Size);
	BtrInitializeBitMap(&Object->TextBitmap, Buffer, MSP_FIXED_BUCKET_NUMBER);

	////
	//// Create background stack decode thread
	////

	//MspDecodeHandle = CreateThread(NULL, 0, MspDecodeProcedure, 
	//	                           NULL, 0, NULL);
	//if (!MspDecodeHandle) {
	//	return GetLastError();
	//}

	return MSP_STATUS_OK;
}

BOOLEAN
MspInsertStackTrace(
	IN PMSP_STACKTRACE_OBJECT Object,
	IN PBTR_STACKTRACE_ENTRY Trace,
	IN ULONG ProcessId
	)
{
	ULONG Bucket;
	ULONG PriorBucket;
	PMSP_STACKTRACE_ENTRY Entry;
	PMSP_STACKTRACE_ENTRY PriorEntry; 

	//
	// N.B. The bucket after hash is always bounded into
	// range of fixed buckets.
	//

	Bucket = MSP_HASH_BUCKET(Trace->Hash);

	//
	// N.B. Two stack trace are idential if they have same hash, same depth,
	// and, have same frame values.
	//

	Entry = Object->u1.Buckets + Bucket;
	if (Entry->Hash == 0 && Entry->Depth == 0 && Entry->Count == 0) {

		ASSERT(!BtrTestBit(&Object->EntryBitmap, Bucket));

		//
		// This is an empty bucket we can use
		//

		Entry->ProcessId  = ProcessId;
		Entry->Hash = Trace->Hash;
		Entry->Count = Trace->Count;
		Entry->Depth = (USHORT)Trace->Depth;
		Entry->Flag = 0;
		Entry->Probe = Trace->Probe;
		Entry->Next = -1;

		RtlCopyMemory(Entry->Frame, Trace->Frame, sizeof(PVOID) * Trace->Depth);

		Object->EntryFirstHitBuckets += 1;
		Object->EntryFilledFixedBuckets += 1;
		BtrSetBit(&Object->EntryBitmap, Bucket);

		if (Object->EntryFilledFixedBuckets == Object->EntryFixedBuckets) {
			Object->EntryFixedBucketsFull = TRUE;
		}

		//
		// N.B. When insert a new stack trace entry, we immediately decode it,
		// this will cause CPU spikes for some time, after process's work load
		// get stable, typically the spikes will get disappeared, since most
		// process's work load has been decoded.
		//

		//MspDecodeStackTrace(Object, Entry);
		return TRUE;
	}

	//
	// Walk the chained buckets if any
	//

	do {

		if (Entry->Hash == Trace->Hash && Entry->Depth == Trace->Depth 
			                           && Entry->Probe == Trace->Probe) {
			return FALSE;
		}
		
		PriorBucket = Bucket;
		PriorEntry = Entry;

		Bucket = Entry->Next;
		Entry = MspGetStackTraceEntry(Object, Bucket);
	}
	while (Entry != NULL);

	if (Bucket != -1) {
		ASSERT(0);
		return FALSE;
	}

	//
	// No luck to hit, allocate new bucket to hold the stack trace
	// MspAllocateExtendEntryBucket is responsible to set prior bucket's next
	// field to point to new bucket, here we assume it's done already after
	// it return
	//

	if (!Object->EntryFixedBucketsFull) {

		//
		// N.B. This indicates that the fixed buckets are possible" not full,
		// we allocate by use bitmap, we try to allocate as near as
		// possible to prior bucket to keep cache locality when walking
		// chained buckets.
		//

		Bucket = BtrFindFirstClearBit(&Object->EntryBitmap, PriorBucket);
		if (Bucket == -1) {
			ASSERT(PriorBucket != 0);
			Bucket = BtrFindFirstClearBitBackward(&Object->EntryBitmap, PriorBucket);
		} 
		
		if (Bucket != -1) { 
			BtrSetBit(&Object->EntryBitmap, Bucket);
		} else  {
			ASSERT(0);
		}

		Entry = Object->u1.Buckets + Bucket;

		//
		// N.B. It's safe to access PriorEntry here, because fixed buckets are not full.
		//

		PriorEntry->Next = Bucket;
		Entry->ProcessId  = ProcessId;
		Entry->Hash = Trace->Hash;
		Entry->Count = Trace->Count;
		Entry->Depth = (USHORT)Trace->Depth;
		Entry->Flag = 0;
		Entry->Probe = Trace->Probe;
		Entry->Next = -1;

		RtlCopyMemory(Entry->Frame, Trace->Frame, sizeof(PVOID) * Trace->Depth);

		Object->EntryFilledFixedBuckets += 1;
		Object->EntryChainedBuckets += 1;

		if (Object->EntryFilledFixedBuckets == Object->EntryFixedBuckets) {
			Object->EntryFixedBucketsFull = TRUE;
		}

		//MspDecodeStackTrace(Object, Entry);
		return TRUE;
	}

	else {
		Entry = MspAllocateExtendEntryBucket(Object, &Bucket);
	}

	Entry->ProcessId  = ProcessId;
	Entry->Hash = Trace->Hash;
	Entry->Count = Trace->Count;
	Entry->Depth = (USHORT)Trace->Depth;
	Entry->Flag = 0;
	Entry->Probe = Trace->Probe;
	Entry->Next = -1;

	RtlCopyMemory(Entry->Frame, Trace->Frame, sizeof(PVOID) * Trace->Depth);

	//
	// N.B. We must get prior entry address via MspGetStackTraceEntry,
	// because a single view can not guarantee that 2 entries are included.
	//

	PriorEntry = MspGetStackTraceEntry(Object, PriorBucket);
	PriorEntry->Next = Bucket;

	//MspDecodeStackTrace(Object, Entry);
	return TRUE;
}

#define IS_BUCKET_MAPPED(_M, _B) \
	(_B <= _M->LastSequence && _B >= _M->FirstSequence)

#define MAXIMUM_STACK_ENTRY_BUCKET(_M) \
	((_M->FileLength / sizeof(MSP_STACKTRACE_ENTRY)) - 1)

#define MAXIMUM_STACK_TEXT_BUCKET(_M) \
	((_M->FileLength / sizeof(MSP_STACKTRACE_TEXT)) - 1)

#define BUCKET_TO_STACK_ENTRY(_M, _B) \
	((PMSP_STACKTRACE_ENTRY)_M->MappedVa + _B - _M->FirstSequence)

#define BUCKET_TO_STACK_TEXT(_M, _B) \
	((PMSP_STACKTRACE_TEXT)_M->MappedVa + _B - _M->FirstSequence)

#define ENTRY_BUCKETS_PER_MAP(_E, _S) \
	(ULONG)(((_E - _S)/ sizeof(MSP_STACKTRACE_ENTRY)))

#define TEXT_BUCKETS_PER_MAP(_E, _S) \
	(ULONG)(((_E - _S)/ sizeof(MSP_STACKTRACE_TEXT)))


PMSP_STACKTRACE_ENTRY	
MspAllocateExtendEntryBucket(
	IN PMSP_STACKTRACE_OBJECT Object,
	OUT PULONG NewBucket
	)
{
	ULONG64 Length;
	ULONG Status;
	ULONG Bucket;
	ULONG64 Start;
	ULONG64 End;
	PMSP_FILE_OBJECT FileObject;
	PMSP_MAPPING_OBJECT MappingObject;
	PMSP_STACKTRACE_ENTRY Entry;

	FileObject = Object->EntryObject;
	MappingObject = FileObject->Mapping;
	Bucket = Object->EntryTotalBuckets;

	if (IS_BUCKET_MAPPED(MappingObject, Bucket)) {
		*NewBucket = Bucket;
		Object->EntryTotalBuckets += 1;
		Object->EntryChainedBuckets += 1;
		return BUCKET_TO_STACK_ENTRY(MappingObject, Bucket);
	}

	//
	// N.B. This indicate that the file object's mapping is out of current bucket range,
	// we need use extended mapping object
	//

	MappingObject = Object->EntryExtendMapping;
	ASSERT(MappingObject != NULL);

	if (MappingObject->MappedVa) {
		if (IS_BUCKET_MAPPED(MappingObject, Bucket)) {
			*NewBucket = Bucket;
			Object->EntryTotalBuckets += 1;
			Object->EntryChainedBuckets += 1;
			return BUCKET_TO_STACK_ENTRY(MappingObject, Bucket);
		}
		else {
			UnmapViewOfFile(MappingObject->MappedVa);
			MappingObject->MappedVa = NULL;
			MappingObject->MappedOffset = 0;
			MappingObject->MappedLength = 0;
		}
	}


repeat:

	if (Bucket < MAXIMUM_STACK_ENTRY_BUCKET(MappingObject)) {

		//
		// N.B. MSP_STACKTRACE_ENTRY is guaranteed to be bounded by map increment,
		// so round down is enough to include it.
		//

		Start = MspUlong64RoundDown(Bucket * sizeof(MSP_STACKTRACE_ENTRY), 
			                         MSP_STACKTRACE_MAP_INCREMENT);
		End = Start + MSP_STACKTRACE_MAP_INCREMENT;
		End = min(End, MappingObject->FileLength);

		MappingObject->MappedVa = MapViewOfFile(MappingObject->MappedObject, 
			                                    FILE_MAP_READ|FILE_MAP_WRITE,
												(DWORD)(Start >> 32),
												(DWORD)Start,
												(SIZE_T)(End - Start));

		if (!MappingObject->MappedVa) {
			Status = GetLastError();
			ASSERT(0);
			return NULL;
		}

		MappingObject->FirstSequence = (ULONG)(Start / sizeof(MSP_STACKTRACE_ENTRY));
		MappingObject->LastSequence = MappingObject->FirstSequence + ENTRY_BUCKETS_PER_MAP(End, Start) - 1;
		MappingObject->MappedOffset = Start;
		MappingObject->MappedLength = (ULONG)(End - Start);

		Entry = BUCKET_TO_STACK_ENTRY(MappingObject, Bucket);
		Entry->Next = -1;
		*NewBucket = Bucket;
		
		Object->EntryTotalBuckets += 1;
		Object->EntryChainedBuckets += 1;

		return Entry;
	}

	CloseHandle(MappingObject->MappedObject);

	Length = FileObject->FileLength + MSP_STACKTRACE_FILE_INCREMENT;
	Status = MspExtendFileLength(FileObject->FileObject, Length);
	if (Status != MSP_STATUS_OK) {
		return NULL;
	}

	MappingObject->MappedObject = CreateFileMapping(FileObject->FileObject, 
													NULL, PAGE_READWRITE, 
													(DWORD)(Length >> 32),
													(DWORD)Length, NULL);
	if (!MappingObject->MappedObject) {
		return NULL;
	}

	//
	// N.B Update file AND extended mapping object's length
	//

	FileObject->FileLength = Length;
	MappingObject->FileLength = Length;

	goto repeat;
	return NULL;
}

PMSP_STACKTRACE_ENTRY
MspGetStackTraceEntry(
	IN PMSP_STACKTRACE_OBJECT Object,
	IN ULONG Bucket
	)
{
	PMSP_MAPPING_OBJECT MappingObject;
	ULONG64 Start, End;

	//
	// Ensure entry is bounded
	//

	if (Bucket == -1) {
		return NULL;
	}

	if (Bucket >= Object->EntryTotalBuckets) {
		return NULL;
	}

	//
	// If the bucket is already mapped, return it,
	// note that the file object's mapping object is
	// always live after creation.
	//

	MappingObject = Object->EntryObject->Mapping;
	if (IS_BUCKET_MAPPED(MappingObject, Bucket)) {
		return Object->u1.Buckets + Bucket;
	}	

	MappingObject = Object->EntryExtendMapping;
	ASSERT(MappingObject != NULL);
	ASSERT(MappingObject->MappedVa != NULL);

	if (IS_BUCKET_MAPPED(MappingObject, Bucket)) {
		return BUCKET_TO_STACK_ENTRY(MappingObject, Bucket);
	}
	
	UnmapViewOfFile(MappingObject->MappedVa);
	
	Start = Bucket * sizeof(MSP_STACKTRACE_ENTRY);
	Start = MspUlong64RoundDown(Start, MSP_STACKTRACE_MAP_INCREMENT);
	End = Start + MSP_STACKTRACE_MAP_INCREMENT;

	//
	// Make sure the End make sense
	//

	End = min(End, MappingObject->FileLength);

	MappingObject->MappedVa = MapViewOfFile(MappingObject->MappedObject, FILE_MAP_READ|FILE_MAP_WRITE,
		                                    (DWORD)(Start >> 32), (DWORD)Start, (SIZE_T)(End - Start));
		         
	if (!MappingObject->MappedVa) {
		return NULL;
	}

	MappingObject->MappedOffset = Start;
	MappingObject->MappedLength = (ULONG)(End - Start);
	MappingObject->FirstSequence = (ULONG)(Start / sizeof(MSP_STACKTRACE_ENTRY));
	MappingObject->LastSequence = (ULONG)(End / sizeof(MSP_STACKTRACE_ENTRY)) - 1;

	return BUCKET_TO_STACK_ENTRY(MappingObject, Bucket);
}

PMSP_STACKTRACE_ENTRY
MspLookupStackTrace(
	IN PMSP_STACKTRACE_OBJECT Object,
	IN PVOID Probe,
	IN ULONG Hash,
	IN ULONG Depth
	)
{
	ULONG Bucket;
	PMSP_STACKTRACE_ENTRY Entry;

	if (Object->EntryFirstHitBuckets == 0) {
		return NULL;
	}

	Bucket = MSP_HASH_BUCKET(Hash);
	Entry = MspGetStackTraceEntry(Object, Bucket);

	while (Entry != NULL && Entry->Probe != NULL) {
		if (Entry->Probe == Probe && Entry->Hash == Hash && Entry->Depth == Depth) {
			return Entry;
		}
		Entry = MspGetStackTraceEntry(Object, Entry->Next);
	}

	return NULL;
}

ULONG 
MspCompareMemoryPointer(
	IN PVOID Source,
	IN PVOID Destine,
	IN ULONG NumberOfPointers
	)
{
	ULONG Number;

	for(Number = 0; Number < NumberOfPointers; Number += 1) {
		if (*(PULONG_PTR)Source != *(PULONG_PTR)Destine) {
			return Number;
		}
	}

	return Number;
}

ULONG
MspDecodeModuleName(
	IN PMSP_STACKTRACE_OBJECT Object,
	IN ULONG64 Address,
	IN PCHAR NameBuffer,
	IN ULONG BufferLength
	)
{
	PIMAGEHLP_MODULE64 Module;
	ULONG64 Buffer[sizeof(IMAGEHLP_MODULE64) / sizeof(ULONG64) + 1];
	ULONG Status;
	PMSP_STACKTRACE_CONTEXT Context;

	Context = Object->Context;
	ASSERT(Context != NULL);

	Module = (PIMAGEHLP_MODULE64)Buffer;
	Module->SizeOfStruct = BspRoundUp(sizeof(IMAGEHLP_MODULE64), 8);
	Status = SymGetModuleInfo64(Context->ProcessHandle, 
		                        Address,
								Module);
	if (Status != TRUE) {

		//
		// N.B. This may be improved by walk module list of target process
		//

		strncpy(NameBuffer, "<Unknown>", BufferLength);
	}
	else {
		strncpy(NameBuffer, Module->ModuleName, BufferLength);
	}

	strlwr(NameBuffer);
	return MSP_STATUS_OK;
}

PMSP_STACKTRACE_ENTRY
MspDecodeStackTrace(
	IN PMSP_STACKTRACE_OBJECT Object,
	IN PMSP_STACKTRACE_ENTRY Trace
	)
{
	ULONG Depth;
	ULONG Status;
	ULONG Length;
	ULONG64 Address;
	ULONG64 Displacement;
	ULONG64 Buffer[(sizeof(IMAGEHLP_SYMBOL64) / sizeof(ULONG64)) + 64];
	CHAR StackTrace[MAX_STACKTRACE_TEXT];
	CHAR ModuleName[64];
	PIMAGEHLP_SYMBOL64 Symbol;
	ULONG DecodedDepth;
	ULONG64 ModuleBase;
	PMSP_STACKTRACE_CONTEXT Context;
	ULONG Bucket;

	Context = Object->Context;
	ASSERT(Context != NULL);

	RtlZeroMemory(StackTrace, MAX_STACKTRACE_TEXT);
	Symbol = (PIMAGEHLP_SYMBOL64)Buffer;
	DecodedDepth = 0;

	for(Depth = 0; Depth < Trace->Depth; Depth += 1) {
		
		Address = (ULONG64) Trace->Frame[Depth];
		RtlZeroMemory(Buffer, sizeof(Buffer));
		Symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
		Symbol->MaxNameLength = 128;

		Status = SymGetSymFromAddr64(Context->ProcessHandle, 
									 Address, 
									 &Displacement, 
									 Symbol);

		MspDecodeModuleName(Object, Address, ModuleName, 64);
		if (Status != TRUE) {
			ModuleBase = SymGetModuleBase64(Context->ProcessHandle, Address);
			Displacement = Address - ModuleBase;
		}

		Length = _snprintf(StackTrace, MAX_STACKTRACE_TEXT - 1, "%s!%s+0x%x", 
			               ModuleName, Symbol->Name, Displacement);

		MspInsertStackTraceText(Object, StackTrace, &Bucket);
		Trace->Text[Depth] = Bucket; 
	}

	SetFlag(Trace->Flag, BTR_FLAG_DECODED);
	return Trace;
}

ULONG
MspGetStackTrace(
	IN PBTR_RECORD_HEADER Record,
	IN PMSP_STACKTRACE_OBJECT Object,
	OUT PMSP_STACKTRACE_DECODED Decoded
	)
{
	ULONG Status;
	PMSP_STACKTRACE_ENTRY Entry;
	PMSP_STACKTRACE_TEXT Text;
	ULONG Number;

	Status = MSP_STATUS_OK;
	MspAcquireCsLock(&Object->Lock);

	Entry = MspLookupStackTrace(Object, (PVOID)Record->Address, 
								Record->StackHash, Record->StackDepth);
	if (!Entry) {
		MspReleaseCsLock(&Object->Lock);
		return MSP_STATUS_NO_STACKTRACE;
	}

	if (!FlagOn(Entry->Flag, BTR_FLAG_DECODED)) {

		MspAcquireContext(Object, Record->ProcessId);
		MspDecodeStackTrace(Object, Entry);
		MspReleaseContext(Object, FALSE);
	}

	for (Number = 0; Number < Entry->Depth; Number += 1) {
		Text = MspGetStackTraceText(Object, Entry->Text[Number]);
		if (!Text) {
			StringCchCopyA(Decoded->Text[Number], MAX_STACKTRACE_TEXT, "N/A");
		} else {
			StringCchCopyA(Decoded->Text[Number], MAX_STACKTRACE_TEXT, Text->Text);
		}
	}

	Decoded->Depth = Entry->Depth;
	MspReleaseCsLock(&Object->Lock);
	return MSP_STATUS_OK;
}

ULONG
MspDecodeStackTraceByRecord(
	IN PBTR_RECORD_HEADER Record,
	IN PMSP_STACKTRACE_OBJECT Object
	)
{
	ULONG Status;
	PMSP_STACKTRACE_ENTRY Entry;

	Status = MSP_STATUS_OK;
	MspAcquireCsLock(&Object->Lock);

	Entry = MspLookupStackTrace(Object, (PVOID)Record->Address, 
								Record->StackHash, Record->StackDepth);
	if (!Entry) {
		MspReleaseCsLock(&Object->Lock);
		return MSP_STATUS_NO_STACKTRACE;
	}

	if (!FlagOn(Entry->Flag, BTR_FLAG_DECODED)) {

		MspAcquireContext(Object, Record->ProcessId);
		MspDecodeStackTrace(Object, Entry);
		MspReleaseContext(Object, FALSE);
	}
	
	MspReleaseCsLock(&Object->Lock);
	return MSP_STATUS_OK;
}

ULONG
MspInsertStackTraceString(
	IN PMSP_STACKTRACE_OBJECT Object,
	IN PMSP_STACKTRACE_ENTRY Entry
	)
{
	ULONG Number;
	ULONG Bucket;
	BOOLEAN Status;

	ASSERT(FlagOn(Entry->Flag, BTR_FLAG_DECODED));

	for (Number = 0; Number < Entry->Depth; Number += 1) {
		Status = MspInsertStackTraceText(Object, Entry->Frame[Number], &Bucket);
		if (!Status) {
			if (Bucket != -1) {
				Entry->Text[Number] = Bucket;
			}
			else {
				Entry->Text[Number] = -1;
			}

		} else {
			Entry->Text[Number] = Bucket;
		}
		MspFree(Entry->Frame[Number]);
	}

	return MSP_STATUS_OK;
}

BOOLEAN
MspInsertStackTraceText(
	IN PMSP_STACKTRACE_OBJECT Object,
	IN PSTR String,
	OUT PULONG NewBucket
	)
{
	ULONG Bucket;
	ULONG PriorBucket;
	PMSP_STACKTRACE_TEXT Entry;
	PMSP_STACKTRACE_TEXT PriorEntry; 
	ULONG Hash;

	//
	// N.B. The bucket after hash is always bounded into
	// range of fixed buckets.
	//

	Hash = MspHashStringBKDR(String);
	Bucket = MSP_HASH_BUCKET(Hash);

	//
	// N.B. Two stack trace are idential if they have same hash, same depth,
	// and, have same frame values.
	//

	Entry = Object->u2.Buckets + Bucket;
	if (Entry->Hash == 0 && Entry->Text[0] == 0) {

		ASSERT(!BtrTestBit(&Object->TextBitmap, Bucket));

		//
		// This is an empty bucket we can use
		//

		Entry->Hash = Hash; 
		Entry->Next = -1;
		StringCchCopyA(Entry->Text, MAX_STACKTRACE_TEXT, String);

		Object->TextFirstHitBuckets += 1;
		Object->TextFilledFixedBuckets += 1;
		BtrSetBit(&Object->TextBitmap, Bucket);

		if (Object->TextFilledFixedBuckets == Object->TextFixedBuckets) {
			Object->TextFixedBucketsFull = TRUE;
		}

		*NewBucket = Bucket;
		return TRUE;
	}

	//
	// Walk the chained buckets if any
	//

	do {

		if (!strncmp(String, Entry->Text, MAX_STACKTRACE_TEXT)) {
			*NewBucket = Bucket;
			return FALSE;
		}
		
		PriorBucket = Bucket;
		PriorEntry = Entry;

		Bucket = Entry->Next;
		Entry = MspGetStackTraceText(Object, Bucket); 
	}
	while (Entry != NULL);

	if (Bucket != -1) {
		ASSERT(0);
		*NewBucket = -1;
		return FALSE;
	}

	//
	// No luck to hit, allocate new bucket to hold the stack trace
	// MspAllocateExtendEntryBucket is responsible to set prior bucket's next
	// field to point to new bucket, here we assume it's done already after
	// it return
	//

	if (!Object->TextFixedBucketsFull) {

		//
		// N.B. This indicates that the fixed buckets are not full,
		// we allocate by use bitmap, we try to allocate as near as
		// possible to prior bucket to keep cache locality when walking
		// chained buckets.
		//

		Bucket = BtrFindFirstClearBit(&Object->TextBitmap, PriorBucket);
		if (Bucket == -1) {
			ASSERT(PriorBucket != 0);
			Bucket = BtrFindFirstClearBitBackward(&Object->TextBitmap, PriorBucket);
		} 
		
		if (Bucket != -1) { 
			BtrSetBit(&Object->TextBitmap, Bucket);
		} else  {
			ASSERT(0);
		}

		Entry = Object->u2.Buckets + Bucket;

		//
		// N.B. It's safe to access PriorEntry here, because fixed buckets are not full.
		//

		PriorEntry->Next = Bucket;

		Object->TextFilledFixedBuckets += 1;
		Object->TextChainedBuckets += 1;

		if (Object->TextFilledFixedBuckets == Object->TextFixedBuckets) {
			Object->TextFixedBucketsFull = TRUE;
		}
		
		*NewBucket = Bucket;
	}

	else {
		Entry = MspAllocateExtendTextBucket(Object, &Bucket);
	}

	Entry->Hash = Hash;
	Entry->Next = -1;
	StringCchCopyA(Entry->Text, MAX_STACKTRACE_TEXT, String);

	//
	// N.B. We must get prior entry address via MspGetStackTraceText,
	// because a single view can not guarantee that 2 entries are included.
	//

	PriorEntry = MspGetStackTraceText(Object, PriorBucket);
	PriorEntry->Next = Bucket;

	*NewBucket = Bucket;
	return TRUE;
}

PMSP_STACKTRACE_TEXT
MspGetStackTraceText(
	IN PMSP_STACKTRACE_OBJECT Object,
	IN ULONG Bucket
	)
{
	PMSP_MAPPING_OBJECT MappingObject;
	ULONG64 Start, End;

	//
	// Ensure entry is bounded
	//

	if (Bucket == -1) {
		return NULL;
	}

	if (Bucket >= Object->TextTotalBuckets) {
		return NULL;
	}

	//
	// If the bucket is already mapped, return it,
	// note that the file object's mapping object is
	// always live after creation.
	//

	MappingObject = Object->TextObject->Mapping;
	if (IS_BUCKET_MAPPED(MappingObject, Bucket)) {
		return Object->u2.Buckets + Bucket;
	}	

	MappingObject = Object->TextExtendMapping;
	ASSERT(MappingObject != NULL);
	ASSERT(MappingObject->MappedVa != NULL);

	if (IS_BUCKET_MAPPED(MappingObject, Bucket)) {
		return BUCKET_TO_STACK_TEXT(MappingObject, Bucket);
	}
	
	UnmapViewOfFile(MappingObject->MappedVa);
	
	Start = Bucket * sizeof(MSP_STACKTRACE_TEXT);
	Start = MspUlong64RoundDown(Start, MSP_STACKTRACE_MAP_INCREMENT);
	End = Start + MSP_STACKTRACE_MAP_INCREMENT;

	//
	// Make sure the End make sense
	//

	End = min(End, MappingObject->FileLength);

	MappingObject->MappedVa = MapViewOfFile(MappingObject->MappedObject, FILE_MAP_READ|FILE_MAP_WRITE,
		                                    (DWORD)(Start >> 32), (DWORD)Start, (SIZE_T)(End - Start));
		         
	if (!MappingObject->MappedVa) {
		return NULL;
	}

	MappingObject->MappedOffset = Start;
	MappingObject->MappedLength = (ULONG)(End - Start);
	MappingObject->FirstSequence = (ULONG)(Start / sizeof(MSP_STACKTRACE_TEXT));
	MappingObject->LastSequence = (ULONG)(End / sizeof(MSP_STACKTRACE_TEXT)) - 1;

	return BUCKET_TO_STACK_TEXT(MappingObject, Bucket);
}

PMSP_STACKTRACE_TEXT
MspAllocateExtendTextBucket(
	IN PMSP_STACKTRACE_OBJECT Object,
	OUT PULONG NewBucket
	)
{
	ULONG64 Length;
	ULONG Status;
	ULONG Bucket;
	PMSP_FILE_OBJECT FileObject;
	PMSP_MAPPING_OBJECT MappingObject;
	PMSP_STACKTRACE_TEXT Entry;

	FileObject = Object->TextObject;
	MappingObject = FileObject->Mapping;
	Bucket = Object->TextTotalBuckets;

	if (IS_BUCKET_MAPPED(MappingObject, Bucket)) {
		*NewBucket = Bucket;
		Object->TextTotalBuckets += 1;
		Object->TextChainedBuckets += 1;
		return BUCKET_TO_STACK_TEXT(MappingObject, Bucket);
	}

	//
	// N.B. This indicate that the file object's mapping is out of current bucket range,
	// we need use extended mapping object
	//

	MappingObject = Object->TextExtendMapping;
	ASSERT(MappingObject != NULL);

	if (MappingObject->MappedVa) {
		if (IS_BUCKET_MAPPED(MappingObject, Bucket)) {
			*NewBucket = Bucket;
			Object->TextTotalBuckets += 1;
			Object->TextChainedBuckets += 1;
			return BUCKET_TO_STACK_TEXT(MappingObject, Bucket);
		}
		else {
			UnmapViewOfFile(MappingObject->MappedVa);
			MappingObject->MappedVa = NULL;
			MappingObject->MappedOffset = 0;
			MappingObject->MappedLength = 0;
		}
	}

repeat:

	if (Bucket < MAXIMUM_STACK_TEXT_BUCKET(MappingObject)) {

		//
		// N.B. MSP_STACKTRACE_ENTRY is guaranteed to be bounded by map increment,
		// so round down is enough to include it.
		//

		ULONG64 Start, End;

		Start = MspUlong64RoundDown(Bucket * sizeof(MSP_STACKTRACE_TEXT), 
			                         MSP_STACKTRACE_MAP_INCREMENT);
		End = Start + MSP_STACKTRACE_MAP_INCREMENT;
		End = min(End, MappingObject->FileLength);

		MappingObject->MappedVa = MapViewOfFile(MappingObject->MappedObject, 
			                                    FILE_MAP_READ|FILE_MAP_WRITE,
												(DWORD)(Start >> 32),
												(DWORD)Start,
												(SIZE_T)(End - Start));

		if (!MappingObject->MappedVa) {
			ULONG Status;
			Status = GetLastError();
			ASSERT(0);
			return NULL;
		}

		MappingObject->FirstSequence = (ULONG)(Start / sizeof(MSP_STACKTRACE_TEXT));
		MappingObject->LastSequence = MappingObject->FirstSequence + TEXT_BUCKETS_PER_MAP(End, Start) - 1;
		MappingObject->MappedOffset = Start;
		MappingObject->MappedLength = (ULONG)(End - Start);

		Entry = BUCKET_TO_STACK_TEXT(MappingObject, Bucket);
		Entry->Next = -1;
		*NewBucket = Bucket;
		
		Object->TextTotalBuckets += 1;
		Object->TextChainedBuckets += 1;

		return Entry;
	}

	CloseHandle(MappingObject->MappedObject);

	Length = FileObject->FileLength + MSP_STACKTRACE_FILE_INCREMENT;
	Status = MspExtendFileLength(FileObject->FileObject, Length);
	if (Status != MSP_STATUS_OK) {
		return NULL;
	}

	MappingObject->MappedObject = CreateFileMapping(FileObject->FileObject, 
													NULL, PAGE_READWRITE, 
													(DWORD)(Length >> 32),
													(DWORD)Length, NULL);
	if (!MappingObject->MappedObject) {
		return NULL;
	}

	//
	// N.B Update file AND extended mapping object's length
	//

	FileObject->FileLength = Length;
	MappingObject->FileLength = Length;

	goto repeat;
	return NULL;
}

ULONG 
MspHashStringBKDR(
	IN PSTR Buffer
	)
{
	ULONG Hash = 0;
	ULONG Seed = 131;

	while (*Buffer) {
		Hash = Hash * Seed + *Buffer;
		Buffer += 1;
	}
	return Hash & 0x7fffffff;
}

ULONG
MspCloseStackTrace(
	IN PMSP_STACKTRACE_OBJECT Object
	)
{
	if (Object->EntryObject) {

		if (Object->EntryExtendMapping) {
			UnmapViewOfFile(Object->EntryExtendMapping->MappedVa);
			CloseHandle(Object->EntryExtendMapping->MappedObject);
			MspFree(Object->EntryExtendMapping);
		}

		MspCloseFileObject(Object->EntryObject, FALSE);
		DeleteFile(Object->EntryObject->Path);

		MspFree(Object->EntryObject->Mapping);
		MspFree(Object->EntryObject);

		Object->EntryObject = NULL;
		Object->EntryExtendMapping = NULL;
	}

	if (Object->TextObject) {

		if (Object->TextExtendMapping) {
			UnmapViewOfFile(Object->TextExtendMapping->MappedVa);
			CloseHandle(Object->TextExtendMapping->MappedObject);
			MspFree(Object->TextExtendMapping);
		}

		MspCloseFileObject(Object->TextObject, FALSE);
		DeleteFile(Object->TextObject->Path);

		MspFree(Object->TextObject->Mapping);
		MspFree(Object->TextObject);

		Object->TextObject = NULL;
		Object->TextExtendMapping = NULL;
	}

	return MSP_STATUS_OK;
}

VOID
MspGetStackCounters(
	OUT PULONG EntryBuckets,
	OUT PULONG TextBuckets
	)
{
	PMSP_DTL_OBJECT DtlObject;

	DtlObject = MspCurrentDtlObject();

	*EntryBuckets = DtlObject->StackTrace.EntryFilledFixedBuckets + 
					DtlObject->StackTrace.EntryChainedBuckets;
	
	*TextBuckets = DtlObject->StackTrace.TextFilledFixedBuckets + 
				   DtlObject->StackTrace.TextChainedBuckets;
}

ULONG CALLBACK
MspDecodeProcedure(
	IN PVOID Context
	)
{
	ULONG NextIndex;
	ULONG RecordCount;
	HANDLE IndexHandle;
	HANDLE DataHandle;
	BTR_FILE_INDEX Index;
	PBTR_RECORD_HEADER Record;
	PMSP_DTL_OBJECT DtlObject;
	PMSP_FILE_OBJECT IndexObject;
	PMSP_FILE_OBJECT DataObject;
	ULONG Status;
	ULONG Complete;

	DtlObject = MspCurrentDtlObject();
	IndexObject = &DtlObject->IndexObject;
	DataObject = &DtlObject->DataObject;

	//
	// Create duplicated data file handle 
	//

    DataHandle = CreateFile(DataObject->Path, GENERIC_READ | GENERIC_WRITE, 
							FILE_SHARE_READ | FILE_SHARE_WRITE, 
							NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (DataHandle == INVALID_HANDLE_VALUE) {
		return GetLastError();
	}

	//
	// Create duplicated index file handle 
	//

    IndexHandle = CreateFile(IndexObject->Path, GENERIC_READ | GENERIC_WRITE, 
							 FILE_SHARE_READ | FILE_SHARE_WRITE, 
							 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (IndexHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		CloseHandle(DataHandle);
		return Status;
	}

	Record = (PBTR_RECORD_HEADER)MspMalloc(BspPageSize * 2);
	RecordCount = MspGetRecordCount(DtlObject);
	NextIndex = 0;

	while (RecordCount == 0) {
		Sleep(1000);
		RecordCount = MspGetRecordCount(DtlObject);
	}
		
	while (TRUE) {
		
		//
		// Check whether UI is accessing stack trace database
		//

		while (MspStackTraceBarrier) {

			//
			// Check whether application is terminating
			//

			if (WAIT_OBJECT_0 == WaitForSingleObject(MspStopEvent, 0)) {
				break;
			}

			Sleep(2000);
		}

		if (NextIndex < RecordCount) {

			//
			// N.B. Because we always sequencially access index and record,
			// we don't need to SetFilePointer()
			//

			ReadFile(IndexHandle, &Index, sizeof(BTR_FILE_INDEX), &Complete, NULL);
			ReadFile(DataHandle, Record, (ULONG)Index.Length, &Complete, NULL);

			MspDecodeStackTraceByRecord(Record, &DtlObject->StackTrace);
			NextIndex += 1;

			DebugTrace("decode: index = %d", NextIndex);

		} else {

			//
			// It's all scanned, update new record count to its current value
			//

			Sleep(1000);
			RecordCount = MspGetRecordCount(DtlObject);
		}
	}

	MspFree(Record);
	CloseHandle(IndexHandle);
	CloseHandle(DataHandle);
	return 0;
}

VOID
MspSetStackTraceBarrier(
	VOID
	)
{
	MspStackTraceBarrier = 1;
}

VOID
MspClearStackTraceBarrier(
	VOID
	)
{
	MspStackTraceBarrier = 0;
}

VOID
MspClearDbghelp(
	VOID
	)
{
	PMSP_DTL_OBJECT DtlObject;

	DtlObject = MspCurrentDtlObject();
	MspAcquireCsLock(&DtlObject->StackTrace.Lock);
	MspReleaseContext(&DtlObject->StackTrace, TRUE);
	MspReleaseCsLock(&DtlObject->StackTrace.Lock);
}