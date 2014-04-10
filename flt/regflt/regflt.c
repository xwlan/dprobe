//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "regflt.h"

#define BUFFER_OVERFLOW L"BUFFER OVERFLOW"
#define STATUS_SUCCESS          ((NTSTATUS)0L)
#define STATUS_BUFFER_TOO_SMALL ((NTSTATUS)0xC0000023L)

typedef struct _KEY_NAME_INFORMATION {
  ULONG  NameLength;
  WCHAR  Name[1];
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;


LONG WINAPI 
RegCreateKeyAEnter(
	IN  HKEY hKey,
	IN  PSTR lpSubKey,
	OUT PHKEY phkResult
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGCREATEKEY_ANSI Data;
	ULONG Status;
	ULONG LastError;
	REGCREATEKEYA_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;

	BtrFltGetContext(&Context);
	Routine = (REGCREATEKEYA_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpSubKey, phkResult);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGCREATEKEY_ANSI),
		                          RegGuid, _RegCreateKeyA);

	Data = FILTER_RECORD_POINTER(Record, REG_REGCREATEKEY_ANSI);
	Data->hKey = hKey;
	Data->phkResult = phkResult;
	Data->Status = Status;
	Data->LastErrorStatus = LastError;
	Data->SubKey[0] = 0;

	if (lpSubKey) {
		StringCchCopyA(Data->SubKey, MAX_PATH, lpSubKey); 
	}

	if (phkResult) {
		Data->hResultKey = *phkResult;
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegCreateKeyADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGCREATEKEY_ANSI Data;

	if (Record->ProbeOrdinal != _RegCreateKeyA) {
		return S_FALSE;
	}	

	Data = (PREG_REGCREATEKEY_ANSI)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=0x%p, lpSubKey=%S, phkResult=0x%p (0x%p), Status=0x%08x, LastErrorStatus=0x%08x",
		Data->hKey, Data->lpSubKey, Data->phkResult, Data->hResultKey, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegCreateKeyWEnter(
	IN  HKEY hKey,
	IN  PWSTR lpSubKey,
	OUT PHKEY phkResult
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGCREATEKEY_UNICODE Data;
	ULONG Status, LastError;
	REGCREATEKEYW_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;

	BtrFltGetContext(&Context);
	Routine = (REGCREATEKEYW_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpSubKey, phkResult);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGCREATEKEY_UNICODE),
		                          RegGuid, _RegCreateKeyW);

	Data = FILTER_RECORD_POINTER(Record, REG_REGCREATEKEY_UNICODE);
	Data->hKey = hKey;
	Data->phkResult = phkResult;
	Data->Status = Status;
	Data->LastErrorStatus = LastError;

	if (lpSubKey) {
		StringCchCopyW(Data->SubKey, MAX_PATH, lpSubKey); 
	}

	if (phkResult) {
		Data->hResultKey = *phkResult;
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG 
RegCreateKeyWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGCREATEKEY_UNICODE Data;

	if (Record->ProbeOrdinal != _RegCreateKeyW) {
		return S_FALSE;
	}	

	Data = (PREG_REGCREATEKEY_UNICODE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=0x%p, lpSubKey=%s, phkResult=0x%p (0x%p), Status=0x%08x, LastErrorStatus=0x%08x",
		Data->hKey, Data->lpSubKey, Data->phkResult, Data->hResultKey, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegCreateKeyExAEnter(
	IN HKEY hKey,
	IN PSTR lpSubKey,
	IN DWORD Reserved,
	IN PSTR lpClass,
	IN DWORD dwOptions,
	IN REGSAM samDesired,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	OUT PHKEY phkResult,
	OUT LPDWORD lpdwDisposition
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGCREATEKEYEX_ANSI Filter;
	REGCREATEKEYEXA_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGCREATEKEYEXA_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpSubKey, Reserved, lpClass, 
		                dwOptions, samDesired, lpSecurityAttributes,
						phkResult, lpdwDisposition);

	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGCREATEKEYEX_ANSI),
		                          RegGuid, _RegCreateKeyExA);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGCREATEKEYEX_ANSI);
	Filter->hKey = hKey;
	Filter->Reserved = Reserved;
	Filter->dwOptions = dwOptions;
	Filter->samDesired = samDesired;
	Filter->lpSecurityAttributes = lpSecurityAttributes;
	Filter->phkResult = phkResult;
	Filter->lpdwDisposition = lpdwDisposition;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;
	Filter->SubKey[0] = 0;

	if (lpSubKey != NULL) {
		StringCchCopyA(Filter->SubKey, MAX_PATH, lpSubKey);
	}

	if (lpClass != NULL) {
		StringCchCopyA(Filter->Class, MAX_PATH, lpClass);
	}

	if (phkResult) {
		Filter->hResultKey = *phkResult;
	}

	if (lpdwDisposition) {
		Filter->dwDisposition = *lpdwDisposition;
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegCreateKeyExADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGCREATEKEYEX_ANSI Data;
	ULONG Status;
	WCHAR Name[MAX_PATH];

	if (Record->ProbeOrdinal != _RegCreateKeyExA) {
		return S_FALSE;
	}	

	Data = (PREG_REGCREATEKEYEX_ANSI)Record->Data;
	Status = RegFormatKnownKeyName(Data->hKey, Name, MAX_PATH);
	if (Status != S_OK) {
		StringCchPrintf(Name, MAX_PATH, L"0x%p", Data->hKey);
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=%s, lpSubKey=%S, lpClass=0x%p (%S), dwOptions=0x%x, samDesired=0x%x"
		L"lpSecurityAttributes=0x%p, phkResult=0x%p (0x%p), lpdwDisposition=0x%p (0x%x)"
		L"Status=0x%08x, LastErrorStatus=0x%08x", 
		Name, Data->SubKey, Data->lpClass, Data->Class, 
		Data->dwOptions, Data->samDesired, Data->lpSecurityAttributes, Data->phkResult, Data->hResultKey, 
		Data->lpdwDisposition, Data->dwDisposition, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegCreateKeyExWEnter(
	IN HKEY hKey,
	IN PWSTR lpSubKey,
	IN DWORD Reserved,
	IN PWSTR lpClass,
	IN DWORD dwOptions,
	IN REGSAM samDesired,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	OUT PHKEY phkResult,
	OUT LPDWORD lpdwDisposition
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGCREATEKEYEX_UNICODE Filter;
	REGCREATEKEYEXW_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGCREATEKEYEXW_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpSubKey, Reserved, lpClass, 
		                dwOptions, samDesired, lpSecurityAttributes,
						phkResult, lpdwDisposition);

	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGCREATEKEYEX_UNICODE),
		                          RegGuid, _RegCreateKeyExW);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGCREATEKEYEX_UNICODE);
	Filter->hKey = hKey;
	Filter->Reserved = Reserved;
	Filter->dwOptions = dwOptions;
	Filter->samDesired = samDesired;
	Filter->lpSecurityAttributes = lpSecurityAttributes;
	Filter->phkResult = phkResult;
	Filter->lpdwDisposition = lpdwDisposition;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;
	Filter->SubKey[0] = 0;

	if (lpSubKey != NULL) {
		StringCchCopyW(Filter->SubKey, MAX_PATH, lpSubKey);
	}

	if (lpClass != NULL) {
		StringCchCopyW(Filter->Class, MAX_PATH, lpClass);
	}

	if (phkResult) {
		Filter->hResultKey = *phkResult;
	}

	if (lpdwDisposition) {
		Filter->dwDisposition = *lpdwDisposition;
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG 
RegCreateKeyExWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGCREATEKEYEX_UNICODE Data;
	ULONG Status;
	WCHAR Name[MAX_PATH];

	if (Record->ProbeOrdinal != _RegCreateKeyExW) {
		return S_FALSE;
	}	

	Data = (PREG_REGCREATEKEYEX_UNICODE)Record->Data;
	Status = RegFormatKnownKeyName(Data->hKey, Name, MAX_PATH);
	if (Status != S_OK) {
		StringCchPrintf(Name, MAX_PATH, L"0x%p", Data->hKey);
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=%s, lpSubKey=%s, lpClass=0x%p (%s), dwOptions=0x%x, samDesired=0x%x"
		L"lpSecurityAttributes=0x%p, phkResult=0x%p (0x%p), lpdwDisposition=0x%p (0x%x)"
		L"Status=0x%08x, LastErrorStatus=0x%08x", 
		Name, Data->SubKey, Data->lpClass, Data->Class, 
		Data->dwOptions, Data->samDesired, Data->lpSecurityAttributes, Data->phkResult, Data->hResultKey, 
		Data->lpdwDisposition, Data->dwDisposition, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegDeleteKeyAEnter(
	IN HKEY hKey,
	IN PSTR lpSubKey
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGDELETEKEY_ANSI Filter;
	REGDELETEKEYA_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGDELETEKEYA_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpSubKey);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGDELETEKEY_ANSI),
		                          RegGuid, _RegDeleteKeyA);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGDELETEKEY_ANSI);
	Filter->hKey = hKey;
	Filter->lpSubKey = lpSubKey;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;

	if (lpSubKey != NULL) {
		StringCchCopyA(Filter->SubKey, MAX_PATH, lpSubKey);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegDeleteKeyADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGDELETEKEY_ANSI Data;

	if (Record->ProbeOrdinal != _RegDeleteKeyA) {
		return S_FALSE;
	}	

	Data = (PREG_REGDELETEKEY_ANSI)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=0x%p, lpSubKey=0x%p (%S), Status=0x%08x, LastErrorStatus=0x%08x", 
		Data->hKey, Data->lpSubKey, Data->SubKey, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegDeleteKeyWEnter(
	IN HKEY hKey,
	IN PWSTR lpSubKey
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGDELETEKEY_UNICODE Filter;
	REGDELETEKEYW_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGDELETEKEYW_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpSubKey);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGDELETEKEY_UNICODE),
		                          RegGuid, _RegDeleteKeyW);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGDELETEKEY_UNICODE);
	Filter->hKey = hKey;
	Filter->lpSubKey = lpSubKey;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;

	if (lpSubKey != NULL) {
		StringCchCopyW(Filter->SubKey, MAX_PATH, lpSubKey);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegDeleteKeyWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGDELETEKEY_UNICODE Data;

	if (Record->ProbeOrdinal != _RegDeleteKeyW) {
		return S_FALSE;
	}	

	Data = (PREG_REGDELETEKEY_UNICODE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=0x%p, lpSubKey=0x%p (%s), Status=0x%08x, LastErrorStatus=0x%08x", 
		Data->hKey, Data->lpSubKey, Data->SubKey, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegDeleteKeyExAEnter(
	IN HKEY hKey,
	IN PSTR lpSubKey,
	IN REGSAM samDesired,
	IN DWORD Reserved
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGDELETEKEYEX_ANSI Filter;
	REGDELETEKEYEXA_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGDELETEKEYEXA_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpSubKey, samDesired, Reserved);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGDELETEKEYEX_ANSI),
		                          RegGuid, _RegDeleteKeyExA);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGDELETEKEYEX_ANSI);
	Filter->hKey = hKey;
	Filter->samDesired = samDesired;
	Filter->Reserved = Reserved;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;

	if (lpSubKey != NULL) {
		StringCchCopyA(Filter->SubKey, MAX_PATH, lpSubKey);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegDeleteKeyExADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGDELETEKEYEX_ANSI Data;

	if (Record->ProbeOrdinal != _RegDeleteKeyExA) {
		return S_FALSE;
	}	

	Data = (PREG_REGDELETEKEYEX_ANSI)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=0x%p, lpSubKey=0x%p (%S), samDesired=0x%x, Status=0x%08x, LastErrorStatus=0x%08x", 
		Data->hKey, Data->lpSubKey, Data->SubKey, Data->samDesired, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegDeleteKeyExWEnter(
	IN HKEY hKey,
	IN PWSTR lpSubKey,
	IN REGSAM samDesired,
	IN DWORD Reserved
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGDELETEKEYEX_UNICODE Filter;
	REGDELETEKEYEXW_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGDELETEKEYEXW_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpSubKey, samDesired, Reserved);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGDELETEKEYEX_UNICODE),
		                          RegGuid, _RegDeleteKeyExW);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGDELETEKEYEX_UNICODE);
	Filter->hKey = hKey;
	Filter->samDesired = samDesired;
	Filter->Reserved = Reserved;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;

	if (lpSubKey != NULL) {
		StringCchCopyW(Filter->SubKey, MAX_PATH, lpSubKey);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegDeleteKeyExWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGDELETEKEYEX_UNICODE Data;

	if (Record->ProbeOrdinal != _RegDeleteKeyExW) {
		return S_FALSE;
	}	

	Data = (PREG_REGDELETEKEYEX_UNICODE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=0x%p, lpSubKey=0x%p (%s), samDesired=0x%x, Status=0x%08x, LastErrorStatus=0x%08x", 
		Data->hKey, Data->lpSubKey, Data->SubKey, Data->samDesired, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegDeleteKeyValueAEnter(
	IN HKEY hKey,
	IN PSTR lpSubKey,
	IN PSTR lpValueName
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGDELETEKEYVALUE_ANSI Filter;
	REGDELETEKEYVALUEA_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGDELETEKEYVALUEA_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpSubKey, lpValueName);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGDELETEKEYVALUE_ANSI),
		                          RegGuid, _RegDeleteKeyValueA);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGDELETEKEYVALUE_ANSI);
	Filter->hKey = hKey;
	Filter->lpSubKey = lpSubKey;
	Filter->lpValueName = lpValueName;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;

	if (lpSubKey != NULL) {
		StringCchCopyA(Filter->SubKey, MAX_PATH, lpSubKey);
	}

	if (lpValueName != NULL) {
		StringCchCopyA(Filter->ValueName, MAX_PATH, lpValueName);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegDeleteKeyValueADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGDELETEKEYVALUE_ANSI Data;

	if (Record->ProbeOrdinal != _RegDeleteKeyValueA) {
		return S_FALSE;
	}	

	Data = (PREG_REGDELETEKEYVALUE_ANSI)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=0x%p, lpSubKey=0x%p (%S), lpValueName=0x%p (%S), Status=0x%08x, LastErrorStatus=0x%08x", 
		Data->hKey, Data->lpSubKey, Data->SubKey, Data->lpValueName, Data->ValueName, 
		Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegDeleteKeyValueWEnter(
	IN HKEY hKey,
	IN PWSTR lpSubKey,
	IN PWSTR lpValueName
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGDELETEKEYVALUE_UNICODE Filter;
	REGDELETEKEYVALUEW_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGDELETEKEYVALUEW_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpSubKey, lpValueName);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGDELETEKEYVALUE_UNICODE),
		                          RegGuid, _RegDeleteKeyValueW);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGDELETEKEYVALUE_UNICODE);
	Filter->hKey = hKey;
	Filter->lpSubKey = lpSubKey;
	Filter->lpValueName = lpValueName;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;

	if (lpSubKey != NULL) {
		StringCchCopyW(Filter->SubKey, MAX_PATH, lpSubKey);
	}

	if (lpValueName != NULL) {
		StringCchCopyW(Filter->ValueName, MAX_PATH, lpValueName);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegDeleteKeyValueWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGDELETEKEYVALUE_UNICODE Data;

	if (Record->ProbeOrdinal != _RegDeleteKeyValueW) {
		return S_FALSE;
	}	

	Data = (PREG_REGDELETEKEYVALUE_UNICODE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=0x%p, lpSubKey=0x%p (%s), lpValueName=0x%p (%s), Status=0x%08x, LastErrorStatus=0x%08x", 
		Data->hKey, Data->lpSubKey, Data->SubKey, Data->lpValueName, Data->ValueName, 
		Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegDeleteValueAEnter(
	IN HKEY hKey,
	IN PSTR lpValueName
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGDELETEVALUE_ANSI Filter;
	REGDELETEVALUEA_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGDELETEVALUEA_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpValueName);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGDELETEVALUE_ANSI),
		                          RegGuid, _RegDeleteValueA);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGDELETEVALUE_ANSI);
	Filter->hKey = hKey;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;

	if (lpValueName != NULL) {
		StringCchCopyA(Filter->ValueName, MAX_PATH, lpValueName);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegDeleteValueADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGDELETEVALUE_ANSI Data;

	if (Record->ProbeOrdinal != _RegDeleteValueA) {
		return S_FALSE;
	}	

	Data = (PREG_REGDELETEVALUE_ANSI)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=0x%p, lpValueName=0x%p (%S), Status=0x%08x, LastErrorStatus=0x%08x", 
		Data->hKey, Data->lpValueName, Data->ValueName, 
		Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegDeleteValueWEnter(
	IN HKEY hKey,
	IN PWSTR lpValueName
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGDELETEVALUE_UNICODE Filter;
	REGDELETEVALUEW_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGDELETEVALUEW_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpValueName);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGDELETEVALUE_UNICODE),
		                          RegGuid, _RegDeleteValueW);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGDELETEVALUE_UNICODE);
	Filter->hKey = hKey;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;

	if (lpValueName != NULL) {
		StringCchCopyW(Filter->ValueName, MAX_PATH, lpValueName);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegDeleteValueWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGDELETEVALUE_UNICODE Data;

	if (Record->ProbeOrdinal != _RegDeleteValueW) {
		return S_FALSE;
	}	

	Data = (PREG_REGDELETEVALUE_UNICODE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=0x%p, lpValueName=0x%p (%s), Status=0x%08x, LastErrorStatus=0x%08x", 
		Data->hKey, Data->lpValueName, Data->ValueName, 
		Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegDeleteTreeEnter(
	IN HKEY hKey,
	IN PWSTR lpSubKey
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGDELETETREE Filter;
	REGDELETETREE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGDELETETREE_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpSubKey);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGDELETETREE),
		                          RegGuid, _RegDeleteTreeW);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGDELETETREE);
	Filter->hKey = hKey;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;

	if (lpSubKey != NULL) {
		StringCchCopyW(Filter->SubKey, MAX_PATH, lpSubKey);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegDeleteTreeDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGDELETETREE Data;

	if (Record->ProbeOrdinal != _RegDeleteTreeW) {
		return S_FALSE;
	}	

	Data = (PREG_REGDELETETREE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=0x%p, lpSubKey=0x%p (%s), Status=0x%08x, LastErrorStatus=0x%08x", 
		Data->hKey, Data->lpSubKey, Data->SubKey, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegEnumKeyExAEnter(
	IN  HKEY hKey,
	IN  DWORD dwIndex,
	OUT PSTR lpName,
	IN  LPDWORD lpcName,
	IN  LPDWORD lpReserved,
	OUT PSTR lpClass,
	OUT LPDWORD lpcClass,
	OUT PFILETIME lpftLastWriteTime
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGENUMKEYEX_ANSI Filter;
	REGENUMKEYEXA_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGENUMKEYEXA_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, dwIndex, lpName, lpcName,
		                lpReserved, lpClass, lpcClass, lpftLastWriteTime);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGENUMKEYEX_ANSI),
		                          RegGuid, _RegEnumKeyExA);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGENUMKEYEX_ANSI);
	RtlZeroMemory(Filter, sizeof(*Filter));

	Filter->hKey = hKey;
	Filter->dwIndex = dwIndex;
	Filter->lpName = lpName;
	Filter->lpcName = lpcName;
	Filter->lpReserved = lpReserved;
	Filter->lpClass = lpClass;
	Filter->lpcClass = lpcClass;
	Filter->lpftLastWriteTime = lpftLastWriteTime;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;

	if (lpName)
		StringCchCopyA(Filter->Name, MAX_PATH, lpName);

	if (lpClass) 
		StringCchCopyA(Filter->Class, MAX_PATH, lpClass);

	if (lpcName)
		Filter->cName = *lpcName;

	if (lpcClass)
		Filter->cClass = *lpcClass;
	
	if (lpftLastWriteTime)
		Filter->LastWriteTime = *lpftLastWriteTime;

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegEnumKeyExADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGENUMKEYEX_ANSI Data;
	ULONG Status;
	WCHAR Name[MAX_PATH];

	if (Record->ProbeOrdinal != _RegEnumKeyExA) {
		return S_FALSE;
	}	

	Data = (PREG_REGENUMKEYEX_ANSI)Record->Data;
	Status = RegFormatKnownKeyName(Data->hKey, Name, MAX_PATH);
	if (Status != S_OK) {
		StringCchPrintf(Name, MAX_PATH, L"0x%p", Data->hKey);
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=%s, dwIndex=0x%x, lpName=0x%p (%S), lpcName=0x%p (0x%x), lpClass=0x%p (%S), lpcClass=0x%p (0x%x)"
		L"lpftLastWriteTime=0x%p, Status=0x%08x, LastErrorStatus=0x%08x", 
		Name, Data->dwIndex, Data->lpName, Data->Name, Data->lpcName, Data->cName, 
		Data->lpClass, Data->Class, Data->lpcClass, Data->cClass, Data->lpftLastWriteTime, 
		Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegEnumKeyExWEnter(
	IN  HKEY hKey,
	IN  DWORD dwIndex,
	OUT PWSTR lpName,
	IN  LPDWORD lpcName,
	IN  LPDWORD lpReserved,
	OUT PWSTR lpClass,
	OUT LPDWORD lpcClass,
	OUT PFILETIME lpftLastWriteTime
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGENUMKEYEX_UNICODE Filter;
	REGENUMKEYEXW_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGENUMKEYEXW_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, dwIndex, lpName, lpcName,
		                lpReserved, lpClass, lpcClass, lpftLastWriteTime);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGENUMKEYEX_UNICODE),
		                          RegGuid, _RegEnumKeyExW);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGENUMKEYEX_UNICODE);
	RtlZeroMemory(Filter, sizeof(*Filter));

	Filter->hKey = hKey;
	Filter->dwIndex = dwIndex;
	Filter->lpName = lpName;
	Filter->lpcName = lpcName;
	Filter->lpReserved = lpReserved;
	Filter->lpClass = lpClass;
	Filter->lpcClass = lpcClass;
	Filter->lpftLastWriteTime = lpftLastWriteTime;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;

	if (lpName)
		StringCchCopyW(Filter->Name, MAX_PATH, lpName);

	if (lpClass) 
		StringCchCopyW(Filter->Class, MAX_PATH, lpClass);

	if (lpcName)
		Filter->cName = *lpcName;

	if (lpcClass)
		Filter->cClass = *lpcClass;
	
	if (lpftLastWriteTime)
		Filter->LastWriteTime = *lpftLastWriteTime;

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegEnumKeyExWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGENUMKEYEX_UNICODE Data;
	ULONG Status;
	WCHAR Name[MAX_PATH];

	if (Record->ProbeOrdinal != _RegEnumKeyExW) {
		return S_FALSE;
	}	

	Data = (PREG_REGENUMKEYEX_UNICODE)Record->Data;
	Status = RegFormatKnownKeyName(Data->hKey, Name, MAX_PATH);
	if (Status != S_OK) {
		StringCchPrintf(Name, MAX_PATH, L"0x%p", Data->hKey);
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=0x%p, dwIndex=0x%x, lpName=0x%p (%s), lpcName=0x%p (0x%x), lpClass=0x%p (%s), lpcClass=0x%p (0x%x)"
		L"lpftLastWriteTime=0x%p, Status=0x%08x, LastErrorStatus=0x%08x", 
		Name, Data->dwIndex, Data->lpName, Data->Name, Data->lpcName, Data->cName, 
		Data->lpClass, Data->Class, Data->lpcClass, Data->cClass, Data->lpftLastWriteTime, 
		Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegSetValueExAEnter(
	IN HKEY hKey,
	IN PSTR lpValueName,
	IN DWORD Reserved,
	IN DWORD dwType,
	IN const BYTE* lpData,
	IN DWORD cbData
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGSETVALUEEX_ANSI Filter;
	REGSETVALUEEXA_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGSETVALUEEXA_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpValueName, Reserved, dwType, lpData, cbData);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGSETVALUEEX_ANSI) + cbData,
		                          RegGuid, _RegSetValueExA);
		                             
	Filter = FILTER_RECORD_POINTER(Record, REG_REGSETVALUEEX_ANSI);
	Filter->hKey = hKey;
	Filter->Reserved = Reserved;
	Filter->dwType = dwType;
	Filter->lpData = (LPBYTE)lpData;
	Filter->cbData = cbData;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;
	Filter->ValueName[0] = 0;

	if (lpValueName != NULL) {
		StringCchCopyA(Filter->ValueName, 32, lpValueName);
	}

	if (cbData > 0) {
		memcpy(&Filter->Data[0], lpData, cbData);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegSetValueExADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGSETVALUEEX_ANSI Data;

	if (Record->ProbeOrdinal != _RegSetValueExA) {
		return S_FALSE;
	}	

	Data = (PREG_REGSETVALUEEX_ANSI)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=0x%p, lpValueName=0x%p (%S), dwType=0x%x, lpData=0x%p, cbData=0x%x"
		L"Status=0x%08x, LastErrorStatus=0x%08x", 
		Data->hKey, Data->lpValueName, Data->ValueName, Data->dwType, Data->lpData, Data->cbData,
		Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegSetValueExWEnter(
	IN HKEY hKey,
	IN PWSTR lpValueName,
	IN DWORD Reserved,
	IN DWORD dwType,
	IN const BYTE* lpData,
	IN DWORD cbData
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGSETVALUEEX_UNICODE Filter;
	REGSETVALUEEXW_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGSETVALUEEXW_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpValueName, Reserved, dwType, lpData, cbData);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGSETVALUEEX_UNICODE) + cbData,
		                          RegGuid, _RegSetValueExW);
		                             
	Filter = FILTER_RECORD_POINTER(Record, REG_REGSETVALUEEX_UNICODE);
	Filter->hKey = hKey;
	Filter->Reserved = Reserved;
	Filter->dwType = dwType;
	Filter->lpData = (LPBYTE)lpData;
	Filter->cbData = cbData;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;
	Filter->ValueName[0] = 0;

	if (lpValueName != NULL) {
		StringCchCopyW(Filter->ValueName, 32, lpValueName);
	}

	if (cbData > 0) {
		memcpy(&Filter->Data[0], lpData, cbData);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegSetValueExWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGSETVALUEEX_UNICODE Data;

	if (Record->ProbeOrdinal != _RegSetValueExW) {
		return S_FALSE;
	}	

	Data = (PREG_REGSETVALUEEX_UNICODE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=0x%p, lpValueName=0x%p (%s), dwType=0x%x, lpData=0x%p, cbData=0x%x"
		L"Status=0x%08x, LastErrorStatus=0x%08x", 
		Data->hKey, Data->lpValueName, Data->ValueName, Data->dwType, Data->lpData, Data->cbData,
		Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegQueryValueExAEnter(
	IN HKEY hKey,
	IN PSTR lpValueName,
	IN LPDWORD lpReserved,
	OUT LPDWORD lpType,
	OUT LPBYTE lpData,
	OUT LPDWORD lpcbData
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGQUERYVALUEEX_ANSI Filter;
	REGQUERYVALUEEXA_ROUTINE Routine;
	LONG Extra = 0;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGQUERYVALUEEXA_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	if (lpcbData)
		Extra = *lpcbData;

	//
	// N.B. Extra data bytes is determined after call of original routine
	//

	Record = BtrFltAllocateRecord(sizeof(REG_REGQUERYVALUEEX_ANSI) + Extra,
		                          RegGuid, _RegQueryValueExA);
		                             
	Filter = FILTER_RECORD_POINTER(Record, REG_REGQUERYVALUEEX_ANSI);
	Filter->hKey = hKey;
	Filter->lpReserved = lpReserved;
	Filter->lpType = lpType;
	Filter->lpData = lpData;
	Filter->lpcbData = lpcbData;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;
	Filter->ValueName[0] = 0;

	if (lpValueName != NULL) {
		StringCchCopyA(Filter->ValueName, MAX_PATH, lpValueName);
	}

	if (lpType)
		Filter->Type = *lpType;

	if (Extra) {
		Filter->cbData = Extra;
	}
	
	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegQueryValueExADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGQUERYVALUEEX_ANSI Data;
	ULONG Status;
	WCHAR Name[MAX_PATH];

	if (Record->ProbeOrdinal != _RegQueryValueExA) {
		return S_FALSE;
	}	

	Data = (PREG_REGQUERYVALUEEX_ANSI)Record->Data;
	Status = RegFormatKnownKeyName(Data->hKey, Name, MAX_PATH);
	if (Status != S_OK) {
		StringCchPrintf(Name, MAX_PATH, L"0x%p", Data->hKey);
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=%s, lpValueName=%S, lpType=0x%p (0x%x), lpData=0x%p, lpcbData=0x%p (0x%x)"
		L"Status=0x%08x, LastErrorStatus=0x%08x", 
		Name, Data->ValueName, Data->lpType, Data->Type, 
		Data->lpData, Data->lpcbData, Data->cbData, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegQueryValueExWEnter(
	IN HKEY hKey,
	IN PWSTR lpValueName,
	IN LPDWORD lpReserved,
	OUT LPDWORD lpType,
	OUT LPBYTE lpData,
	OUT LPDWORD lpcbData
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGQUERYVALUEEX_UNICODE Filter;
	REGQUERYVALUEEXW_ROUTINE Routine;
	LONG Extra = 0;
	BOOLEAN ValidDataSize;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGQUERYVALUEEXW_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	if (hKey == HKEY_PERFORMANCE_DATA && Status == ERROR_MORE_DATA) {
		
		//
		// N.B. Refer to MSDN, under this condition, lpcbData is not undefined
		//

		ValidDataSize = FALSE;
	} else {
		ValidDataSize = TRUE;
	}

	if (lpcbData && ValidDataSize) {
		Extra = *lpcbData;
	}

	//
	// N.B. Extra data bytes is determined after call of original routine
	//

	Record = BtrFltAllocateRecord(sizeof(REG_REGQUERYVALUEEX_UNICODE),
			                      RegGuid, _RegQueryValueExW);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGQUERYVALUEEX_UNICODE);
	Filter->hKey = hKey;
	Filter->lpReserved = lpReserved;
	Filter->lpType = lpType;
	Filter->lpData = lpData;
	Filter->lpcbData = lpcbData;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;
	Filter->ValueName[0] = 0;

	if (lpValueName != NULL) {
		StringCchCopyW(Filter->ValueName, MAX_PATH, lpValueName);
	}

	if (lpType)
		Filter->Type = *lpType;

	if (Extra) {
		Filter->cbData = Extra;
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegQueryValueExWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGQUERYVALUEEX_UNICODE Data;
	ULONG Status;
	WCHAR Name[MAX_PATH];

	if (Record->ProbeOrdinal != _RegQueryValueExW) {
		return S_FALSE;
	}	

	Data = (PREG_REGQUERYVALUEEX_UNICODE)Record->Data;
	Status = RegFormatKnownKeyName(Data->hKey, Name, MAX_PATH);
	if (Status != S_OK) {
		StringCchPrintf(Name, MAX_PATH, L"0x%p", Data->hKey);
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=%s, lpValueName=%s, lpType=0x%p (0x%x), lpData=0x%p, lpcbData=0x%p (0x%x)"
		L"Status=0x%08x, LastErrorStatus=0x%08x", 
		Name, Data->ValueName, Data->lpType, Data->Type, 
		Data->lpData, Data->lpcbData, Data->cbData, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegOpenKeyExAEnter(
	IN  HKEY hKey,
	IN  PSTR lpSubKey,
	IN  DWORD ulOptions,
	IN  REGSAM samDesired,
	OUT PHKEY phkResult
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGOPENKEYEX_ANSI Filter;
	REGOPENKEYEXA_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGOPENKEYEXA_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpSubKey, ulOptions, samDesired, phkResult);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGOPENKEYEX_ANSI),
		                          RegGuid, _RegOpenKeyExA);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGOPENKEYEX_ANSI);
	Filter->hKey = hKey;
	Filter->ulOptions = ulOptions;
	Filter->samDesired = samDesired;
	Filter->phkResult = phkResult;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;
	Filter->SubKey[0] = 0;

	if (lpSubKey != NULL) 
		StringCchCopyA(Filter->SubKey, MAX_PATH, lpSubKey);

	if (phkResult)
		Filter->hResultKey = *phkResult;

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegOpenKeyExADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGOPENKEYEX_ANSI Data;
	WCHAR Name[MAX_PATH];
	ULONG Status;

	if (Record->ProbeOrdinal != _RegOpenKeyExA) {
		return S_FALSE;
	}	

	Data = (PREG_REGOPENKEYEX_ANSI)Record->Data;

	Status = RegFormatKnownKeyName(Data->hKey, Name, MAX_PATH);
	if (Status != S_OK) {
		StringCchPrintf(Name, MAX_PATH, L"0x%p", Data->hKey);
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=%s, lpSubKey=%S, ulOptions=0x%x, samDesired=0x%x, phkResult=0x%p (0x%p), Status=0x%08x, LastErrorStatus=0x%08x", 
		Name, Data->SubKey, Data->ulOptions, Data->samDesired, Data->phkResult, Data->hResultKey, 
		Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegOpenKeyExWEnter(
	IN  HKEY hKey,
	IN  PWSTR lpSubKey,
	IN  DWORD ulOptions,
	IN  REGSAM samDesired,
	OUT PHKEY phkResult
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGOPENKEYEX_UNICODE Filter;
	REGOPENKEYEXW_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGOPENKEYEXW_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpSubKey, ulOptions, samDesired, phkResult);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGOPENKEYEX_UNICODE),
		                          RegGuid, _RegOpenKeyExW);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGOPENKEYEX_UNICODE);
	Filter->hKey = hKey;
	Filter->ulOptions = ulOptions;
	Filter->samDesired = samDesired;
	Filter->phkResult = phkResult;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;
	Filter->SubKey[0] = 0;

	if (lpSubKey != NULL) {
		StringCchCopyW(Filter->SubKey, MAX_PATH, lpSubKey);
	}

	if (phkResult)
		Filter->hResultKey = *phkResult;

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegOpenKeyExWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGOPENKEYEX_UNICODE Data;
	ULONG Status;
	WCHAR Name[MAX_PATH];

	if (Record->ProbeOrdinal != _RegOpenKeyExW) {
		return S_FALSE;
	}	

	Data = (PREG_REGOPENKEYEX_UNICODE)Record->Data;
	Status = RegFormatKnownKeyName(Data->hKey, Name, MAX_PATH);
	if (Status != S_OK) {
		StringCchPrintf(Name, MAX_PATH, L"0x%p", Data->hKey);
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=%s, lpSubKey=%s, ulOptions=0x%x, samDesired=0x%x, phkResult=0x%p (0x%p), Status=0x%08x, LastErrorStatus=0x%08x", 
		Name, Data->SubKey, Data->ulOptions, Data->samDesired, Data->phkResult, Data->hResultKey, 
		Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegQueryInfoKeyAEnter(
	IN  HKEY hKey,
	OUT PSTR lpClass,
	OUT LPDWORD lpcClass,
	IN  LPDWORD lpReserved,
	OUT LPDWORD lpcSubKeys,
	OUT LPDWORD lpcMaxSubKeyLen,
	OUT LPDWORD lpcMaxClassLen,
	OUT LPDWORD lpcValues,
	OUT LPDWORD lpcMaxValueNameLen,
	OUT LPDWORD lpcMaxValueLen,
	OUT LPDWORD lpcbSecurityDescriptor,
	OUT PFILETIME lpftLastWriteTime
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGQUERYINFOKEY_ANSI Filter;
	REGQUERYINFOKEYA_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGQUERYINFOKEYA_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpClass, lpcClass, lpReserved, lpcSubKeys,
		                lpcMaxSubKeyLen, lpcMaxClassLen, lpcValues, lpcMaxValueNameLen,
						lpcMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGQUERYINFOKEY_ANSI),
		                          RegGuid, _RegQueryInfoKeyA);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGQUERYINFOKEY_ANSI);
	Filter->hKey = hKey;
	Filter->lpClass = lpClass;
	Filter->lpcClass = lpcClass;
	Filter->lpReserved = lpReserved;
	Filter->lpcSubKeys = lpcSubKeys;
	Filter->lpcMaxSubKeyLen = lpcMaxSubKeyLen;
	Filter->lpcMaxClassLen = lpcMaxClassLen;
	Filter->lpcValues = lpcValues;
	Filter->lpcMaxValueNameLen = lpcMaxValueNameLen;
	Filter->lpcMaxValueLen = lpcMaxValueLen;
	Filter->lpcbSecurityDescriptor = lpcbSecurityDescriptor;
	Filter->lpftLastWriteTime = lpftLastWriteTime;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;
	
	if (lpClass && lpcClass) {
		Filter->cClass = *lpcClass;
		StringCchCopyA(Filter->Class, MAX_PATH, lpClass);
	}

	if (Filter->lpcSubKeys) {
		Filter->cSubKeys = *lpcSubKeys;
	}

	if (Filter->lpcMaxSubKeyLen) {
		Filter->cMaxSubKeyLen = *lpcMaxSubKeyLen;
	}

	if (Filter->lpcMaxClassLen) {
		Filter->cMaxClassLen = *lpcMaxClassLen;
	}

	if (Filter->lpcValues) {
		Filter->cValues = *lpcValues;
	}

	if (Filter->lpcMaxValueNameLen) {
		Filter->cMaxValueNameLen = *lpcMaxValueNameLen;
	}

	if (Filter->lpcMaxValueLen) {
		Filter->cMaxValueLen = *lpcMaxValueLen;
	}

	if (Filter->lpcbSecurityDescriptor) {
		Filter->cbSecurityDescriptor = *lpcbSecurityDescriptor;
	}

	if (Filter->lpftLastWriteTime) {
		memcpy(&Filter->LastWriteTime, lpftLastWriteTime, sizeof(FILETIME));
	}
	
	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegQueryInfoKeyADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGQUERYINFOKEY_ANSI Data;

	if (Record->ProbeOrdinal != _RegQueryInfoKeyA) {
		return S_FALSE;
	}	

	Data = (PREG_REGQUERYINFOKEY_ANSI)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=0x%p, lpClass=0x%p (%S), lpcClass=0x%p (0x%x), lpcSubKeys=0x%p (0x%x)"
		L"lpcMaxSubKeyLen=0x%p (0x%x), lpcMaxClassLen=0x%p (0x%x), lpcValues=0x%p (0x%x)"
		L"lpcMaxValueNameLen=0x%p (0x%x), lpcMaxValueLen=0x%p (0x%x), lpcbSecurityDescriptor=0x%p (0x%x)"
		L"lpftLastWriteTime=0x%p, Status=0x%08x, LastErrorStatus=0x%08x", 
		Data->hKey, Data->lpClass, Data->Class, Data->lpcClass, Data->cClass, 
		Data->lpcSubKeys, Data->cSubKeys, Data->lpcMaxSubKeyLen, Data->cMaxSubKeyLen, 
		Data->lpcMaxClassLen, Data->cMaxClassLen, Data->lpcValues, Data->cValues,
		Data->lpcMaxValueNameLen, Data->cMaxValueNameLen, Data->lpcMaxValueLen, Data->cMaxValueLen,
		Data->lpcbSecurityDescriptor, Data->cbSecurityDescriptor, Data->lpftLastWriteTime, 
		Data->Status, Data->LastErrorStatus);

	return S_OK;
}

