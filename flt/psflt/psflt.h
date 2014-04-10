//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _PSFLT_H_
#define _PSFLT_H_

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

#define PsMajorVersion 1
#define PsMinorVersion 0

//
// UUID identify PS filter
//

extern UUID PsUuid;

typedef enum _PS_PROBE_ORDINAL {

	// 
	// process
	//

	_CreateProcess,
	_OpenProcess,
	_ExitProcess,
	_TerminateProcess,

	//
	// thread
	//

	_CreateThread,
	_CreateRemoteThread,
	_OpenThread,
	_ExitThread,
	_TerminateThread,
	_SuspendThread,
	_ResumeThread,
	_GetThreadContext,
	_SetThreadContext,
	_TlsAlloc,
	_TlsFree,
	_TlsSetValue,
	_TlsGetValue,
	_QueueUserWorkItem,
	_QueueUserAPC,

	//
	// ldr
	//

	_LoadLibraryEx,
	_GetProcAddress,
	_FreeLibrary,
	_GetModuleFileName,
	_GetModuleHandle,
	_GetModuleHandleExA,
	_GetModuleHandleExW,

	PS_PROBE_NUMBER,

} PS_PROBE_ORDINAL, *PPS_PROBE_ORDINAL;

//
// Process Object
//

typedef struct _PS_PROCESS_RECORD {

	ULONG Ordinal;
	
	union {
		
		struct {
			WCHAR Name[MAX_PATH];
			WCHAR Cmdline[MAX_PATH];
			DWORD CreationFlag;
			WCHAR WorkDir[MAX_PATH];
			PROCESS_INFORMATION Info;
		} Create;
		
		struct {
			ULONG ProcessId;
			WCHAR Name[MAX_PATH];
		} Open;
		
		struct {
			ULONG ExitCode;
		} Exit;

		struct {
			HANDLE ProcessHandle;
			WCHAR Name[MAX_PATH];
			ULONG ExitCode;
		} Terminate;
	};

} PS_PROCESS_RECORD, *PPS_PROCESS_RECORD;

//
// Universal decode routine for process routine
//

ULONG
PsProcessDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

HANDLE WINAPI 
OpenProcessEnter(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  DWORD dwProcessId
	);

typedef HANDLE 
(WINAPI *OPENPROCESS)(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  DWORD dwProcessId
	);

BOOL WINAPI
CreateProcessWEnter(
    __in_opt    LPCWSTR lpApplicationName,
    __inout_opt LPWSTR lpCommandLine,
    __in_opt    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    __in_opt    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    __in        BOOL bInheritHandles,
    __in        DWORD dwCreationFlags,
    __in_opt    LPVOID lpEnvironment,
    __in_opt    LPCWSTR lpCurrentDirectory,
    __in        LPSTARTUPINFOW lpStartupInfo,
    __out       LPPROCESS_INFORMATION lpProcessInformation
    );

typedef BOOL 
(WINAPI *CREATEPROCESSW)(
    __in_opt    LPCWSTR lpApplicationName,
    __inout_opt LPWSTR lpCommandLine,
    __in_opt    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    __in_opt    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    __in        BOOL bInheritHandles,
    __in        DWORD dwCreationFlags,
    __in_opt    LPVOID lpEnvironment,
    __in_opt    LPCWSTR lpCurrentDirectory,
    __in        LPSTARTUPINFOW lpStartupInfo,
    __out       LPPROCESS_INFORMATION lpProcessInformation
    );

VOID WINAPI 
ExitProcessEnter(
	__in UINT uExitCode
	);

typedef VOID 
(WINAPI *EXITPROCESS)(
	__in UINT uExitCode
	);

BOOL WINAPI 
TerminateProcessEnter(
	__in  HANDLE hProcess,
	__in  UINT uExitCode
	);

typedef BOOL 
(WINAPI *TERMINATEPROCESS)(
	__in  HANDLE hProcess,
	__in  UINT uExitCode
	);

//
//
// Thread Object
//

