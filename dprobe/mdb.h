//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _MDB_H_
#define _MDB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dprobe.h"
#include "sqlapi.h"

typedef enum _MDB_STATUS {
	STATUS_MDB_OK = S_OK,
	STATUS_MDB_OPENFAILED,
	STATUS_MDB_EXECFAILED,
	STATUS_MDB_QUERYFAILED,
	STATUS_MDB_NODATAITEM,
	STATUS_MDB_MISMATCH,
	STATUS_MDB_UNKNOWN,
} MDB_STATUS;

typedef enum _MDB_DATA_TYPE {
	MdbBoolean,
	MdbInteger,
	MdbFloat,
	MdbUnicode,
	MdbBinary,
} MDB_DATA_TYPE;

//
// N.B. The data order is strictly ordered in SQLite MetaData table
// as primary key.
//

typedef enum _MDB_DATA_OFFSET {

	//
	// Symbol
	//

	MdbSymbolPath,
	MdbOfflineDecode,

	//
	// General
	//

	MdbSearchEngine,
	MdbAutoScroll,
	MdbMinidumpFull,

	//
	// Advanced
	//

	MdbEnableKernel,

	//
	// Others
	//

	MdbEulaAccept,

} MDB_DATA_OFFSET;

typedef struct _MDB_DATA_ITEM {
	PWSTR Key;
	PWSTR Value;
	MDB_DATA_TYPE Type;
} MDB_DATA_ITEM, *PMDB_DATA_ITEM;

extern MDB_DATA_ITEM MdbData[];

ULONG
MdbInitialize(
	IN BOOLEAN Load	
	);

VOID
MdbClose(
	VOID
	);

ULONG
MdbLoadMetaData(
	IN PSQL_DATABASE SqlObject
	);

PMDB_DATA_ITEM
MdbGetData(
	IN MDB_DATA_OFFSET Offset
	);

ULONG
MdbSetData(
	IN MDB_DATA_OFFSET Offset,
	IN PWSTR Value
	);

BOOLEAN 
MdbQueryCallback(
	IN ULONG Row,
	IN ULONG Column,
	IN const UCHAR *Data,
	IN PVOID Context
	);

ULONG
MdbSetDefaultValues(
	IN PWSTR Path
	);

VOID
MdbDumpMetaData(
	VOID
	);

VOID
MdbGetSymbolPath(
	IN PWCHAR Buffer,
	IN ULONG Length
	);

VOID
MdbGetCachePath(
	IN PWCHAR Buffer, 
	IN ULONG Length
	);

VOID
MdbCreatePath(
	VOID
	);

ULONG
MdbEnsureEula(
	VOID
	);

#ifdef __cplusplus
}
#endif

#endif