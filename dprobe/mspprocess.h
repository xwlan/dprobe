#ifndef _MSP_PROCESS_H_
#define _MSP_PROCESS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mspdtl.h"
#include "mspport.h"
#include "mspprobe.h"
#include "mspqueue.h"

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
	MSP_STATE_REPORT,
} MSP_PROCESS_STATE, *PMSP_PROCESS_STATE;

//
// Forward declaration
//
struct _MSP_QUEUE;
struct _MSP_CACHE;

typedef struct _MSP_PROCESS {

	LIST_ENTRY ListEntry;

	ULONG ProcessId;
	ULONG SessionId;
	MSP_PROCESS_STATE State;

	ULONG ThreadId;
	HANDLE ThreadHandle;
	HANDLE ProcessHandle;
	HANDLE CompleteEvent;

	struct _MSP_PROBE_TABLE *ProbeTable;
	struct _MSP_COUNTER_GROUP *Counter;
	struct _MSP_MESSAGE_PORT *PortObject;
	struct _MSP_QUEUE *QueueObject;
	struct _MSP_DTL_OBJECT *DtlObject;

	LIST_ENTRY FilterList;
	SLIST_HEADER StackList;
	
	FILETIME StartTime;
	FILETIME EndTime;

	WCHAR UserName[MAX_PATH];
	WCHAR BaseName[MAX_PATH];
	WCHAR FullName[MAX_PATH];
	WCHAR Argument[MAX_PATH];

} MSP_PROCESS, *PMSP_PROCESS;

typedef struct _MSP_DTL_PROCESS_ENTRY {
	ULONG ProcessId;
	ULONG ProbeCount;
	ULONG ProbeOffset;
	FILETIME StartTime;
	FILETIME EndTime;
	WCHAR UserName[MAX_PATH];
	WCHAR BaseName[MAX_PATH];
	WCHAR FullName[MAX_PATH];
	WCHAR Argument[MAX_PATH];
} MSP_DTL_PROCESS_ENTRY, *PMSP_DTL_PROCESS_ENTRY;

typedef struct _MSP_DTL_PROCESS {
	ULONG TotalLength;	
	ULONG NumberOfProcess;
	ULONG FilterOffset;
	ULONG NumberOfFilters;
	MSP_DTL_PROCESS_ENTRY Entries[ANYSIZE_ARRAY];
} MSP_DTL_PROCESS, *PMSP_DTL_PROCESS;

typedef struct _MSP_THREAD_CONTEXT {
	HANDLE CompleteEvent;
	PMSP_PROCESS Object;
} MSP_THREAD_CONTEXT, *PMSP_THREAD_CONTEXT;

typedef struct _MSP_FAILURE_ENTRY {
	LIST_ENTRY ListEntry;
	BOOLEAN Activate;
	BOOLEAN DllLevel;
	BOOLEAN Spare[2];
	ULONG Status;
	WCHAR DllName[MAX_PATH];
	WCHAR ApiName[MAX_PATH];
} MSP_FAILURE_ENTRY, *PMSP_FAILURE_ENTRY;

typedef enum _MSP_COMMAND_TYPE {
	CommandProbe,
	CommandStop,
	CommandStartDebug,
} MSP_COMMAND_TYPE, *PMSP_COMMAND_TYPE;

typedef struct _MSP_USER_COMMAND {

	LIST_ENTRY ListEntry;

	MSP_COMMAND_TYPE CommandType;
	ULONG ProcessId;
	ULONG Status;

	MSP_QUEUE_CALLBACK Callback;
	PVOID CallbackContext;

	union {

		struct {
			BTR_PROBE_TYPE Type;
			HANDLE MappingObject;
			PVOID MappedAddress;
			ULONG CommandLength;
			ULONG CommandCount;
			ULONG FailureCount;
			LIST_ENTRY CommandList;
			LIST_ENTRY FailureList;
		};

		struct {
			ULONG Flag;
		};
		
		struct {
			WCHAR ImagePath[MAX_PATH];
			WCHAR Argument[MAX_PATH];
			WCHAR WorkPath[MAX_PATH];
			HANDLE CompleteEvent;
		};
	};

} MSP_USER_COMMAND, *PMSP_USER_COMMAND;

