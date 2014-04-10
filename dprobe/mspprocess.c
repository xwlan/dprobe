#include "mspdtl.h"
#include "mspprocess.h"
#include "mspport.h"
#include "msputility.h"
#include "mspprobe.h"
#include "mspldr.h"
#include "btr.h"

#define MSP_PORT_BUFFER_SIZE  1024*64 

//
// Global stack trace queue
//

DECLSPEC_CACHEALIGN
SLIST_HEADER MspStackTraceQueue;

//
// Schedule timer 
//

HANDLE MspTimerThreadHandle;

//
// Schedule queue object
//

PMSP_QUEUE MspCommandQueue;


ULONG CALLBACK
MspProcessProcedure(
	IN PVOID Context
	)
{
	ULONG Status;
	PMSP_PROCESS Process;
	PMSP_MESSAGE_PORT Port;
	PMSP_QUEUE Queue;
	PMSP_QUEUE_PACKET Packet;
	PMSP_THREAD_CONTEXT ThreadContext;
	HANDLE CompleteEvent; 

	ThreadContext = (PMSP_THREAD_CONTEXT)Context;
	Process = (PMSP_PROCESS)ThreadContext->Object;
	CompleteEvent = ThreadContext->CompleteEvent;

	Process->ThreadId = GetCurrentThreadId();

	//
	// Create queue object
	//

	Status = MspCreateQueue(&Queue);
	if (Status != MSP_STATUS_OK) {
		return Status;
	}

	ASSERT(Queue != NULL);
	Process->QueueObject = Queue;

	//
	// Create message port object
	//

	Port = MspCreatePort(Process, MSP_PORT_BUFFER_LENGTH);
	if (!Port) {
		return MSP_STATUS_ERROR;
	}

	Process->PortObject = Port;
	Status = MspConnectPort(Port);

	//
	// Signal event to notify caller the completion
	//

	if (Status != MSP_STATUS_OK) {
		Process->State = MSP_STATE_INIT_FAILED;
		SetEvent(ThreadContext->CompleteEvent);
		MspCloseQueue(Process->QueueObject);
		MspClosePort(Process->PortObject);
		return Status;
	}

	Process->State = MSP_STATE_INIT_SUCCEED;
	SetEvent(ThreadContext->CompleteEvent);

	while (TRUE) {

		Status = MspDeQueuePacket(Process->QueueObject, &Packet);
		if (Status != MSP_STATUS_OK) {
			break;
		}

		if (FlagOn(Packet->PacketFlag, MSP_PACKET_CONTROL)) {
			Status = MspOnControl(Process, Packet);
		}
		
		else if (FlagOn(Packet->PacketFlag, MSP_PACKET_TIMER)) {
			Status = MspOnStackTrace(Process, Packet);

			//
			// Timer packet must be freed
			//

			MspFree(Packet);
		}

		if (Status != MSP_STATUS_OK) {
			break;
		}
	}

	//
	// N.B. This is the case of ipc broken, or application
	// simply terminated
	//

	if (Process->State == MSP_STATE_STARTED) {
		Process->State = MSP_STATE_STOPPED;
	}

	MspRemoveAllProbes(Process);
	MspClosePort(Process->PortObject);
	MspCloseQueue(Process->QueueObject);

	return MSP_STATUS_OK;
}

ULONG
MspOnStackTrace(
	IN PMSP_PROCESS Process,
	IN PMSP_QUEUE_PACKET Packet
	)
{
	ULONG Status;
	ULONG Length;
	PBTR_MESSAGE_STACKTRACE Message;
	PBTR_MESSAGE_STACKTRACE_ACK Ack;
	PBTR_STACKTRACE_ENTRY Trace;
	PMSP_MESSAGE_PORT Port;

	Port = Process->PortObject;

	Message = (PBTR_MESSAGE_STACKTRACE)Port->Buffer;
	Message->Header.PacketType = MESSAGE_STACKTRACE;
	Message->Header.Length = sizeof(BTR_MESSAGE_STACKTRACE);

	Status = MspSendMessage(Port, &Message->Header, &Length);	
	if (Status != MSP_STATUS_OK) {
		return Status;
	}

	Ack = (PBTR_MESSAGE_STACKTRACE_ACK)Port->Buffer;
	Status = MspGetMessage(Port, &Ack->Header, Port->BufferLength, &Length);
	if (Status != MSP_STATUS_OK) {
		return Status;
	}

	if (!Ack->NumberOfTraces) {
		return MSP_STATUS_OK;
	}

	Length = sizeof(BTR_STACKTRACE_ENTRY) * Ack->NumberOfTraces;
	Trace = (PBTR_STACKTRACE_ENTRY)_aligned_malloc(Length, 16);
	RtlZeroMemory(Trace, Length);

	RtlCopyMemory(Trace, &Ack->Entries[0], Length);
	Trace->s1.ProcessId = Port->ProcessId;
	Trace->s1.NumberOfEntries = Ack->NumberOfTraces;

	InterlockedPushEntrySList(&MspStackTraceQueue, &Trace->ListEntry); 
	return MSP_STATUS_OK;	
}

ULONG CALLBACK
MspStackTraceProcedure(
	IN PVOID Context
	)
{
	ULONG Status;
	HANDLE Timer;
	LARGE_INTEGER DueTime;
	PSLIST_ENTRY ListEntry;
	PBTR_STACKTRACE_ENTRY Entry;
	PMSP_STACKTRACE_OBJECT Object;
	ULONG Number;

	DueTime.QuadPart = -10000 * 1000;
	Timer = CreateWaitableTimer(NULL, FALSE, NULL);
	SetWaitableTimer(Timer, &DueTime, 1000, NULL, NULL, FALSE);

	Object = MspGetStackTraceObject(&MspDtlObject);

	while (TRUE) {

		Status = WaitForSingleObject(Timer, INFINITE); 
		if (Status == WAIT_OBJECT_0) {

			while (ListEntry = InterlockedPopEntrySList(&MspStackTraceQueue)) {

				Entry = CONTAINING_RECORD(ListEntry, BTR_STACKTRACE_ENTRY, ListEntry);
				MspAcquireCsLock(&Object->Lock);

				for(Number = 0; Number < Entry->s1.NumberOfEntries; Number += 1) {
					MspInsertStackTrace(Object, Entry + Number, Entry->s1.ProcessId);				
				}

				MspReleaseCsLock(&Object->Lock);

				_aligned_free(Entry);

			}
		}
	}
}

ULONG
MspOnStart(
	IN PMSP_PROCESS Process,
	IN PMSP_CONTROL Control
	)
{
	PBTR_MESSAGE_CACHE Message;
	PBTR_MESSAGE_ACK Ack;
	ULONG CompleteBytes;
	ULONG Status;
	HANDLE ProcessHandle;
	HANDLE IndexFileHandle;
	HANDLE DataFileHandle;
	HANDLE SharedDataHandle;
	HANDLE PerfHandle;
	HANDLE Handle;
	PMSP_FILE_OBJECT IndexObject;
	PMSP_FILE_OBJECT DataObject;
	PMSP_MESSAGE_PORT Port;

	Port = Process->PortObject;
	ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Port->ProcessId);
	if (!ProcessHandle) {
		return MSP_STATUS_ACCESS_DENIED;
	}

	IndexObject = &MspDtlObject.IndexObject;
	DataObject = &MspDtlObject.DataObject;

	IndexFileHandle = MspDuplicateHandle(ProcessHandle, 
		                                 IndexObject->FileObject);
	if (!IndexFileHandle) {
		CloseHandle(ProcessHandle);
		return MSP_STATUS_ACCESS_DENIED;
	}

	//
	// Duplicate data file handle
	//

	DataFileHandle = MspDuplicateHandle(ProcessHandle, 
		                                DataObject->FileObject);
	if (!DataFileHandle) {
		CloseHandle(ProcessHandle);
		return MSP_STATUS_ACCESS_DENIED;
	}

	//
	// Duplicate shared data handle
	//

	SharedDataHandle = MspDuplicateHandle(ProcessHandle, MspSharedDataHandle);
	if (!SharedDataHandle) {
		CloseHandle(ProcessHandle);
		return MSP_STATUS_ACCESS_DENIED;
	}

	//
	// Performance is process level sharing
	//

	Handle = MspCreateCounterGroup(&MspDtlObject, Process);
	if (!Handle) {
		CloseHandle(ProcessHandle);
		return MSP_STATUS_ACCESS_DENIED;
	}

	PerfHandle = MspDuplicateHandle(ProcessHandle, Handle);
	if (!PerfHandle) {
		CloseHandle(ProcessHandle);
		return MSP_STATUS_ACCESS_DENIED;
	}
	CloseHandle(ProcessHandle);

	//
	// Build and set cache information to target process.
	//

	Message = (PBTR_MESSAGE_CACHE)Port->Buffer;
	Message->Header.PacketType = MESSAGE_CACHE;
	Message->Header.Length = sizeof(BTR_MESSAGE_CACHE);
	Message->IndexFileHandle = IndexFileHandle;
	Message->DataFileHandle = DataFileHandle;
	Message->SharedDataHandle = SharedDataHandle;
	Message->PerfHandle = PerfHandle;

	Status = MspSendMessage(Port, &Message->Header, &CompleteBytes);
	if (Status != S_OK) {
		return Status;
	}
		
	Ack = (PBTR_MESSAGE_ACK)Port->Buffer;
	Status = MspGetMessage(Port, &Ack->Header, Port->BufferLength, &CompleteBytes);
	if (Status != S_OK) {
		return Status;
	}

	return MSP_STATUS_OK;
}

ULONG
MspOnStop(
	IN PMSP_PROCESS Process,
	IN PMSP_CONTROL Control
	)
{
	ULONG Status;
	ULONG CompleteBytes;
	PMSP_MESSAGE_PORT Port;
	PBTR_MESSAGE_STOP Packet;
	PBTR_MESSAGE_ACK Ack;

	UNREFERENCED_PARAMETER(Control);

	Port = Process->PortObject;

	Packet = (PBTR_MESSAGE_STOP)Port->Buffer;
	Packet->Header.PacketType = MESSAGE_STOP_REQUEST;
	Packet->Header.Length = sizeof(BTR_MESSAGE_STOP);

	Status = MspSendMessage(Port, &Packet->Header, &CompleteBytes);	
	if (Status != MSP_STATUS_OK) {
		return Status;
	}

	Ack = (PBTR_MESSAGE_ACK)Port->Buffer;
	Status = MspGetMessage(Port, &Ack->Header, Port->BufferLength, &CompleteBytes);
	if (Status != MSP_STATUS_OK) {
		return Status;
	}

	Control->Result = Ack->Status;
	return MSP_STATUS_OK;
}
	