LONG WINAPI 
RegQueryInfoKeyWEnter(
	IN  HKEY hKey,
	OUT PWSTR lpClass,
	OUT LPDWORD lpcClass,
	IN  LPDWORD lpReserved,
	OUT LPDWORD lpcSubKeys,
	OUT LPDWORD lpcMaxSubKeyLen,
	OUT LPDWORD lpcMaxClassLen,
	OUT LPDWORD lpcValues,
	OUT LPDWORD lpcMaxValueNameLen,
	OUT LPDWORD lpcMaxValueLen,
	OUT LPDWORD lpcbSecurityDescriptor,
	OUT PFILETIME lpftLastWriteTime
	)
{
	PBTR_FILTER_RECORD Record;
	PREG_REGQUERYINFOKEY_UNICODE Filter;
	REGQUERYINFOKEYW_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Status, LastError;

	BtrFltGetContext(&Context);
	Routine = (REGQUERYINFOKEYW_ROUTINE)Context.Destine;
	Status = (*Routine)(hKey, lpClass, lpcClass, lpReserved, lpcSubKeys,
		                lpcMaxSubKeyLen, lpcMaxClassLen, lpcValues, lpcMaxValueNameLen,
						lpcMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime);
	LastError = GetLastError();	
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(REG_REGQUERYINFOKEY_UNICODE),
		                          RegGuid, _RegQueryInfoKeyW);

	Filter = FILTER_RECORD_POINTER(Record, REG_REGQUERYINFOKEY_UNICODE);
	Filter->hKey = hKey;
	Filter->lpClass = lpClass;
	Filter->lpcClass = lpcClass;
	Filter->lpReserved = lpReserved;
	Filter->lpcSubKeys = lpcSubKeys;
	Filter->lpcMaxSubKeyLen = lpcMaxSubKeyLen;
	Filter->lpcMaxClassLen = lpcMaxClassLen;
	Filter->lpcValues = lpcValues;
	Filter->lpcMaxValueNameLen = lpcMaxValueNameLen;
	Filter->lpcMaxValueLen = lpcMaxValueLen;
	Filter->lpcbSecurityDescriptor = lpcbSecurityDescriptor;
	Filter->lpftLastWriteTime = lpftLastWriteTime;
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;

	if (lpClass && lpcClass) {
		Filter->cClass = *lpcClass;
		StringCchCopyW(Filter->Class, MAX_PATH, lpClass);
	}

	if (Filter->lpcSubKeys) {
		Filter->cSubKeys = *lpcSubKeys;
	}

	if (Filter->lpcMaxSubKeyLen) {
		Filter->cMaxSubKeyLen = *lpcMaxSubKeyLen;
	}

	if (Filter->lpcMaxClassLen) {
		Filter->cMaxClassLen = *lpcMaxClassLen;
	}

	if (Filter->lpcValues) {
		Filter->cValues = *lpcValues;
	}

	if (Filter->lpcMaxValueNameLen) {
		Filter->cMaxValueNameLen = *lpcMaxValueNameLen;
	}

	if (Filter->lpcMaxValueLen) {
		Filter->cMaxValueLen = *lpcMaxValueLen;
	}

	if (Filter->lpcbSecurityDescriptor) {
		Filter->cbSecurityDescriptor = *lpcbSecurityDescriptor;
	}

	if (Filter->lpftLastWriteTime) {
		memcpy(&Filter->LastWriteTime, lpftLastWriteTime, sizeof(FILETIME));
	}
	
	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RegQueryInfoKeyWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PREG_REGQUERYINFOKEY_UNICODE Data;

	if (Record->ProbeOrdinal != _RegQueryInfoKeyW) {
		return S_FALSE;
	}	

	Data = (PREG_REGQUERYINFOKEY_UNICODE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hKey=0x%p, lpClass=0x%p (%s), lpcClass=0x%p (0x%x), lpcSubKeys=0x%p (0x%x)"
		L"lpcMaxSubKeyLen=0x%p (0x%x), lpcMaxClassLen=0x%p (0x%x), lpcValues=0x%p (0x%x)"
		L"lpcMaxValueNameLen=0x%p (0x%x), lpcMaxValueLen=0x%p (0x%x), lpcbSecurityDescriptor=0x%p (0x%x)"
		L"lpftLastWriteTime=0x%p, Status=0x%08x, LastErrorStatus=0x%08x", 
		Data->hKey, Data->lpClass, Data->Class, Data->lpcClass, Data->cClass, 
		Data->lpcSubKeys, Data->cSubKeys, Data->lpcMaxSubKeyLen, Data->cMaxSubKeyLen, 
		Data->lpcMaxClassLen, Data->cMaxClassLen, Data->lpcValues, Data->cValues,
		Data->lpcMaxValueNameLen, Data->cMaxValueNameLen, Data->lpcMaxValueLen, Data->cMaxValueLen,
		Data->lpcbSecurityDescriptor, Data->cbSecurityDescriptor, Data->lpftLastWriteTime, 
		Data->Status, Data->LastErrorStatus);

	return S_OK;
}

