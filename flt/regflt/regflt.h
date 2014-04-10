//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _REGFLT_H_
#define _REGFLT_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#define _BTR_SDK_
#include "btrsdk.h"
#include <strsafe.h>

#ifndef ASSERT
 #include <assert.h>
 #define ASSERT assert
#endif

#define RegMajorVersion  1
#define RegMinorVersion  0

//
// Registry filter GUID
//

extern GUID RegGuid;

//
// Registry probe ordinals
//

typedef enum _REG_PROBE_ORDINAL {
	_RegCreateKeyA,
	_RegCreateKeyW,
	_RegCreateKeyExA,
	_RegCreateKeyExW,
	_RegDeleteKeyA,
	_RegDeleteKeyW,
	_RegDeleteKeyExA,
	_RegDeleteKeyExW,
	_RegDeleteKeyValueA,
	_RegDeleteKeyValueW,
	_RegDeleteValueA,
	_RegDeleteValueW,
	_RegDeleteTreeW,
	_RegEnumKeyExA,
	_RegEnumKeyExW,
	_RegSetValueExA,
	_RegSetValueExW,
	_RegQueryValueExA,
	_RegQueryValueExW,
	_RegOpenKeyExA,
	_RegOpenKeyExW,
	_RegQueryInfoKeyA,
	_RegQueryInfoKeyW,
	REG_PROBE_NUMBER,
} REG_PROBE_ORDINAL, *PREG_PROBE_ORDINAL;

typedef struct _REG_REGCREATEKEY_ANSI {
	HKEY hKey;
	PSTR lpSubKey;
	PHKEY phkResult;
	CHAR SubKey[MAX_PATH];
	HKEY hResultKey;
	LONG Status;
	ULONG LastErrorStatus;
} REG_REGCREATEKEY_ANSI, *PREG_REGCREATEKEY_ANSI;

typedef struct _REG_REGCREATEKEY_UNICODE{
	HKEY hKey;
	PWSTR lpSubKey;
	PHKEY phkResult;
	WCHAR SubKey[MAX_PATH];
	HKEY hResultKey;
	LONG Status;
	ULONG LastErrorStatus;
} REG_REGCREATEKEY_UNICODE, *PREG_REGCREATEKEY_UNICODE;

//
// RegCreateKey 
//

LONG WINAPI 
RegCreateKeyAEnter(
	IN  HKEY hKey,
	IN  PSTR lpSubKey,
	OUT PHKEY phkResult
	);

typedef LONG 
(WINAPI *REGCREATEKEYA_ROUTINE)(
	IN  HKEY hKey,
	IN  PSTR lpSubKey,
	OUT PHKEY phkResult
	);

ULONG 
RegCreateKeyADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

LONG WINAPI 
RegCreateKeyWEnter(
	IN  HKEY hKey,
	IN  PWSTR lpSubKey,
	OUT PHKEY phkResult
	);

typedef LONG
(WINAPI *REGCREATEKEYW_ROUTINE)(
	IN  HKEY hKey,
	IN  PWSTR lpSubKey,
	OUT PHKEY phkResult
	);

ULONG 
RegCreateKeyWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

// 
// RegCreateKeyEx
//

typedef struct _REG_REGCREATEKEYEX_ANSI{
	HKEY hKey;
	PSTR lpSubKey;
	DWORD Reserved;
	PSTR lpClass; 
	ULONG dwOptions;
	REGSAM samDesired;
	LPSECURITY_ATTRIBUTES lpSecurityAttributes;
	PHKEY phkResult;
	LPDWORD lpdwDisposition;
	CHAR SubKey[MAX_PATH];
	CHAR Class[MAX_PATH]; 
	HKEY hResultKey;
	DWORD dwDisposition;
	LONG Status;
	ULONG LastErrorStatus;
} REG_REGCREATEKEYEX_ANSI, *PREG_REGCREATEKEYEX_ANSI;

typedef struct _REG_REGCREATEKEYEX_UNICODE{
	HKEY hKey;
	PWSTR lpSubKey;
	DWORD Reserved;
	PWSTR lpClass; 
	ULONG dwOptions;
	REGSAM samDesired;
	LPSECURITY_ATTRIBUTES lpSecurityAttributes;
	PHKEY phkResult;
	LPDWORD lpdwDisposition;
	WCHAR SubKey[MAX_PATH];
	WCHAR Class[MAX_PATH]; 
	HKEY hResultKey;
	DWORD dwDisposition;
	LONG Status;
	ULONG LastErrorStatus;
} REG_REGCREATEKEYEX_UNICODE, *PREG_REGCREATEKEYEX_UNICODE;

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
	);

