//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "mdb.h"
#include "sql.h"
#include <stdlib.h>
#include "euladlg.h"

WCHAR MdbPath[MAX_PATH];
PSQL_DATABASE MdbObject;
CRITICAL_SECTION MdbLock;

MDB_DATA_ITEM MdbData[] = {
	
	//
	// Symbol
	//

	{ L"MdbSymbolPath", NULL, MdbUnicode },
	{ L"MdbOfflineDecode", NULL, MdbBoolean },

	//
	// General
	//

	{ L"MdbSearchEngine",  NULL, MdbUnicode },
	{ L"MdbAutoScroll",    NULL, MdbBoolean },
	{ L"MdbMinidumpFull",  NULL, MdbBoolean },

	//
	// Advanced
	//

	{ L"MdbEnableKernel", NULL, MdbBoolean },
	
	//
	// Others
	//
	
	{ L"MdbEulaAccept", NULL, MdbBoolean },

};

BOOLEAN 
MdbQueryCallback(
	IN ULONG Row,
	IN ULONG Column,
	IN const UCHAR *Data,
	IN PVOID Context
	)
{
	PWCHAR Unicode;
	WCHAR Buffer[MAX_PATH];

	BspConvertUTF8ToUnicode((PCHAR)Data, &Unicode);
	if (!wcscmp(Unicode, (PWSTR)Context)) {
		StringCchPrintf(Buffer, MAX_PATH, L"MdbQueryCallback: R=%d, C=%d, %ws", 
			            Row, Column, Unicode);
		BspFree(Unicode);
		DebugTraceW(Buffer);
		return TRUE;
	}

	BspFree(Unicode);
	return FALSE;
}

ULONG
MdbSetDefaultValues(
	IN PWSTR Path
	)
{
	ULONG Status;
	WCHAR Sql[MAX_PATH];
	WCHAR Buffer[MAX_PATH];

	if (!MdbObject) {
		Status = SqlOpen(Path, &MdbObject);
		if (Status != S_OK) {
			return Status;
		}
	}

	SqlExecute(MdbObject, L"DROP TABLE MetaData IF EXISTS MetaData");
	Status = SqlExecute(MdbObject, L"CREATE TABLE MetaData (Key text PRIMARY KEY, Value text)");
	if (Status != SQLITE_OK) {
		return Status;
	}
	
	SqlBeginTransaction(MdbObject);

	Buffer[0] = L'\0';
	GetEnvironmentVariable(L"_NT_SYMBOL_PATH", Buffer, MAX_PATH);
	StringCchPrintf(Sql, MAX_PATH, L"INSERT INTO MetaData VALUES(\"MdbSymbolPath\", \"%ws\")", Buffer);
	SqlExecute(MdbObject, Sql);
	
	SqlExecute(MdbObject, L"INSERT INTO MetaData VALUES(\"MdbOfflineDecode\", \"0\")");

	SqlExecute(MdbObject, L"INSERT INTO MetaData VALUES(\"MdbSearchEngine\", \"http://www.google.com\")");
	SqlExecute(MdbObject, L"INSERT INTO MetaData VALUES(\"MdbAutoScroll\", \"0\")");
	SqlExecute(MdbObject, L"INSERT INTO MetaData VALUES(\"MdbMinidumpFull\", \"0\")");

	SqlExecute(MdbObject, L"INSERT INTO MetaData VALUES(\"MdbEnableKernel\", \"0\")");
	SqlExecute(MdbObject, L"INSERT INTO MetaData VALUES(\"MdbEulaAccept\", \"0\")");

	SqlCommitTransaction(MdbObject);
	return S_OK;
}

ULONG
MdbInitialize(
	IN BOOLEAN Load	
	)
{
	ULONG Status;

	//
	// Create .\sym and .\log path if it does not exist
	//

	MdbCreatePath();

	wcscpy_s(MdbPath, MAX_PATH, SdkProcessPathW);
	StringCchCat(MdbPath, MAX_PATH, L"\\dprobe.db");

	Status = SqlOpen(MdbPath, &MdbObject);
	if (Status != S_OK) {
		MdbObject = NULL;
		return STATUS_MDB_OPENFAILED;
	}

	InitializeCriticalSection(&MdbLock);

	if (Load) {
		return MdbLoadMetaData(MdbObject);
	} else {
		MdbSetDefaultValues(MdbPath);
		return MdbLoadMetaData(MdbObject);
	}
}

VOID
MdbClose(
	VOID
	)
{
	SqlClose(MdbObject);
	MdbObject = NULL;
	DeleteCriticalSection(&MdbLock);
}

ULONG
MdbLoadMetaData(
	IN PSQL_DATABASE SqlObject
	)
{
	ULONG Status;
	ULONG i, j;
	PSQL_TABLE Table; 
	PSTR *Value;
	ULONG Index;
	PWCHAR Buffer;

	Status = SqlGetTable(SqlObject, L"SELECT * FROM MetaData", &Table);
	if (Status != S_OK) {
		return Status;
	}

	Value = Table->Table;
	for(i = 0; i < Table->NumberOfRows; i += 1) {
		for(j = 0; j < Table->NumberOfColumns; j += 1) {
			Index = (i * Table->NumberOfColumns) + Table->NumberOfColumns + j;
			if (j == 1) {
				BspConvertUTF8ToUnicode(Value[Index], &Buffer);
				MdbData[i].Value = Buffer;
			}
		}
	}

	SqlFreeTable(SqlObject, Table);

#ifdef _DEBUG
	MdbDumpMetaData();
#endif

	return Status;
}

