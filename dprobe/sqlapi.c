//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "sqlapi.h"
#include "bsp.h"
#include <strsafe.h>
#include <assert.h>

SQL_PROFILE SqlProfile;
CRITICAL_SECTION SqlProfileLock;

CONST CHAR*
SqlVersion(
	VOID
	)
{
	const char *Version;
	Version = sqlite3_libversion();
	return Version;
}

ULONG
SqlInitialize(
	IN ULONG Option,
	...
	)
{
	ULONG ThreadId;
	HANDLE ThreadHandle;

	InitializeCriticalSection(&SqlProfileLock);
	ThreadHandle = CreateThread(NULL, 0, SqlProfileThread, NULL, 0, &ThreadId);
	CloseHandle(ThreadHandle);
	return S_OK;
}


ULONG
SqlOpen(
	IN PWSTR Path,
	OUT PSQL_DATABASE *Database
	)
{
	PCHAR Buffer;
	PSQL_DATABASE Object;
	sqlite3 *SqlHandle;
	int Status;

	BspConvertUnicodeToUTF8(Path, &Buffer);
	Status = sqlite3_open(Buffer, &SqlHandle);
	BspFree(Buffer);

	if (Status == SQLITE_OK) {

		Object = (PSQL_DATABASE)BspMalloc(sizeof(SQL_DATABASE));
		RtlZeroMemory(Object, sizeof(SQL_DATABASE));

		Object->SqlHandle = SqlHandle;
		StringCchCopy(Object->Name, MAX_PATH, Path);

		*Database = Object;
		return S_OK;

	}

	return Status;
}

ULONG
SqlClose(
	IN PSQL_DATABASE Database
	)
{
	int Status;

	ASSERT(Database->SqlHandle != NULL);
	Status = sqlite3_close(Database->SqlHandle);

	if (Status != SQLITE_OK) {
		return Status;
	}

	BspFree(Database);
	return Status;
}

ULONG
SqlShutdown(
	VOID
	)
{
	ULONG Status;

	Status = sqlite3_shutdown();
	return Status;
}

ULONG
SqlExecute(
	IN PSQL_DATABASE Database,
	IN PWSTR Statement
	)
{
	PCHAR Message;
	PCHAR SqlStatement = NULL;
	int Status;

	if (!SqlIsOpen(Database)) {
		return Database->LastErrorStatus;
	}

	Message = NULL;
	BspConvertUnicodeToUTF8(Statement, &SqlStatement);

	Status = sqlite3_exec(Database->SqlHandle, SqlStatement, NULL, NULL, &Message);
	if (Status != SQLITE_OK) {
		Database->LastErrorStatus = Status;
	}

	if (Message != NULL) {
		sqlite3_free(Message);
	}

	if (SqlStatement) {
		BspFree(SqlStatement);
	}

	return Status;
}

BOOLEAN
SqlIsOpen(
	IN PSQL_DATABASE Database
	)
{
	if (!Database->SqlHandle) {
		Database->LastErrorStatus = SQLITE_ERROR;
		return FALSE;
	}

	return TRUE;
}

ULONG
SqlQuery(
	IN PSQL_DATABASE Database,
	IN PWSTR Statement
	)
{
	int Status;
	int ColumnNumber;
	int RowNumber;
	PCHAR SqlStatement;
	sqlite3_stmt *stmt;
	const UCHAR *Text;
	int i;
	BOOLEAN Abort;

	if (!SqlIsOpen(Database)) {
		return Database->LastErrorStatus;
	}

	BspConvertUnicodeToUTF8(Statement, &SqlStatement);
	Status = sqlite3_prepare_v2(Database->SqlHandle, SqlStatement, -1, &stmt, NULL);
	BspFree(SqlStatement);

	if(Status != SQLITE_OK){
		Database->LastErrorStatus = Status;
		return Status;
	}
		
	Abort = FALSE;
	RowNumber = 0;
	ColumnNumber = sqlite3_column_count(stmt);
	
	do{
		Status = sqlite3_step(stmt);

		switch (Status) {
		   case SQLITE_DONE:
			   break;

		   case SQLITE_ROW:
			   for(i = 0; i < ColumnNumber; i++){
				   
				   //
				   // N.B. Data pointer will be invalid for next round,
				   // when callback return, the Data pointer is considered
				   // invalid.
				   //

				   Text = (const UCHAR *)sqlite3_column_text(stmt, i);
				   Abort = (*Database->TextCallback)(RowNumber, i, Text, Database->TextContext);
			   }
			   break;

		   default:
			   break;
		}

	} while (Status == SQLITE_ROW && Abort != TRUE);

	sqlite3_finalize(stmt);
	return Status;
}