typedef LONG 
(WINAPI *REGCREATEKEYEXA_ROUTINE)(
	IN HKEY hKey,
	IN PSTR lpSubKey,
	IN DWORD Reserved,
	IN PSTR lpClass,
	IN DWORD dwOptions,
	IN REGSAM samDesired,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	OUT PHKEY phkResult,
	OUT LPDWORD lpdwDisposition
	);

ULONG
RegCreateKeyExADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

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
	);

typedef LONG 
(WINAPI *REGCREATEKEYEXW_ROUTINE)(
	IN HKEY hKey,
	IN PWSTR lpSubKey,
	IN DWORD Reserved,
	IN PWSTR lpClass,
	IN DWORD dwOptions,
	IN REGSAM samDesired,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	OUT PHKEY phkResult,
	OUT LPDWORD lpdwDisposition
	);

ULONG
RegCreateKeyExWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// RegDeleteKey
//

typedef struct _REG_REGDELETEKEY_ANSI{
	HKEY hKey;
	PSTR lpSubKey;
	CHAR SubKey[MAX_PATH];
	LONG Status;
	ULONG LastErrorStatus;
} REG_REGDELETEKEY_ANSI, *PREG_REGDELETEKEY_ANSI;

typedef struct _REG_REGDELETEKEY_UNICODE {
	HKEY hKey;
	PWSTR lpSubKey;
	WCHAR SubKey[MAX_PATH];
	LONG Status;
	ULONG LastErrorStatus;
} REG_REGDELETEKEY_UNICODE, *PREG_REGDELETEKEY_UNICODE;

LONG WINAPI 
RegDeleteKeyAEnter(
	IN HKEY hKey,
	IN PSTR lpSubKey
	);

typedef LONG 
(WINAPI *REGDELETEKEYA_ROUTINE)(
	IN HKEY hKey,
	IN PSTR lpSubKey
	);

ULONG
RegDeleteKeyADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

LONG WINAPI 
RegDeleteKeyWEnter(
	IN HKEY hKey,
	IN PWSTR lpSubKey
	);

typedef LONG 
(WINAPI *REGDELETEKEYW_ROUTINE)(
	IN HKEY hKey,
	IN PWSTR lpSubKey
	);

ULONG 
RegDeleteKeyWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// RegDeleteKeyEx
//

typedef struct _REG_REGDELETEKEYEX_ANSI{
	HKEY hKey;
	PSTR lpSubKey;
	REGSAM samDesired;
	DWORD Reserved;
	CHAR SubKey[MAX_PATH];
	LONG Status;
	ULONG LastErrorStatus;
} REG_REGDELETEKEYEX_ANSI, *PREG_REGDELETEKEYEX_ANSI;

typedef struct _REG_REGDELETEKEYEX_UNICODE {
	HKEY hKey;
	PWSTR lpSubKey;
	REGSAM samDesired;
	DWORD Reserved;
	WCHAR SubKey[MAX_PATH];
	LONG Status;
	ULONG LastErrorStatus;
} REG_REGDELETEKEYEX_UNICODE, *PREG_REGDELETEKEYEX_UNICODE;

LONG WINAPI 
RegDeleteKeyExAEnter(
	IN HKEY hKey,
	IN PSTR lpSubKey,
	IN REGSAM samDesired,
	IN DWORD Reserved
	);

typedef LONG 
(WINAPI *REGDELETEKEYEXA_ROUTINE)(
	IN HKEY hKey,
	IN PSTR lpSubKey,
	IN REGSAM samDesired,
	IN DWORD Reserved
	);

ULONG 
RegDeleteKeyExADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

LONG WINAPI 
RegDeleteKeyExWEnter(
	IN HKEY hKey,
	IN PWSTR lpSubKey,
	IN REGSAM samDesired,
	IN DWORD Reserved
	);

typedef LONG 
(WINAPI *REGDELETEKEYEXW_ROUTINE)(
	IN HKEY hKey,
	IN PWSTR lpSubKey,
	IN REGSAM samDesired,
	IN DWORD Reserved
	);

ULONG 
RegDeleteKeyExWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// RegDeleteKeyValue
//