typedef enum _KEY_INFORMATION_CLASS {
	KeyBasicInformation,
	KeyCachedInformation,
	KeyNameInformation,
	KeyNodeInformation,
	KeyFullInformation,
	KeyVirtualizationInformation
} KEY_INFORMATION_CLASS;

NTSTATUS NTAPI 
NtQueryKey(
    IN HANDLE  KeyHandle,
    IN KEY_INFORMATION_CLASS  KeyInformationClass,
    OUT PVOID  KeyInformation,
    IN ULONG  Length,
    OUT PULONG  ResultLength
    );

typedef NTSTATUS
(NTAPI *NTQUERYKEY)(
    IN HANDLE  KeyHandle,
    IN KEY_INFORMATION_CLASS  KeyInformationClass,
    OUT PVOID  KeyInformation,
    IN ULONG  Length,
    OUT PULONG  ResultLength
	);

//
// N.B. STATUS_BUFFER_TOO_SMALL can return if the buffer
// is too small.
//

NTQUERYKEY NtQueryKeyPtr = NULL;

ULONG
RegQueryFullPathFromKey(
	IN HKEY Key,
	IN PWCHAR Buffer,
	IN ULONG BufferLength,
	OUT PULONG ActualLength
	)
{
	NTSTATUS Status;
	ULONG ResultLength;

	if (!NtQueryKeyPtr) {
		
		HMODULE Ntdll;
		Ntdll = GetModuleHandle(L"ntdll.dll");
		NtQueryKeyPtr = (NTQUERYKEY)GetProcAddress(Ntdll, "NtQueryKey");

		if (!NtQueryKeyPtr) {
			Buffer[0] = 0;
			*ActualLength = 0;
			return GetLastError();
		}
	}

	(*NtQueryKeyPtr)(Key, KeyNameInformation, NULL, 0, &ResultLength);
	if (BufferLength <= ResultLength) {
		return STATUS_BUFFER_TOO_SMALL;
	}

	Status = (*NtQueryKeyPtr)(Key, KeyNameInformation, Buffer, BufferLength, &ResultLength);
	if (Status == STATUS_SUCCESS) {
		Buffer[ResultLength / sizeof(WCHAR)] = L'\0';
		*ActualLength = ResultLength;
	}

	return Status;
}