ULONG
MspOnFilterProbe(
	IN PMSP_PROCESS Process,
	IN PMSP_CONTROL Control
	)
{
	ULONG Status;
	ULONG CompleteBytes;
	PMSP_MESSAGE_PORT Port;
	PBTR_MESSAGE_FILTER Request;
	PBTR_MESSAGE_FILTER_ACK Ack;
	PBTR_FILTER Filter;
	ULONG Number;
	PWSTR Path;
	PVOID Buffer;
	PBTR_FILTER Clone;

	Control->Result = MSP_STATUS_OK;
	Path = Control->Filter.Path;

	//
	// Check filter control parameters
	//

	if (!MspIsValidFilterPath(Path)) {
		Control->Result = MSP_STATUS_INVALID_PARAMETER;
		return MSP_STATUS_OK;
	}

	Filter = MspQueryFilterByPath(Process, Path);
	if (!Filter) {

		Filter = MspQueryDecodeFilterByPath(Path);
		if (!Filter) {

			Filter = MspLoadFilter(Path);
			if (!Filter) {
				Control->Result = MSP_STATUS_NO_FILTER;
				return MSP_STATUS_OK;
			}
		}
		
		//
		// Clone a filter object and insert into current process' filter list
		//

		Clone = (PBTR_FILTER)MspMalloc(sizeof(BTR_FILTER));
		RtlCopyMemory(Clone, Filter, sizeof(BTR_FILTER));

		Buffer = MspMalloc(MAX_FILTER_PROBE/sizeof(CHAR));
		RtlZeroMemory(Buffer, MAX_FILTER_PROBE/sizeof(CHAR));
		BtrInitializeBitMap(&Clone->BitMap, Buffer, MAX_FILTER_PROBE);

		InsertHeadList(&Process->FilterList, &Clone->ListEntry);
		Filter = Clone;
	}

	Port = Process->PortObject;
	Request = (PBTR_MESSAGE_FILTER)Port->Buffer;
	StringCchCopy(Request->Path, MAX_PATH, Path);
	Request->Activate = Control->Filter.Activate;
	Request->Count = Control->Filter.Count;
	Request->Status = 0;

	for(Number = 0; Number < Control->Filter.Count; Number += 1) {
		Request->Entry[Number].Number = Control->Filter.Entry[Number].Number;
		Request->Entry[Number].Status = 0;
		Request->Entry[Number].Address = 0;
	}

	Request->Header.PacketType = MESSAGE_FILTER;
	Request->Header.Length = FIELD_OFFSET(BTR_MESSAGE_FILTER, Entry[Request->Count]);

	Status = MspSendMessage(Port, &Request->Header, &CompleteBytes);	
	if (Status != MSP_STATUS_OK) {
		Control->Result = Status;
		return Status;
	}

	Ack = (PBTR_MESSAGE_FILTER_ACK)Port->Buffer;
	Status = MspGetMessage(Port, &Ack->Header, Port->BufferLength, &CompleteBytes);
	if (Status != MSP_STATUS_OK) {
		Control->Result = Status;
		return Status;
	}

	for(Number = 0; Number < Control->Filter.Count; Number += 1) {

		//
		// If operation is completed successfully, clear or set correspondant bit
		// in filter object's bitmap.
		//

		if (Ack->Entry[Number].Status == S_OK) {

			if (Ack->Activate) {
				BtrSetBit(&Filter->BitMap, Ack->Entry[Number].Number);

				//
				// Address only make sense when status is S_OK for activate
				//

				Control->Filter.Entry[Number].Address = Ack->Entry[Number].Address;
			}
			else {
				BtrClearBit(&Filter->BitMap, Ack->Entry[Number].Number);
			}
		}

		Control->Filter.Entry[Number].Status = Ack->Entry[Number].Status;
	}

	Control->Result = Ack->Status;
	return MSP_STATUS_OK;
}

ULONG
MspOnFastProbe(
	IN PMSP_PROCESS Process,
	IN PMSP_CONTROL Control
	)
{
	ULONG Status;
	ULONG CompleteBytes;
	PMSP_MESSAGE_PORT Port;
	PBTR_MESSAGE_FAST Request;
	PBTR_MESSAGE_FAST_ACK Ack;
	ULONG Number;

	Control->Result = MSP_STATUS_OK;

	Port = Process->PortObject;
	Request = (PBTR_MESSAGE_FAST)Port->Buffer;
	Request->Activate = Control->Fast.Activate;
	Request->Count = Control->Fast.Count;

	for(Number = 0; Number < Control->Fast.Count; Number += 1) {
		Request->Entry[Number].Address = Control->Fast.Entry[Number].Address;
		Request->Entry[Number].Status = 0;
	}

	Request->Header.PacketType = MESSAGE_FAST;
	Request->Header.Length = FIELD_OFFSET(BTR_MESSAGE_FAST, Entry[Request->Count]);

	if (Request->Header.Length > Port->BufferLength) {
		__debugbreak();
	}

	Status = MspSendMessage(Port, &Request->Header, &CompleteBytes);	
	if (Status != MSP_STATUS_OK) {
		Control->Result = Status;
		return Status;
	}

	Ack = (PBTR_MESSAGE_FAST_ACK)Port->Buffer;
	Status = MspGetMessage(Port, &Ack->Header, Port->BufferLength, &CompleteBytes);
	if (Status != MSP_STATUS_OK) {
		Control->Result = Status;
		return Status;
	}

	for(Number = 0; Number < Control->Fast.Count; Number += 1) {
		Control->Fast.Entry[Number].Status = Ack->Entry[Number].Status;
	}

	return MSP_STATUS_OK;
}

ULONG
MspOnLargeProbe(
	IN PMSP_PROCESS Process,
	IN PMSP_CONTROL Control
	)
{
	ULONG Status;
	ULONG CompleteBytes;
	PMSP_MESSAGE_PORT Port;
	PBTR_MESSAGE_USER_COMMAND Request;
	PBTR_MESSAGE_USER_COMMAND_ACK Ack;

	Control->Result = MSP_STATUS_OK;

	//
	// N.B. The MappingObject field must already be duplicated
	// into target address space by MspOnCommand.
	//

	Port = Process->PortObject;
	Request = (PBTR_MESSAGE_USER_COMMAND)Port->Buffer;
	Request->Header.PacketType = MESSAGE_USER_COMMAND;
	Request->Header.Length = sizeof(BTR_MESSAGE_USER_COMMAND);
	Request->MappingObject = Control->Large.MappingObject;
	Request->TotalLength = Control->Large.Length;
	Request->NumberOfCommands = Control->Large.NumberOfCommands;

	Status = MspSendMessage(Port, &Request->Header, &CompleteBytes);	
	if (Status != MSP_STATUS_OK) {
		Control->Result = Status;
		return Status;
	}

	Ack = (PBTR_MESSAGE_USER_COMMAND_ACK)Port->Buffer;
	Status = MspGetMessage(Port, &Ack->Header, Port->BufferLength, &CompleteBytes);
	if (Status != MSP_STATUS_OK) {
		Control->Result = Status;
		return Status;
	}

	return MSP_STATUS_OK;
}

ULONG
MspOnControl(
	IN PMSP_PROCESS Process,
	IN PMSP_QUEUE_PACKET Packet
	)
{
	ULONG Status;
	PMSP_CONTROL Control;
	
	Status = MSP_STATUS_OK;
	Control = (PMSP_CONTROL)Packet->Packet;

	switch (Control->Code) {

		case MSP_START_TRACE:

			if (Process->State != MSP_STATE_STARTED) {
				Status = MspOnStart(Process, Control);
				if (Status == MSP_STATUS_OK) {
					Process->State = MSP_STATE_STARTED;
				}

			} else {
				Status = MSP_STATUS_OK;
			}

			break;

		case MSP_STOP_TRACE:

			if (Process->State != MSP_STATE_STARTED) {
				Status = MSP_STATUS_OK;
			} else {
				MspOnStop(Process, Control);
				Process->State = MSP_STATE_STOPPED;
				Status = MSP_STATUS_STOPPED;
			}
			break;

		case MSP_FILTER_PROBE:

			if (Process->State != MSP_STATE_STARTED) {
				Status = MSP_STATUS_OK;
			} else {
				Status = MspOnFilterProbe(Process, Control);
			}
			break;

		case MSP_FAST_PROBE:

			if (Process->State != MSP_STATE_STARTED) {
				Status = MSP_STATUS_OK;
			} else {
				Status = MspOnFastProbe(Process, Control);
			}
			break;

		case MSP_LARGE_PROBE:
			if (Process->State != MSP_STATE_STARTED) {
				Status = MSP_STATUS_OK;
			} else {
				Status = MspOnLargeProbe(Process, Control);
			}
			break;

		default:
			break;
	}

	Control->Result = Status;

	ASSERT(Packet->CompleteEvent != NULL);
	if (Packet->CompleteEvent != NULL) {
		SetEvent(Packet->CompleteEvent);
	}

	return Status;
}

PMSP_PROCESS
MspLookupProcess(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG ProcessId
	)
{
	PLIST_ENTRY ListEntry;
	PMSP_PROCESS Process;

	MspAcquireCsLock(&DtlObject->ProcessListLock);
	ListEntry = DtlObject->ProcessListHead.Flink;

	while (ListEntry != &DtlObject->ProcessListHead) {
		Process = CONTAINING_RECORD(ListEntry, MSP_PROCESS, ListEntry);
		if (Process->ProcessId == ProcessId) {
			MspReleaseCsLock(&DtlObject->ProcessListLock);
			return Process;
		}
		ListEntry = ListEntry->Flink;
	}

	MspReleaseCsLock(&DtlObject->ProcessListLock);
	return NULL;
}

BOOLEAN
MspIsProbing(
	IN ULONG ProcessId
	)
{
	PMSP_PROCESS Process;

	Process = MspLookupProcess(&MspDtlObject, ProcessId);
	if (!Process || Process->State != MSP_STATE_STARTED) {
		return FALSE;
	}

	return TRUE;
}