typedef struct _REG_REGDELETEKEYVALUE_ANSI{
	HKEY hKey;
	PSTR lpSubKey;
	PSTR lpValueName;
	CHAR SubKey[MAX_PATH];
	CHAR ValueName[32];
	LONG Status;
	ULONG LastErrorStatus;
} REG_REGDELETEKEYVALUE_ANSI, *PREG_REGDELETEKEYVALUE_ANSI;

typedef struct _REG_REGDELETEKEYVALUE_UNICODE {
	HKEY hKey;
	PWSTR lpSubKey;
	PWSTR lpValueName;
	WCHAR SubKey[MAX_PATH];
	WCHAR ValueName[32];
	LONG Status;
	ULONG LastErrorStatus;
} REG_REGDELETEKEYVALUE_UNICODE, *PREG_REGDELETEKEYVALUE_UNICODE;

LONG WINAPI 
RegDeleteKeyValueAEnter(
	IN HKEY hKey,
	IN PSTR lpSubKey,
	IN PSTR lpValueName
	);

typedef LONG 
(WINAPI *REGDELETEKEYVALUEA_ROUTINE)(
	IN HKEY hKey,
	IN PSTR lpSubKey,
	IN PSTR lpValueName
	);

ULONG 
RegDeleteKeyValueADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

LONG WINAPI 
RegDeleteKeyValueWEnter(
	IN HKEY hKey,
	IN PWSTR lpSubKey,
	IN PWSTR lpValueName
	);

typedef LONG 
(WINAPI *REGDELETEKEYVALUEW_ROUTINE)(
	IN HKEY hKey,
	IN PWSTR lpSubKey,
	IN PWSTR lpValueName
	);

ULONG 
RegDeleteKeyValueWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// RegDeleteValue
//

typedef struct _REG_REGDELETEVALUE_ANSI{
	HKEY hKey;
	PSTR lpValueName;
	CHAR ValueName[32];
	LONG Status;
	ULONG LastErrorStatus;
} REG_REGDELETEVALUE_ANSI, *PREG_REGDELETEVALUE_ANSI;

typedef struct _REG_REGDELETEVALUE_UNICODE {
	HKEY hKey;
	PWSTR lpValueName;
	WCHAR ValueName[32];
	LONG Status;
	ULONG LastErrorStatus;
} REG_REGDELETEVALUE_UNICODE, *PREG_REGDELETEVALUE_UNICODE;

LONG WINAPI 
RegDeleteValueAEnter(
	IN HKEY hKey,
	IN PSTR lpValueName
	);

typedef LONG 
(WINAPI *REGDELETEVALUEA_ROUTINE)(
	IN HKEY hKey,
	IN PSTR lpValueName
	);

ULONG 
RegDeleteValueADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

LONG WINAPI 
RegDeleteValueWEnter(
	IN HKEY hKey,
	IN PWSTR lpValueName
	);

typedef LONG 
(WINAPI *REGDELETEVALUEW_ROUTINE)(
	IN HKEY hKey,
	IN PWSTR lpValueName
	);

ULONG 
RegDeleteValueWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// RegDeleteTree
//

typedef struct _REG_REGDELETETREE {
	HKEY hKey;
	PWSTR lpSubKey;
	WCHAR SubKey[MAX_PATH];
	LONG Status;
	ULONG LastErrorStatus;
} REG_REGDELETETREE, *PREG_REGDELETETREE;

LONG WINAPI 
RegDeleteTreeEnter(
	IN HKEY hKey,
	IN PWSTR lpSubKey
	);

typedef LONG 
(WINAPI *REGDELETETREE_ROUTINE)(
	IN HKEY hKey,
	IN PWSTR lpSubKey
	);

ULONG
RegDeleteTreeDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// RegEnumKeyEx
//

typedef struct _REG_REGENUMKEYEX_ANSI{
	HKEY hKey;
	DWORD dwIndex;
	PSTR lpName;
	LPDWORD lpcName;
	LPDWORD lpReserved;
	PSTR lpClass;
	LPDWORD lpcClass;
	PFILETIME lpftLastWriteTime;
	CHAR Name[MAX_PATH];
	DWORD cName;
	CHAR Class[MAX_PATH];
	DWORD cClass;
	FILETIME LastWriteTime;
	LONG Status;
	ULONG LastErrorStatus;
} REG_REGENUMKEYEX_ANSI, *PREG_REGENUMKEYEX_ANSI;