typedef struct _PS_THREAD_RECORD {

	ULONG Ordinal;
	
	union {
		
		struct {
			DWORD CreationFlag;
			ULONG ThreadId;
			PVOID Parameter;
			PVOID StartAddress;
		} Create;
		
		struct {
			ULONG ProcessId;
			WCHAR Name[MAX_PATH];
			DWORD CreationFlag;
			ULONG ThreadId;
			PVOID Parameter;
			PVOID StartAddress;
		} CreateRemote;
		
		struct {
			HANDLE ThreadHandle;
			ULONG ThreadId;
		} Open;
		
		struct {
			ULONG ThreadId;
			ULONG ExitCode;
		} Exit;

		struct {
			HANDLE ThreadHandle;
			ULONG ThreadId;
			ULONG ExitCode;
		} Terminate;

		struct {
			HANDLE ThreadHandle;
			ULONG ThreadId;
			ULONG SuspendCount;
		} Suspend;

		struct {
			HANDLE ThreadHandle;
			ULONG ThreadId;
			ULONG SuspendCount;
		} Resume;

		struct {
			HANDLE ThreadHandle;
			ULONG ContextFlag;
			ULONG_PTR Xip;
			ULONG_PTR Xsp;
		} GetContext;

		struct {
			HANDLE ThreadHandle;
			ULONG ContextFlag;
			ULONG_PTR Xip;
			ULONG_PTR Xsp;
		} SetContext;

		struct {
			DWORD Index;
		} TlsAlloc;

		struct {
			DWORD Index;
		} TlsFree;

		struct {
			DWORD Index;
			PVOID Value;
		} TlsSet;
		
		struct {
			DWORD Index;
			PVOID Value;
		} TlsGet;

		struct {
			PVOID Function;
			PVOID Context;
			ULONG Flag;
		} QueueWorkItem;

		struct {
			PVOID ApcRoutine;
			HANDLE ThreadHandle;
			ULONG_PTR Data;
		} QueueApc;
	};

} PS_THREAD_RECORD, *PPS_THREAD_RECORD;

//
// Universal decode routine for thread routine
//

ULONG
PsThreadDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

HANDLE WINAPI 
CreateThreadEnter(
	__in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	__in  SIZE_T dwStackSize,
	__in  LPTHREAD_START_ROUTINE lpStartAddress,
	__in  LPVOID lpParameter,
	__in  DWORD dwCreationFlags,
	__out LPDWORD lpThreadId
	);

typedef HANDLE 
(WINAPI *CREATETHREAD)(
	__in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	__in  SIZE_T dwStackSize,
	__in  LPTHREAD_START_ROUTINE lpStartAddress,
	__in  LPVOID lpParameter,
	__in  DWORD dwCreationFlags,
	__out LPDWORD lpThreadId
	);

HANDLE WINAPI 
CreateRemoteThreadEnter(
	__in  HANDLE hProcess,
	__in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	__in  SIZE_T dwStackSize,
	__in  LPTHREAD_START_ROUTINE lpStartAddress,
	__in  LPVOID lpParameter,
	__in  DWORD dwCreationFlags,
	__out LPDWORD lpThreadId
	);

typedef HANDLE 
(WINAPI *CREATEREMOTETHREAD)(
	__in  HANDLE hProcess,
	__in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	__in  SIZE_T dwStackSize,
	__in  LPTHREAD_START_ROUTINE lpStartAddress,
	__in  LPVOID lpParameter,
	__in  DWORD dwCreationFlags,
	__out LPDWORD lpThreadId
	);

HANDLE WINAPI 
OpenThreadEnter(
	__in DWORD dwDesiredAccess,
	__in BOOL bInheritHandle,
	__in DWORD dwThreadId
	);

typedef HANDLE 
(WINAPI *OPENTHREAD)(
	__in DWORD dwDesiredAccess,
	__in BOOL bInheritHandle,
	__in DWORD dwThreadId
	);

DWORD WINAPI 
SuspendThreadEnter(
	__in HANDLE hThread
	);

typedef DWORD 
(WINAPI *SUSPENDTHREAD)(
	__in HANDLE hThread
	);

