//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009 - 2010
//

#ifndef _DEBUGGER_H_
#define _DEBUGGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dprobe.h"

typedef struct _PSP_DEBUGGEE {
	ULONG ProcessId;
	WCHAR FullPath[MAX_PATH];
} PSP_DEBUGGEE, *PPSP_DEBUGGEE;

#pragma pack(push, 1)

typedef struct _PSP_HIJACK_BLOCK {
	CHAR PushImmRet[5];
	CHAR Pushad;
	CHAR Pushfd;
	CHAR PushImmDllPath[5];
	CHAR CallLibraryW[5];
	CHAR Popfd;
	CHAR Popad;
	CHAR Ret;
	CHAR Int3[4];
	WCHAR FullPath[MAX_PATH];
} PSP_HIJACK_BLOCK, *PPSP_HIJACK_BLOCK;

#pragma pack(pop)

typedef struct _PSP_HIJACK_THREAD {
	LIST_ENTRY ListEntry;
	ULONG ThreadId;
	HANDLE ThreadHandle;
	CONTEXT Context;
} PSP_HIJACK_THREAD, *PPSP_HIJACK_THREAD;

//
// Define debug complete status value
//

#define DBG_COMPLETE 1

ULONG WINAPI
PspDebuggerThread(
	IN PVOID StartContext
	);

PPSP_HIJACK_BLOCK 
PspCreateHijackBlock(
	IN PVOID BaseVa,
	IN PVOID ReturnAddress,
	IN PWSTR FullPath,
	IN PVOID LoadLibraryVa
	);

ULONG
PspLoadLibrary(
	IN ULONG ProcessId,
	IN PWSTR DllFullPath
	);

ULONG
PspQueryMostBusyThread(
	IN ULONG ProcessId,
	OUT PULONG ThreadId
	);

ULONG
PspScanHijackableThreads(
	IN ULONG ProcessId,
	OUT PLIST_ENTRY ThreadListHead
	);

VOID
PspCleanHijackThreads(
	IN PLIST_ENTRY HijackListHead
	);

PVOID
PspQueryPebAddress(
	IN ULONG ProcessId
	);

typedef DEBUG_EVENT *PDEBUG_EVENT;
				   
typedef enum PSP_DEBUG_STATE {
	PSP_DEBUG_STATE_NULL,
	PSP_DEBUG_INIT_BREAK,
	PSP_DEBUG_INTERCEPT_BREAK,
	PSP_DEBUG_COMPLETE_BREAK,
} PSP_DEBUG_STATE;

typedef struct _PSP_DEBUG_THREAD {
	LIST_ENTRY ListEntry;
	ULONG ThreadId;
	HANDLE ThreadHandle;
	CONTEXT Context;
	BOOLEAN BreakInThread;
} PSP_DEBUG_THREAD, *PPSP_DEBUG_THREAD;

typedef struct _PSP_DEBUG_CONTEXT {
	HANDLE ProcessHandle;
	HANDLE ThreadHandle;
	ULONG ProcessId;
	ULONG ThreadId;
	LIST_ENTRY ThreadList;
	CONTEXT Context;
	ULONG_PTR LoadLibraryPtr;
	ULONG_PTR SystemCallReturn;
	ULONG_PTR DbgUiRemoteBreakin;
	ULONG_PTR DbgBreakPointRet;
	ULONG_PTR AdjustedEsp;
	PVOID ReturnAddress;
	ULONG_PTR DllFullPath;
	ULONG State;
	CHAR CodeByte;
	BOOLEAN Success;
} PSP_DEBUG_CONTEXT, *PPSP_DEBUG_CONTEXT;

ULONG
PspDebugOnCreateProcess(
	IN PDEBUG_EVENT Event
	);

ULONG
PspDebugOnExitProcess(
	IN PDEBUG_EVENT Event
	);

ULONG
PspDebugOnCreateThread(
	IN PDEBUG_EVENT Event
	);

ULONG
PspDebugOnExitThread(
	IN PDEBUG_EVENT Event
	);

ULONG
PspDebugOnLoadDll(
	IN PDEBUG_EVENT Event
	);

ULONG
PspDebugOnUnloadDll(
	IN PDEBUG_EVENT Event
	);

ULONG
PspDebugOnException(
	IN PDEBUG_EVENT Event
	);

ULONG
PspDebugOnDebugPrint(
	IN PDEBUG_EVENT Event
	);

ULONG CALLBACK
PspDebugProcedure(
	IN PVOID Context 
	);

VOID
PspDebugInitialize(
	VOID
	);

PPSP_DEBUG_THREAD
PspDebugInsertThread(
	IN PDEBUG_EVENT Event
	);

PPSP_DEBUG_THREAD
PspDebugLookupThread(
	IN ULONG ProcessId 
	);

#ifdef __cplusplus
}
#endif

#endif