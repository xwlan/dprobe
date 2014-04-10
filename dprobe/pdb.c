//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2010
//

#include "bsp.h"
#include "pdb.h"
#include <stdio.h>

BSP_LOCK PdbGlobalLock;

ULONG
PdbInitialize(
	VOID
	)
{
	BspInitializeLock(&PdbGlobalLock);
	return S_OK;
}

ULONG
PdbCreateObject(
	IN PDTL_OBJECT DtlObject	
	)
{
	ULONG Status;
	PPDB_OBJECT PdbObject;

	PdbObject = (PPDB_OBJECT)BspMalloc(sizeof(PDB_OBJECT));
	PdbObject->NumberOfProcess = 0;
	PdbObject->SymbolPath = NULL;
	PdbObject->Current = NULL;
	
	InitializeListHead(&PdbObject->ContextListHead);
	InitializeListHead(&PdbObject->HashTable);
	BspInitializeLock(&PdbObject->ObjectLock);

	DtlObject->PdbObject = PdbObject;
	if (DtlObject->DataObject.Mode != DTL_FILE_CREATE) {
		return S_OK;
	}

	/*if (WorkMode != CaptureMode) {
		return S_OK;
	}*/

	return S_OK;
}

ULONG
PdbUninitialize(
	IN HANDLE ProcessHandle
	)
{
	return 0;
}

VOID
PdbSetSymbolPath(
	IN PPDB_OBJECT Object,
	IN PWSTR Path
	)
{
	ULONG Length;

	Length = (ULONG)wcsnlen_s(Path, MAX_PATH);
	if (!Length) {
		return;
	}

	if (Object->SymbolPath != NULL) {
		BspFree(Object->SymbolPath);
	}

	Object->SymbolPath = (PWCHAR)BspMalloc((Length + 1) * sizeof(WCHAR));
	StringCchCopy(Object->SymbolPath, MAX_PATH, Path);
}

ULONG
PdbGetNameFromAddress(
	IN PPDB_OBJECT Object,
	IN HANDLE ProcessHandle,
	IN ULONG64 Address,
	IN PULONG64 Displacement,
	OUT PSYMBOL_INFO Symbol
	)
{
	ULONG Status;

	Status = SymFromAddr(ProcessHandle, Address, Displacement, Symbol);
	return Status;
}

ULONG
PdbGetLineFromAddress(
	IN PPDB_OBJECT Object,
	IN HANDLE ProcessHandle,
	IN ULONG64 Address,
	OUT PULONG Displacement,
	OUT PIMAGEHLP_LINE64 Line
	)
{
	ULONG Status;

	Status = SymGetLineFromAddr64(ProcessHandle, Address, Displacement, Line);
	return Status;
}

ULONG
PdbSetDefaultOption(
	IN PPDB_OBJECT Object
	)
{
	SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | 
		          SYMOPT_INCLUDE_32BIT_MODULES | SYMOPT_LOAD_LINES | 
				  SYMOPT_OMAP_FIND_NEAREST);
	return 0;
}

VOID
PdbAcquireLock(
	VOID
	)
{
	BspAcquireLock(&PdbGlobalLock);
}

VOID
PdbReleaseLock(
	VOID
	)
{
	BspReleaseLock(&PdbGlobalLock);
}

