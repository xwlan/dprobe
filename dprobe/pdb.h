//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2010
//

#ifndef _PDB_H_
#define _PDB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <dbghelp.h>
#include "dprobe.h"
#include "btr.h"

#define PDB_FLAG_FREE      ((ULONG)0x00000000)
#define PDB_FLAG_DECODED   ((ULONG)0x00000001)

#define PDB_CLUSTER_SIZE   ((ULONG)4096)

typedef struct _PDB_HASH {
	ULONG StackHash;
	ULONG Flag;
	ULONG SharedKeyCount;
	ULONG StackDepth;
	PSTR StackTrace[MAX_STACK_DEPTH];
	LIST_ENTRY ListEntry;
} PDB_HASH, *PPDB_HASH;

typedef struct _PDB_HASH_TABLE{
	ULONG ProcessId;
	ULONG HashCount;
	ULONG FillCount;
	PDB_HASH Cluster[PDB_CLUSTER_SIZE];
	LIST_ENTRY ListEntry;
} PDB_HASH_TABLE, *PPDB_HASH_TABLE;

typedef struct _PDB_FILE_HEADER {
	ULONG Signature;
	ULONG NumberOfProcess;
} PDB_FILE_HEADER, *PPDB_FILE_HEADER;

typedef struct _PDB_CONTEXT {
	LIST_ENTRY ListEntry;
	ULONG ProcessId;
	HANDLE ProcessHandle;
} PDB_CONTEXT, *PPDB_CONTEXT;

typedef struct _PDB_OBJECT {
	LIST_ENTRY ListEntry;
	BSP_LOCK ObjectLock;
	PWSTR SymbolPath;
	PPDB_CONTEXT Current;
	LIST_ENTRY ContextListHead;
	LIST_ENTRY HashTable;
	ULONG NumberOfProcess;
} PDB_OBJECT, *PPDB_OBJECT;

ULONG
PdbInitialize(
	VOID
	);

ULONG
PdbCreateObject(
	IN PDTL_OBJECT DtlObject	
	);

VOID
PdbSetSymbolPath(
	IN PPDB_OBJECT Object,
	IN PWSTR SymbolPath
	);

ULONG
PdbGetNameFromAddress(
	IN PPDB_OBJECT Object,
	IN HANDLE ProcessHandle,
	IN ULONG64 Address,
	IN PULONG64 Displacement,
	OUT PSYMBOL_INFO Symbol
	);

ULONG
PdbGetLineFromAddress(
	IN PPDB_OBJECT Object,
	IN HANDLE ProcessHandle,
	IN ULONG64 Address,
	OUT PULONG Displacement,
	OUT PIMAGEHLP_LINE64 Line
	);

PPDB_HASH
PdbGetStackTrace(
	IN PPDB_OBJECT Object,
	IN PBTR_RECORD_HEADER Record
	);

PPDB_HASH
PdbLookupStackTrace(
	IN PPDB_OBJECT Object,
	IN PBTR_RECORD_HEADER Record 
	);

PPDB_HASH
PdbDecodeStackTrace(
	IN PPDB_OBJECT Object,
	IN PBTR_RECORD_HEADER Record,
	IN PPDB_HASH Hash
	);

HANDLE
PdbSwitchContext(
	IN PPDB_OBJECT Object,
	IN ULONG ProcessId
	);

ULONG
PdbSetDefaultOption(
	IN PPDB_OBJECT Object	
	);

ULONG
PdbGetModuleName(
	IN PPDB_OBJECT Object,
	IN ULONG64 Address,
	IN PCHAR NameBuffer,
	IN ULONG BufferLength
	);

ULONG
PdbWriteCluster(
	IN HANDLE FileHandle,
	IN PPDB_HASH_TABLE Table
	);

ULONG
PdbLoadStackTraceDatabase(
	VOID
	);

ULONG
PdbDebugStackTrace(
	IN PPDB_HASH Hash
	);

VOID
PdbFreeResource(
	IN PPDB_OBJECT Object	
	);

VOID
PdbFreeHashResource(
	IN PPDB_HASH Hash
	);

VOID
PdbAcquireLock(
	VOID
	);

VOID
PdbReleaseLock(
	VOID
	);

ULONG
PdbWriteStackTraceToDtl(
	IN PPDB_OBJECT Object,
	IN HANDLE FileHandle
	);

ULONG
PdbLoadStackTraceFromDtl(
	IN PPDB_OBJECT Object,
	IN HANDLE FileHandle	
	);

ULONG
PdbWriteStackTraceToTxt(
	IN PPDB_OBJECT Object,
	IN PWSTR Path
	);

ULONG CALLBACK
PdbDecodeThread(
	IN PVOID Context
	);

#ifdef __cplusplus
}
#endif

#endif