ULONG 
MspCreateProcess(
	IN ULONG ProcessId,
	OUT PMSP_PROCESS *Process
	)
{
	ULONG Status;
	PMSP_PROCESS Object;
	ULONG ThreadId;
	HANDLE ThreadHandle;
	HANDLE CompleteEvent;
	MSP_THREAD_CONTEXT Context;
	HANDLE ProcessHandle;

	Object = MspLookupProcess(&MspDtlObject, ProcessId);
	if (Object != NULL) {
		*Process = Object;
		return MSP_STATUS_OK;
	}

	if (!MspIsLegitimateProcess(ProcessId)) {
		*Process = NULL;
		return MSP_STATUS_WOW64_ERROR;
	}

	ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
	if (!ProcessHandle) {

		Status = GetLastError();
		if (Status == ERROR_ACCESS_DENIED) {
			MspWriteLogEntry(ProcessId, LogLevelError, "access denied to open pid %u", ProcessId);
			return MSP_STATUS_ACCESS_DENIED;
		}

		return MSP_STATUS_INVALID_PARAMETER;
	}

	Status = MspLoadRuntime(ProcessId, ProcessHandle);
	if (Status != MSP_STATUS_OK) {
		MspWriteLogEntry(ProcessId, LogLevelError, "failed to load runtime into pid %u, error=0x%08x", 
			             ProcessId, Status);
		return Status;
	} else {
		MspWriteLogEntry(ProcessId, LogLevelInfo, "succeeded to load runtime into pid %u", ProcessId);
	}

	Object = (PMSP_PROCESS)MspMalloc(sizeof(MSP_PROCESS));
	Object->ProcessId = ProcessId;

	CompleteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	Context.CompleteEvent = CompleteEvent;
	Context.Object = Object;

	//
	// Create work thread for target process
	//

	ThreadHandle = CreateThread(NULL, 0, MspProcessProcedure, (PVOID)&Context, 0, &ThreadId);

	//
	// Wait thread finish its initialization
	//

	WaitForSingleObject(CompleteEvent, INFINITE);

	if (Object->State != MSP_STATE_INIT_SUCCEED) {

		MspUnloadRuntime(ProcessId);
		TerminateThread(ThreadHandle, 0);
		CloseHandle(ThreadHandle);
		CloseHandle(ProcessHandle);

		MspFree(Object);
		MspWriteLogEntry(ProcessId, LogLevelError, "failed to initialize session for pid %u", ProcessId);
		return MSP_STATUS_INIT_FAILED;
	}

	Object->CompleteEvent = CompleteEvent;
	Object->ThreadHandle = ThreadHandle;
	Object->ProcessHandle = ProcessHandle;

	MspCreateProbeTable(Object, MSP_PROBE_TABLE_SIZE);

	//
	// Clone a copy of global filter list
	//

	MspInitializeProcessFilterList(Object);

	//
	// Insert process object into dtl object's process list
	//

	InsertTailList(&MspDtlObject.ProcessListHead, &Object->ListEntry);

	//
	// N.B. ProcessCount never get decreased, it's a counter to indicate
	// both active and inactive processes.
	//

	InterlockedIncrement(&MspDtlObject.ProcessCount);
	
	MspQueryProcessName(Object, ProcessId);
	MspWriteLogEntry(Object->ProcessId, LogLevelInfo, "session is created for path='%S', status=0x%x", 
					 Object->FullName, Status);

	*Process = Object;
	return MSP_STATUS_OK;
}

ULONG 
MspReCreateProcess(
	OUT PMSP_PROCESS Object 
	)
{
	ULONG Status;
	HANDLE ThreadHandle;
	ULONG ThreadId;
	MSP_THREAD_CONTEXT Context;

	Status = MspLoadRuntime(Object->ProcessId, Object->ProcessHandle);
	if (Status != MSP_STATUS_OK) {
		return Status;
	}

	ResetEvent(Object->CompleteEvent);
	Context.CompleteEvent = Object->CompleteEvent;
	Context.Object = Object;

	//
	// Create work thread for target process
	//

	ThreadHandle = CreateThread(NULL, 0, MspProcessProcedure, (PVOID)&Context, 0, &ThreadId);

	WaitForSingleObject(Object->CompleteEvent, INFINITE);

	if (Object->State != MSP_STATE_INIT_SUCCEED) {
		MspUnloadRuntime(Object->ProcessId);
		TerminateThread(ThreadHandle, 0);
		CloseHandle(ThreadHandle);
		return MSP_STATUS_INIT_FAILED;
	}

	Object->ThreadHandle = ThreadHandle;
	MspWriteLogEntry(Object->ProcessId, LogLevelInfo, "session is created for path='%S', status=0x%x", 
					 Object->FullName, Status);

	return MSP_STATUS_OK;
}

ULONG 
MspStartProcess(
	IN PMSP_PROCESS Object
	)
{
	ULONG Status;
	PMSP_CONTROL Control;
	PMSP_QUEUE_PACKET Packet;
	HANDLE CompleteEvent;

	if (Object->State == MSP_STATE_STARTED) {
		return MSP_STATUS_OK;
	}

	ASSERT(Object->ProcessHandle != NULL);

	CompleteEvent = Object->CompleteEvent;
	ResetEvent(CompleteEvent);

	Control = (PMSP_CONTROL)MspMalloc(sizeof(MSP_CONTROL));
	Control->Code = MSP_START_TRACE;

	Packet = (PMSP_QUEUE_PACKET)MspMalloc(sizeof(MSP_QUEUE_PACKET));
	Packet->PacketFlag = MSP_PACKET_CONTROL | MSP_PACKET_WAIT;
	Packet->Packet = (PVOID)Control;
	Packet->CompleteEvent = CompleteEvent;

	//
	// Queue notification to worker thread and wait stop
	// control to be completed
	//

	MspQueuePacket(Object->QueueObject, Packet);

	Status = WaitForSingleObject(CompleteEvent, INFINITE);
	if (Status != WAIT_OBJECT_0) {
		Status = GetLastError();
	} else {
		Status = MSP_STATUS_OK;
	}
	Status = Control->Result;

	MspFree(Control);
	MspFree(Packet);
	
	if (Status == MSP_STATUS_OK) {
		InterlockedIncrement(&MspDtlObject.ActiveProcessCount);
	}

	MspWriteLogEntry(Object->ProcessId, LogLevelInfo, "MspStartProcess(), status=0x%x", Status);
	return Status;
}

ULONG 
MspStopProcess(
	PMSP_PROCESS Object
	)
{
	ULONG Status;
	PMSP_CONTROL Control;
	PMSP_QUEUE_PACKET Packet;
	HANDLE CompleteEvent;

	CompleteEvent = Object->CompleteEvent;
	ResetEvent(CompleteEvent);

	Control = (PMSP_CONTROL)MspMalloc(sizeof(MSP_CONTROL));
	Control->Code = MSP_STOP_TRACE;

	Packet = (PMSP_QUEUE_PACKET)MspMalloc(sizeof(MSP_QUEUE_PACKET));
	Packet->PacketFlag = MSP_PACKET_CONTROL | MSP_PACKET_WAIT;
	Packet->Packet = (PVOID)Control;
	Packet->CompleteEvent = CompleteEvent;

	//
	// Queue notification to worker thread and wait stop
	// control to be completed
	//

	MspQueuePacket(Object->QueueObject, Packet);
	WaitForSingleObject(CompleteEvent, INFINITE);
	CloseHandle(CompleteEvent);

	//
	// Wait trace procedure clean queue, port objects and exit
	//

	Status = WaitForSingleObject(Object->ThreadHandle, INFINITE);
	if (Status != WAIT_OBJECT_0) {
		TerminateThread(Object->ThreadHandle, 0); 
	}
	CloseHandle(Object->ThreadHandle);

	Status = Control->Result;
	MspFree(Control);
	MspFree(Packet);

	//
	// Mark object's state as stopped
	//

	Object->State = MSP_STATE_STOPPED;
	InterlockedDecrement(&MspDtlObject.ActiveProcessCount);

	MspWriteLogEntry(Object->ProcessId, LogLevelInfo, "MspStopProcess(), status=0x%x", Status);
	return Status;
}

ULONG
MspStopAllProcess(
	VOID
	)
{
	ULONG Status;
	PMSP_CONTROL Control;
	PMSP_QUEUE_PACKET Packet;
	HANDLE CompleteEvent;
	PMSP_PROCESS Object;
	PLIST_ENTRY ListEntry;

	//
	// N.B. First of all, must delete timer queue to ensure
	// it's not access dtl object's process list
	//

	while (IsListEmpty(&MspDtlObject.ProcessListHead) != TRUE) {

		ListEntry = RemoveHeadList(&MspDtlObject.ProcessListHead);			
		Object = CONTAINING_RECORD(ListEntry, MSP_PROCESS, ListEntry);

		if (Object->State != MSP_STATE_STARTED) {
			continue;
		}

		CompleteEvent = Object->CompleteEvent;
		ResetEvent(CompleteEvent);

		Control = (PMSP_CONTROL)MspMalloc(sizeof(MSP_CONTROL));
		Control->Code = MSP_STOP_TRACE;

		Packet = (PMSP_QUEUE_PACKET)MspMalloc(sizeof(MSP_QUEUE_PACKET));
		Packet->PacketFlag = MSP_PACKET_CONTROL | MSP_PACKET_WAIT;
		Packet->Packet = (PVOID)Control;
		Packet->CompleteEvent = CompleteEvent;

		//
		// Queue notification to worker thread and wait stop
		// control to be completed
		//

		MspQueuePacket(Object->QueueObject, Packet);
		
		Status = WaitForSingleObject(CompleteEvent, 1000);
		if (Status == WAIT_TIMEOUT || Status == WAIT_FAILED) {
			TerminateThread(Object->ThreadHandle, 0);
			Status = MSP_STATUS_TIMEOUT;
		} else {
			Status = MSP_STATUS_OK;
		}

		CloseHandle(CompleteEvent);
		CloseHandle(Object->ThreadHandle);

		MspFree(Control);
		MspFree(Packet);

		MspWriteLogEntry(0, LogLevelInfo, "MspStopProcess(), status=0x%x", Status);
	}

	return MSP_STATUS_OK;
}

ULONG 
MspFilterControl(
	IN PMSP_PROCESS Object,
	IN PWSTR FilterName,
	IN PBTR_FILTER_CONTROL Control,
	OUT PBTR_FILTER_CONTROL *Ack
	)
{
	return MSP_STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
MspIsLegitimateOrdinal(
	IN PBTR_FILTER Filter,
	IN ULONG Ordinal
	)
{
	if (Ordinal < Filter->ProbesCount) {
		return TRUE;
	}

	return FALSE;
}

PBTR_FILTER
MspQueryFilterByPath(
	IN PMSP_PROCESS Process,
	IN PWSTR FilterPath
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_FILTER Filter;

	ASSERT(Process != NULL);
	
	if (!Process) {
		return NULL;
	}

	ListEntry = Process->FilterList.Flink;
	while (ListEntry != &Process->FilterList) {
		Filter = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);
		if (!_wcsicmp(Filter->FilterFullPath, FilterPath)) {
			return Filter;
		}
		ListEntry = ListEntry->Flink;
	}
	return NULL;
}

PBTR_FILTER
MspQueryDecodeFilterByPath(
	IN PWSTR FilterPath
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_FILTER Filter;

	ListEntry = MspFilterList.Flink;
	while (ListEntry != &MspFilterList) {
		Filter = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);
		if (!_wcsicmp(Filter->FilterFullPath, FilterPath)) {
			return Filter;
		}
		ListEntry = ListEntry->Flink;
	}
	return NULL;
}

PBTR_FILTER
MspQueryDecodeFilterByGuid(
	IN GUID *FilterGuid
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_FILTER Filter;

	ListEntry = MspFilterList.Flink;
	while (ListEntry != &MspFilterList) {
		Filter = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);
		if (IsEqualGUID(&Filter->FilterGuid, FilterGuid)) {
			return Filter;
		}
		ListEntry = ListEntry->Flink;
	}
	return NULL;
}

