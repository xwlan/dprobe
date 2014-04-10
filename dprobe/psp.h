//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2010
//

#ifndef _PSP_H_
#define _PSP_H_

#ifdef __cplusplus
extern "C" {
#endif

#pragma once

#include <windows.h>
#include <tchar.h>
#include "bsp.h"
#include "btr.h"
#include "dtl.h"

#pragma warning (disable : 4996)

typedef enum _PSP_PROBE_STATE {
	ProbeNull,
	ProbeRunning,
	ProbeCreating,       
	ProbeEditing,
	ProbeRemoving,
	ProbeAbort,
	ProbeCommit,
} PSP_PROBE_STATE, *PPSP_PROBE_STATE;

typedef enum _PSP_PROBE_FLAG {
	FlagCachedObject = 0x00000001,
} PSP_PROBE_FLAG, *PPSP_PROBE_FLAG;

typedef enum _PSP_PROBE_LIMITS {
	MAX_ARGUMENT_COUNT = 16,
	MAX_MODULE_NAME_LENGTH = 64,
	MAX_ARGUMENT_NAME_LENGTH = 64,
	MAX_FUNCTION_NAME_LENGTH = MAX_PATH,
} PSP_PROBE_LIMITS, *PPSP_PROBE_LIMITS;

typedef enum _PSP_FILTER_STATE {
	FilterCreating,   
	FilterLoading,
	FilterEditing,
	FilterRunning
} PSP_FILTER_STATE;

typedef struct _PSP_PROBE {
	LIST_ENTRY Entry;
	PSP_PROBE_STATE Current;
	PSP_PROBE_STATE Result;
	ULONG_PTR Address;
	BTR_RETURN_TYPE ReturnType;
	WCHAR ModuleName[MAX_MODULE_NAME_LENGTH];
	WCHAR MethodName[MAX_FUNCTION_NAME_LENGTH];
	WCHAR UndMethodName[MAX_FUNCTION_NAME_LENGTH];
} PSP_PROBE, *PPSP_PROBE;

//
// N.B. PSP_BITMAP_SIZE defines maximum 512 bits to represent
// addon probe number.
//

#define PSP_BITMAP_SIZE   64
#define PSP_BITMAP_BITS  512

typedef struct _PSP_FILTER {

	LIST_ENTRY ListEntry;
	
	//
	// N.B. FilterObject is used in the following steps:
	// 1, filter module is loaded, dprobe get the object via BtrFltGetObject
	// 2, filter module is registered in target process, the information is passed back
	//    to dprobe, dprobe update all fields except DecodeProcedure of Probes.
	//
	// The DecodeProcedure is called by dprobe to decode filter records.
	//

	PBTR_FILTER FilterObject;
	BOOLEAN Registered;
	PSP_FILTER_STATE State;

	//
	// Both pending/commit bitmap has maximum 512 bits.
	//

	PBTR_BITMAP PendingBitMap;
    PBTR_BITMAP CommitBitMap;

} PSP_FILTER, *PPSP_FILTER;

typedef enum _PSP_PROCESS_STATE {
	ProcessCreating,
	ProcessRunning,
	ProcessStopping,
} PSP_PROCESS_STATE;

typedef struct _PSP_PROCESS {
	LIST_ENTRY Entry;
	ULONG ProcessId;
	ULONG SessionId;
	WCHAR Name[MAX_PATH];
	WCHAR FullPathName[MAX_PATH];
	WCHAR CommandLine[MAX_PATH];
	PSP_PROCESS_STATE State;	
	LIST_ENTRY ExpressList;
	LIST_ENTRY AddOnList;
} PSP_PROCESS, *PPSP_PROCESS;

#define PSP_OPEN_PROCESS_FLAG (PROCESS_QUERY_INFORMATION |  \
                               PROCESS_CREATE_THREAD     |  \
							   PROCESS_VM_OPERATION      |  \
							   PROCESS_VM_WRITE) 