HANDLE
PdbSwitchContext(
	IN PPDB_OBJECT Object,
	IN ULONG ProcessId
	)
{
	PLIST_ENTRY ListEntry;
	PPDB_CONTEXT Context;
	HANDLE ProcessHandle;
	
	//
	// Check whether it's already in requested context
	//

	if (Object->Current != NULL) {
		if (Object->Current->ProcessId == ProcessId) {
			return Object->Current->ProcessHandle;
		}
	}

	ListEntry = Object->ContextListHead.Flink;

	while (ListEntry != &Object->ContextListHead) {
		Context = CONTAINING_RECORD(ListEntry, PDB_CONTEXT, ListEntry);
		if (Context->ProcessId == ProcessId) { 

			SymCleanup(Object->Current->ProcessHandle);

			//
			// N.B. SymInitialize() effectively call SymLoadModule64 for all
			// loaded modules of target process
			//

			SymInitializeW(Context->ProcessHandle, Object->SymbolPath, TRUE);
			SymSetSearchPathW(Context->ProcessHandle, Object->SymbolPath);
			PdbSetDefaultOption(Object);

			Object->Current = Context;
			return Object->Current;
		}
		ListEntry = ListEntry->Flink;
	}

	if (Object->Current != NULL) {
		SymCleanup(Object->Current->ProcessHandle);
	}

	//
	// N.B. SE_DEBUG_NAME must be acquired before open target process 
	//

	ProcessHandle = OpenProcess(PSP_OPEN_PROCESS_FLAG, FALSE, ProcessId);
	if (ProcessHandle != NULL) {

		Context = (PPDB_CONTEXT)BspMalloc(sizeof(PDB_CONTEXT));	
		Context->ProcessId = ProcessId;
		Context->ProcessHandle = ProcessHandle;
		InsertTailList(&Object->ContextListHead, &Context->ListEntry);

		//
		// N.B. SymInitialize() effectively call SymLoadModule64 for all
		// loaded modules of target process
		//

		SymInitializeW(Context->ProcessHandle, Object->SymbolPath, TRUE);
		SymSetSearchPathW(Context->ProcessHandle, Object->SymbolPath);
		PdbSetDefaultOption(Object);
		Object->Current = Context;
		return Context;
	}

	return NULL;
}

PPDB_HASH_TABLE
PdbGetHashTable(
	IN PPDB_OBJECT Object,
	IN ULONG ProcessId
	)
{
	PLIST_ENTRY ListEntry;
	PPDB_HASH_TABLE Table;
	PPDB_HASH Hash;
	ULONG Number;

	ListEntry = Object->HashTable.Flink;

	while (ListEntry != &Object->HashTable) {

		Table = CONTAINING_RECORD(ListEntry, PDB_HASH_TABLE, ListEntry);
	
		if (Table->ProcessId == ProcessId) {
			return Table;
		}

		ListEntry = ListEntry->Flink;
	}

	//
	// Allocate a new hash table object
	//

	Table = (PPDB_HASH_TABLE)BspMalloc(sizeof(PDB_HASH_TABLE));
	ASSERT(Table != NULL);

	for(Number = 0; Number < PDB_CLUSTER_SIZE; Number += 1) {
		Hash = &Table->Cluster[Number];
		InitializeListHead(&Hash->ListEntry);
	}

	Table->ProcessId = ProcessId;
	Table->HashCount = 0;
	Table->FillCount = 0;

	InsertHeadList(&Object->HashTable, &Table->ListEntry);
	Object->NumberOfProcess += 1;
	return Table;
}

PPDB_HASH
PdbLookupStackTrace(
	IN PPDB_OBJECT Object,
	IN PBTR_RECORD_HEADER Record
	)
{
	ULONG Key;
	ULONG ProcessId;
	ULONG StackHash;
	PPDB_HASH_TABLE Table;	
	PPDB_HASH Hash;
	PLIST_ENTRY ListEntry;
	PPDB_HASH HashEntry;

	ProcessId = Record->ProcessId;
	StackHash = Record->StackHash;

	Table = PdbGetHashTable(Object, ProcessId);
	ASSERT(Table != NULL);

	Key = StackHash % PDB_CLUSTER_SIZE;
	Hash = &Table->Cluster[Key];
	
	if (FlagOn(Hash->Flag, PDB_FLAG_DECODED)) {

		if (Hash->StackHash == StackHash) {
			return Hash;
		}
		
		ListEntry = Hash->ListEntry.Flink;
		while (ListEntry != &Hash->ListEntry) {
			HashEntry = CONTAINING_RECORD(ListEntry, PDB_HASH, ListEntry);		
			if (HashEntry->StackHash == StackHash) {
				return HashEntry;
			}
			ListEntry = ListEntry->Flink;
		}

		HashEntry = (PPDB_HASH)BspMalloc(sizeof(PDB_HASH));
		HashEntry->StackHash = StackHash;
		InsertTailList(&Hash->ListEntry, &HashEntry->ListEntry);	

		Hash->SharedKeyCount += 1;
		Table->HashCount += 1;

		return HashEntry;
	}

	Hash->StackHash = StackHash;
	Hash->SharedKeyCount = 0;
	InitializeListHead(&Hash->ListEntry);

	Table->HashCount += 1;
	Table->FillCount += 1;

	return Hash;	
}