BOOLEAN
RegIsPredefinedKey(
	IN HKEY Key
	)
{
	if (Key == HKEY_CLASSES_ROOT || Key == HKEY_CURRENT_USER ||
		Key == HKEY_LOCAL_MACHINE || Key == HKEY_USERS || 
		Key == HKEY_PERFORMANCE_DATA || Key == HKEY_PERFORMANCE_TEXT ||
		Key == HKEY_PERFORMANCE_NLSTEXT || Key == HKEY_CURRENT_CONFIG ||
		Key == HKEY_DYN_DATA) {
		return TRUE;
	}

	return FALSE;
}

ULONG 
RegFormatKnownKeyName(
	IN HKEY Key,
	IN PWCHAR Buffer,
	IN ULONG BufferLength
	)
{
	ULONG Status = S_OK;

	switch ((ULONG_PTR)Key) {

		case HKEY_CLASSES_ROOT:
			StringCchCopy(Buffer, BufferLength - 1, L"HKEY_CLASS_ROOT");
			break;

		case HKEY_CURRENT_USER:
			StringCchCopy(Buffer, BufferLength - 1, L"HKEY_CURRENT_USER");
			break;

		case HKEY_LOCAL_MACHINE:
			StringCchCopy(Buffer, BufferLength - 1, L"HKEY_LOCAL_MACHINE");
			break;

		case HKEY_USERS:
			StringCchCopy(Buffer, BufferLength - 1, L"HKEY_USER");
			break;

		case HKEY_PERFORMANCE_DATA:
			StringCchCopy(Buffer, BufferLength - 1, L"HKEY_PERFORMANCE_DATA");
			break;

		case HKEY_PERFORMANCE_TEXT:
			StringCchCopy(Buffer, BufferLength - 1, L"HKEY_PERFORMANCE_TEXT");
			break;

		case HKEY_PERFORMANCE_NLSTEXT:
			StringCchCopy(Buffer, BufferLength - 1, L"HKEY_PERFORMANCE_NLSTEXT");
			break;

		case HKEY_CURRENT_CONFIG:
			StringCchCopy(Buffer, BufferLength - 1, L"HKEY_CURRENT_CONFIG");
			break;

		case HKEY_DYN_DATA:
			StringCchCopy(Buffer, BufferLength - 1, L"HKEY_DYN_DATA");
			break;
		default:
			Status = S_FALSE;
			break;
	}

	return Status;
}