typedef enum _PSP_MESSAGE {
	WM_PROBE_FIRST    = WM_USER + 100,
	WM_PROBE          = WM_PROBE_FIRST,
	WM_PROBE_ACK      = WM_USER + 101,
	WM_PROBE_STATUS   = WM_USER + 102,
	WM_PROBE_DATA     = WM_USER + 103,
	WM_PROBE_LAST     = WM_PROBE_DATA,
} PSP_MESSAGE;

typedef enum _PSP_INFORMATION {
	PspInformation,
	PspWarning,
	PspError
} PSP_INFORMATION;

typedef enum _PSP_COMMAND_TYPE {
	ManualProbe,
	AddOnProbe,
	ImportProbes,
	StopProbing,
} PSP_COMMAND_TYPE;

typedef struct _PSP_COMMAND {
	PSP_COMMAND_TYPE Type;
	PPSP_PROCESS Process;
	HWND hWndBar;
	ULONG Status;
	ULONG_PTR Information;
} PSP_COMMAND, *PPSP_COMMAND;

typedef struct _PSP_THREAD_CONTEXT {
	ULONG ProcessId;
	HANDLE CompleteEvent;
} PSP_THREAD_CONTEXT, *PPSP_THREAD_CONTEXT;

#define PSP_PORT_SIGNATURE      L"dprobe"

//
// N.B. The buffer length must match the btr runtime buffer 
// length, otherwise, a buffer overflow can occur, we can
// add buffer negotiation in the IPC to dynamically tune 
// buffer size, in next release.
//

#if defined (_M_IX86)
	#define PSP_PORT_BUFFER_LENGTH  (1024 * 1024)
#elif defined (_M_X64)
	#define PSP_PORT_BUFFER_LENGTH  (1024 * 1024)
#endif

typedef enum _PSP_PORT_STATE {
	BtrPortFree,
	BtrPortAllocated
} PSP_PORT_STATE;

#pragma pack(push, 16)

typedef struct _PSP_PORT {
	DECLSPEC_ALIGN(16) SLIST_HEADER StackTraceList;
	LIST_ENTRY ListEntry;
	PSP_PORT_STATE State;
	ULONG ProcessId;
	ULONG ThreadId;
	HANDLE PortHandle;
	HANDLE CompleteHandle;
	HANDLE TimerHandle;
	ULONG TimerPeriod;
	PVOID Buffer;
	ULONG BufferLength;
	OVERLAPPED Overlapped;
	struct _MSP_STACKTRACE_OBJECT *StackTrace;
} PSP_PORT, *PPSP_PORT;

#pragma pack(pop)

typedef struct _PSP_RECORD {
	BTR_RECORD_TYPE Type;
	ULONG Count;
	ULONG DataLength;
	CHAR Data[ANYSIZE_ARRAY];
} PSP_RECORD, *PPSP_RECORD;

ULONG
PspConnectPort(
	IN ULONG ProcessId,
	IN ULONG BufferLength,
	IN ULONG Timeout,
	OUT PPSP_PORT Port
	);

ULONG
PspWaitPortIoComplete(
	IN PPSP_PORT Port,
	IN OVERLAPPED *Overlapped,
	IN ULONG Milliseconds,
	OUT PULONG CompleteBytes
	);

ULONG
PspDisconnectPort(
	IN PPSP_PORT Port
	);

ULONG
PspSendMessage(
	IN PPSP_PORT Port,
	IN PBTR_MESSAGE_HEADER Message,
	IN OVERLAPPED *Overlapped,
	OUT PULONG CompleteBytes 
	);

ULONG
PspWaitMessage(
	IN PPSP_PORT Port,
	OUT PBTR_MESSAGE_HEADER Message,
	IN ULONG BufferLength,
	OUT PULONG CompleteBytes,
	IN OVERLAPPED *Overlapped
	);

ULONG
PspAuditMessage(
	IN PBTR_MESSAGE_HEADER Header,
	IN BTR_MESSAGE_TYPE Type,
	IN ULONG MessageLength
	);

ULONG
PspSendMessageWaitReply(
	IN PPSP_PORT Port,
	IN PBTR_MESSAGE_HEADER Message,
	OUT PBTR_MESSAGE_HEADER Reply
	);

ULONG 
PspCreateSession(
	IN ULONG ProcessId,
	OUT PULONG SessionId
	);