PBTR_FILTER 
MspLoadFilter(
	IN PWSTR FilterPath
	)
{
	PBTR_FILTER Filter;
	HMODULE DllHandle;
	BTR_FLT_GETOBJECT Routine;

	__try {
		DllHandle = LoadLibrary(FilterPath);
		if (!DllHandle) {
			MspWriteLogEntry(0, LogLevelError, "LoadLibrary('%S'), failed, status=%u", 
			                 FilterPath, GetLastError());
			return NULL;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

		//
		// record the exception in log if any
		//

		MspWriteLogEntry(0, LogLevelError, "LoadLibrary('%S'), exception occurred!", 
			             FilterPath);
		return NULL;
	}

	Routine = (BTR_FLT_GETOBJECT)GetProcAddress(DllHandle, "BtrFltGetObject");
	if (!Routine) {

		FreeLibrary(DllHandle);
		MspWriteLogEntry(0, LogLevelError, "GetProcAddress(BtrFltGetObject) failed for '%S'", 
			             FilterPath);
		return NULL;
	}

	__try {
		Filter = (*Routine)(FILTER_MODE_DECODE);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		
		//
		// ignore
		//

		Filter = NULL;
		MspWriteLogEntry(0, LogLevelError, "BtrFltGetObject() failed for '%S'", FilterPath);
	}

	if (Filter != NULL) {

		GetModuleFileName(DllHandle, Filter->FilterFullPath, MAX_PATH);

		//
		// N.B. Everytime a filter is locally loaded, we insert a clone into
		// decode filter list, this help to decode records without even start
		// a trace session.
		//

		Filter->BitMap.Buffer = NULL;
		Filter->BitMap.SizeOfBitMap = 0;
		Filter->BaseOfDll = 0;
		Filter->SizeOfImage = 0;

		MspInsertDecodeFilter(Filter);
	}

	return Filter;
}

VOID
MspUnloadFilter(
	IN PBTR_FILTER Filter
	)
{
	HMODULE DllHandle;

	ASSERT(Filter != NULL);

	DllHandle = GetModuleHandle(Filter->FilterFullPath);
	if (DllHandle) {
		FreeLibrary(DllHandle);
	}
}

BTR_DECODE_CALLBACK 
MspGetDecodeCallback(
	IN PBTR_FILTER_RECORD Record
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_FILTER Filter;
	BTR_DECODE_CALLBACK Callback;

	Callback = NULL;
	ListEntry = MspFilterList.Flink;

	//
	// N.B. In fact it's hard to judge whether a record is valid,
	// we must use exception handler to protect memcmp().
	//

	__try {
	
		while (ListEntry != &MspFilterList) {
			Filter = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);	
			if (memcmp(&Filter->FilterGuid, &Record->FilterGuid, sizeof(UUID)) == 0) {
				Callback = Filter->Probes[Record->ProbeOrdinal].DecodeCallback;
				return Callback;
			}
			ListEntry = ListEntry->Flink;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		Callback = NULL;
	}

	return Callback;
}

VOID
MspInsertDecodeFilter(
	IN PBTR_FILTER Filter
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_FILTER Object;
	
	ListEntry = MspFilterList.Flink;

	while(ListEntry != &MspFilterList) {
		Object = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);
		if (_wcsicmp(Object->FilterFullPath, Filter->FilterFullPath) == 0) {
			return;
		}
		ListEntry = ListEntry->Flink;
	}

	//
	// Clone a filter object and insert into global filter list,
	// this list is typically used for decoding purpose. note that
	// we does not allocate bitmap object for this filter object,
	// we can not check probe bitmap, it's valid only in trace object's
	// filter object instance.
	//

	Object = (PBTR_FILTER)MspMalloc(sizeof(BTR_FILTER));
	RtlCopyMemory(Object, Filter, sizeof(BTR_FILTER));
	InsertHeadList(&MspFilterList, &Object->ListEntry);
}

ULONG 
MspDecodeProbeW(
	IN PBTR_FILTER_RECORD Record,
	IN PWSTR NameBuffer,
	IN ULONG NameLength,
	IN PWSTR ModuleBuffer,
	IN ULONG ModuleLength
	)
{
	PBTR_FILTER Filter;

	if (!Record) {
		return MSP_STATUS_INVALID_PARAMETER;
	}	

	Filter = MspLookupDecodeFilter(Record);
	if (!Filter) {
		return MSP_STATUS_NO_FILTER;
	}

	__try {
		StringCchCopy(NameBuffer, NameLength, Filter->Probes[Record->ProbeOrdinal].ApiName);	
		StringCchCopy(ModuleBuffer, ModuleLength, Filter->Probes[Record->ProbeOrdinal].DllName);	
	}
	__except(1) {
		return MSP_STATUS_ERROR;
	}

	return MSP_STATUS_OK;
}

ULONG 
MspDecodeProbeA(
	IN PBTR_FILTER_RECORD Record,
	IN PSTR NameBuffer,
	IN ULONG NameLength,
	IN PSTR ModuleBuffer,
	IN ULONG ModuleLength
	)
{
	PBTR_FILTER Filter;
	PCHAR Buffer;

	if (!Record) {
		return MSP_STATUS_INVALID_PARAMETER;
	}	

	Filter = MspLookupDecodeFilter(Record);
	if (!Filter) {
		return MSP_STATUS_NO_FILTER;
	}

	__try {

		MspConvertUnicodeToAnsi(Filter->Probes[Record->ProbeOrdinal].ApiName, &Buffer);
		StringCchCopyA(NameBuffer, NameLength, Buffer);
		MspFree(Buffer);

		MspConvertUnicodeToAnsi(Filter->Probes[Record->ProbeOrdinal].DllName, &Buffer);
		StringCchCopyA(ModuleBuffer, ModuleLength, Buffer);
		MspFree(Buffer);
	}
	__except(1) {
		return MSP_STATUS_ERROR;
	}

	return MSP_STATUS_OK;
}

ULONG 
MspDecodeDetailA(
	IN PBTR_FILTER_RECORD Record,
	IN PSTR Buffer,
	IN ULONG Length
	)
{
	ULONG Status;
	PWSTR Unicode;
	PSTR Ansi;

	Unicode = (PWSTR)MspMalloc(Length * sizeof(WCHAR));

	Status = MspDecodeDetailW(Record, Unicode, Length);
	if (Status == MSP_STATUS_OK) {

		MspConvertUnicodeToAnsi(Unicode, &Ansi);
		if (Ansi != NULL) {
			StringCchCopyA(Buffer, Length, Ansi);
			MspFree(Ansi);
		}
	}

	MspFree(Unicode);
	return Status;
}

ULONG 
MspDecodeDetailW(
	IN PBTR_FILTER_RECORD Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	BTR_DECODE_CALLBACK Callback;
	
	//
	// Simple validation of record before lookup its decode routine
	//

	if (Record->Base.RecordType != RECORD_FILTER || 
		Record->Base.TotalLength < sizeof(BTR_FILTER_RECORD)) {
		return MSP_STATUS_BAD_RECORD;
	}

	Callback = MspGetDecodeCallback(Record);
	if (!Callback) {
		return MSP_STATUS_NO_DECODE;
	}
	
	__try {
		(*Callback)(Record, Length, Buffer);
	}
	
	__except(EXCEPTION_EXECUTE_HANDLER) {
		
		//
		// Ingore this exception
		//

		MspWriteLogEntry(0, LogLevelError, "decode exception");
	}

	return MSP_STATUS_OK;
}

PBTR_FILTER
MspLookupDecodeFilter(
	IN PBTR_FILTER_RECORD Record
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_FILTER Filter = NULL;

	ListEntry = MspFilterList.Flink;

	//
	// N.B. In fact it's hard to judge whether a record is valid,
	// we must use exception handler to protect memcmp().
	//

	__try {
		while (ListEntry != &MspFilterList) {
			Filter = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);	
			if (memcmp(&Filter->FilterGuid, &Record->FilterGuid, sizeof(UUID)) == 0) {
				return Filter;
			}
			ListEntry = ListEntry->Flink;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
	
		Filter = NULL;
	}

	return Filter;
}

SIZE_T 
MspConvertUnicodeToAnsi(
	IN PWSTR Source, 
	OUT PCHAR *Destine
	)
{
	int Length;
	int RequiredLength;
	PCHAR Buffer;

	Length = (int)wcslen(Source) + 1;
	RequiredLength = WideCharToMultiByte(CP_UTF8, 0, Source, Length, 0, 0, 0, 0);

	Buffer = (PCHAR)MspMalloc(RequiredLength);
	Buffer[0] = 0;

	WideCharToMultiByte(CP_UTF8, 0, Source, Length, Buffer, RequiredLength, 0, 0);
	*Destine = Buffer;

	return Length;
}

VOID 
MspConvertAnsiToUnicode(
	IN PCHAR Source,
	OUT PWCHAR *Destine
	)
{
	int Length;
	PWCHAR Buffer;

	if (!Source) {
		*Destine = NULL;
		return;
	}

	Length = (int)strlen(Source) + 1;
	Buffer = (PWCHAR)MspMalloc(Length * sizeof(WCHAR));
	Buffer[0] = L'0';

	MultiByteToWideChar(CP_UTF8, 0, Source, Length, Buffer, Length);
	*Destine = Buffer;
}

VOID
MspUnloadFilters(
	VOID
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_FILTER Filter;
	HMODULE DllHandle;

	__try {

		while(IsListEmpty(&MspFilterList) != TRUE) {
		
			ListEntry = RemoveHeadList(&MspFilterList);
			Filter = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);

			while (DllHandle = GetModuleHandle(Filter->FilterFullPath)) {
				FreeLibrary(DllHandle);
			}
			
			MspFree(Filter);
		}
	}
	__except(1) {
	}

	return;
}

HANDLE
MspStartTimer(
	VOID
	)
{
	MspTimerThreadHandle = CreateThread(NULL, 0, MspTimerProcedure, NULL, 0, NULL);
	return MspTimerThreadHandle;
}

ULONG CALLBACK 
MspTimerProcedure(
	IN PVOID Context
	)
{
	HANDLE Timer;
	HANDLE Wait[2];
	LARGE_INTEGER DueTime;
	ULONG Status;
	BOOLEAN FlushLog;

	DueTime.QuadPart = -10000 * 1000;
	Timer = CreateWaitableTimer(NULL, FALSE, NULL);
	SetWaitableTimer(Timer, &DueTime, 1000, NULL, NULL, FALSE);

	FlushLog = MspIsLoggingEnabled();

	Wait[0] = MspStopEvent;
	Wait[1] = Timer;

	while (TRUE) {

		Status = WaitForMultipleObjects(2, Wait, FALSE, INFINITE); 
		if (Status == WAIT_OBJECT_0) {
			break;			
		}

		if (Status == WAIT_OBJECT_0 + 1) {
			
			//
			// Post timer packet to active session
			//

			MspPostTimerPacket();

			//
			// Flush logging if any
			//

			if (FlushLog) {
				MspLogProcedure(NULL);
			}
		}

	}

	CancelWaitableTimer(Timer);
	CloseHandle(Timer);
	return 0;
}

ULONG
MspPostTimerPacket(
	VOID
	)
{
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PMSP_PROCESS Process;
	PMSP_QUEUE_PACKET Packet;

	MspAcquireCsLock(&MspDtlObject.ProcessListLock);

	ListHead = &MspDtlObject.ProcessListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {

		Process = CONTAINING_RECORD(ListEntry, MSP_PROCESS, ListEntry);

		//
		// N.B. Queue timer packet only to active running process
		//

		if (Process->State == MSP_STATE_STARTED) {

			ASSERT(Process->QueueObject != NULL);

			Packet = MspMalloc(sizeof(MSP_QUEUE_PACKET));
			Packet->PacketFlag = MSP_PACKET_TIMER | MSP_PACKET_FREE;
			Packet->Packet = NULL;
			Packet->CompleteEvent = NULL;

			MspQueuePacket(Process->QueueObject, Packet);
		}

		ListEntry = ListEntry->Flink;
	}
	
	MspReleaseCsLock(&MspDtlObject.ProcessListLock);
	return 0;
}