PPDB_HASH
PdbDecodeStackTrace(
	IN PPDB_OBJECT Object,
	IN PBTR_RECORD_HEADER Record,
	IN PPDB_HASH Hash
	)
{
	ULONG Depth;
	ULONG Status;
	ULONG Length;
	ULONG64 Address;
	ULONG64 Displacement;
	ULONG64 Buffer[(sizeof(IMAGEHLP_SYMBOL64) / sizeof(ULONG64)) + 64];
	CHAR StackTrace[MAX_SYM_NAME * 2];
	CHAR ModuleName[64];
	PIMAGEHLP_SYMBOL64 Symbol;
	PSTR Decoded;
	ULONG DecodedDepth;

	Symbol = (PIMAGEHLP_SYMBOL64)Buffer;
	DecodedDepth = 0;

	ASSERT(!FlagOn(Hash->Flag, PDB_FLAG_DECODED));

	for(Depth = 0; Depth < Record->StackDepth; Depth += 1) {
		
//		Address = (ULONG64) Record->StackTrace[Depth];
		RtlZeroMemory(Buffer, sizeof(Buffer));
		Symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
		Symbol->MaxNameLength = 128;

		Status = SymGetSymFromAddr64(Object->Current->ProcessHandle, 
									 Address, 
									 &Displacement, 
									 Symbol);

		//
		// N.B. The stack trace captured by RtlCaptureStackTrace is not accurate, which can
		// contain noisy frames, we filter them here, just ignore the error and continue.
		// we will develop our own stack walk algorithm for btr, next version ;) of course.
		//

		if (Status != TRUE) {
			continue;
		}

		PdbGetModuleName(Object, Address, ModuleName, 64);
		Length = sprintf(StackTrace, "%s!%s+0x%x", ModuleName, Symbol->Name, Displacement);
		Decoded = BspMalloc(Length + 1);
		strcpy(Decoded, StackTrace); 

		Hash->StackTrace[DecodedDepth] = Decoded;
		DecodedDepth += 1;
	}

	//
	// N.B. If parsed stack depth is zero, we treat it as non-decoded
	// stack trace.
	//

	if (DecodedDepth == 0) {
		DecodedDepth = 1;
		Hash->StackTrace[0] = "N/A";
	}
	
	SetFlag(Hash->Flag, PDB_FLAG_DECODED);
	Hash->StackDepth = DecodedDepth;

	return Hash;
}

PPDB_HASH
PdbGetStackTrace(
	IN PPDB_OBJECT Object,
	IN PBTR_RECORD_HEADER Record
	)
{
	HANDLE ProcessHandle;
	PPDB_HASH Hash;

	//
	// Lookup the stack trace database to check whether the stack trace is already
	// decoded.
	//

	PdbAcquireLock();

	Hash = PdbLookupStackTrace(Object, Record);
	ASSERT(Hash != NULL);

	if (FlagOn(Hash->Flag, PDB_FLAG_DECODED)) {
		PdbReleaseLock();
		return Hash; 	
	}	

	//
	// Stack trace not yet decoded, switch decode context and 
	// parse the record's stack trace
	//

	ProcessHandle = PdbSwitchContext(Object, Record->ProcessId);
	if (ProcessHandle == NULL) {
		PdbReleaseLock();
		return NULL;
	}
	
	PdbDecodeStackTrace(Object, Record, Hash);
	PdbReleaseLock();
	return Hash;
}

