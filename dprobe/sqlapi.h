//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _SQLAPI_H_
#define _SQLAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include "sqlite3.h"
#include "bsp.h"

//
// N.B. If the callback returns TRUE, the query process
// will be aborted.
//

typedef BOOLEAN 
(*SQLQUERY_CALLBACK)(
	IN ULONG Row,
	IN ULONG Column,
	IN const UCHAR *Data,
	IN PVOID Context
	);

typedef VOID
(*SQLPROFILE_CALLBACK)(
	IN PVOID Context,
	IN CONST CHAR *Statement,
	IN ULONG64 Time 
	);

typedef struct _SQL_DATABASE {
	WCHAR Name[MAX_PATH];	
	sqlite3 *SqlHandle;
	int LastErrorStatus;
	SQLQUERY_CALLBACK TextCallback;
	PVOID TextContext;
} SQL_DATABASE, *PSQL_DATABASE;

//
// N.B. Table is composed of a M*N UTF8 string matrix,
// the first row is column names, and NumberOfRows does
// not include first row.
//

typedef struct _SQL_TABLE {
	CHAR **Table;
	ULONG NumberOfRows;
	ULONG NumberOfColumns;
} SQL_TABLE, *PSQL_TABLE;

typedef struct _SQL_PROFILE {
	ULONG64 MemoryAllocated;
	ULONG64 MemoryAllocatedMaximum;
	ULONG64 SqlExecuteAverage;
	ULONG64 SqlExecuteMaximum;
	PWSTR SqlStatementMaximum;
} SQL_PROFILE, *PSQL_PROFILE;

ULONG
SqlOpen(
	IN PWSTR Path,
	OUT PSQL_DATABASE *Database
	);

ULONG
SqlClose(
	IN PSQL_DATABASE Database
	);

ULONG
SqlShutdown(
	VOID
	);

ULONG
SqlExecute(
	IN PSQL_DATABASE Database,
	IN PWSTR Statement
	);

ULONG
SqlQuery(
	IN PSQL_DATABASE Database,
	IN PWSTR Statement
	);

BOOLEAN
SqlIsOpen(
	IN PSQL_DATABASE Database
	);

BOOLEAN
SqlBeginTransaction(
	IN PSQL_DATABASE Database
	);

BOOLEAN
SqlCommitTransaction(
	IN PSQL_DATABASE Database
	);

BOOLEAN
SqlRollbackTransaction(
	IN PSQL_DATABASE Database
	);

//
// N.B. SqlGetTable is primarily for small size table,
// because it fetch all data into memory, it can have
// huge overhead if the table is big. For big table,
// register a sqlquery callback and call SqlQuery, the
// callback will get invoked for every column data,
// after usage, SqlFreeTable must be called to release
// allocated resources.
//

ULONG
SqlGetTable(
	IN PSQL_DATABASE Database,
	IN PWSTR Statement,
	OUT PSQL_TABLE *Table
	);

ULONG
SqlFreeTable(
	IN PSQL_DATABASE Database,
	IN PSQL_TABLE Table
	);

ULONG
SqlRegisterProfileCallback(
	IN PSQL_DATABASE Database,
	IN SQLPROFILE_CALLBACK ProfileCallback,
	IN PVOID Context
	);

ULONG
SqlRegisterQueryCallback(
	IN PSQL_DATABASE Database,
	IN SQLQUERY_CALLBACK Callback,
	IN PVOID Context
	);

ULONG
SqlUnregisterQueryCallback(
	IN PSQL_DATABASE Database,
	IN SQLQUERY_CALLBACK Callback
	);

CONST CHAR*
SqlVersion(
	VOID
	);

ULONG
SqlInitialize(
	IN ULONG Option,
	...
	);

ULONG
SqlQueryProfile(
	OUT PSQL_PROFILE Profile 
	);

ULONG CALLBACK
SqlProfileThread(
	IN PVOID Context
	);

#ifdef __cplusplus
}
#endif

#endif