typedef struct _REG_REGENUMKEYEX_UNICODE{
	HKEY hKey;
	DWORD dwIndex;
	PWSTR lpName;
	LPDWORD lpcName;
	LPDWORD lpReserved;
	PWSTR lpClass;
	LPDWORD lpcClass;
	PFILETIME lpftLastWriteTime;
	WCHAR Name[MAX_PATH];
	DWORD cName;
	WCHAR Class[MAX_PATH];
	DWORD cClass;
	FILETIME LastWriteTime;
	LONG Status;
	ULONG LastErrorStatus;
} REG_REGENUMKEYEX_UNICODE, *PREG_REGENUMKEYEX_UNICODE;

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
	);

typedef LONG 
(WINAPI *REGENUMKEYEXA_ROUTINE)(
	IN  HKEY hKey,
	IN  DWORD dwIndex,
	OUT PSTR lpName,
	IN  LPDWORD lpcName,
	IN  LPDWORD lpReserved,
	OUT PSTR lpClass,
	OUT LPDWORD lpcClass,
	OUT PFILETIME lpftLastWriteTime
	);

ULONG 
RegEnumKeyExADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

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
	);

typedef LONG 
(WINAPI *REGENUMKEYEXW_ROUTINE)(
	IN  HKEY hKey,
	IN  DWORD dwIndex,
	OUT PWSTR lpName,
	IN  LPDWORD lpcName,
	IN  LPDWORD lpReserved,
	OUT PWSTR lpClass,
	OUT LPDWORD lpcClass,
	OUT PFILETIME lpftLastWriteTime
	);

ULONG 
RegEnumKeyExWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// RegSetValueEx
//

typedef struct _REG_REGSETVALUEEX_ANSI{
	HKEY hKey;
	PSTR lpValueName;
	DWORD Reserved;
	DWORD dwType;
	PCHAR lpData;
	DWORD cbData;
	CHAR ValueName[32];
	LONG Status;
	ULONG LastErrorStatus;
	CHAR Data[ANYSIZE_ARRAY];
} REG_REGSETVALUEEX_ANSI, *PREG_REGSETVALUEEX_ANSI;

typedef struct _REG_REGSETVALUEEX_UNICODE {
	HKEY hKey;
	PWSTR lpValueName;
	DWORD Reserved;
	DWORD dwType;
	PCHAR lpData;
	DWORD cbData;
	WCHAR ValueName[32];
	LONG Status;
	ULONG LastErrorStatus;
	CHAR Data[ANYSIZE_ARRAY];
} REG_REGSETVALUEEX_UNICODE, *PREG_REGSETVALUEEX_UNICODE;

LONG WINAPI 
RegSetValueExAEnter(
	IN HKEY hKey,
	IN PSTR lpValueName,
	IN DWORD Reserved,
	IN DWORD dwType,
	IN const BYTE* lpData,
	IN DWORD cbData
	);

typedef LONG 
(WINAPI *REGSETVALUEEXA_ROUTINE)(
	IN HKEY hKey,
	IN PSTR lpValueName,
	IN DWORD Reserved,
	IN DWORD dwType,
	IN const BYTE* lpData,
	IN DWORD cbData
	);

ULONG
RegSetValueExADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

LONG WINAPI 
RegSetValueExWEnter(
	IN HKEY hKey,
	IN PWSTR lpValueName,
	IN DWORD Reserved,
	IN DWORD dwType,
	IN const BYTE* lpData,
	IN DWORD cbData
	);

typedef LONG
(WINAPI *REGSETVALUEEXW_ROUTINE)(
	IN HKEY hKey,
	IN PWSTR lpValueName,
	IN DWORD Reserved,
	IN DWORD dwType,
	IN const BYTE* lpData,
	IN DWORD cbData
	);

ULONG 
RegSetValueExWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// RegQueryValueEx
//

typedef struct _REG_REGQUERYVALUEEX_ANSI{

	HKEY hKey;
	PSTR lpValueName;
	LPDWORD lpReserved;
	LPDWORD lpType;
	LPBYTE lpData;
	LPDWORD lpcbData;

	DWORD Type;
	DWORD cbData;	
	CHAR ValueName[MAX_PATH];

	LONG Status;
	ULONG LastErrorStatus;

	CHAR Data[ANYSIZE_ARRAY]; 

} REG_REGQUERYVALUEEX_ANSI, *PREG_REGQUERYVALUEEX_ANSI;

