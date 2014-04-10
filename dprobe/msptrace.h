//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2011
//

#ifndef _TRACE_H_
#define _TRACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include "mspdefs.h"
#include "list.h"
#include "mspqueue.h"
#include "mspport.h"

//
// MSP FLAG
//

#define MSP_FLAG_LOGGING   0x00000001


typedef enum _MSP_PROCESS_STATE {
	MSP_STATE_NULL,
	MSP_STATE_INIT_FAILED,
	MSP_STATE_INIT_SUCCEED,
	MSP_STATE_STARTED,
	MSP_STATE_ERROR_STOPPED,
	MSP_STATE_STOPPED,
} MSP_PROCESS_STATE, *PMSP_PROCESS_STATE;

typedef ULONG
(WINAPI *MSP_TRACE_CALLBACK)(
	IN UUID *TraceUuid,
	IN PVOID Buffer,
	IN ULONG Length 
	);

typedef ULONG
(WINAPI *MSP_ERROR_CALLBACK)(
	IN UUID *TraceUuid,
	IN ULONG ErrorCode
	);

typedef enum _MSP_CONTROLCODE {
	MSP_START_TRACE       = 0x00000001,
	MSP_STOP_TRACE        = 0x00000002,
	MSP_REGISTER_FILTER   = 0x00000004,
	MSP_UNREGISTER_FILTER = 0x00000008,
	MSP_PROBE_ON          = 0x00000010,
	MSP_PROBE_OFF         = 0x00000020,
} MSP_CONTROLCODE, *PMSP_CONTROLCODE;

typedef struct _MSP_CONTROL {

	MSP_CONTROLCODE ControlCode;
	ULONG Result;

	union {
		
		struct {
			HANDLE IndexFileHandle;
			HANDLE SharedDataHandle;
		} Start;

		struct {
			ULONGLONG Alignment;
		} Stop;

		struct {
			WCHAR FilterName[MAX_PATH];
			ULONG ResultBits[16];
		} Filter;

		struct {
			WCHAR FilterName[MAX_PATH];
			ULONG Ordinal;
		} Probe;

	} u;

} MSP_CONTROL, *PMSP_CONTROL;

//
// Forward declaration
//
struct _MSP_QUEUE;
struct _MSP_CACHE;

typedef struct _MSP_TRACE {
	LIST_ENTRY ListEntry;
	GUID TraceId;
	PSTR Uuid;
	ULONG ProcessId;
	ULONG ThreadId;
	MSP_TRACE_STATE State;
	MSP_TRACE_CALLBACK TraceCallback;
	MSP_ERROR_CALLBACK ErrorCallback;
	PMSP_MESSAGE_PORT PortObject;
	struct _MSP_QUEUE *QueueObject;
	HANDLE CompleteEvent;
	HANDLE ThreadHandle;
	HANDLE ProcessHandle;
	WCHAR DataPath[MAX_PATH];
	LIST_ENTRY FilterList;
} MSP_TRACE, *PMSP_TRACE;

typedef struct _MSP_THREAD_CONTEXT {
	HANDLE CompleteEvent;
	PMSP_TRACE TraceObject;
} MSP_THREAD_CONTEXT, *PMSP_THREAD_CONTEXT;

//
// MSP API
//

ULONG WINAPI
MspInitialize2(
	IN MSP_MODE Mode,
	IN ULONG Flag,
	IN PWSTR BtrPath,
	IN ULONG BtrFeature,
	IN PWSTR CachePath
	);

ULONG WINAPI
MspCreateTrace(
	IN ULONG ProcessId,
	IN MSP_TRACE_CALLBACK TraceCallback,
	IN MSP_ERROR_CALLBACK ErrorCallback,
	OUT UUID *TraceUuid
	);

ULONG WINAPI
MspRegisterFilter(
	IN UUID *TraceUuid,
	IN PWSTR FilterPath
	);

ULONG WINAPI
MspUnregisterFilter(
	IN UUID *TraceUuid,
	IN PWSTR FilterPath
	);

ULONG WINAPI
MspTurnOnProbe(
	IN UUID *TraceUuid,
	IN PWSTR FilterPath,
	IN ULONG Ordinal
	);

ULONG WINAPI
MspTurnOffProbe(
	IN UUID *TraceUuid,
	IN PWSTR FilterPath,
	IN ULONG Ordinal
	);

ULONG WINAPI
MspStartTrace(
	IN UUID *TraceUuid
	);

ULONG WINAPI
MspStopTrace(
	IN UUID *TraceUuid
	);