ULONG
RegFormatValue(
	IN PCHAR Data,
	IN ULONG DataLength,
	IN ULONG DataType,
	OUT PWCHAR Buffer,
	IN ULONG BufferLength,
	IN BOOLEAN Unicode
	)
{
	PWCHAR Pointer;
	ULONG i;

	switch (DataType) {

		case REG_SZ:
		case REG_EXPAND_SZ:
		case REG_MULTI_SZ:

			//
			// N.B. When capture registry data, REG_MULTI_SZ type has been modified to
			// concatenate all string into a single one.
			//

			if (Unicode) {
				StringCchCopyW(Buffer, BufferLength - 1, (PWCHAR)Data);
			} else {
				//RtlInitAnsiString(&AnsiString, (PSTR)Data);
				//RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
				//StringCchPrintfW(Buffer, BufferLength - 1, L"%s", UnicodeString.Buffer);
				//RtlFreeUnicodeString(&UnicodeString);
			}

			break;

		case REG_BINARY:
		case REG_NONE:
		case REG_LINK:
		case REG_RESOURCE_LIST:
		case REG_FULL_RESOURCE_DESCRIPTOR:
		case REG_RESOURCE_REQUIREMENTS_LIST:
			
			//
			// Maximum allow 16 hexdecimal chars.
			//

			if (DataLength > 16) {
				DataLength = 16;
			}

			Pointer = Buffer;

			for(i = 0; i < DataLength; i++) {
				StringCchPrintfW(Pointer, BufferLength - 1 - (i * 3), L"%02X ", (UCHAR)Data[i]);	
				Pointer += 3; 
			}
			
			break;

		case REG_DWORD:
			StringCchPrintfW(Buffer, BufferLength - 1, L"%d", (PULONG)Data);
			break;

		case REG_QWORD:
			StringCchPrintfW(Buffer, BufferLength - 1, L"%I64X", (PULONG64)Data);
			break;

			break;

	}

	return S_OK;
}