typedef enum _MSP_DECODE_TYPE {
	DECODE_NONE,
	DECODE_PROCESS,
	DECODE_SEQUENCE,
	DECODE_TIMESTAMP,
	DECODE_PID,
	DECODE_TID,
	DECODE_PROBE,
	DECODE_PROVIDER,
	DECODE_RETURN,
	DECODE_DURATION,
	DECODE_DETAIL,
	DECODE_LAST,
} MSP_DECODE_TYPE, *PMSP_DECODE_TYPE;

//
// Process Object Manipulation
//

ULONG
MspCreateProcess(
	IN ULONG ProcessId,
	OUT PMSP_PROCESS *Process
	);

ULONG 
MspReCreateProcess(
	OUT PMSP_PROCESS Object 
	);

PMSP_PROCESS
MspLookupProcess(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG ProcessId
	);

BOOLEAN
MspIsProbing(
	IN ULONG ProcessId
	);

ULONG
MspStartProcess(
	IN PMSP_PROCESS Object 
	);

ULONG 
MspStopProcess(
	IN PMSP_PROCESS Object 
	);

ULONG
MspFilterControl(
	IN PMSP_PROCESS Object,
	IN PWSTR FilterPath,
	IN PBTR_FILTER_CONTROL Control,
	OUT PBTR_FILTER_CONTROL *Ack
	);