BOOLEAN
SqlBeginTransaction(
	IN PSQL_DATABASE Database
	)
{
	ULONG Status;

	Status = SqlExecute(Database, L"BEGIN TRANSACTION");
	return (Status == SQLITE_OK) ? TRUE : FALSE;
}

BOOLEAN
SqlCommitTransaction(
	IN PSQL_DATABASE Database
	)
{
	ULONG Status;

	Status = SqlExecute(Database, L"COMMIT TRANSACTION");
	return (Status == SQLITE_OK) ? TRUE : FALSE;
}

BOOLEAN
SqlRollbackTransaction(
	IN PSQL_DATABASE Database
	)
{
	ULONG Status;

	Status = SqlExecute(Database, L"ROLLBACK TRANSACTION");
	return (Status == SQLITE_OK) ? TRUE : FALSE;
}

ULONG
SqlGetTable(
	IN PSQL_DATABASE Database,
	IN PWSTR Statement,
	OUT PSQL_TABLE *Table
	)
{
	PCHAR SqlStatement;
	CHAR **Result; 
	PCHAR Message;
	int NumberOfRows;
	int NumberOfColumns;
	PSQL_TABLE Object;
	int Status;

	Message = NULL;
	BspConvertUnicodeToUTF8(Statement, &SqlStatement);

	Status = sqlite3_get_table(Database->SqlHandle, SqlStatement, &Result, 
		                       &NumberOfRows, &NumberOfColumns, &Message);

	if (Status != SQLITE_OK) {
		Database->LastErrorStatus = Status;
		return Status;
	}

	Object = (PSQL_TABLE)BspMalloc(sizeof(SQL_TABLE));
	Object->Table = Result;
	Object->NumberOfRows = NumberOfRows;
	Object->NumberOfColumns = NumberOfColumns;

	*Table = Object;
	return S_OK;
}

ULONG
SqlFreeTable(
	IN PSQL_DATABASE Database,
	IN PSQL_TABLE Table
	)
{
	ASSERT(Table->Table != NULL);

	sqlite3_free_table(Table->Table);
	BspFree(Table);
	return S_OK;
}

ULONG
SqlRegisterQueryCallback(
	IN PSQL_DATABASE Database,
	IN SQLQUERY_CALLBACK Callback,
	IN PVOID Context
	)
{
	ASSERT(Database != NULL);

	Database->TextCallback = Callback;
	Database->TextContext = Context;
	return S_OK;
}

ULONG
SqlUnregisterQueryCallback(
	IN PSQL_DATABASE Database,
	IN SQLQUERY_CALLBACK Callback
	)
{
	ASSERT(Database->TextCallback == Callback);
	Database->TextCallback = NULL;
	return S_OK;
}

ULONG
SqlRegisterProfileCallback(
	IN PSQL_DATABASE Database,
	IN SQLPROFILE_CALLBACK ProfileCallback,
	IN PVOID Context
	)
{
	sqlite3_profile(Database->SqlHandle, ProfileCallback, Context);
	return S_OK;
}

ULONG
SqlQueryProfile(
	OUT PSQL_PROFILE Profile 
	)
{
	EnterCriticalSection(&SqlProfileLock);
	RtlCopyMemory(Profile, &SqlProfile, sizeof(SqlProfile));
	LeaveCriticalSection(&SqlProfileLock);

	return S_OK;
}

ULONG CALLBACK
SqlProfileThread(
	IN PVOID Context
	)
{
	PSQL_DATABASE Database;
	
	Database = (PSQL_DATABASE)Context;
	UNREFERENCED_PARAMETER(Database);

	return S_OK;
}