typedef struct _REG_REGQUERYVALUEEX_UNICODE {

	HKEY hKey;
	PWSTR lpValueName;
	LPDWORD lpReserved;
	LPDWORD lpType;
	LPBYTE lpData;
	LPDWORD lpcbData;

	DWORD Type;
	DWORD cbData;	
	WCHAR ValueName[MAX_PATH];

	LONG Status;
	ULONG LastErrorStatus;

	CHAR Data[ANYSIZE_ARRAY]; 

} REG_REGQUERYVALUEEX_UNICODE, *PREG_REGQUERYVALUEEX_UNICODE;

LONG WINAPI 
RegQueryValueExAEnter(
	IN HKEY hKey,
	IN PSTR lpValueName,
	IN LPDWORD lpReserved,
	OUT LPDWORD lpType,
	OUT LPBYTE lpData,
	OUT LPDWORD lpcbData
	);

typedef LONG 
(WINAPI *REGQUERYVALUEEXA_ROUTINE)(
	IN HKEY hKey,
	IN PSTR lpValueName,
	IN LPDWORD lpReserved,
	OUT LPDWORD lpType,
	OUT LPBYTE lpData,
	OUT LPDWORD lpcbData
	);

ULONG
RegQueryValueExADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

LONG WINAPI 
RegQueryValueExWEnter(
	IN HKEY hKey,
	IN PWSTR lpValueName,
	IN LPDWORD lpReserved,
	OUT LPDWORD lpType,
	OUT LPBYTE lpData,
	OUT LPDWORD lpcbData
	);

typedef LONG
(WINAPI *REGQUERYVALUEEXW_ROUTINE)(
	IN HKEY hKey,
	IN PWSTR lpValueName,
	IN LPDWORD lpReserved,
	OUT LPDWORD lpType,
	OUT LPBYTE lpData,
	OUT LPDWORD lpcbData
	);

ULONG 
RegQueryValueExWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// RegOpenKeyEx
//

typedef struct _REG_REGOPENKEYEX_ANSI{
	HKEY hKey;
	PSTR lpSubKey;
	DWORD ulOptions;
	REGSAM samDesired;
	PHKEY phkResult;
	HKEY hResultKey;
	CHAR SubKey[MAX_PATH];
	LONG Status;
	ULONG LastErrorStatus;
} REG_REGOPENKEYEX_ANSI, *PREG_REGOPENKEYEX_ANSI;

typedef struct _REG_REGOPENKEYEX_UNICODE{
	HKEY hKey;
	PWSTR lpSubKey;
	DWORD ulOptions;
	REGSAM samDesired;
	PHKEY phkResult;
	HKEY hResultKey;
	WCHAR SubKey[MAX_PATH];
	LONG Status;
	ULONG LastErrorStatus;
} REG_REGOPENKEYEX_UNICODE, *PREG_REGOPENKEYEX_UNICODE;

LONG WINAPI 
RegOpenKeyExAEnter(
	IN  HKEY hKey,
	IN  PSTR lpSubKey,
	IN  DWORD ulOptions,
	IN  REGSAM samDesired,
	OUT PHKEY phkResult
	);

typedef LONG 
(WINAPI *REGOPENKEYEXA_ROUTINE)(
	IN  HKEY hKey,
	IN  PSTR lpSubKey,
	IN  DWORD ulOptions,
	IN  REGSAM samDesired,
	OUT PHKEY phkResult
	);

ULONG 
RegOpenKeyExADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

LONG WINAPI 
RegOpenKeyExWEnter(
	IN  HKEY hKey,
	IN  PWSTR lpSubKey,
	IN  DWORD ulOptions,
	IN  REGSAM samDesired,
	OUT PHKEY phkResult
	);

typedef LONG 
(WINAPI *REGOPENKEYEXW_ROUTINE)(
	IN  HKEY hKey,
	IN  PWSTR lpSubKey,
	IN  DWORD ulOptions,
	IN  REGSAM samDesired,
	OUT PHKEY phkResult
	);

ULONG 
RegOpenKeyExWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// RegQueryInfoKey
//