DWORD WINAPI 
ResumeThreadEnter(
	__in HANDLE hThread
	);

typedef DWORD 
(WINAPI *RESUMETHREAD)(
	__in HANDLE hThread
	);

VOID WINAPI 
ExitThreadEnter(
	__in DWORD dwExitCode
	);

typedef VOID 
(WINAPI *EXITTHREAD)(
	__in DWORD dwExitCode
	);

BOOL WINAPI 
TerminateThreadEnter(
	__in HANDLE hThread,
	__in DWORD dwExitCode
	);

typedef BOOL 
(WINAPI *TERMINATETHREAD)(
	__in HANDLE hThread,
	__in DWORD dwExitCode
	);

BOOL WINAPI 
GetThreadContextEnter(
	__in HANDLE hThread,
	__in LPCONTEXT lpContext
	);

typedef BOOL 
(WINAPI *GETTHREADCONTEXT)(
	__in HANDLE hThread,
	__in LPCONTEXT lpContext
	);

BOOL WINAPI 
SetThreadContextEnter(
	__in HANDLE hThread,
	__in const CONTEXT* lpContext
	);

typedef BOOL 
(WINAPI *SETTHREADCONTEXT)(
	__in HANDLE hThread,
	__in const CONTEXT* lpContext
	);

DWORD WINAPI 
TlsAllocEnter(
	VOID
	);

typedef DWORD 
(WINAPI *TLSALLOC)(
	VOID
	);

BOOL WINAPI 
TlsFreeEnter(
	__in DWORD dwTlsIndex
	);

typedef BOOL 
(WINAPI *TLSFREE)(
	__in DWORD dwTlsIndex	
	);

LPVOID WINAPI 
TlsGetValueEnter(
	__in DWORD dwTlsIndex
	);

typedef LPVOID 
(WINAPI *TLSGETVALUE)(
	__in DWORD dwTlsIndex	
	);

BOOL WINAPI 
TlsSetValueEnter(
	__in DWORD dwTlsIndex,
	__in LPVOID lpTlsValue
	);

typedef BOOL 
(WINAPI *TLSSETVALUE)(
	__in DWORD dwTlsIndex,
	__in LPVOID lpTlsValue
	);

//
// Miscellaneous
//

BOOL WINAPI 
QueueUserWorkItemEnter(
	__in  LPTHREAD_START_ROUTINE Function,
	__in  PVOID Context,
	__in  ULONG Flags
	);

typedef BOOL 
(WINAPI *QUEUEUSERWORKITEM)(
	__in  LPTHREAD_START_ROUTINE Function,
	__in  PVOID Context,
	__in  ULONG Flags
	);

DWORD WINAPI 
QueueUserAPCEnter(
	__in  PAPCFUNC pfnAPC,
	__in  HANDLE hThread,
	__in  ULONG_PTR dwData
	);

typedef DWORD 
(WINAPI *QUEUEUSERAPC)(
	__in  PAPCFUNC pfnAPC,
	__in  HANDLE hThread,
	__in  ULONG_PTR dwData
	);

//
// Loader Routine
//

//
// GetModuleHandle 
//

typedef HMODULE
(WINAPI *GETMODULEHANDLE)(
	__in  LPCTSTR lpModuleName
	);

typedef struct _PS_GETMODULEHANDLE {
	LPCTSTR lpModuleName;
	USHORT NameLength;
	WCHAR Name[ANYSIZE_ARRAY];
} PS_GETMODULEHANDLE, *PPS_GETMODULEHANDLE;

HMODULE WINAPI 
GetModuleHandleEnter(
	__in  LPCTSTR lpModuleName
	);

ULONG
GetModuleHandleDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// GetModuleHandleExA
//

typedef BOOL 
(WINAPI *GETMODULEHANDLEEXA)(
	__in  DWORD dwFlags,
	__in  PSTR lpModuleName,
	__out HMODULE* phModule
	);