ULONG
PdbGetModuleName(
	IN PPDB_OBJECT Object,
	IN ULONG64 Address,
	IN PCHAR NameBuffer,
	IN ULONG BufferLength
	)
{
	PIMAGEHLP_MODULE64 Module;
	ULONG64 Buffer[sizeof(IMAGEHLP_MODULE64) / sizeof(ULONG64) + 1];
	ULONG Status;

	Module = (PIMAGEHLP_MODULE64)Buffer;
	Module->SizeOfStruct = BspRoundUp(sizeof(IMAGEHLP_MODULE64), 8);
	Status = SymGetModuleInfo64(Object->Current->ProcessHandle, 
		                        Address,
								Module);
	if (Status != TRUE) {
		strncpy(NameBuffer, "<Unknown>", BufferLength);
	}
	else {
		strncpy(NameBuffer, Module->ModuleName, BufferLength);
	}

	strlwr(NameBuffer);
	return ERROR_SUCCESS;
}

VOID
PdbFreeHashResource(
	IN PPDB_HASH Hash
	)
{
	PSTR StackTrace;
	PLIST_ENTRY ListEntry;
	ULONG i;

	if (!FlagOn(Hash->Flag, PDB_FLAG_DECODED)){
		return;
	}

	for(i = 0; i < MAX_STACK_DEPTH; i++) {
		StackTrace = Hash->StackTrace[i];
		if (StackTrace != NULL) {
			BspFree(StackTrace);
		}
		else {
			break;
		}
	}	

	if (Hash->SharedKeyCount > 0) {
		
		PPDB_HASH ShareKeyHash;
		while (IsListEmpty(&Hash->ListEntry) != TRUE) {
	
			ListEntry = RemoveHeadList(&Hash->ListEntry);
			ShareKeyHash = CONTAINING_RECORD(ListEntry, PDB_HASH, ListEntry);	
			
			for(i = 0; i < MAX_STACK_DEPTH; i++) {
				StackTrace = ShareKeyHash->StackTrace[i];
				if (StackTrace != NULL) {
					BspFree(StackTrace);
				}
				else {
					break;
				}
			}

			BspFree(ShareKeyHash);
		}
	}
}

VOID
PdbFreeResource(
	IN PPDB_OBJECT Object	
	)
{
	ULONG i;
	PLIST_ENTRY ListEntry;
	PPDB_HASH_TABLE Table;
	PPDB_CONTEXT Context;

	while (IsListEmpty(&Object->HashTable) != TRUE) {

		ListEntry = RemoveHeadList(&Object->HashTable);
		Table = CONTAINING_RECORD(ListEntry, PDB_HASH_TABLE, ListEntry);

		for(i = 0; i < PDB_CLUSTER_SIZE; i++) {
			PdbFreeHashResource(&Table->Cluster[i]);
		}

		BspFree(Table);
	}

	if (Object->Current != NULL) {
		SymCleanup(Object->Current->ProcessHandle);
		Object->Current = NULL;
	}
	
	while (IsListEmpty(&Object->ContextListHead) != TRUE) {
		ListEntry = RemoveHeadList(&Object->ContextListHead);
		Context = CONTAINING_RECORD(ListEntry, PDB_CONTEXT, ListEntry);
		CloseHandle(Context->ProcessHandle);
		BspFree(Context);
	}
}