typedef struct _REG_REGQUERYINFOKEY_ANSI {

	HKEY hKey;
	PSTR lpClass;
	LPDWORD lpcClass;
	LPDWORD lpReserved;
	LPDWORD lpcSubKeys;
	LPDWORD lpcMaxSubKeyLen;
	LPDWORD lpcMaxClassLen;
	LPDWORD lpcValues;
	LPDWORD lpcMaxValueNameLen;
	LPDWORD lpcMaxValueLen;
	LPDWORD lpcbSecurityDescriptor;
	PFILETIME lpftLastWriteTime;

	DWORD cClass;
	DWORD cSubKeys;
	DWORD cMaxSubKeyLen;
	DWORD cMaxClassLen;
	DWORD cValues;
	DWORD cMaxValueNameLen;
	DWORD cMaxValueLen;
	DWORD cbSecurityDescriptor;
	FILETIME LastWriteTime;
	CHAR Class[MAX_PATH];

	LONG Status;
	ULONG LastErrorStatus;

} REG_REGQUERYINFOKEY_ANSI, *PREG_REGQUERYINFOKEY_ANSI;

typedef struct _REG_REGQUERYINFOKEY_UNICODE{
	
	HKEY hKey;
	PWSTR lpClass;
	LPDWORD lpcClass;
	LPDWORD lpReserved;
	LPDWORD lpcSubKeys;
	LPDWORD lpcMaxSubKeyLen;
	LPDWORD lpcMaxClassLen;
	LPDWORD lpcValues;
	LPDWORD lpcMaxValueNameLen;
	LPDWORD lpcMaxValueLen;
	LPDWORD lpcbSecurityDescriptor;
	PFILETIME lpftLastWriteTime;

	DWORD cClass;
	DWORD cSubKeys;
	DWORD cMaxSubKeyLen;
	DWORD cMaxClassLen;
	DWORD cValues;
	DWORD cMaxValueNameLen;
	DWORD cMaxValueLen;
	DWORD cbSecurityDescriptor;
	FILETIME LastWriteTime;
	WCHAR Class[MAX_PATH];

	LONG Status;
	ULONG LastErrorStatus;

} REG_REGQUERYINFOKEY_UNICODE, *PREG_REGQUERYINFOKEY_UNICODE;

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
	);

typedef LONG 
(WINAPI *REGQUERYINFOKEYA_ROUTINE)(
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
	);

ULONG 
RegQueryInfoKeyADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

LONG WINAPI 
RegQueryInfoKeyWEnter(
	IN  HKEY hKey,
	OUT PWSTR lpClass,
	OUT LPDWORD lpcClasseg,
	IN  LPDWORD lpReserved,
	OUT LPDWORD lpcSubKeys,
	OUT LPDWORD lpcMaxSubKeyLen,
	OUT LPDWORD lpcMaxClassLen,
	OUT LPDWORD lpcValues,
	OUT LPDWORD lpcMaxValueNameLen,
	OUT LPDWORD lpcMaxValueLen,
	OUT LPDWORD lpcbSecurityDescriptor,
	OUT PFILETIME lpftLastWriteTime
	);

typedef LONG 
(WINAPI *REGQUERYINFOKEYW_ROUTINE)(
	IN  HKEY hKey,
	OUT PWSTR lpClass,
	OUT LPDWORD lpcClasseg,
	IN  LPDWORD lpReserved,
	OUT LPDWORD lpcSubKeys,
	OUT LPDWORD lpcMaxSubKeyLen,
	OUT LPDWORD lpcMaxClassLen,
	OUT LPDWORD lpcValues,
	OUT LPDWORD lpcMaxValueNameLen,
	OUT LPDWORD lpcMaxValueLen,
	OUT LPDWORD lpcbSecurityDescriptor,
	OUT PFILETIME lpftLastWriteTime
	);

ULONG 
RegQueryInfoKeyWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// Registry Internal Routines
//

BOOLEAN
RegIsPredefinedKey(
	IN HKEY Key
	);

ULONG
RegQueryFullPathFromKey(
	IN HKEY hKey,
	IN PWCHAR Buffer,
	IN ULONG BufferLength,
	OUT PULONG ActualLength
	);

ULONG 
RegFormatKnownKeyName(
	IN HKEY Key,
	IN PWCHAR Buffer,
	IN ULONG BufferLength
	);

ULONG
RegFormatValue(
	IN PCHAR Data,
	IN ULONG DataLength,
	IN ULONG DataType,
	OUT PWCHAR Buffer,
	IN ULONG BufferLength,
	IN BOOLEAN Unicode
	);

#ifdef __cplusplus
}
#endif

#endif