ULONG
MspCallCommandCallback(
	IN PMSP_USER_COMMAND Command
	)
{
	ULONG Status;

	__try {
		(*Command->Callback)(Command, Command->CallbackContext);
		Status = MSP_STATUS_OK;
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
		Status = MSP_STATUS_EXCEPTION;
	}

	return Status;
}

ULONG
MspBuildLargeRequest(
	IN PMSP_PROCESS Process,
	IN PMSP_USER_COMMAND Command,
	OUT PMSP_CONTROL *Control
	)
{
	ULONG Number;
	PLIST_ENTRY ListEntry;
	PBTR_USER_COMMAND From;
	PBTR_USER_COMMAND To;
	ULONG Length;
	HANDLE Handle;
	HANDLE Duplicate;
	PMSP_CONTROL Large;

	Length = MspUlongRoundUp(Command->CommandLength, 4096);
	Handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
							   0, Length, NULL ); 
	if (!Handle) {
		return MSP_STATUS_OUTOFMEMORY;
	}

	To = (PBTR_USER_COMMAND)MapViewOfFile(Handle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
	if (!To) {
		CloseHandle(Handle);
		return MSP_STATUS_OUTOFMEMORY;
	}

	Duplicate = MspDuplicateHandle(Process->ProcessHandle, Handle);
	if (!Duplicate) {
		return MSP_STATUS_ACCESS_DENIED;
	}
		                       
	Command->MappingObject = Handle;
	Command->MappedAddress = To;

	Number = 0;
	ListEntry = Command->CommandList.Flink;
	while (ListEntry != &Command->CommandList) {
		From = CONTAINING_RECORD(ListEntry, BTR_USER_COMMAND, ListEntry);
		RtlCopyMemory(To, From, From->Length);
		To = (PBTR_USER_COMMAND)((PUCHAR)To + From->Length);
		ListEntry = ListEntry->Flink;

		Number += 1;
	}

	ASSERT(Number == Command->CommandCount);

	Large = (PMSP_CONTROL)MspMalloc(sizeof(MSP_CONTROL));
	RtlZeroMemory(Large, sizeof(*Large));
	Large->Code = MSP_LARGE_PROBE;
	Large->Large.MappingObject = Duplicate;
	Large->Large.NumberOfCommands = Command->CommandCount;
	Large->Large.Length = Length;

	*Control = Large;
	return MSP_STATUS_OK;
}

ULONG
MspCommitLargeRequest(
	IN PMSP_PROCESS Process,
	IN PMSP_USER_COMMAND Command
	)
{
	ULONG Number;
	ULONG i;
	PBTR_USER_COMMAND BtrCommand;
	PBTR_USER_PROBE BtrProbe;
	PMSP_FAILURE_ENTRY Failure;
	PBTR_FILTER Filter;
	ULONG Ordinal;
	WCHAR Buffer[MAX_PATH];
	PWCHAR Unicode;

	BtrCommand = (PBTR_USER_COMMAND)Command->MappedAddress;

	for (Number = 0; Number < Command->CommandCount; Number += 1) {

		//
		// Scan dll level failure
		//

		if (BtrCommand->Status != S_OK) {

			Failure = (PMSP_FAILURE_ENTRY)MspMalloc(sizeof(MSP_FAILURE_ENTRY));
			RtlZeroMemory(Failure, sizeof(*Failure));

			Failure->DllLevel = TRUE;
			Failure->Status = BtrCommand->Status;

			if (Command->Type == PROBE_FAST) {
				StringCchCopyW(Failure->DllName, MAX_PATH, BtrCommand->DllName);
			}

			//
			// Filter use its filter display name, not dll path
			//

			if (Command->Type == PROBE_FILTER) {
				Filter = MspQueryFilterByPath(Process, BtrCommand->DllName);
				ASSERT(Filter != NULL);
				StringCchCopyW(Failure->DllName, MAX_PATH, Filter->FilterName);
			}

			InsertTailList(&Command->FailureList, &Failure->ListEntry);
			Command->FailureCount += 1;

			BtrCommand = (PBTR_USER_COMMAND)((PUCHAR)BtrCommand + BtrCommand->Length);
			continue;
		}

		if (Command->Type == PROBE_FILTER) {
			Filter = MspQueryFilterByPath(Process, BtrCommand->DllName);
		}

		for(i = 0; i < BtrCommand->Count; i++) {

			BtrProbe = &BtrCommand->Probe[i];

			if (BtrProbe->Status != S_OK) {

				Failure = (PMSP_FAILURE_ENTRY)MspMalloc(sizeof(MSP_FAILURE_ENTRY));
				Failure->Activate = BtrProbe->Activate;
				Failure->Status = BtrProbe->Status;

				if (Command->Type == PROBE_FAST) {
					StringCchCopy(Failure->DllName, MAX_PATH, BtrCommand->DllName);
					MspConvertAnsiToUnicode(BtrProbe->ApiName, &Unicode);
					StringCchCopy(Failure->ApiName, MAX_PATH, Unicode);
					MspFree(Unicode);
				}

				if (Command->Type == PROBE_FILTER) {

					StringCchCopy(Failure->DllName, MAX_PATH, Filter->FilterName);
					Ordinal = BtrProbe->Number;

					StringCchPrintf(Buffer, MAX_PATH, L"%s!%s", Filter->Probes[Ordinal].DllName,
						            Filter->Probes[Ordinal].ApiName );
					StringCchCopy(Failure->ApiName, MAX_PATH, Buffer );
				}
				
				InsertTailList(&Command->FailureList, &Failure->ListEntry);
				Command->FailureCount += 1;

				continue;
			}	

			//
			// Insert or remove probe according its activation state
			//

			if (Command->Type == PROBE_FAST) {

				MspConvertAnsiToUnicode(BtrProbe->ApiName, &Unicode);

				if (BtrProbe->Activate) {
					MspInsertProbe(Process, PROBE_FAST,
								   ReturnUIntPtr, BtrProbe->Address, 
								   BtrProbe->ObjectId, BtrProbe->Counter,
						           BtrCommand->DllName, Unicode, NULL, 0);
				}
				else {
					MspRemoveProbe(Process, PROBE_FAST,
						           BtrProbe->Address, BtrCommand->DllName, Unicode, NULL, 0, NULL );
				}

				MspFree(Unicode);
			}
			
			if (Command->Type == PROBE_FILTER) {

				Ordinal = BtrProbe->Number;

				if (BtrProbe->Activate) {
					MspInsertProbe(Process, PROBE_FILTER,
								   Filter->Probes[Ordinal].ReturnType, BtrProbe->Address,
								   BtrProbe->ObjectId, BtrProbe->Counter,
						           NULL, NULL, Filter, Ordinal);
				}
				else {
					MspRemoveProbe(Process, PROBE_FILTER,
								   BtrProbe->Address, NULL, NULL, Filter, Ordinal, NULL);
				}
			}
		}

		//
		// Move to next command
		//

		BtrCommand = (PBTR_USER_COMMAND)((PUCHAR)BtrCommand + BtrCommand->Length);
	}

	UnmapViewOfFile(Command->MappedAddress);
	Command->MappedAddress = NULL;

	CloseHandle(Command->MappingObject);
	Command->MappingObject = NULL;

	return MSP_STATUS_OK;
}

ULONG
MspCommitStopRequest(
	IN PMSP_PROCESS Process,
	IN PMSP_USER_COMMAND Command
	)
{
	UNREFERENCED_PARAMETER(Command);

	//MspRemoveAllProbes(Process);
	return MSP_STATUS_OK;
}

ULONG
MspOnCommand(
	IN PMSP_QUEUE_PACKET Packet
	)
{
	PMSP_USER_COMMAND Command;
	
	Command = (PMSP_USER_COMMAND)Packet->Packet;
	
	switch (Command->CommandType) {
	
		case CommandProbe:
			return MspOnProbeCommand(Packet);
	
		case CommandStop:
			return MspOnStopCommand(Packet);

		case CommandStartDebug:
			return MspOnStartDebugCommand(Packet);
	}
	
	return MSP_STATUS_OK;
}

ULONG
MspOnStartDebugCommand(
	IN PMSP_QUEUE_PACKET Packet
	)
{
	PMSP_PROCESS Object;
	PMSP_USER_COMMAND Command;
	ULONG Status;
	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi = {0};
	HANDLE CompleteEvent;
	HANDLE SuccessEvent;
	HANDLE ErrorEvent;
	HANDLE Handle[2];
	WCHAR Buffer[MAX_PATH * 2];
	PWCHAR WorkPath;
	PUCHAR CodePtr;

	Command = (PMSP_USER_COMMAND)Packet->Packet;
	ASSERT(Command->CommandType == CommandStartDebug);

	si.cb = sizeof(si);

	if (wcslen(Command->Argument) != 0) {
		StringCchPrintf(Buffer, MAX_PATH * 2, L"%s %s", 
			            Command->ImagePath, Command->Argument);
	} else {
		StringCchPrintf(Buffer, MAX_PATH, L"%s", Command->ImagePath);
	}

	if (!wcslen(Command->WorkPath)) {
		WorkPath = NULL;	
	} else {
		WorkPath = Command->WorkPath;	
	}

	__try {

		CompleteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		SuccessEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		ErrorEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		Handle[0] = SuccessEvent;
		Handle[1] = ErrorEvent;

		Status = CreateProcess(NULL, Buffer, NULL, NULL, FALSE, 
							   DEBUG_ONLY_THIS_PROCESS | CREATE_SUSPENDED, 
							   NULL, WorkPath, &si, &pi);

		if (Status != TRUE) {
			Status = GetLastError();
			__leave;
		}

		MspInjectPreExecute(pi.dwProcessId, pi.hProcess, pi.hThread, Command->ImagePath, 
							CompleteEvent, SuccessEvent, ErrorEvent, &CodePtr);

		//
		// Wait until btr finish its stage0 initialization
		//

		Status = WaitForMultipleObjects(2, Handle, FALSE, INFINITE);
		if (Status == (WAIT_OBJECT_0 + 1)) {
			TerminateProcess(pi.hProcess, 0);
			__leave;
		}

		Object = NULL;
		Status = MspCreateProcess(pi.dwProcessId, &Object);
		if (Status != MSP_STATUS_OK) {
			TerminateProcess(pi.hProcess, 0);
			__leave;
		}

		Status = MspStartProcess(Object);
		if (Status != MSP_STATUS_OK) {
			TerminateProcess(pi.hProcess, 0);
			__leave;
		}


	}
	__finally {
		
		CloseHandle(Handle[0]);
		CloseHandle(Handle[1]);

		//if (pi.hProcess != NULL) {
		//	MspFreePreExecuteBlock(pi.hProcess, pi.hThread, CodePtr);
		//}

		if (pi.hProcess) {
			CloseHandle(pi.hProcess);
		}

		if (pi.hThread) {
			CloseHandle(pi.hThread);
		}
	}

	//
	// N.B. We must fill a CompleteEvent to command object,
	// caller will use this event to release execution of
	// target's entry point.
	//

	Command->Status = Status;

	if (Command->Status == MSP_STATUS_OK) {
		Command->ProcessId = pi.dwProcessId;
		Command->CompleteEvent = CompleteEvent;
	} else {
		CloseHandle(CompleteEvent);
	}

	MspCallCommandCallback(Command);

	MspFree(Packet);
	return MSP_STATUS_OK;
}