ULONG
PdbWriteCluster(
	IN HANDLE FileHandle,
	IN PPDB_HASH_TABLE Table
	)
{
	ULONG HashNumber;
	ULONG StackNumber;
	ULONG StackTraceLength;
	ULONG CompleteBytes;
	ULONG HashEntryCount;
	ULONG MaximumSharedKeyCount;
	PPDB_HASH Hash;
	PPDB_HASH SharedKeyHash;
	PLIST_ENTRY ListEntry;

	HashEntryCount = 0;
	MaximumSharedKeyCount = 0;

	for (HashNumber = 0; HashNumber < PDB_CLUSTER_SIZE; HashNumber++) {

		Hash = &Table->Cluster[HashNumber];
		WriteFile(FileHandle, Hash, sizeof(PDB_HASH), &CompleteBytes, NULL);
		
		if (!FlagOn(Hash->Flag, PDB_FLAG_DECODED)) {
			continue;
		}

		HashEntryCount += 1;

		for (StackNumber = 0; StackNumber < Hash->StackDepth; StackNumber++) {
			 StackTraceLength = (ULONG)strlen(Hash->StackTrace[StackNumber]) + 1;
			 WriteFile(FileHandle, &StackTraceLength, sizeof(ULONG), &CompleteBytes, NULL);
			 WriteFile(FileHandle, Hash->StackTrace[StackNumber], StackTraceLength, &CompleteBytes, NULL);
		}

		if (Hash->SharedKeyCount > 0) {

			if (Hash->SharedKeyCount > MaximumSharedKeyCount) {
				MaximumSharedKeyCount = Hash->SharedKeyCount;
			}

			ListEntry = Hash->ListEntry.Flink;
			while (ListEntry != &Hash->ListEntry) {
				
				SharedKeyHash = CONTAINING_RECORD(ListEntry, PDB_HASH, ListEntry);
				WriteFile(FileHandle, SharedKeyHash, sizeof(PDB_HASH), &CompleteBytes, NULL);
					
				for(StackNumber = 0; StackNumber < SharedKeyHash->StackDepth; StackNumber++) {
				
					StackTraceLength = (ULONG)strlen(SharedKeyHash->StackTrace[StackNumber]) + 1;
					WriteFile(FileHandle, &StackTraceLength, sizeof(ULONG), &CompleteBytes, NULL);
					WriteFile(FileHandle, SharedKeyHash->StackTrace[StackNumber], StackTraceLength, &CompleteBytes, NULL);
					
				}

				HashEntryCount += 1;
				ListEntry = ListEntry->Flink;
			}
		}
	}

	return S_OK;	
}

ULONG
PdbWriteStackTraceToDtl(
	IN PPDB_OBJECT Object,
	IN HANDLE FileHandle
	)
{
	ULONG TableLength;
	ULONG CompleteBytes;
	PLIST_ENTRY ListEntry;
	PPDB_HASH_TABLE Table;
	PCHAR Buffer;
	int Number;

	if (!Object->NumberOfProcess) {
		return S_OK;
	}

	ListEntry = Object->HashTable.Flink;
	TableLength = FIELD_OFFSET(PDB_HASH_TABLE, Cluster[0]);

	Number = 0;
	Buffer = (PCHAR)BspMalloc(sizeof(ULONG) + Object->NumberOfProcess * TableLength);
	*(PULONG)Buffer = Object->NumberOfProcess;

	while (ListEntry != &Object->HashTable) {
		Table = CONTAINING_RECORD(ListEntry, PDB_HASH_TABLE, ListEntry);
		RtlCopyMemory(Buffer + sizeof(ULONG) + Number * TableLength, Table, TableLength);
		ListEntry = ListEntry->Flink;
		Number += 1;
	}

	ASSERT(Object->NumberOfProcess == Number);

	WriteFile(FileHandle, Buffer, sizeof(ULONG) + TableLength * Object->NumberOfProcess, &CompleteBytes, NULL);
	BspFree(Buffer);
	
	ListEntry = Object->HashTable.Flink;
	while (ListEntry != &Object->HashTable) {
		Table = CONTAINING_RECORD(ListEntry, PDB_HASH_TABLE, ListEntry);
		PdbWriteCluster(FileHandle, Table);
		ListEntry = ListEntry->Flink;
	}

	return S_OK;
}

ULONG
PdbDebugStackTrace(
	IN PPDB_HASH Hash
	)
{
	ULONG StackNumber;
	CHAR Buffer[1024];

	if (!Hash->StackDepth) {
		return 0;
	}

	DebugTrace("\r\n PDB_HASH -> hash=0x%08x, slot=%d, depth=%d, shared=%d", 
			  Hash->StackHash, Hash->StackHash % PDB_CLUSTER_SIZE, 
			  Hash->StackDepth, Hash->SharedKeyCount);

	for(StackNumber = 0; StackNumber < Hash->StackDepth; StackNumber++) {
		sprintf(Buffer, "%02d %s", StackNumber, Hash->StackTrace[StackNumber]);
		DebugTrace(Buffer);
	}

	return 0;
}

