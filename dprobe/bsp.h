//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _BSP_H_
#define _BSP_H_

#ifdef __cplusplus
extern "C" {
#endif

#pragma warning(disable : 4996)

#include <windows.h>
#include <tchar.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <assert.h>
#include "ntapi.h"
#include "list.h"

typedef CRITICAL_SECTION	BSP_LOCK;
typedef LPCRITICAL_SECTION	PBSP_LOCK;

typedef struct _BSP_MODULE {
	LIST_ENTRY ListEntry;
	PVOID BaseVa;
	ULONG Size;
	PWSTR Name;
	PWSTR FullPath;
	PWSTR Version;
	PWSTR TimeStamp;
	PWSTR Company;
	PWSTR Description;
	PVOID Context;
} BSP_MODULE, *PBSP_MODULE;

typedef struct _BSP_THREAD {
	LIST_ENTRY ListEntry;
	ULONG ThreadId;
	ULONG Priority;
	PVOID StartAddress;
} BSP_THREAD, *PBSP_THREAD;

typedef struct _BSP_PROCESS {

	LIST_ENTRY ListEntry;
	PVOID PebAddress;
	ULONG ProcessId;
	ULONG ParentId;
	ULONG SessionId;
	PWSTR Name;
	PWSTR FullPath;
	PWSTR CommandLine;
	
	ULONG Priority;
	ULONG ThreadCount;
	FILETIME CreateTime; 
	LARGE_INTEGER KernelTime;
	LARGE_INTEGER UserTime;
	ULONG KernelHandleCount;
	ULONG GdiHandleCount;
	ULONG UserHandleCount;
	ULONG_PTR VirtualBytes;
	ULONG_PTR PrivateBytes;
	ULONG_PTR WorkingSetBytes;
	LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;

} BSP_PROCESS, *PBSP_PROCESS;

typedef struct _BSP_EXPORT_DESCRIPTOR {
	LIST_ENTRY Entry;
	PSTR Name;
	PSTR Forward;
	ULONG Ordinal;
	ULONG Rva;
	ULONG_PTR Va;
} BSP_EXPORT_DESCRIPTOR, *PBSP_EXPORT_DESCRIPTOR;

typedef struct _BSP_CALLBACK {
	LIST_ENTRY ListEntry;
	PVOID Callback;
	PVOID Context;
} BSP_CALLBACK, *PBSP_CALLBACK;

//
// Internal logging support types
//

typedef enum _BSP_LOG_SOURCE {
	LOG_SOURCE_BSP,
	LOG_SOURCE_PSP,
	LOG_SOURCE_DTL,
	LOG_SOURCE_PDB,
	LOG_SOURCE_MDB,
	LOG_SOURCE_BTR,
	LOG_SOURCE_GUI,
	LOG_SOURCE_NUMBER,
} BSP_LOG_SOURCE, *PBSP_LOG_SOURCE;

typedef enum _BSP_LOG_LEVEL {
	LOG_LEVEL_INFORMATION,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_CRITICAL,
	LOG_LEVEL_NUMBER,
} BSP_LOG_LEVEL, *PBSP_LOG_LEVEL;

typedef struct _BSP_LOG_ENTRY {
	LIST_ENTRY ListEntry;
	SYSTEMTIME TimeStamp;
	BSP_LOG_LEVEL Level;
	BSP_LOG_SOURCE Source;
	WCHAR Message[MAX_PATH];
} BSP_LOG_ENTRY, *PBSP_LOG_ENTRY;

ULONG
BspInitialize(
	VOID
	);

BOOLEAN 
BspIsWow64Process(
	IN HANDLE ProcessHandle	
	);

PVOID
BspMalloc(
	IN ULONG Length
	);

VOID
BspFree(
	IN PVOID BaseVa
	);

BOOLEAN
BspIsVistaAbove(
	VOID
	);

BOOLEAN
BspIsWindowsXPAbove(
	VOID
	);

ULONG 
BspAdjustPrivilege(
    IN PWSTR PrivilegeName, 
    IN BOOLEAN Enable
    );

//
// Process Routine
//

ULONG
BspAllocateQueryBuffer(
	IN SYSTEM_INFORMATION_CLASS InformationClass,
	OUT PVOID *Buffer,
	OUT PULONG BufferLength,
	OUT PULONG ActualLength
	);

VOID
BspReleaseQueryBuffer(
	IN PVOID StartVa
	);

ULONG
BspQueryProcessList(
	OUT PLIST_ENTRY ListHead
	);

PBSP_PROCESS
BspQueryProcessById(
	IN PLIST_ENTRY ListHead,
	IN ULONG ProcessId
	);

ULONG
BspQueryProcessInformation(
	IN ULONG ProcessId,
	OUT PSYSTEM_PROCESS_INFORMATION *Information
	);

ULONG
BspQueryProcessByName(
	IN PWSTR Name,
	OUT PLIST_ENTRY ListHead
	);

VOID
BspFreeProcess(
	IN PBSP_PROCESS Process
	);

VOID
BspFreeProcessList(
	IN PLIST_ENTRY ListHead
	);

ULONG
BspQueryModule(
	IN ULONG ProcessId,
	IN BOOLEAN Version,
	OUT PLIST_ENTRY ListHead
	);

PBSP_MODULE
BspGetModuleByName(
	IN PLIST_ENTRY ListHead,
	IN PWSTR ModuleName
	);

PBSP_MODULE
BspGetModuleByAddress(
	IN PLIST_ENTRY ListHead,
	IN PVOID StartVa
	);

VOID
BspFreeModuleList(
	IN PLIST_ENTRY ListHead
	);

//
// PE API
//

PVOID
BspMapImageFile(
	IN PWSTR ModuleName,
	OUT PHANDLE FileHandle,
	OUT PHANDLE ViewHandle
	);

VOID
BspUnmapImageFile(
	IN PVOID  MappedBase,
	IN HANDLE FileHandle,
	IN HANDLE ViewHandle
	);

ULONG
BspRvaToOffset(
	IN ULONG Rva,
	IN PIMAGE_SECTION_HEADER Headers,
	IN ULONG NumberOfSections
	);

ULONG
BspEnumerateDllExport(
	IN PWSTR ModuleName,
	IN PBSP_MODULE Module, 
	OUT PULONG Count,
	OUT PBSP_EXPORT_DESCRIPTOR *Descriptor
	);

PVOID
BspGetDllForwardAddress(
	IN ULONG ProcessId,
	IN PSTR DllForward
	);

VOID
BspGetForwardDllName(
	IN PSTR Forward,
	OUT PCHAR DllName,
	IN ULONG Length
	);

VOID
BspGetForwardApiName(
	IN PSTR Forward,
	OUT PCHAR ApiName,
	IN ULONG Length
	);

VOID
BspFreeModule(
	IN PBSP_MODULE Module 
	);

ULONG
BspEnumerateDllExportByName(
	IN ULONG ProcessId,
	IN PWSTR ModuleName,
	IN PWSTR ApiName,
	OUT PULONG Count,
	OUT PLIST_ENTRY ApiList
	);

VOID
BspFreeExportDescriptor(
	IN PBSP_EXPORT_DESCRIPTOR Descriptor,
	IN ULONG Count
	);

VOID
BspInitializeLock(
	IN PBSP_LOCK Lock
	);

VOID
BspAcquireLock(
	IN PBSP_LOCK Lock
	);

VOID
BspReleaseLock(
	IN PBSP_LOCK Lock
	);

VOID
BspDestroyLock(
	IN PBSP_LOCK Lock
	);

BOOLEAN
BspTryAcquireLock(
	IN PBSP_LOCK Lock	
	);

double
BspComputeMilliseconds(
	IN ULONG Duration
	);

ULONG
BspRoundUp(
	IN ULONG Value,
	IN ULONG Unit 
	);

ULONG_PTR
BspRoundUpUlongPtr(
	IN ULONG_PTR Value,
	IN ULONG_PTR Unit 
	);

ULONG 
BspRoundDown(
	IN ULONG Value,
	IN ULONG Unit
	);

ULONG64
BspRoundUpUlong64(
	IN ULONG64 Value,
	IN ULONG64 Unit
	);

ULONG64
BspRoundDownUlong64(
	IN ULONG64 Value,
	IN ULONG64 Unit
	);

BOOLEAN
BspIsUlong64Aligned(
	IN ULONG64 Value,
	IN ULONG64 Unit
	);

VOID
DebugTrace(
	IN PSTR Format,
	...
	);

VOID
BspSetFlag(
	OUT PULONG Flag,
	IN ULONG Bit
	);

VOID
BspClearFlag(
	OUT PULONG Flag,
	IN ULONG Bit
	);

BOOLEAN
BspFlagOn(
	IN ULONG Flag,
	IN ULONG Bit
	);

PVOID
BspAllocateGuardedChunk(
	IN ULONG NumberOfPages	
	);

VOID
BspFreeGuardedChunk(
	IN PVOID Address
	);

PVOID
BspQueryTebAddress(
	IN HANDLE ThreadHandle
	);

PVOID
BspQueryPebAddress(
	IN HANDLE ProcessHandle 
	);

SIZE_T 
BspConvertUnicodeToUTF8(
	IN PWSTR Source, 
	OUT PCHAR *Destine
	);

VOID 
BspConvertUTF8ToUnicode(
	IN PCHAR Source,
	OUT PWCHAR *Destine
	);

ULONG 
BspGoldenHash(
	IN PVOID Address
	);

BOOLEAN
BspGetSystemRoutine(
	VOID
	);

PULONG_PTR
BspGetCurrentStackBase(
	VOID
	);

VOID
BspWriteLogEntry(
	IN BSP_LOG_LEVEL Level,
	IN BSP_LOG_SOURCE Source,
	IN PWSTR Message
	);

VOID
BspCleanLogEntry(
	VOID
	);

ULONG
BspInjectPreExecute(
	IN ULONG ProcessId,
	IN HANDLE ProcessHandle,
	IN HANDLE ThreadHandle,
	IN PWSTR ImagePath,
	IN HANDLE CompleteEvent,
	IN HANDLE SuccessEvent, 
	IN HANDLE ErrorEvent 
	);

ULONG
BspWaitDllLoaded(
	IN ULONG ProcessId,
	IN HANDLE ProcessHandle,
	IN HANDLE ThreadHandle
	);

ULONG
BspCaptureMemoryDump(
	IN ULONG ProcessId,
	IN BOOLEAN Full,
	IN PWSTR Path,
	IN PEXCEPTION_POINTERS Pointers,
	IN BOOLEAN Local
	);

ULONG
BspGetFullPathName(
	IN PWSTR BaseName,
	IN PWCHAR Buffer,
	IN USHORT Length,
	OUT PUSHORT ActualLength
	);

ULONG
BspGetProcessPath(
	IN PWCHAR Buffer,
	IN USHORT Length,
	OUT PUSHORT ActualLength
	);

BOOLEAN
BspGetModuleInformation(
	IN ULONG ProcessId,
	IN PWSTR DllName,
	IN BOOLEAN FullPath,
	OUT HMODULE *DllHandle,
	OUT PULONG_PTR Address,
	OUT SIZE_T *Size
	);

PVOID
BspGetRemoteApiAddress(
	IN ULONG ProcessId,
	IN PWSTR DllName,
	IN PSTR ApiName
	);

//
// CRT unDName() routine, we have to use it to decorate
// C++ symbol name, because dbghelp is single threaded,
// we typically call unDName() in UI thread, dbghelp is
// called in worker thread to decode stack records.
//

char* __cdecl
__unDName(
	OUT PCHAR Buffer, 
	IN PSTR MangledName, 
	IN ULONG BufferLength,
    IN PVOID MallocPtr, 
	IN PVOID FreePtr,
    IN USHORT Flag
	);

VOID
BspUnDecorateSymbolName(
	IN PWSTR DecoratedName,
	IN PWSTR UnDecoratedName,
	IN ULONG UndecoratedLength,
	IN ULONG Flags
	);

//
// Extern declaration of variables
//

extern ULONG BspMajorVersion;
extern ULONG BspMinorVersion;
extern ULONG BspPageSize;
extern ULONG BspProcessorNumber;
extern HANDLE BspHeapHandle;

extern LIST_ENTRY BspLogList;
extern BSP_LOCK BspLogLock;
extern ULONG BspLogCount;

extern BOOLEAN BspIs64Bits;
extern BOOLEAN BspIsWow64;

#ifndef ASSERT
#define ASSERT assert
#endif

#ifdef __cplusplus
}
#endif

#endif