VOID
MspFreePreExecuteBlock(
	IN HANDLE ProcessHandle,
	IN HANDLE ThreadHandle,
	IN PUCHAR CodePtr
	)
{
	DWORD Count;
	CONTEXT Context;
	ULONG Status;
	ULONG_PTR Pc;

	while (TRUE) {
	
		Count = SuspendThread(ThreadHandle);
		if (Count == -1) {
			return;
		}

		Status = GetThreadContext(ThreadHandle, &Context);
		if (Status != TRUE) {
			ResumeThread(ThreadHandle);
			return;
		}

#if defined(_M_IX86)
		Pc = (ULONG_PTR)Context.Eip;
#elif defined(_M_X64)
		Pc = (ULONG_PTR)Context.Rip;
#endif
		
		//
		// If the thread is still executing pre-exection block, just let it run 
		//

		if (Pc >= (ULONG_PTR)CodePtr && Pc < (ULONG_PTR)CodePtr + BspPageSize) {

			ResumeThread(ThreadHandle);
			SwitchToThread();

		} else {
		
			//
			// It's safe to deallocate the pre-execution block
			//
		
			VirtualFreeEx(ProcessHandle, CodePtr, 0, MEM_RELEASE);
			ResumeThread(ThreadHandle);
			return;
		}
	}
}

ULONG
MspOnStopCommand(
	IN PMSP_QUEUE_PACKET Packet
	)
{
	PMSP_PROCESS Object;
	PMSP_CONTROL Control;
	PMSP_USER_COMMAND Command;

	Command = (PMSP_USER_COMMAND)Packet->Packet;
	ASSERT(Command->CommandType == CommandStop);
	
	Object = MspLookupProcess(&MspDtlObject, Command->ProcessId);
	if (!Object || Object->State != MSP_STATE_STARTED) {
		Command->Status = MSP_STATUS_OK;
		MspCallCommandCallback(Command);
		return MSP_STATUS_OK;
	}

	Control = (PMSP_CONTROL)MspMalloc(sizeof(MSP_CONTROL));
	RtlZeroMemory(Control, sizeof(MSP_CONTROL));

	Control->Code = MSP_STOP_TRACE;
	Control->Context = Command;

	Packet->Packet = Control;
	Packet->PacketFlag = MSP_PACKET_CONTROL | MSP_PACKET_WAIT;

	//
	// N.B. Ensure the packet is not used by work thread 
	//

	ClearFlag(Packet->PacketFlag, MSP_PACKET_FREE);

	ResetEvent(Object->CompleteEvent);
	Packet->CompleteEvent = Object->CompleteEvent;

	//
	// Queue packet to worker thread and wait its completion
	//

	MspQueuePacket(Object->QueueObject, Packet);
	WaitForSingleObject(Packet->CompleteEvent, INFINITE);
	
	//
	// Commit stop request to remove all probes
	//

	MspCommitStopRequest(Object, Command);

	Command->Status = Control->Result;
	MspCallCommandCallback(Command);

	MspFree(Control);
	MspFree(Packet);

	return MSP_STATUS_OK;
}

ULONG
MspOnProbeCommand(
	IN PMSP_QUEUE_PACKET Packet
	)
{
	ULONG Status;
	PMSP_PROCESS Object;
	PMSP_CONTROL Control;
	PMSP_USER_COMMAND Command;
	
	Command = (PMSP_USER_COMMAND)Packet->Packet;
	ASSERT(Command->CommandType == CommandProbe);

	//
	// N.B. If process object is not available, launch 
	// worker thread for target process and get its process object
	//

	Object = MspLookupProcess(&MspDtlObject, Packet->ProcessId);
	if (!Object) {

		//
		// Create process and connect to target process 
		//

		Status = MspCreateProcess(Packet->ProcessId, &Object);
		if (Status != MSP_STATUS_OK) {

			Command->Status = Status;
			MspCallCommandCallback(Command);

			if (FlagOn(Packet->PacketFlag, MSP_PACKET_FREE)) {
				MspFree(Packet);
			}

			return MSP_STATUS_OK;
		}

	}

	if (Object->State != MSP_STATE_STARTED) {

		if (Object->State == MSP_STATE_STOPPED) {

			//
			// N.B. This is the case a process was stopped and
			// re-probe again
			//

			Status = MspReCreateProcess(Object);
			if (Status != MSP_STATUS_OK) {

				Command->Status = Status;
				MspCallCommandCallback(Command);

				if (FlagOn(Packet->PacketFlag, MSP_PACKET_FREE)) {
					MspFree(Packet);
				}
				return MSP_STATUS_OK;
			}

		}

		Status = MspStartProcess(Object);
		if (Status != MSP_STATUS_OK) {

			Command->Status = Status;
			MspCallCommandCallback(Command);
			
			if (FlagOn(Packet->PacketFlag, MSP_PACKET_FREE)) {
				MspFree(Packet);
			}

			return MSP_STATUS_OK;
		}

	}

	//
	// Build large probe request
	//

	Status = MspBuildLargeRequest(Object, Command, &Control);
	if (Status != MSP_STATUS_OK) {

		Command->Status = Status;

		if (FlagOn(Packet->PacketFlag, MSP_PACKET_FREE)) {
			MspFree(Packet);
		}

		return MSP_STATUS_OK;
	}

	//
	// Change the packet type to be MSP_PACKET_CONTROL,
	// attach command object pointer to control's context
	//

	Packet->PacketFlag = MSP_PACKET_CONTROL | MSP_PACKET_WAIT;
	Packet->Packet = (PVOID)Control;
	Control->Context = Command;

	//
	// N.B. Ensure the packet is not used by work thread 
	//

	ClearFlag(Packet->PacketFlag, MSP_PACKET_FREE);

	ResetEvent(Object->CompleteEvent);
	Packet->CompleteEvent = Object->CompleteEvent;

	//
	// Queue packet to worker thread and wait its completion
	//

	MspQueuePacket(Object->QueueObject, Packet);
	WaitForSingleObject(Packet->CompleteEvent, INFINITE);
	
	Command->Status = Control->Result;
	MspCommitLargeRequest(Object, Command);

	//
	// Execute command callback to notify user the status
	// note that command object belongs to user, we never
	// free it! Typically command callback will free it.
	//

	MspCallCommandCallback(Command);

	//
	// Start index procedure of live dtl object
	//

	if (!MspDtlObject.IndexerStarted) {
		Status = MspStartIndexProcedure(&MspDtlObject.IndexObject);
		if (Status == MSP_STATUS_OK) {
			MspDtlObject.IndexerStarted = TRUE;
		}
	}

	if (!MspDtlObject.StackTraceStarted) {
		MspStartStackTraceProcedure(&MspDtlObject);
	}

	MspFree(Control);
	MspFree(Packet);
	
	return MSP_STATUS_OK;
}

VOID
MspFreeFailureList(
	IN PMSP_USER_COMMAND Command
	)
{
	PLIST_ENTRY ListEntry;
	PMSP_FAILURE_ENTRY Failure;

	while (IsListEmpty(&Command->FailureList) != TRUE) {
		ListEntry = RemoveHeadList(&Command->FailureList);
		Failure = CONTAINING_RECORD(ListEntry, MSP_FAILURE_ENTRY, ListEntry);
		MspFree(Failure);
	}
}

//
// N.B. MspQueueCommand is executed asynchronous,
// so command object's callback must be valid,
// when the command is completed, user get notification
// via callback function.
//

ULONG
MspQueueUserCommand(
	IN PMSP_USER_COMMAND Command
	)
{
	PMSP_QUEUE_PACKET Packet;

	ASSERT(Command->Callback != NULL);
	ASSERT(Command->CallbackContext != NULL);

	if (!Command->Callback || !Command->CallbackContext) {
		return MSP_STATUS_INVALID_PARAMETER;
	}

	//
	// Queue the packet into MspCommandQueue asynchronously
	//

	Packet = (PMSP_QUEUE_PACKET)MspMalloc(sizeof(MSP_QUEUE_PACKET));
	RtlZeroMemory(Packet, sizeof(MSP_QUEUE_PACKET));

	Packet->PacketFlag = MPS_PACKET_COMMAND | MSP_PACKET_WAIT | MSP_PACKET_FREE;
	Packet->ProcessId = Command->ProcessId;
	Packet->Callback = Command->Callback;
	Packet->CallbackContext = Command->CallbackContext;
	Packet->Packet = Command;

	MspQueuePacket(MspCommandQueue, Packet);
	return MSP_STATUS_OK;
}

ULONG CALLBACK
MspCommandProcedure(
	IN PVOID Context
	)
{
	ULONG Status;
	PMSP_QUEUE_PACKET Packet;

	MspCreateQueue(&MspCommandQueue);

	while (TRUE) {

		Status = MspDeQueuePacket(MspCommandQueue, &Packet);

		if (Status == MSP_STATUS_OK) {
			MspOnCommand(Packet);
		}

		if (Status != MSP_STATUS_OK) {
			break;
		}
	}

	return MSP_STATUS_OK;

}

BOOLEAN
MspIsIdle(
	VOID
	)
{
	ULONG Current;

	Current = InterlockedCompareExchange(&MspDtlObject.ActiveProcessCount, 0, 0);
	return Current ? FALSE : TRUE;
}

ULONG
MspGetFilterList(
	IN ULONG ProcessId,
	OUT PLIST_ENTRY ListHead
	)
{
	ULONG Status;
	PMSP_PROCESS Process;

	Process = MspLookupProcess(&MspDtlObject, ProcessId);
	if (!Process) {
	
		//
		// N.B. If process object is not yet existing, 
		// we must create it to make a clone of filter list,
		// this has a little bit side effect, MspCreateProcess
		// inject runtime into target address space also, so
		// we should consider implement MspCreateProcess as
		// only do basic initialization work.
		//

		Status = MspCreateProcess(ProcessId, &Process);	
		if (Status != MSP_STATUS_OK) {
			return Status;
		}
	}

	MspCloneProcessFilterList(Process, ListHead);
	return MSP_STATUS_OK;
}

VOID
MspFreeFilterList(
	IN PLIST_ENTRY ListHead
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_FILTER Filter;

	while (IsListEmpty(ListHead) != TRUE) {
		ListEntry = RemoveHeadList(ListHead);
		Filter = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);
		if (Filter->BitMap.Buffer) {
			MspFree(Filter->BitMap.Buffer);
		}
		MspFree(Filter);
	}
}