ULONG
PspCreateThread(
	IN PPSP_THREAD_CONTEXT Context,
	OUT PULONG ThreadId
	);

ULONG WINAPI
PspSessionThread(
	IN PVOID Context
	);

ULONG
PspStartSession(
	IN PPSP_PORT Port
	);

PBTR_MESSAGE_EXPRESS
PspBuildManualRequest(
	IN PVOID Buffer,
	IN ULONG Length,
	IN PPSP_PROBE Command 
	);

PBTR_MESSAGE_RECORD
PspBuildRecordRequest(
	IN PPSP_PORT Port,
	IN PVOID Buffer,
	IN ULONG Length
	);

PBTR_MESSAGE_CACHE
PspBuildCacheRequest(
	IN PPSP_PORT Port,
	IN PVOID Buffer,
	IN ULONG Length,
	IN HANDLE IndexFileHandle,
	IN HANDLE DataFileHandle,
	IN HANDLE SharedDataHandle,
	IN HANDLE PerfHandle
	);

PBTR_MESSAGE_FILTER
PspBuildAddOnRequest(
	IN PPSP_PORT Port,
	IN PPSP_FILTER AddOn
	);

PBTR_MESSAGE_RECORD
PspBuildFetchRequest(
	IN PPSP_PORT Port,
	IN PVOID Buffer,
	IN ULONG Length
	);

PBTR_MESSAGE_STOP
PspBuildStopRequest(
	IN PVOID Buffer,
	IN ULONG Length,
	IN PPSP_PROBE Command 
	);

ULONG
PspMessageProcedure(
	IN PPSP_PORT Port
	);

ULONG
PspUpdateStackTrace(
	IN PPSP_PORT Port
	);

ULONG
PspCommandProcedure(
	IN PPSP_PORT Port,
	IN MSG *Msg 
	);

BOOLEAN
PspSetWaitableTimer(
	IN PPSP_PORT Port
	);

ULONG
PspInitialize(
	IN ULONG Features	
	);

PPSP_PROCESS
PspQueryProcessObject(
	IN ULONG ProcessId
	);

VOID
PspLockProcessList(
	VOID
	);

VOID
PspUnlockProcessList(
	VOID
	);

ULONG
PspCommitManualProbe(
	IN PPSP_COMMAND Command
	);

ULONG
PspStopProbe(
	IN ULONG ProcessId
	);

ULONG
PspLoadProbeRuntime(
	IN ULONG ProcessId
	);

ULONG
PspStartProbeRuntime(
	IN ULONG ProcessId
	);

BOOLEAN
PspIsRuntimeLoaded(
	IN ULONG ProcessId
	);

PPSP_PROBE
PspIsRegisteredProbe(
	IN PPSP_PROCESS Process,
	IN PWSTR ModuleName,
	IN PWSTR ApiName,
	IN PSP_COMMAND_TYPE Type
	);

ULONG
PspDispatchCommand(
	IN PPSP_COMMAND Command 
	);

PPSP_COMMAND
PspCreateCommandObject(
	IN PSP_COMMAND_TYPE Type,
	IN PPSP_PROCESS Process
	);

ULONG
PspFreeCommand(
	IN PPSP_COMMAND Command
	);

PPSP_PROBE
PspCreateProbe(
	IN PWSTR ModuleName,
	IN PWSTR MethodName,
	IN PWSTR UndMethodName
	);

VOID
PspFreeProbe(
	IN PPSP_PROBE Probe
	);

VOID
PspFreeFilter(
	IN PPSP_FILTER Filter
	);

VOID
PspFreeProcess(
	IN PPSP_PROCESS Process
	);

ULONG
PspGetSession(
	IN ULONG ProcessId
	);

ULONG
PspGetFilePath(
	IN PWSTR Buffer,
	IN ULONG Length
	);

ULONG
PspGetRuntimeFullPathName(
	IN PWCHAR Buffer,
	IN USHORT Length,
	OUT PUSHORT ActualLength
	);

PPSP_PROCESS
PspCreateProcessObject(
	VOID
	);

BOOLEAN
PspRemoveProcess(
	IN ULONG ProcessId
	);

ULONG
PspActiveProcessCount(
	VOID
	);