ULONG
MspLoadRuntime(
	IN ULONG ProcessId,
	IN HANDLE ProcessHandle
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
MspQueryDecodeFilterByPath(
	IN PWSTR FilterPath
	);

PBTR_FILTER
MspQueryDecodeFilterByGuid(
	IN GUID *FilterGuid 
	);

PBTR_FILTER
MspLoadFilter(
	IN PWSTR FilterPath
	);

VOID
MspUnloadFilter(
	IN PBTR_FILTER Filter
	);

ULONG
MspStopAllProcess(
	VOID
	);

BTR_DECODE_CALLBACK 
MspGetDecodeCallback(
	IN PBTR_FILTER_RECORD Record 
	);

VOID
MspInsertDecodeFilter(
	IN PBTR_FILTER Filter
	);

ULONG
MspDecodeDetailA(
	IN PBTR_FILTER_RECORD Record,
	IN PSTR Buffer,
	IN ULONG Length
	);

ULONG
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

ULONG
MspOnControl(
	IN PMSP_PROCESS Process,
	IN PMSP_QUEUE_PACKET Packet 
	);

ULONG
MspOnStart(
	IN PMSP_PROCESS Process,
	IN PMSP_CONTROL Control
	);

ULONG
MspOnStop(
	IN PMSP_PROCESS Process,
	IN PMSP_CONTROL Control
	);

ULONG
MspOnFilterProbe(
	IN PMSP_PROCESS Process,
	IN PMSP_CONTROL Control
	);

ULONG
MspOnFastProbe(
	IN PMSP_PROCESS Process,
	IN PMSP_CONTROL Control
	);

ULONG
MspOnLargeProbe(
	IN PMSP_PROCESS Process,
	IN PMSP_CONTROL Control
	);

ULONG
MspOnStackTrace(
	IN PMSP_PROCESS Process,
	IN PMSP_QUEUE_PACKET Packet
	);

ULONG
MspOnTimerExpire(
	IN PMSP_PROCESS Process,
	IN PMSP_QUEUE_PACKET Packet 
	);

ULONG
MspOnCommand(
	IN PMSP_QUEUE_PACKET Packet
	);

ULONG
MspOnProbeCommand(
	IN PMSP_QUEUE_PACKET Packet
	);

ULONG
MspOnStopCommand(
	IN PMSP_QUEUE_PACKET Packet
	);

ULONG
MspOnStartDebugCommand(
	IN PMSP_QUEUE_PACKET Packet
	);

VOID
MspFreePreExecuteBlock(
	IN HANDLE ProcessHandle,
	IN HANDLE ThreadHandle,
	IN PUCHAR CodePtr
	);

ULONG
MspCommitLargeRequest(
	IN PMSP_PROCESS Process,
	IN PMSP_USER_COMMAND Command
	);

ULONG
MspCommitStopRequest(
	IN PMSP_PROCESS Process,
	IN PMSP_USER_COMMAND Command
	);

HANDLE
MspStartTimer(
	VOID
	);

ULONG CALLBACK 
MspTimerProcedure(
	IN PVOID Context
	);

ULONG
MspPostTimerPacket(
	VOID
	);

ULONG CALLBACK
MspProcessProcedure(
	IN PVOID Context
	);

ULONG CALLBACK
MspCommandProcedure(
	IN PVOID Context
	);

ULONG
MspQueueUserCommand(
	IN PMSP_USER_COMMAND Command
	);

//
// MSP API
//

BOOLEAN
MspIsIdle(
	VOID
	);

ULONG
MspGetFilterList(
	IN ULONG ProcessId,
	OUT PLIST_ENTRY ListHead
	);

VOID
MspFreeFilterList(
	IN PLIST_ENTRY ListHead
	);

ULONG
MspInitializeFilterList(
	IN PWSTR Path
	);

ULONG
MspInitializeProcessFilterList(
	IN PMSP_PROCESS Process
	);

ULONG
MspCloneProcessFilterList(
	IN PMSP_PROCESS Process,
	OUT PLIST_ENTRY ListHead
	);

BOOLEAN
MspIsProcessActivated(
	IN ULONG ProcessId
	);

BOOLEAN
MspIsFilterActivated(
	IN ULONG ProcesId,
	IN PBTR_FILTER Filter,
	IN ULONG Number
	);

BOOLEAN
MspIsFastProbeActivated(
	IN ULONG ProcessId,
	IN PVOID Address,
	IN PWSTR DllName,
	IN PWSTR ApiName
	);

ULONG
MspDecodeProcessName(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
MspDecodeProbeName(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
MspDecodeFilterName(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
MspQueryProcessName(
	IN PMSP_PROCESS Process,
	IN ULONG ProcessId
	);

ULONG
MspStartStackTraceProcedure(
	IN PMSP_DTL_OBJECT DtlObject
	);

PBTR_FILTER
MspGetFilterObject(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG ProcessId,
	IN GUID *FilterGuid
	);

PBTR_FILTER
MspQueryFilterByGuid(
	IN PMSP_PROCESS Process,
	IN GUID *FilterGuid
	);

PBTR_FILTER
MspQueryFilterByPath(
	IN PMSP_PROCESS Process,
	IN PWSTR FilterPath
	);

PMSP_DTL_OBJECT
MspCurrentDtlObject(
	VOID
	);

ULONG 
MspDecodeReturn(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
MspDecodeDuration(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
MspDecodeProvider(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
MspDecodeProbe(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length,
	IN BOOLEAN HasDepth
	);

ULONG
MspDecodeTimeStamp(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
MspDecodeThreadId(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
MspDecodeProcessId(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
MspDecodeDetailFast(
	IN PBTR_FAST_RECORD Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
MspDecodeDetailFilter(
	IN PBTR_FILTER_RECORD Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
MspDecodeDetail(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
MspDecodeSequence(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
MspDecodeRecord(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record,
	IN MSP_DECODE_TYPE Type,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
MspGetRecordSequence(
	IN PBTR_RECORD_HEADER Record
	);

VOID
MspFreeFailureList(
	IN PMSP_USER_COMMAND Command
	);

ULONG
MspGetActiveProcessCount(
	IN PMSP_DTL_OBJECT DtlObject
	);

VOID
MspLockProcessList(
	VOID
	);

VOID
MspUnlockProcessList(
	VOID
	);

HANDLE
MspCreateCounterGroup(
	IN PMSP_DTL_OBJECT DtlObject,
	IN struct _MSP_PROCESS *Process
	);

VOID
MspStopCounterGroup(
	IN PMSP_PROCESS Process
	);

ULONG
MspInsertCounterEntry(
	IN struct _MSP_PROCESS *Process,
	IN struct _MSP_PROBE *Probe,
	IN ULONG CounterBucket
	);

ULONG
MspRemoveCounterEntry(
	IN struct _MSP_PROCESS *Process,
	IN struct _MSP_PROBE *Probe
	);

extern LIST_ENTRY MspFilterList;
extern WCHAR MspRuntimePath[];
extern HANDLE MspBtrHandle;
extern HANDLE MspTimerThreadHandle;

extern DECLSPEC_CACHEALIGN
SLIST_HEADER MspStackTraceQueue;

#ifdef __cplusplus
}
#endif
#endif