ULONG
MspInitializeFilterList(
	IN PWSTR Path
	)
{
	ULONG Status;
	WIN32_FIND_DATA Data;
	HANDLE FindHandle;
	WCHAR Buffer[MAX_PATH];
	WCHAR FilterPath[MAX_PATH];
	
	Status = FALSE;
	InitializeListHead(&MspFilterList);

	StringCchPrintf(Buffer, MAX_PATH, Path);
	StringCchCat(Buffer, MAX_PATH, L"*.flt");

	FindHandle = FindFirstFile(Buffer, &Data);
	if (FindHandle != INVALID_HANDLE_VALUE) {
		Status = TRUE;
	}

	while (Status) {
		
		StringCchPrintf(FilterPath, MAX_PATH, Path);
		StringCchCat(FilterPath, MAX_PATH, Data.cFileName);

		MspLoadFilter(FilterPath);
		Status = (ULONG)FindNextFile(FindHandle, &Data);
	}

	return MSP_STATUS_OK;

}
	
ULONG
MspInitializeProcessFilterList(
	IN PMSP_PROCESS Process
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_FILTER Object;
	PBTR_FILTER Clone;
	PVOID Buffer;
	ULONG Size;
	
	InitializeListHead(&Process->FilterList);
	ListEntry = MspFilterList.Flink;

	while(ListEntry != &MspFilterList) {

		Object = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);
		Clone = (PBTR_FILTER)MspMalloc(sizeof(BTR_FILTER));

		RtlCopyMemory(Clone, Object, sizeof(BTR_FILTER));

		//
		// Initialize filter bitamp, at least allocate 16 bytes even if
		// it has only a few probes.
		//

		Size = (Object->ProbesCount + 32) / 32;
		Size = min(4, Size);

		Buffer = MspMalloc(Size * 4);
		RtlZeroMemory(Buffer, Size * 4);
		BtrInitializeBitMap(&Clone->BitMap, Buffer, Object->ProbesCount);

		InsertTailList(&Process->FilterList, &Clone->ListEntry);
		ListEntry = ListEntry->Flink;
	}

	return MSP_STATUS_OK;	
}

ULONG
MspCloneProcessFilterList(
	IN PMSP_PROCESS Process,
	OUT PLIST_ENTRY ListHead
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_FILTER Object;
	PBTR_FILTER Clone;
	PVOID Buffer;
	ULONG Size;
	
	InitializeListHead(ListHead);
	ListEntry = Process->FilterList.Flink;

	while(ListEntry != &Process->FilterList) {

		Object = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);
		Clone = (PBTR_FILTER)MspMalloc(sizeof(BTR_FILTER));

		RtlCopyMemory(Clone, Object, sizeof(BTR_FILTER));

		//
		// Allocate bitmap buffer for clone object
		//

		Size = (Object->ProbesCount + 8) / 8;
		Buffer = MspMalloc(Size);

		//
		// Copy source bitmap's filter bits
		//

		RtlCopyMemory(Buffer, Object->BitMap.Buffer, Size);
		BtrInitializeBitMap(&Clone->BitMap, Buffer, Object->ProbesCount);

		InsertTailList(ListHead, &Clone->ListEntry);
		ListEntry = ListEntry->Flink;
	}

	return MSP_STATUS_OK;	
}

BOOLEAN
MspIsProcessActivated(
	IN ULONG ProcessId
	)
{
	PMSP_PROCESS Process;

	Process = MspLookupProcess(&MspDtlObject, ProcessId);
	return Process ? TRUE : FALSE;
}

BOOLEAN
MspIsFastProbeActivated(
	IN ULONG ProcessId,
	IN PVOID Address,
	IN PWSTR DllName,
	IN PWSTR ApiName
	)
{
	PMSP_PROBE Probe;
	PMSP_PROCESS Process;

	//
	// N.B. If process object does not exist, we postpone
	// its creation till user queue a probe command to
	// command queue.
	//

	Process = MspLookupProcess(&MspDtlObject, ProcessId);
	if (!Process) {
		return FALSE;
	}

	Probe = MspLookupProbe(Process, PROBE_FAST,
		                   Address, DllName, ApiName, NULL );

	if (!Probe || Probe->State != ActivatedProbe) {
		return FALSE;
	}

	return TRUE;
}

BOOLEAN
MspIsFilterActivated(
	IN ULONG ProcessId,
	IN PBTR_FILTER Filter,
	IN ULONG Number
	)
{
	PMSP_PROCESS Process;

	//
	// N.B. If process object does not exist, we postpone
	// its creation till user queue a probe command to
	// command queue.
	//

	Process = MspLookupProcess(&MspDtlObject, ProcessId);
	if (!Process) {
		return FALSE;
	}

	return BtrTestBit(&Filter->BitMap, Number);
}

ULONG
MspDecodeProcessName(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	PMSP_PROCESS Process;

	Process = MspLookupProcess(DtlObject, Record->ProcessId);
	if (Process != NULL) {
	
		StringCchCopy(Buffer, Length, Process->BaseName);
	}
	else {
	
		StringCchCopy(Buffer, Length, L"N/A");
	}

	return MSP_STATUS_OK;
}

#define MSP_UNDNAME_MASK (UNDNAME_NO_ACCESS_SPECIFIERS|UNDNAME_NO_ALLOCATION_LANGUAGE|\
						  UNDNAME_NO_ALLOCATION_MODEL|UNDNAME_NO_ARGUMENTS|UNDNAME_NO_FUNCTION_RETURNS|\
						  UNDNAME_NO_RETURN_UDT_MODEL|UNDNAME_NO_THISTYPE)

ULONG
MspDecodeProbeName(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	PMSP_PROBE Fast;
	PBTR_FILTER_PROBE Filter;
	WCHAR Undecorate[MAX_PATH];

	if (Record->RecordType == RECORD_FAST) {

		Fast = MspLookupFastProbe(DtlObject, Record);

		if (Fast != NULL) {

			if (!wcschr(Fast->ApiName, L'?')) {
				StringCchCopy(Buffer, Length, Fast->ApiName);
			}
			else {
				StringCchCopy(Undecorate, Length, Fast->ApiName);
				BspUnDecorateSymbolName(Undecorate, Buffer, Length, MSP_UNDNAME_MASK);
			}
		}
		else {
			StringCchCopy(Buffer, Length, L"N/A");
		}

		return MSP_STATUS_OK;

	}

	if (Record->RecordType == RECORD_FILTER) {

		Filter = MspLookupFilterProbe(DtlObject, Record);

		if (Filter != NULL) {
			StringCchCopy(Buffer, Length, Filter->ApiName);
		}
		else {
			StringCchCopy(Buffer, Length, L"N/A");
		}

		return MSP_STATUS_OK;
	}

	else {
		StringCchCopy(Buffer, Length, L"N/A");
	}

	return MSP_STATUS_OK;
}

ULONG
MspDecodeFilterName(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	PBTR_FILTER Filter;

	Filter = MspLookupFilter(DtlObject, Record->ProcessId, 
		                     &((PBTR_FILTER_RECORD)Record)->FilterGuid);
	if (Filter != NULL) {
		StringCchCopy(Buffer, Length, Filter->FilterName);
	}
	else {
		StringCchCopy(Buffer, Length, L"N/A");
	}

	return MSP_STATUS_OK;
}

ULONG
MspStartStackTraceProcedure(
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	HANDLE Handle;
	Handle = CreateThread(NULL, 0, MspStackTraceProcedure, NULL, 0, NULL);
	if (Handle) {
		CloseHandle(Handle);
		DtlObject->StackTraceStarted = TRUE;
		return MSP_STATUS_OK;
	}

	return MSP_STATUS_ERROR;
}

PBTR_FILTER
MspGetFilterObject(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG ProcessId,
	IN GUID *FilterGuid
	)
{
	PMSP_PROCESS Process;

	Process = MspLookupProcess(DtlObject, ProcessId);
	if (Process) {
		return MspQueryFilterByGuid(Process, FilterGuid);
	}

	return NULL;
}