VOID
PspCopyAddOnList(
	IN PPSP_PROCESS Process
	);

ULONG
PspLookupProcessName(
	IN ULONG ProcessId,
	IN PWCHAR Buffer,
	IN ULONG BufferLength
	);

ULONG
PspLookupProbeByAddress(
	IN ULONG ProcessId,
	IN ULONG_PTR Address,
	IN PWCHAR Buffer,
	IN ULONG BufferLength
	);

ULONG
PspLaunchQueryMode(
	IN PWSTR FullPath
	);

PBTR_RECORD_HEADER
PspGetRecordBase(
	IN PPSP_RECORD Record
	);

ULONG
PspQueueRecord(
	IN PPSP_PORT Port,
	IN PBTR_MESSAGE_RECORD_ACK Record,
	IN BTR_RECORD_TYPE Type
	);

PBTR_FILTER
PspGetFilterRegistration(
	IN PWSTR FilterName
	);

PPSP_FILTER
PspCreateAddOnObject(
	VOID
	);

VOID
PspCopyAddOnBitMap(
	IN PVOID Destine,
	IN PVOID Source
	);

PPSP_FILTER
PspGetAddOnObject(
	IN PPSP_FILTER RootObject,
	IN PPSP_FILTER TargetObject
	);

PPSP_FILTER
PspQueryAddOnObjectByName(
	IN PPSP_PROCESS Process,
	IN PWSTR Name
	);

PBTR_FILTER
PspLookupAddOnByTag(
	IN PBTR_FILTER_RECORD Record
	);

PPSP_FILTER
PspQueryGlobalAddOnObject(
	IN PWSTR Path
	);

ULONG
PspBuildAddOnList(
	IN PWSTR Directory	
	);

PPSP_FILTER
PspQueryAddOnByPath(
	IN PPSP_PROCESS Process,
	IN PWCHAR AddOnPath 
	);

ULONG
PspOnCommandAddOnProbe(
	IN PPSP_PORT Port,
	IN PPSP_COMMAND Command
	);

ULONG
PspOnCommandImportProbe(
	IN PPSP_PORT Port,
	IN PPSP_COMMAND Command
	);

ULONG
PspOnCommandManualProbe(
	IN PPSP_PORT Port,
	IN PPSP_COMMAND Command
	);

ULONG
PspOnCommandStop(
	IN PPSP_PORT Port,
	IN PPSP_COMMAND Command
	);

ULONG
PspGetRunningManualProbeCount(
	IN PPSP_PROCESS Process
	);

ULONG
PspGetRunningAddOnCount(
	IN PPSP_PROCESS Process
	);

typedef struct _DTL_OBJECT *PDTL_OBJECT;
typedef struct _BSP_QUEUE *PBSP_QUEUE;

ULONG
PspCreateDtlObject(
	VOID 
	);

PDTL_OBJECT 
PspGetDtlObject(
	IN PPSP_PORT Port
	);

VOID
PspClearDtlObject(
	VOID
	);

ULONG
PspCreateQueueObject(
	VOID
	);

PBSP_QUEUE
PspGetQueueObject(
	VOID
	);

BOOLEAN
PspIsProbing(
	VOID
	);

ULONG
PspStopAllSession(
	VOID
	);

ULONG
PspCreateProcessWithDpc(
	IN PWSTR ImagePath,
	IN PWSTR Argument,
	IN PWSTR DpcPath
	);

ULONG
PspImportDpc(
	IN ULONG ProcessId,
	IN PWSTR ImageName,
	IN PWSTR DpcPath 
	);

ULONG CALLBACK
PspStackTraceProcedure(
	IN PVOID Context
	);

extern WCHAR PspRuntimeFullPath[];

extern LIST_ENTRY PspProcessList;
extern BSP_LOCK PspProcessLock;
extern LIST_ENTRY PspAddOnList;
extern struct _DTL_OBJECT *PspDtlObject;
extern struct _BSP_QUEUE *PspQueueObject;

extern ULONG PspRuntimeFeatures;
extern SLIST_HEADER PspStackTraceQueue;

#ifdef __cplusplus
}
#endif

#endif