PMDB_DATA_ITEM
MdbGetData(
	IN MDB_DATA_OFFSET Offset
	)
{
	PMDB_DATA_ITEM Item;

	EnterCriticalSection(&MdbLock);
	Item = &MdbData[Offset];
	LeaveCriticalSection(&MdbLock);

	return Item;
}

ULONG
MdbSetData(
	IN MDB_DATA_OFFSET Offset,
	IN PWSTR Value
	)
{
	ULONG Status;
	PMDB_DATA_ITEM Item;
	WCHAR Sql[MAX_PATH];
	PWCHAR Buffer;
	SIZE_T Length;

	ASSERT(Value != NULL);

	EnterCriticalSection(&MdbLock);

	Item = &MdbData[Offset];
	if (Item->Value != NULL && !wcsicmp(Value, Item->Value)) {
		LeaveCriticalSection(&MdbLock);
		return S_OK;
	}

	StringCchPrintf(Sql, MAX_PATH, L"UPDATE MetaData SET Value=\"%s\" WHERE Key=\"%s\"", Value, Item->Key);

	ASSERT(MdbObject != NULL);
	SqlBeginTransaction(MdbObject);

	Status = SqlExecute(MdbObject, Sql);
	if (Status != S_OK) {
		SqlRollbackTransaction(MdbObject);
		LeaveCriticalSection(&MdbLock);
		return Status;
	}

	SqlCommitTransaction(MdbObject);

#ifdef _DEBUG
	{
		SqlRegisterQueryCallback(MdbObject, MdbQueryCallback, Value);
		StringCchPrintf(Sql, MAX_PATH, L"SELECT Value FROM MetaData WHERE Key=\"%ws\"", Item->Key);
		SqlQuery(MdbObject, Sql);
		SqlUnregisterQueryCallback(MdbObject, MdbQueryCallback);
	}
#endif

	if (Item->Value != NULL) {
		BspFree(Item->Value);
	}

	StringCchLength(Value, MAX_PATH, &Length);
	ASSERT(Length != 0);

	Buffer = (PWCHAR)BspMalloc((ULONG)(Length + 1) * sizeof(WCHAR));
	StringCchCopy(Buffer, Length + 1, Value);
	Item->Value = Buffer;

	LeaveCriticalSection(&MdbLock);
	return S_OK;
}

VOID
MdbDumpMetaData(
	VOID
	)
{
	ULONG i;
	ULONG Number;
	WCHAR Buffer[MAX_PATH * 2];

	Number = sizeof(MdbData) / sizeof(MDB_DATA_ITEM);
	EnterCriticalSection(&MdbLock);

	for(i = 0; i < Number; i++) {
		StringCchPrintf(Buffer, MAX_PATH * 2, L"#%02d %ws = %ws", i, MdbData[i].Key, MdbData[i].Value);	
		DebugTraceW(Buffer);
	}

	LeaveCriticalSection(&MdbLock);
}

VOID
MdbGetSymbolPath(
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	WCHAR Path[MAX_PATH];
	WCHAR SymbolPath[MAX_PATH * 2];

	Path[0] = 0;
	SymbolPath[0] = 0;

	GetEnvironmentVariable(L"_NT_SYMBOL_PATH", SymbolPath, MAX_PATH);
	if (!wcslen(SymbolPath)) {
	
		//
		// _NT_SYMBOL_PATH is not set, use dprobe private symbol path instead
		//

		GetCurrentDirectory(MAX_PATH, Path);
		StringCchPrintf(SymbolPath, MAX_PATH * 2, 
			            L"%s;SRV*%s\\sym*http://msdl.microsoft.com/download/symbols", 
						Path, Path);
	}

	StringCchCopy(Buffer, Length, SymbolPath);
	MdbSetData(MdbSymbolPath, SymbolPath);
}

VOID
MdbGetCachePath(
	IN PWCHAR Buffer, 
	IN ULONG Length
	)
{
	WCHAR Path[MAX_PATH];

	GetCurrentDirectory(MAX_PATH, Path);
	StringCchPrintf(Buffer, Length, L"%s\\log", Path);
}

VOID
MdbCreatePath(
	VOID
	)
{
	WCHAR Buffer[MAX_PATH];

	//
	// Create sym path
	//

	StringCchPrintfW(Buffer, MAX_PATH, L"%s\\sym", SdkProcessPathW);
	CreateDirectoryW(Buffer, NULL);

	//
	// Create log path
	//

	StringCchPrintfW(Buffer, MAX_PATH, L"%s\\log", SdkProcessPathW);
	CreateDirectoryW(Buffer, NULL);
}

ULONG
MdbEnsureEula(
	VOID
	)
{
	INT_PTR Return;
	PMDB_DATA_ITEM Item;

	Item = MdbGetData(MdbEulaAccept);
	if (!wcsicmp(Item->Value, L"1")) {
		return S_OK;
	}

	Return = EulaDialog(NULL);
	if (Return == IDOK) {
		MdbSetData(MdbEulaAccept, L"1");
		return S_OK;
	}

	return S_FALSE;
}