ULONG WINAPI
MspFilterControl(
	IN UUID *TraceUuid,
	IN PWSTR FilterPath,
	IN PBTR_FILTER_CONTROL Control,
	OUT PBTR_FILTER_CONTROL *Ack
	);

VOID WINAPI
MspUninitialize2(
	VOID	
	);

//
// Trace Lookup 
//

PMSP_TRACE
MspLookupTraceByUuid(
	IN UUID *TraceUuid 
	);

PMSP_TRACE
MspLookupTraceByProcessId(
	IN ULONG ProcessId
	);

//
// Loader Routine 
//

ULONG
MspLoadRuntime(
	IN ULONG ProcessId
	);

ULONG
MspUnloadRuntime(
	IN ULONG ProcessId
	);

ULONG
MspValidateCachePath(
	IN PWSTR Path
	);

ULONG
MspAdjustPrivilege(
    IN PWSTR PrivilegeName, 
    IN BOOLEAN Enable
    ); 

BOOLEAN
MspIsLegitimateOrdinal(
	IN PBTR_FILTER Filter,
	IN ULONG Ordinal
	);

PBTR_FILTER
MspQueryFilterObject(
	IN PMSP_TRACE Trace,
	IN PWSTR FilterPath
	);

PBTR_FILTER WINAPI
MspLoadFilter(
	IN PWSTR FilterPath
	);

VOID
MspUnloadFilter(
	IN PBTR_FILTER Filter
	);

ULONG
MspStopAllTraces(
	VOID
	);

VOID
MspDeleteTrace(
	IN PMSP_TRACE Trace
	);

BTR_DECODE_CALLBACK 
MspGetDecodeCallback(
	IN PBTR_FILTER_RECORD Record 
	);

VOID
MspInsertDecodeFilter(
	IN PBTR_FILTER Filter
	);

ULONG WINAPI
MspDecodeDetailA(
	IN PBTR_FILTER_RECORD Record,
	IN PSTR Buffer,
	IN ULONG Length
	);

ULONG WINAPI
MspDecodeDetailW(
	IN PBTR_FILTER_RECORD Record,
	IN PWSTR Buffer,
	IN ULONG Length
	);

ULONG 
MspDecodeProbeA(
	IN PBTR_FILTER_RECORD Record,
	IN PSTR NameBuffer,
	IN ULONG NameLength,
	IN PSTR ModuleBuffer,
	IN ULONG ModuleLength
	);

ULONG 
MspDecodeProbeW(
	IN PBTR_FILTER_RECORD Record,
	IN PWSTR NameBuffer,
	IN ULONG NameLength,
	IN PWSTR ModuleBuffer,
	IN ULONG ModuleLength
	);

PBTR_FILTER
MspLookupDecodeFilter(
	IN PBTR_FILTER_RECORD Record
	);

SIZE_T 
MspConvertUnicodeToAnsi(
	IN PWSTR Source, 
	OUT PCHAR *Destine
	);

VOID 
MspConvertAnsiToUnicode(
	IN PCHAR Source,
	OUT PWCHAR *Destine
	);

VOID
MspUnloadFilters(
	VOID
	);

#define MSP_PORT_SIGNATURE      L"dprobe"
#define MSP_PORT_BUFFER_LENGTH  1024 * 1024 * 4

//
// Command Routine
//

PBTR_MESSAGE_RECORD
MspBuildRecordRequest(
	IN PMSP_MESSAGE_PORT Port,
	IN PVOID Buffer,
	IN ULONG Length
	);

PBTR_MESSAGE_FILTER
MspBuildFilterRequest(
	IN PMSP_MESSAGE_PORT Port,
	IN PWSTR FilterPath
	);

PBTR_MESSAGE_CACHE
MspBuildCacheRequest(
	IN PMSP_MESSAGE_PORT Port,
	IN PVOID Buffer,
	IN ULONG Length,
	IN HANDLE IndexFileHandle,
	IN HANDLE DataFileHandle,
	IN HANDLE SharedDataHandle,
	IN HANDLE PerfHandle
	);

ULONG
MspOnCommand(
	IN PMSP_TRACE Trace,
	IN PMSP_QUEUE_PACKET Notification
	);

ULONG
MspOnTimerExpire(
	IN PMSP_TRACE Trace,
	IN PMSP_QUEUE_PACKET Notification
	);

ULONG CALLBACK
MspTraceProcedure(
	IN PVOID Context
	);

ULONG CALLBACK
MspTimerProcedure(
	IN PVOID Context
	);

extern LIST_ENTRY MspTraceList;
extern LIST_ENTRY MspDecodeFilterList;
extern HMODULE MspDllHandle;
extern WCHAR MspRuntimePath[];

#ifdef __cplusplus
}
#endif

#endif