PBTR_FILTER
MspQueryFilterByGuid(
	IN PMSP_PROCESS Process,
	IN GUID *FilterGuid
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_FILTER Filter;

	if (!Process) {
		return NULL;
	}

	ListEntry = Process->FilterList.Flink;
	while (ListEntry != &Process->FilterList) {
		Filter = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);
		if (IsEqualGUID(&Filter->FilterGuid, FilterGuid)) {
			return Filter;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

ULONG
MspDecodeReturn(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	switch (Record->ReturnType) {

		case ReturnVoid:
			StringCchCopy(Buffer, Length, L"N/A");
			break;

		case ReturnInt8:
		case ReturnUInt8:
			StringCchPrintf(Buffer, Length, L"0x%02x", (UCHAR)Record->Return);
			break;

		case ReturnUInt16:
		case ReturnInt16:
			StringCchPrintf(Buffer, Length, L"0x%04x", (USHORT)Record->Return);
			break;

		case ReturnUInt32:
		case ReturnInt32:

#if defined(_M_IX86)
		case ReturnPtr:
		case ReturnIntPtr:
		case ReturnUIntPtr:
#endif
			StringCchPrintf(Buffer, Length, L"0x%08x", (ULONG)Record->Return);
			break;

		case ReturnUInt64:
		case ReturnInt64:

#if defined(_M_X64)
		case ReturnPtr:
		case ReturnIntPtr:
		case ReturnUIntPtr:
#endif
			StringCchPrintf(Buffer, Length, L"0x%I64x", (ULONG64)Record->Return);
			break;

			//
			// N.B. x86 and x64 deal with floating point data differently.
			// for x86 floating point data is extended automatically by FPU,
			// so it's in fact 8 bytes for both float/double in memory or FPU registers.
			// for x64, CPU treat float as 4 bytes, double as 8 bytes. To correctly decode
			// float point data, we need explicitly dereference appropriate data size.
			//

#if defined(_M_IX86)

		case ReturnFloat:
		case ReturnDouble:
			StringCchPrintf(Buffer, Length, L"%f", *(double *)&Record->Return);
			break;

#elif defined(_M_X64)

		case ReturnFloat:
			StringCchPrintf(Buffer, Length, L"%f", *(float *)&Record->Return);
			break;

		case ReturnDouble:
			StringCchPrintf(Buffer, Length, L"%f", *(double *)&Record->Return);
			break;
#endif

		default:

			//
			// N.B. If there's no ReturnType specified, we just return its pointer sized value.
			//

#if defined(_M_IX86)
			StringCchPrintf(Buffer, Length, L"0x%x", (ULONG_PTR)Record->Return);
#elif defined(_M_X64)
			StringCchPrintf(Buffer, Length, L"0x%I64x", (ULONG_PTR)Record->Return);
#endif
			break;

	}

	return MSP_STATUS_OK;
}

ULONG
MspDecodeDuration(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	double Duration;

	Duration = BspComputeMilliseconds(Record->Duration);
	StringCchPrintf(Buffer, Length, L"%f", Duration);
	return MSP_STATUS_OK;
}

ULONG
MspDecodeProvider(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	if (Record->RecordType == RECORD_FAST) {
		StringCchCopy(Buffer, Length, L"Fast Probe");
	} else {
		MspDecodeFilterName(DtlObject, Record, Buffer, Length);
	}

	return MSP_STATUS_OK;
}
 
ULONG
MspDecodeProbe(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length,
	IN BOOLEAN HasDepth
	)

{
	ULONG i;
	ULONG Depth;
	ULONG Indent = 0;

	Buffer[0] = 0;
	Depth = (ULONG)Record->CallDepth;

	if (!HasDepth) {
		return MspDecodeProbeName(DtlObject, Record, Buffer, Length);
	}

	for(i = 0; i < Depth; i++) {
		wcscat(Buffer, L"  ");
	}

	if (Depth > 1) {
		wcscat(Buffer, L"-> ");
		Indent = (ULONG)wcslen(Buffer);
	}

	MspDecodeProbeName(DtlObject, Record, Buffer + Indent, Length - Indent);
	return MSP_STATUS_OK;
}

ULONG
MspDecodeTimeStamp(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	SYSTEMTIME Time;

	FileTimeToSystemTime(&Record->FileTime, &Time);
	StringCchPrintf(Buffer, Length, L"%02d:%02d:%02d:%03d", 
					Time.wHour, Time.wMinute,Time.wSecond, Time.wMilliseconds);

	return MSP_STATUS_OK;
}

ULONG
MspDecodeThreadId(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	StringCchPrintf(Buffer, Length, L"%d", Record->ThreadId);
	return MSP_STATUS_OK;
}

ULONG
MspDecodeProcessId(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	StringCchPrintf(Buffer, Length, L"%d", Record->ProcessId);
	return MSP_STATUS_OK;
}

ULONG
MspDecodeSequence(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	StringCchPrintf(Buffer, Length, L"%d", Record->Sequence);
	return MSP_STATUS_OK;
}

ULONG
MspGetRecordSequence(
	IN PBTR_RECORD_HEADER Record
	)
{
	return Record->Sequence;
}

ULONG
MspDecodeDetailFast(
	IN PBTR_FAST_RECORD Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
#if defined (_M_IX86)

	StringCchPrintf(Buffer, Length, L"reg: ecx(0x%08x) edx(0x%08x) stack:(0x%08x 0x%08x 0x%08x 0x%08x)",
					Record->Argument.Ecx, Record->Argument.Edx, Record->Argument.Stack[0],
					Record->Argument.Stack[1], Record->Argument.Stack[2], Record->Argument.Stack[3]);
	
#elif defined (_M_X64)
	StringCchPrintf(Buffer, Length, L"reg: rcx(0x%I64x) rdx(0x%I64x) r8(0x%I64x) r9(0x%I64x) xmm0:(%.3f) xmm1:(%.3f)"
					L"stack: 0x%I64x 0x%I64x",
					Record->Argument.Rcx, Record->Argument.Rdx, 
					Record->Argument.R8, Record->Argument.R9,
					Record->Argument.Xmm0.m128d_f64[0], 
					Record->Argument.Xmm1.m128d_f64[0]);
#endif

	return S_OK;
}

ULONG
MspDecodeDetailFilter(
	IN PBTR_FILTER_RECORD Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	BTR_DECODE_CALLBACK Callback;
	PBTR_FILTER Filter;

	Filter = MspQueryDecodeFilterByGuid(&Record->FilterGuid);

	if (Filter != NULL) {

		Callback = Filter->Probes[Record->ProbeOrdinal].DecodeCallback;

		if (Callback != NULL) {

			__try {
				(*Callback)(Record, Length, Buffer);
			}
			__except(EXCEPTION_EXECUTE_HANDLER) {
				StringCchCopy(Buffer, Length, L"Decode Failure");
			}
		}

		else {
			StringCchCopy(Buffer, Length, L"No Decode Callback");
		}
	} 
	
	else {
		StringCchCopy(Buffer, Length, L"No Matching Filter");
	}

	return MSP_STATUS_OK;
}

ULONG
MspDecodeDetail(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	if (Record->RecordType == RECORD_FAST) {
		return MspDecodeDetailFast((PBTR_FAST_RECORD)Record, Buffer, Length);
	}
	
	if (Record->RecordType == RECORD_FILTER) {
		return MspDecodeDetailFilter((PBTR_FILTER_RECORD)Record, Buffer, Length);
	}

	StringCchCopy(Buffer, Length, L"Unknown Record Type");
	return MSP_STATUS_OK;
}

ULONG
MspDecodeRecord(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record,
	IN MSP_DECODE_TYPE Type,
	IN PWCHAR Buffer,
	IN ULONG Length
	)
{
	switch (Type) {
	
		case DECODE_PROCESS:
			MspDecodeProcessName(DtlObject, Record, Buffer, Length);
			break;
		case DECODE_SEQUENCE:
			MspDecodeSequence(Record, Buffer, Length);
			break;

		case DECODE_TIMESTAMP:
			MspDecodeTimeStamp(Record, Buffer, Length);
			break;

		case DECODE_PID:
			MspDecodeProcessId(Record, Buffer, Length);
			break;

		case DECODE_TID:
			MspDecodeThreadId(Record, Buffer, Length);
			break;

		case DECODE_PROBE:
			MspDecodeProbe(DtlObject, Record, Buffer, Length, TRUE);
			break;

		case DECODE_PROVIDER:
			MspDecodeProvider(DtlObject, Record, Buffer, Length);
			break;

		case DECODE_RETURN:
			MspDecodeReturn(Record, Buffer, Length);
			break;

		case DECODE_DURATION:
			MspDecodeDuration(Record, Buffer, Length);
			break;

		case DECODE_DETAIL:
			MspDecodeDetail(Record, Buffer, Length);
			break;

		default:
			ASSERT(0);
	}

	return 0;
}

ULONG
MspGetActiveProcessCount(
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	PLIST_ENTRY ListEntry;
	PMSP_PROCESS Process;
	ULONG Count;

	Count = 0;

	MspAcquireCsLock(&DtlObject->ProcessListLock);
	ListEntry = DtlObject->ProcessListHead.Flink;

	while (ListEntry != &DtlObject->ProcessListHead) {
		Process = CONTAINING_RECORD(ListEntry, MSP_PROCESS, ListEntry);
		if (Process->State == MSP_STATE_STARTED) {
			Count += 1;
		}
		ListEntry = ListEntry->Flink;
	}

	MspReleaseCsLock(&DtlObject->ProcessListLock);
	
	return Count;
}

VOID
MspLockProcessList(
	VOID
	)
{
	MspAcquireCsLock(&MspDtlObject.ProcessListLock);
}

VOID
MspUnlockProcessList(
	VOID
	)
{
	MspReleaseCsLock(&MspDtlObject.ProcessListLock);
}

PMSP_STACKTRACE_OBJECT
MspGetStackTraceObject(
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	return &DtlObject->StackTrace;
}

HANDLE
MspCreateCounterGroup(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PMSP_PROCESS Process
	)
{
	ULONG Size;
	HANDLE Handle;
	PBTR_COUNTER_TABLE Table;
	PMSP_COUNTER_GROUP Group;
	PMSP_COUNTER_OBJECT Counter;
	
	//
	// Round up to allocation granularity 64KB
	//

	Size = MspUlongRoundUp(sizeof(BTR_COUNTER_TABLE), 1024 * 64);
	Handle = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, Size, NULL);
	if (!Handle) {
		return NULL;
	}

	Table = (PBTR_COUNTER_TABLE)MapViewOfFile(Handle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
	if (!Table) {
		CloseHandle(Handle);
		return NULL;
	}

	MspInitSpinLock(&Table->Lock, 100);
	Table->NumberOfBuckets = PROBE_BUCKET_NUMBER;
	Table->ProcessId = Process->ProcessId;

	Group = (PMSP_COUNTER_GROUP)MspMalloc(sizeof(MSP_COUNTER_GROUP));
	RtlZeroMemory(Group, sizeof(MSP_COUNTER_GROUP));

	Group->Level = CounterGroupLevel;
	Group->Table = Table;
	Group->Handle = Handle;
	StringCchCopy(Group->Name, MAX_PATH, Process->BaseName);
	InitializeListHead(&Group->ActiveListHead);
	InitializeListHead(&Group->RetireListHead);

	Counter = &DtlObject->CounterObject;
	Group->Object = Counter;

	Group->Process = Process;
	Process->Counter = Group;

	return Handle;
}

ULONG
MspInsertCounterEntry(
	IN struct _MSP_PROCESS *Process,
	IN struct _MSP_PROBE *Probe,
	IN ULONG CounterBucket
	)
{
	PMSP_COUNTER_GROUP Counter;
	PMSP_COUNTER_ENTRY Entry;
	PBTR_COUNTER_TABLE Table;

	Counter = Process->Counter;
	Table = Counter->Table;

	Entry = (PMSP_COUNTER_ENTRY)MspMalloc(sizeof(MSP_COUNTER_ENTRY));
	RtlZeroMemory(Entry, sizeof(MSP_COUNTER_ENTRY));
	
	//
	// Directly attach the mapped pointer into entry object
	//

	Probe->Counter = Entry;
	Entry->Level = CounterEntryLevel;
	Entry->Active = &Table->Entries[CounterBucket];
	Entry->Group = Process->Counter;
	Entry->Retire.Address = Probe->Address;

	StringCchCopy(Entry->Active->Name, MAX_PATH, Probe->DllName);
	StringCchCat(Entry->Active->Name, MAX_PATH, L"!");
	StringCchCat(Entry->Active->Name, MAX_PATH, Probe->ApiName);

	InsertTailList(&Counter->ActiveListHead, &Entry->ListEntry);
	Counter->NumberOfEntries += 1;

	return MSP_STATUS_OK;
}

ULONG
MspRemoveCounterEntry(
	IN struct _MSP_PROCESS *Process,
	IN struct _MSP_PROBE *Probe
	)
{
	PMSP_COUNTER_GROUP Counter;
	PMSP_COUNTER_ENTRY Entry;
	PBTR_COUNTER_ENTRY Active;

	Counter = Process->Counter;
	Entry = Probe->Counter;
	Active = Entry->Active;

	RemoveEntryList(&Entry->ListEntry);

	//
	// Copy and save active state to retire fields and insert
	// into retire list
	//

	Entry->Retire.NumberOfCalls = Active->NumberOfCalls;
	Entry->Retire.NumberOfExceptions = Active->NumberOfExceptions;
	Entry->Retire.NumberOfStackTraces = Active->NumberOfStackTraces;
	Entry->Retire.MinimumDuration = Active->MinimumDuration;
	Entry->Retire.MaximumDuration = Active->MaximumDuration;
	StringCchCopy(Entry->Retire.Name, MAX_PATH, Active->Name);

	//
	// N.B. Active field must be nulled, caller test Active
	// to determine whether it's an active or retired node.
	//

	Entry->Active = NULL;

	InsertTailList(&Counter->RetireListHead, &Entry->ListEntry);
	return 0;
}

VOID
MspStopCounterGroup(
	IN PMSP_PROCESS Process
	)
{
	PMSP_COUNTER_GROUP Group;
	PBTR_COUNTER_TABLE Table;

	Group = Process->Counter;
	Table = Group->Table;

	Group->Retire.NumberOfCalls = Table->NumberOfCalls;
	Group->Retire.NumberOfStackTraces = Table->NumberOfStackTraces;
	Group->Retire.NumberOfExceptions = Table->NumberOfExceptions;

	UnmapViewOfFile(Table);
	CloseHandle(Group->Handle);
	Group->Handle = NULL;
	Group->Table = NULL;
}