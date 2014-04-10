//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "mspdefs.h"
#include "bsp.h"

#ifdef __cplusplus
extern "C" {
#endif 

//
// Log Routine
//

#define MSP_LOG_TEXT_LIMIT   (MAX_PATH * 2)

typedef enum _MSP_LOG_LEVEL {
	LogLevelInfo,
	LogLevelError,
} MSP_LOG_LEVEL, *PMSP_LOG_LEVEL;

typedef struct _MSP_LOG_ENTRY {
	LIST_ENTRY ListEntry;
	SYSTEMTIME TimeStamp;
	ULONG ProcessId;
	MSP_LOG_LEVEL Level;
	CHAR Text[MSP_LOG_TEXT_LIMIT];
} MSP_LOG_ENTRY, *PMSP_LOG_ENTRY;

typedef struct _MSP_LOG { 
	CRITICAL_SECTION Lock;
	LIST_ENTRY ListHead;
	ULONG Flag;
	ULONG ListDepth;
	HANDLE FileObject;
	CHAR Path[MAX_PATH];
} MSP_LOG, *PMSP_LOG;

ULONG
MspInitializeLog(
	IN ULONG Flag	
	);

ULONG
MspUninitializeLog(
	VOID
	);

VOID
MspWriteLogEntry(
	IN ULONG ProcessId,
	IN MSP_LOG_LEVEL Level,
	IN PSTR Format,
	...
	);

ULONG
MspCurrentLogListDepth(
	VOID
	);

ULONG CALLBACK
MspLogProcedure(
	IN PVOID Context
	);

BOOLEAN
MspIsLoggingEnabled(
	VOID
	);

VOID
MspGetLogFilePath(
	OUT PCHAR Buffer,
	IN ULONG Length
	);

PVOID
MspMalloc(
	IN SIZE_T Length
	);
VOID
MspFree(
	IN PVOID Address
	);

HANDLE
MspCreatePrivateHeap(
	VOID
	);

VOID
MspDestroyPrivateHeap(
	IN HANDLE NormalHeap
	);

BOOLEAN
MspGetModuleInformation(
	IN ULONG ProcessId,
	IN PWSTR DllName,
	IN BOOLEAN FullPath,
	OUT HMODULE *DllHandle,
	OUT PULONG_PTR Address,
	OUT SIZE_T *Size
	);

PVOID
MspGetRemoteApiAddress(
	IN ULONG ProcessId,
	IN PWSTR DllName,
	IN PSTR ApiName
	);

ULONG
MspLoadRuntime(
	IN ULONG ProcessId,
	IN HANDLE ProcessHandle
	);

BOOLEAN
MspIsRuntimeLoaded(
	IN ULONG ProcessId
	);

BOOLEAN
MspIsWindowsXPAbove(
	VOID
	);

BOOLEAN 
MspIsWow64Process(
	IN HANDLE ProcessHandle	
	);

BOOLEAN
MspIsLegitimateProcess(
	IN ULONG ProcessId
	);

BOOLEAN
MspIsValidFilterPath(
	IN PWSTR Path
	);

ULONG
MspQueryProcessFullPath(
	IN ULONG ProcessId,
	IN PWSTR Buffer,
	IN ULONG Length,
	OUT PULONG Result
	);


ULONG_PTR
MspUlongPtrRoundDown(
	IN ULONG_PTR Value,
	IN ULONG_PTR Align
	);

ULONG_PTR
MspUlongPtrRoundUp(
	IN ULONG_PTR Value,
	IN ULONG_PTR Align
	);

ULONG
MspUlongRoundDown(
	IN ULONG Value,
	IN ULONG Align
	);

ULONG
MspUlongRoundUp(
	IN ULONG Value,
	IN ULONG Align
	);

ULONG64
MspUlong64RoundDown(
	IN ULONG64 Value,
	IN ULONG64 Align
	);

ULONG64
MspUlong64RoundUp(
	IN ULONG64 Value,
	IN ULONG64 Align
	);

BOOLEAN
MspIsUlongAligned(
	IN ULONG Value,
	IN ULONG Unit
	);

BOOLEAN
MspIsUlong64Aligned(
	IN ULONG64 Value,
	IN ULONG64 Unit
	);

double
MspComputeMilliseconds(
	IN ULONG Duration
	);

VOID
MspComputeHardwareFrequency(
	VOID
	);

VOID
MspCleanCache(
	IN PWSTR Path,
	IN ULONG Level
	);

ULONG
MspIsManagedModule(
	IN PWSTR Path,
	OUT BOOLEAN *Managed
	);

ULONG
MspGetImageType(
	IN PWSTR Path,
	OUT PULONG Type
	);

BOOLEAN
MspIsRuntimeFilterDll(
	IN PWSTR Path
	);

//
// WOW64 related data
//

extern BOOLEAN MspIs64Bits;
extern BOOLEAN MspIsWow64;

typedef BOOL 
(WINAPI *ISWOW64PROCESS)(
	IN HANDLE, 
	OUT PBOOL
	);

extern ISWOW64PROCESS MspIsWow64ProcessPtr;

//
// N.B. Runtime is loaded via RtlCreateUserThread for NT 6+
//
//
//typedef struct _CLIENT_ID {
//	HANDLE UniqueProcess;
//	HANDLE UniqueThread;
//} CLIENT_ID, *PCLIENT_ID;

typedef NTSTATUS
(NTAPI *RTL_CREATE_USER_THREAD)(
    IN HANDLE Process,
    IN PSECURITY_DESCRIPTOR ThreadSecurityDescriptor,
    IN BOOLEAN CreateSuspended,
    IN ULONG ZeroBits,
    IN SIZE_T MaximumStackSize,
    IN SIZE_T CommittedStackSize,
    IN LPTHREAD_START_ROUTINE StartAddress,
    IN PVOID Parameter,
    OUT PHANDLE Thread,
    OUT PCLIENT_ID ClientId
    );

extern HANDLE MspStopEvent;
extern HANDLE MspPrivateHeap;

extern ULONG MspMajorVersion;
extern ULONG MspMinorVersion;
extern ULONG MspPageSize;
extern ULONG MspProcessorNumber;
extern RTL_CREATE_USER_THREAD MspRtlCreateUserThread;

#ifdef __cplusplus
}
#endif

#endif