typedef struct _PS_GETMODULEHANDLEEXA {
	DWORD dwFlags;
	PSTR lpModuleName;
	HMODULE phModule;
	USHORT NameLength;
	CHAR Name[ANYSIZE_ARRAY];
} PS_GETMODULEHANDLEEXA, *PPS_GETMODULEHANDLEEXA;

BOOL WINAPI 
GetModuleHandleExAEnter(
	__in  DWORD dwFlags,
	__in  PSTR lpModuleName,
	__out HMODULE* phModule
	);

ULONG
GetModuleHandleExADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// GetModuleHandleExW
//

typedef struct _PS_GETMODULEHANDLEEXW {
	DWORD dwFlags;
	PWSTR lpModuleName;
	HMODULE phModule;
	USHORT NameLength;
	WCHAR Name[ANYSIZE_ARRAY];
} PS_GETMODULEHANDLEEXW, *PPS_GETMODULEHANDLEEXW;

typedef BOOL 
(WINAPI *GETMODULEHANDLEEXW)(
	__in  DWORD dwFlags,
	__in  PWSTR lpModuleName,
	__out HMODULE* phModule
	);

BOOL WINAPI 
GetModuleHandleExWEnter(
	__in  DWORD dwFlags,
	__in  PWSTR lpModuleName,
	__out HMODULE* phModule
	);

ULONG
GetModuleHandleExWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// GetModuleFileName
//

typedef struct _PS_GETMODULEFILENAME {
	HMODULE hModule;
	PWSTR lpFilename;
	DWORD nSize;
	USHORT NameLength;
	WCHAR Name[ANYSIZE_ARRAY];
} PS_GETMODULEFILENAME, *PPS_GETMODULEFILENAME;

typedef DWORD 
(WINAPI *GETMODULEFILENAME)(
	__in  HMODULE hModule,
	__out LPTSTR lpFilename,
	__in  DWORD nSize
	);

ULONG
GetModuleFileNameDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

DWORD WINAPI 
GetModuleFileNameEnter(
	__in  HMODULE hModule,
	__out PWSTR lpFilename,
	__in  DWORD nSize
	);

//
// GetProcAddress
//

typedef struct _PS_GETPROCADDRESS {
	HMODULE hModule;
	PSTR lpProcName;
	WCHAR Module[MAX_PATH];
	USHORT NameLength;
	CHAR Name[ANYSIZE_ARRAY];
} PS_GETPROCADDRESS, *PPS_GETPROCADDRESS;

typedef FARPROC 
(WINAPI *GETPROCADDRESS)(
	__in  HMODULE hModule,
	__in  PSTR lpProcName
	);

FARPROC WINAPI 
GetProcAddressEnter(
	__in  HMODULE hModule,
	__in  PSTR lpProcName
	);

ULONG
GetProcAddressDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// LoadLibraryExW
//

typedef struct _PS_LOADLIBRARYEX {
	PWSTR lpFileName;
	HANDLE hFile;
	DWORD dwFlags;
	USHORT NameLength;
	WCHAR Name[ANYSIZE_ARRAY];
} PS_LOADLIBRARYEX, *PPS_LOADLIBRARYEX;

typedef HMODULE 
(WINAPI *LOADLIBRARYEX)(
	__in  PWSTR lpFileName,
	HANDLE  hFile,
	__in  DWORD dwFlags
	);

HMODULE WINAPI 
LoadLibraryExEnter(
	__in  PWSTR lpFileName,
	HANDLE  hFile,
	__in  DWORD dwFlags
	);

ULONG
LoadLibraryExDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FreeLibrary
//

typedef struct _PS_FREELIBRARY {
	HMODULE hModule;
	USHORT References;
	USHORT NameLength;
	WCHAR Name[ANYSIZE_ARRAY];
} PS_FREELIBRARY, *PPS_FREELIBRARY;

typedef BOOL 
(WINAPI *FREELIBRARY)(
	__in  HMODULE hModule
	);

BOOL WINAPI 
FreeLibraryEnter(
	__in  HMODULE hModule
	);

ULONG
FreeLibraryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

#ifdef __cplusplus
}
#endif

#endif