ULONG
PdbReadCluster(
	IN HANDLE FileHandle,
	IN PPDB_HASH_TABLE Table
	)
{
	ULONG CompleteBytes;
	PPDB_HASH Hash;
	PPDB_HASH SharedKeyHash;
	ULONG HashNumber;
	ULONG StackNumber;
	ULONG SharedNumber;
	ULONG StackTraceLength;
	PSTR StackTrace;

	for (HashNumber = 0; HashNumber < PDB_CLUSTER_SIZE; HashNumber++) {

		Hash = &Table->Cluster[HashNumber];
		ReadFile(FileHandle, Hash, sizeof(PDB_HASH), &CompleteBytes, NULL);
		
		InitializeListHead(&Hash->ListEntry);
		if (!FlagOn(Hash->Flag, PDB_FLAG_DECODED)) {
			continue;
		}

		for (StackNumber = 0; StackNumber < Hash->StackDepth; StackNumber++) {
			 ReadFile(FileHandle, &StackTraceLength, sizeof(ULONG), &CompleteBytes, NULL);
			 StackTrace = (PCHAR)BspMalloc(StackTraceLength);
			 ReadFile(FileHandle, StackTrace, StackTraceLength, &CompleteBytes, NULL);
			 Hash->StackTrace[StackNumber] = StackTrace;
		}

		for(SharedNumber = 0; SharedNumber < Hash->SharedKeyCount; SharedNumber += 1) {

			SharedKeyHash = (PPDB_HASH)BspMalloc(sizeof(PDB_HASH));
			ReadFile(FileHandle, SharedKeyHash, sizeof(PDB_HASH), &CompleteBytes, NULL); 

			ASSERT(FlagOn(SharedKeyHash->Flag, PDB_FLAG_DECODED));

			for (StackNumber = 0; StackNumber < SharedKeyHash->StackDepth; StackNumber++) {
				 ReadFile(FileHandle, &StackTraceLength, sizeof(ULONG), &CompleteBytes, NULL);
				 StackTrace = (PCHAR)BspMalloc(StackTraceLength);
				 ReadFile(FileHandle, StackTrace, StackTraceLength, &CompleteBytes, NULL);
				 SharedKeyHash->StackTrace[StackNumber] = StackTrace;
			}
	 		
			InsertTailList(&Hash->ListEntry, &SharedKeyHash->ListEntry);

		}
	}

	return ERROR_SUCCESS;
}

ULONG
PdbLoadStackTraceFromDtl(
	IN PPDB_OBJECT Object,
	IN HANDLE FileHandle	
	)
{
	ULONG TableLength;
	ULONG CompleteBytes;
	PLIST_ENTRY ListEntry;
	PPDB_HASH_TABLE Table;
	ULONG Number;

	InitializeListHead(&Object->HashTable);

	ReadFile(FileHandle, &Object->NumberOfProcess, sizeof(ULONG), &CompleteBytes, NULL);
	ASSERT(Object->NumberOfProcess != 0);

	TableLength = FIELD_OFFSET(PDB_HASH_TABLE, Cluster[0]);

	for(Number = 0; Number < Object->NumberOfProcess; Number += 1) {
		Table = (PPDB_HASH_TABLE) BspMalloc(sizeof(PDB_HASH_TABLE));
		ReadFile(FileHandle, Table, TableLength, &CompleteBytes, NULL);
		InsertTailList(&Object->HashTable, &Table->ListEntry);
	}

	ListEntry = Object->HashTable.Flink;

	while (ListEntry != &Object->HashTable) {
		Table = CONTAINING_RECORD(ListEntry, PDB_HASH_TABLE, ListEntry);
		PdbReadCluster(FileHandle, Table);
		ListEntry = ListEntry->Flink;
	}
	
	ListEntry = Object->HashTable.Flink;
	while (ListEntry != &Object->HashTable) {
		Table = CONTAINING_RECORD(ListEntry, PDB_HASH_TABLE, ListEntry);
		ListEntry = ListEntry->Flink;
	}

	return S_OK;	
}

ULONG
PdbWriteStackTraceToTxt(
	IN PPDB_OBJECT Object,
	IN PWSTR Path
	)
{
	ASSERT(0);
	return S_OK;
}


ULONG CALLBACK
PdbDecodeThread(
	IN PVOID Context
	)
{
	return 0;
}
