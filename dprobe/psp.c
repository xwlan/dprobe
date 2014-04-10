//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#include "bsp.h"
#include "psp.h"
#include "bitmap.h"
#include "frame.h"
#include "main.h"
#include "dtl.h"
#include "queue.h"
#include "debugger.h"
#include "injectldr.h"
#include "perf.h"
#include "mspdtl.h"

#pragma warning(disable : 4995)

#define PSP_PORT_NUMBER  16 

PSP_PORT PspPortTable[PSP_PORT_NUMBER];

LIST_ENTRY PspProcessList;
LIST_ENTRY PspPortList;
LIST_ENTRY PspAddOnList;

BSP_LOCK PspProcessLock;

WCHAR PspRuntimeFullPath[MAX_PATH];
ULONG PspRuntimeFeatures;

PPSP_PORT
PspCreatePortObject(
	VOID
	);

VOID
PspFreePort(
	IN PPSP_PORT Port
	);

VOID 
PspBuildOverlapped(
	OUT OVERLAPPED *Overlapped,
	IN HANDLE EventHandle
	);

LONG WINAPI
PspExceptionFilter(
	IN EXCEPTION_POINTERS *Pointers
	);

ULONG
PspGetRuntimeFullPathName(
	IN PWCHAR Buffer,
	IN USHORT Length,
	OUT PUSHORT ActualLength
	);

PPSP_PORT
PspCreatePortObject(
	VOID
	)
{
	int i;
	HANDLE EventHandle;

	for(i = 0; i < PSP_PORT_NUMBER; i++) {
		if (PspPortTable[i].State == BtrPortFree) {
			EventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
			PspPortTable[i].CompleteHandle = EventHandle;
			PspPortTable[i].State = BtrPortAllocated;
			InitializeSListHead(&PspPortTable[i].StackTraceList);
			return &PspPortTable[i];
		}
	}

	return NULL;
}

VOID
PspFreePort(
	IN PPSP_PORT Port
	)
{
	InterlockedFlushSList(&Port->StackTraceList);
	memset(Port, 0, sizeof(PSP_PORT));
	Port->State = BtrPortFree;

}

VOID 
PspBuildOverlapped(
	OUT OVERLAPPED *Overlapped,
	IN HANDLE EventHandle
	)
{
	ASSERT(EventHandle != NULL);
	
	Overlapped->Internal = 0;
	Overlapped->InternalHigh = 0;
	Overlapped->Offset = 0;
	Overlapped->OffsetHigh = 0;
	Overlapped->hEvent = EventHandle;
}

ULONG
PspConnectPort(
	IN ULONG ProcessId,
	IN ULONG BufferLength,
	IN ULONG Timeout,
	OUT PPSP_PORT Port
	)
{
	HANDLE FileHandle;
	WCHAR NameBuffer[64];
	PVOID Buffer;
	ULONG Status;

	Port->ThreadId = GetCurrentThreadId();
	Port->ProcessId = ProcessId;

	wsprintf(NameBuffer, L"\\\\.\\pipe\\btr\\%u", ProcessId);

	while (TRUE) {

		FileHandle = CreateFile(NameBuffer,
								GENERIC_READ|GENERIC_WRITE,
								0,
								NULL,
								OPEN_EXISTING,
								FILE_FLAG_OVERLAPPED,
								NULL);

		if (FileHandle != INVALID_HANDLE_VALUE){
			DebugTrace("connected to pid=%d", ProcessId);
			break;
		}

		Status = GetLastError();
		if (Status != ERROR_PIPE_BUSY) {
			return Status;
		}

		if (!WaitNamedPipe(NameBuffer, Timeout)) { 
			return GetLastError();
		} 
	}

	Port->PortHandle = FileHandle;
	PspBuildOverlapped(&Port->Overlapped, Port->CompleteHandle);

	Buffer = VirtualAlloc(NULL, BufferLength, MEM_COMMIT, PAGE_READWRITE);
	if (Buffer == NULL) {
		Status = GetLastError();
		CloseHandle(Port->CompleteHandle);
		CloseHandle(FileHandle);
		return Status;
	}
	
	Port->Buffer = Buffer;
	Port->BufferLength = BufferLength;

	return ERROR_SUCCESS;
}

ULONG
PspFreeCommand(
	IN PPSP_COMMAND Command
	)
{
	BspFree(Command);
	return  ERROR_SUCCESS;
}

PBTR_MESSAGE_CACHE
PspBuildCacheRequest(
	IN PPSP_PORT Port,
	IN PVOID Buffer,
	IN ULONG Length,
	IN HANDLE IndexFileHandle,
	IN HANDLE DataFileHandle,
	IN HANDLE SharedDataHandle,
	IN HANDLE PerfHandle
	)
{
	PBTR_MESSAGE_CACHE Packet;

	ASSERT(Length > sizeof(BTR_MESSAGE_CACHE));

	Packet = (PBTR_MESSAGE_CACHE)Port->Buffer;
	Packet->Header.PacketType = MESSAGE_CACHE;
	Packet->Header.Length = sizeof(BTR_MESSAGE_CACHE);
	Packet->IndexFileHandle = IndexFileHandle;
	Packet->DataFileHandle = DataFileHandle;
	Packet->SharedDataHandle = SharedDataHandle;
	Packet->PerfHandle = PerfHandle;

	return Packet;
}

PBTR_MESSAGE_RECORD
PspBuildRecordRequest(
	IN PPSP_PORT Port,
	IN PVOID Buffer,
	IN ULONG Length
	)
{
	return 0;
}

PBTR_MESSAGE_FILTER
PspBuildAddOnRequest(
	IN PPSP_PORT Port,
	IN PPSP_FILTER AddOn
	)
{
	return 0;
}

ULONG
PspMessageProcedure(
	IN PPSP_PORT Port
	)
{
	return 0;	
}

SLIST_HEADER PspStackTraceQueue;

ULONG
PspUpdateStackTrace(
	IN PPSP_PORT Port
	)
{
	ULONG Status;
	ULONG CompleteBytes;
	PBTR_MESSAGE_STACKTRACE Record;
	PBTR_MESSAGE_STACKTRACE_ACK Ack;
	PBTR_STACKTRACE_ENTRY Trace;
	ULONG Size;

	Record = (PBTR_MESSAGE_STACKTRACE)Port->Buffer;
	Record->Header.PacketType = MESSAGE_STACKTRACE;
	Record->Header.Length = sizeof(BTR_MESSAGE_STACKTRACE);

	Status = PspSendMessage(Port, &Record->Header, NULL, &CompleteBytes);	
	if (Status != ERROR_SUCCESS) {
		return Status;
	}

	Ack = (PBTR_MESSAGE_STACKTRACE_ACK)Port->Buffer;
	Status = PspWaitMessage(Port, &Ack->Header, Port->BufferLength, &CompleteBytes, NULL);
	if (Status != ERROR_SUCCESS) {
		return Status;
	}

	if (!Ack->NumberOfTraces) {
		return S_OK;
	}

	Size = sizeof(BTR_STACKTRACE_ENTRY) * Ack->NumberOfTraces;
	Trace = (PBTR_STACKTRACE_ENTRY)_aligned_malloc(Size, 16);
	RtlZeroMemory(Trace, Size);

	RtlCopyMemory(Trace, &Ack->Entries[0], Size);
	Trace->s1.ProcessId = Port->ProcessId;
	Trace->s1.NumberOfEntries = Ack->NumberOfTraces;

	InterlockedPushEntrySList(&PspStackTraceQueue, &Trace->ListEntry); 
	return Status;	
}

//
// Global record sequence number
//

ULONG PspRecordSequence = 0;

ULONG CALLBACK
PspStackTraceProcedure(
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

	Object = MspGetStackTraceObject(MspDtlObject);

	while (TRUE) {
		Status = WaitForSingleObject(Timer, INFINITE); 
		if (Status == WAIT_OBJECT_0) {

			while (ListEntry = InterlockedPopEntrySList(&PspStackTraceQueue)) {

				Entry = CONTAINING_RECORD(ListEntry, BTR_STACKTRACE_ENTRY, ListEntry);
				MspAcquireCsLock(&Object->Lock);
				MspAcquireContext(Object, Entry->s1.ProcessId);

				for(Number = 0; Number < Entry->s1.NumberOfEntries; Number += 1) {
					MspInsertStackTrace(Object, Entry + Number, Entry->s1.ProcessId);				
				}

				MspReleaseContext(Object, TRUE);
				MspReleaseCsLock(&Object->Lock);

				_aligned_free(Entry);

			}
		}
	}
}

ULONG
PspQueueRecord(
	IN PPSP_PORT Port,
	IN PBTR_MESSAGE_RECORD_ACK Record,
	IN BTR_RECORD_TYPE Type
	)
{
	ULONG Length;
	PDTL_IO_BUFFER Buffer;
	PBSP_QUEUE_NOTIFICATION Notification;

	ASSERT(Record->Header.Length <= Port->BufferLength);

	if (!Record->NumberOfRecords) {
		return S_OK;
	}

	Length = Record->Header.Length - FIELD_OFFSET(BTR_MESSAGE_RECORD_ACK, Data);
	Buffer = (PDTL_IO_BUFFER)BspMalloc(sizeof(DTL_IO_BUFFER));
	Buffer->Buffer = BspMalloc(Length);
	Buffer->Length = Length;
	Buffer->NumberOfRecords = Record->NumberOfRecords;

	//
	// Copy all packets to intermediate buffer
	//

	RtlCopyMemory(Buffer->Buffer, Record->Data, Length);

	Notification = (PBSP_QUEUE_NOTIFICATION)BspMalloc(sizeof(BSP_QUEUE_NOTIFICATION));
	Notification->Type = BSP_QUEUE_IO;
	Notification->Packet = Buffer;

	BspQueueNotification(PspQueueObject, Notification);
	return S_OK;
}

ULONG
PspCreateQueueObject(
	VOID
	)
{
	ULONG Status;

	if (PspQueueObject) {
		return S_OK;
	}

	Status = BspCreateQueue(&PspQueueObject);
	return Status;
}

PBSP_QUEUE
PspGetQueueObject(
	VOID
	)
{
	return PspQueueObject;
}

ULONG
PspOnCommandStop(
	IN PPSP_PORT Port,
	IN PPSP_COMMAND Command
	)
{
	ULONG Status;
	ULONG CompleteBytes;
	BTR_MESSAGE_STOP Packet;
	PBTR_MESSAGE_ACK Ack;

	Packet.Header.PacketType = MESSAGE_STOP_REQUEST;
	Packet.Header.Length = sizeof(Packet);
	
	Status = PspSendMessage(Port, &Packet.Header, NULL, &CompleteBytes);
	if (Status != S_OK) {
		Command->Information = 0;
		Command->Status = Status;
		PostMessage(Command->hWndBar, WM_PROBE_ACK, 0, (LPARAM)Command);
		return Status;
	}
	
	Ack = (PBTR_MESSAGE_ACK)Port->Buffer;
	Status = PspWaitMessage(Port, &Ack->Header, Port->BufferLength, &CompleteBytes, NULL);
	Command->Information = 0;
	Command->Status = Status;

	PostMessage(Command->hWndBar, WM_PROBE_ACK, 0, (LPARAM)Command);
	return S_FALSE;	
}

ULONG
PspOnCommandManualProbe(
	IN PPSP_PORT Port,
	IN PPSP_COMMAND Command
	)
{
	ULONG Status;
	PPSP_PROCESS Process;
	PLIST_ENTRY ListEntry;
	PPSP_PROBE Probe;
	PBTR_MESSAGE_EXPRESS Packet;
	PBTR_MESSAGE_ACK Ack;
	ULONG CompleteBytes;

	Status = S_OK;
	Process = Command->Process;
	ListEntry = Process->ExpressList.Flink;

	while (ListEntry != &Process->ExpressList) {
		
		Probe = CONTAINING_RECORD(ListEntry, PSP_PROBE, Entry);
		if (Probe->Current == ProbeRunning) {
			ListEntry = ListEntry->Flink;
			continue;
		}

		Packet = PspBuildManualRequest(Port->Buffer, Port->BufferLength, Probe);	
		Status = PspSendMessage(Port, &Packet->Header, NULL, &CompleteBytes);
		if (Status != S_OK) {
			Probe->Result = ProbeAbort;
			break;
		}

		Ack = (PBTR_MESSAGE_ACK)Port->Buffer;
		Status = PspWaitMessage(Port, &Ack->Header, sizeof(*Ack), &CompleteBytes, NULL);
		if (Status != S_OK) {
			Probe->Result = ProbeAbort;
			break;
		}

		Probe->Address = (ULONG_PTR)Ack->Information;

		if (Ack->Status == 0) {
			Probe->Result = ProbeCommit;
		} else {
			Probe->Result = ProbeAbort;
		}

		ListEntry = ListEntry->Flink;
	}

	Command->Status = Status;
	PspCommitManualProbe(Command);
	return Status;
}

ULONG
PspOnCommandImportProbe(
	IN PPSP_PORT Port,
	IN PPSP_COMMAND Command
	)
{
	ULONG Status;
	PPSP_PROCESS Process;
	
	Status = S_FALSE;
	Process = Command->Process;

	if (IsListEmpty(&Process->ExpressList) != TRUE) {
		
		Command->Type = ManualProbe;
		Status = PspOnCommandManualProbe(Port, Command);

		if (Status != S_OK) {
			Command->Type = ImportProbes;
			Command->Status = Status;
			return Status;
		}
	}

	Command->Type = AddOnProbe;
	Status = PspOnCommandAddOnProbe(Port, Command);

	if (Status != S_OK) {
		Command->Status = Status;
		Command->Type = ImportProbes;
		return Status;
	}

	Command->Status = Status;
	Command->Type = ImportProbes;
	return Status;
}

ULONG
PspOnCommandAddOnProbe(
	IN PPSP_PORT Port,
	IN PPSP_COMMAND Command
	)
{
	return 0;
}

PPSP_FILTER
PspQueryAddOnObjectByName(
	IN PPSP_PROCESS Process,
	IN PWSTR Name
	)
{
	PLIST_ENTRY ListEntry;
	PPSP_FILTER Object;

	ListEntry = Process->AddOnList.Flink;
	while (ListEntry != &Process->AddOnList) {
		Object = CONTAINING_RECORD(ListEntry, PSP_FILTER, ListEntry);
		if (_wcsicmp(Object->FilterObject->FilterName, Name) == 0) {
			return Object;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

PPSP_FILTER
PspGetAddOnObject(
	IN PPSP_FILTER RootObject,
	IN PPSP_FILTER TargetObject
	)
{
	PLIST_ENTRY ListEntry;
	PPSP_FILTER Object;

	if (RootObject == TargetObject) {
		return RootObject;
	}

	ListEntry = RootObject->ListEntry.Flink;
	while (ListEntry != &RootObject->ListEntry) {
		Object = CONTAINING_RECORD(ListEntry, PSP_FILTER, ListEntry);
		if (Object == TargetObject) {
			return Object;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

ULONG
PspOnCommandProbe(
	IN PPSP_PORT Port,
	IN PPSP_COMMAND Command
	)
{
	ULONG Status;
	ASSERT(Command != NULL);
	
	switch (Command->Type) {

		case ManualProbe:
			Status = PspOnCommandManualProbe(Port, Command);
			PostMessage(Command->hWndBar, WM_PROBE_ACK, 0, (LPARAM)Command);
			break;

		case AddOnProbe:
			Status = PspOnCommandAddOnProbe(Port, Command);
			PostMessage(Command->hWndBar, WM_PROBE_ACK, 0, (LPARAM)Command);
			break;

		case ImportProbes:
			Status = PspOnCommandImportProbe(Port, Command);
			PostMessage(Command->hWndBar, WM_PROBE_ACK, 0, (LPARAM)Command);
			break;

		case StopProbing:
			Status = PspOnCommandStop(Port, Command);
			break;

		default:
			ASSERT(0);
	}

	return Status;
}

ULONG
PspOnCommandQuit(
	IN PPSP_PORT Port
	)
{
	return S_OK;
}

ULONG
PspCommandProcedure(
	IN PPSP_PORT Port,
	IN MSG *Msg 
	)
{
	ULONG Status;
	PPSP_COMMAND Command;

	ASSERT(Msg != NULL);

	switch (Msg->message) {

		case WM_PROBE:
			Command = (PPSP_COMMAND)Msg->lParam;
			Status = PspOnCommandProbe(Port, Command);
			break;

		case WM_QUIT:
			Status = PspOnCommandQuit(Port);
			break;

		default:
			Status = ERROR_SUCCESS;
	}

	return Status;
}

BOOLEAN
PspSetWaitableTimer(
	IN PPSP_PORT Port
	)
{
	LARGE_INTEGER DueTime;
	BOOLEAN Status;

	//
	// N.B. DueTime is set at 100ns granularity, check
	// MSDN for further details.
	//

	DueTime.QuadPart = -10000 * Port->TimerPeriod;

	Status = SetWaitableTimer(Port->TimerHandle, 
		                      &DueTime, 
							  Port->TimerPeriod, 
							  NULL, 
							  NULL, 
							  FALSE);	

	return Status;
}

PBTR_MESSAGE_EXPRESS
PspBuildManualRequest(
	IN PVOID Buffer,
	IN ULONG Length,
	IN PPSP_PROBE Probe 
	)
{
	
	return 0;
}

ULONG
PspStartSession(
	IN PPSP_PORT Port
	)
{
	PBTR_MESSAGE_CACHE Packet;
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

	//
	// Send cache information to target process
	//

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

	Handle = MspCreatePerformanceItem(Port->ProcessId);
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

	Port->StackTrace = MspGetStackTraceObject(MspDtlObject);

	//
	// Build and set cache information to target process.
	//

	Packet = PspBuildCacheRequest(Port, Port->Buffer, Port->BufferLength,
		                          IndexFileHandle, DataFileHandle, SharedDataHandle,
								  PerfHandle);

	Status = PspSendMessage(Port, &Packet->Header, NULL, &CompleteBytes);
	if (Status != S_OK) {
		return Status;
	}
		
	Ack = (PBTR_MESSAGE_ACK)Port->Buffer;
	Status = PspWaitMessage(Port, &Ack->Header, Port->BufferLength, &CompleteBytes, NULL);
	if (Status != S_OK) {
		return Status;
	}

	return Ack->Status;
}

ULONG WINAPI
PspSessionThread(
	IN PVOID StartContext
	)
{
	ULONG Status;
	ULONG ProcessId;
	PPSP_PORT Port;
	PPSP_THREAD_CONTEXT Context;
	HANDLE TimerHandle;
	static ULONG NumberOfSessions = 0;
	MSG Msg;

	ASSERT(StartContext != NULL);

	Context = (PPSP_THREAD_CONTEXT)StartContext;
	ProcessId = Context->ProcessId;

	Port = PspCreatePortObject();
	if (Port == NULL) {
		SetEvent(Context->CompleteEvent);
		PspRemoveProcess(ProcessId);
		return 0;
	}

	Port->ProcessId = ProcessId;
	Port->ThreadId = GetCurrentThreadId();

	Status = PspConnectPort(ProcessId, PSP_PORT_BUFFER_LENGTH, 1000 * 10, Port);
	if (Status != ERROR_SUCCESS) {
		DebugTrace("PspConnectPort() failed, error=%d", Status);
		PspFreePort(Port);
		SetEvent(Context->CompleteEvent);
		PspRemoveProcess(ProcessId);
		return 0;
	}
	
	//
	// Start session
	//

	Status = PspStartSession(Port);
	if (Status != S_OK) {
		DebugTrace("PspStartSession() failed, error=%d", Status);
		PspFreePort(Port);
		SetEvent(Context->CompleteEvent);
		PspRemoveProcess(ProcessId);
		return 0;
	}

	if (NumberOfSessions == 0) {
		MspStartIndexProcedure(&MspDtlObject.IndexObject);
	}

	NumberOfSessions += 1;

	//
	// N.B. Before we signal CompleteEvent, we need ensure current thread
	// has a message queue available to receive message posted from dispatcher,
	// otherwise, a race condition may occur, dispatcher will immediately post
	// WM_PROBE after CompleteEvent is signaled.
	// 

	TimerHandle = CreateWaitableTimer(NULL, FALSE, NULL);
	Port->TimerHandle = TimerHandle;
	
	//
	// Set 100ms interval
	//

	Port->TimerPeriod = 500;

	PeekMessage(&Msg, NULL, 0, 0, PM_NOREMOVE);
	SetEvent(Context->CompleteEvent);
	PspSetWaitableTimer(Port);

	while (TRUE) {

		Status = MsgWaitForMultipleObjects(1, 
										   &Port->TimerHandle, 
									       FALSE, 
									       INFINITE, 
									       QS_POSTMESSAGE);
		if (Status == WAIT_OBJECT_0) {
			
			//Status = PspMessageProcedure(Port);
			Status = PspUpdateStackTrace(Port);
			if (Status != ERROR_SUCCESS) {
				break;
			}

		}

		else if (Status == WAIT_OBJECT_0 + 1) {

			Status = PeekMessage(&Msg, NULL, WM_PROBE_FIRST, WM_PROBE_LAST, PM_REMOVE);
			if (Status == TRUE) {
				Status = PspCommandProcedure(Port, &Msg);
				if (Status != S_OK) {
					break;
				}
			}
		} 

		else {
			Status = GetLastError();
			PostMessage(hWndMain, WM_PROBE_STATUS, (WPARAM)PspError, (LPARAM)Status);
			break;
		}
	}

	//
	// N.B. This is required to stop perfmon for this target
	//

	PerfDeleteTask(Port->ProcessId);

	//
	// Delete probing menu of this task
	//

	FrameDeleteProbingMenu(Port->ProcessId);
	
	PspRemoveProcess(ProcessId);
	PspDisconnectPort(Port);
	PspFreePort(Port);
	return 0;
}

ULONG
PspWaitPortIoComplete(
	IN PPSP_PORT Port,
	IN OVERLAPPED *Overlapped,
	IN ULONG Milliseconds,
	OUT PULONG CompleteBytes
	)
{
	ULONG Status;
	ASSERT(Overlapped != NULL);

	if (Overlapped == NULL) {
		return ERROR_INVALID_PARAMETER;
	}

	Status = WaitForSingleObject(Overlapped->hEvent, Milliseconds);
	if (Status != WAIT_OBJECT_0) {
		CancelIo(Port->PortHandle);
		SetEvent(Overlapped->hEvent);
		return Status;
	}

	Status = GetOverlappedResult(Port->PortHandle, 
						         Overlapped,
                                 CompleteBytes,
						         FALSE);
	if (Status != TRUE) {
		Status = GetLastError();
		return Status;
	}
	
	return ERROR_SUCCESS;
}

ULONG
PspDisconnectPort(
	IN PPSP_PORT Port
	)
{
	DebugTrace("PspDisconnectPort: Target PID=%u, TID=%u", Port->ProcessId, Port->ThreadId);

	if (Port->TimerHandle) {
		CloseHandle(Port->TimerHandle);
		Port->TimerHandle = NULL;
	}

	if (Port->PortHandle) {
		CloseHandle(Port->PortHandle);
		Port->PortHandle = NULL;
	}

	if (Port->Overlapped.hEvent) {
		CloseHandle(Port->Overlapped.hEvent);
		Port->Overlapped.hEvent = NULL;
	}

	if (Port->Buffer) {
		VirtualFree(Port->Buffer, 0, MEM_RELEASE);
		Port->Buffer = NULL;
		Port->BufferLength = 0;
		Port->State = BtrPortFree;
	}
	
	return S_OK;
}

ULONG
PspWaitMessage(
	IN PPSP_PORT Port,
	OUT PBTR_MESSAGE_HEADER Message,
	IN ULONG BufferLength,
	OUT PULONG CompleteBytes,
	IN OVERLAPPED *Overlapped
	)
{
	BOOLEAN Status;
	ULONG ErrorCode;
	HRESULT Result;

	if (BufferLength > Port->BufferLength) {
		return S_FALSE;
	}

	if (Overlapped != NULL) {
		Status = ReadFileEx(Port->PortHandle, 
			              Message, 
						  BufferLength, 
						  Overlapped,
						  NULL);

		if (Status != TRUE) {
			ErrorCode = GetLastError();
			switch (ErrorCode) {
		  	    case ERROR_IO_PENDING:
					Result = ERROR_IO_PENDING;
					break;
				default:
					Result = ErrorCode;
			}
			return Result;
		} 
		else {
			return S_OK;
		}
	}

	PspBuildOverlapped(&Port->Overlapped, Port->CompleteHandle);
	Status = ReadFile(Port->PortHandle, 
		              Message, 
					  BufferLength, 
					  CompleteBytes, 
					  &Port->Overlapped);

	if (Status != TRUE) {
		ErrorCode = GetLastError();
		switch (ErrorCode) {
	  	    case ERROR_IO_PENDING:
				Result = PspWaitPortIoComplete(Port, 
											   &Port->Overlapped,
											   INFINITE,
											   CompleteBytes);
				break;
			default:
				Result = ErrorCode;
		}
		return Result;
	} 
	
	return S_OK;
}

ULONG
PspSendMessage(
	IN PPSP_PORT Port,
	IN PBTR_MESSAGE_HEADER Message,
	IN OVERLAPPED *Overlapped,
	OUT PULONG CompleteBytes 
	)
{
	BOOLEAN Status;
	HRESULT Result;
	ULONG ErrorCode;

	if (Message->Length > Port->BufferLength) {
		return S_FALSE;
	}

	if (Overlapped != NULL) {
		
		Status = WriteFile(Port->PortHandle,
			               Message, 
						   Message->Length, 
						   CompleteBytes, 
						   Overlapped);

		if (Status != TRUE) {
			ErrorCode = GetLastError();
			switch (ErrorCode) {
		  	    case ERROR_IO_PENDING:
					Result = ERROR_IO_PENDING;
					break;
				default:
					Result = S_FALSE;
			}
			return Result;
		} 

		return S_OK;
	}

	PspBuildOverlapped(&Port->Overlapped, Port->CompleteHandle);	
	Status = WriteFile(Port->PortHandle, 
		               Message, 
					   Message->Length, 
					   CompleteBytes, 
					   &Port->Overlapped);

	if (Status != TRUE) {
		ErrorCode = GetLastError();
		switch (ErrorCode) {
	  	    case ERROR_IO_PENDING:
				Result = PspWaitPortIoComplete(Port, 
											   &Port->Overlapped, 
											   1000 * 10,
											   CompleteBytes);
											   
				break;
			default:
				Result = S_FALSE;
		}
		return Result;
	} 
	
	return S_OK;
}

ULONG
PspSendMessageWaitReply(
	IN PPSP_PORT Port,
	IN PBTR_MESSAGE_HEADER Message,
	OUT PBTR_MESSAGE_HEADER Reply
	)
{
	ULONG CompleteBytes;
	ULONG Status;

	Status = PspSendMessage(Port, Message, NULL, &CompleteBytes);
	if (Status != ERROR_SUCCESS) {
		return Status;
	}

	Status = PspWaitMessage(Port, Reply, Reply->Length, &CompleteBytes, NULL);
	return Status;
}

ULONG
PspWaitMessageSendReply(
	IN PPSP_PORT Port,
	IN PBTR_MESSAGE_HEADER Reply,
	OUT PBTR_MESSAGE_HEADER Message
	)
{
	ULONG CompleteBytes;
	HRESULT Status;

	Status = PspWaitMessage(Port, Message, Port->BufferLength, &CompleteBytes,NULL);
	if (Status != ERROR_SUCCESS) {
		return Status;
	}

	Status = PspSendMessage(Port, Reply, NULL,&CompleteBytes);
	return Status;
}

LONG WINAPI
PspExceptionFilter(
	IN EXCEPTION_POINTERS *Pointers
	)
{
	return EXCEPTION_EXECUTE_HANDLER;

}

ULONG
PspInitialize(
	ULONG Features	
	)
{
	int i;
	USHORT Length;
	ULONG Status;
	HMODULE DllHandle;
	WCHAR Path[MAX_PATH + 1];

	ASSERT(FlagOn(Features, FeatureRemote));

	PspRuntimeFeatures = Features;

	InitializeListHead(&PspProcessList);
	BspInitializeLock(&PspProcessLock);
	
	InitializeListHead(&PspPortList);
	InitializeListHead(&PspAddOnList);

	for(i = 0; i < PSP_PORT_NUMBER; i++) {
		PspPortTable[i].State = BtrPortFree;
	}
	
	PspGetRuntimeFullPathName(PspRuntimeFullPath, MAX_PATH, &Length);

	DllHandle = LoadLibrary(PspRuntimeFullPath);
	if (!DllHandle) {
		Status = GetLastError();
		return Status;
	}

	GetCurrentDirectory(MAX_PATH, Path);
	PspBuildAddOnList(Path);
	
	//
	// Delete garbage files last time remained
	//
	
	ASSERT(!PspDtlObject);
	DtlDeleteGarbageFiles(Path);

	PspCreateDtlObject();
	PspCreateQueueObject();

	InitializeSListHead(&PspStackTraceQueue);
	CreateThread(NULL, 0, PspStackTraceProcedure, NULL, 0, NULL);
	return S_OK;
}

PPSP_PROCESS
PspQueryProcessObject(
	IN ULONG ProcessId
	)
{
	PLIST_ENTRY ListEntry;
	PPSP_PROCESS Object;

	PspLockProcessList();

	ListEntry = PspProcessList.Flink;
	while (ListEntry != &PspProcessList) {

		Object = CONTAINING_RECORD(ListEntry, PSP_PROCESS, Entry);
		if (Object->ProcessId == ProcessId) {
			PspUnlockProcessList();
			return Object;
		}

		ListEntry = ListEntry->Flink;
	}

	PspUnlockProcessList();
	return NULL;
}

VOID
PspLockProcessList(
	VOID
	)
{
	BspAcquireLock(&PspProcessLock);
}

VOID
PspUnlockProcessList(
	VOID
	)
{
	BspReleaseLock(&PspProcessLock);
}

VOID
PspCommitCachedPart(
	IN PPSP_PROBE Probe
	)
{
}

ULONG
PspCommitManualProbe(
	IN PPSP_COMMAND Command
	)
{
	PPSP_PROBE Probe;
	PPSP_PROCESS Process;
	PDTL_MANUAL_GROUP Group;
	PLIST_ENTRY ListEntry;
 
	Process = Command->Process;
	Group = DtlCreateManualGroup(PspDtlObject, Process);
	ListEntry = Process->ExpressList.Flink;

	if (Process->State == ProcessCreating) {

		while (ListEntry != &Process->ExpressList) {
			
			Probe = CONTAINING_RECORD(ListEntry, PSP_PROBE, Entry);
			ListEntry = ListEntry->Flink;
			
			if (Probe->Result == ProbeCommit) {
				Probe->Current = ProbeRunning;
				DtlCommitProbe(PspDtlObject, Group, Probe);

			} else {
				RemoveEntryList(&Probe->Entry);	
				PspFreeProbe(Probe);
			}
		}

		//
		// N.B. BUG
		//

		//if (IsListEmpty(&Process->ExpressList)) {
		//	PspFreeProcess(Process);
		//	return S_FALSE;

		//} else {

			Process->State = ProcessRunning;
			PspLockProcessList();
			InsertTailList(&PspProcessList, &Process->Entry);
			PspUnlockProcessList();
			return S_OK;
		//}
	}

	ListEntry = Process->ExpressList.Flink;
	while (ListEntry != &Process->ExpressList) {
		
		Probe = CONTAINING_RECORD(ListEntry, PSP_PROBE, Entry);
		ListEntry = ListEntry->Flink;

		switch (Probe->Current) {

			case ProbeCreating:
				if (Probe->Result == ProbeCommit) {
					Probe->Current = ProbeRunning;
					DtlCommitProbe(PspDtlObject, Group, Probe);
				} else {
					RemoveEntryList(&Probe->Entry);
					PspFreeProbe(Probe);
				}
				break;

			case ProbeEditing:
				if (Probe->Result == ProbeCommit) {
					Probe->Current = ProbeRunning;
					PspCommitCachedPart(Probe);
					DtlCommitProbe(PspDtlObject, Group, Probe);
				} else {
					RemoveEntryList(&Probe->Entry);
					PspFreeProbe(Probe);
				}
				break;

			case ProbeRemoving:
				RemoveEntryList(&Probe->Entry);
				PspFreeProbe(Probe);
				break;
		}

	}

	return S_OK;
}

ULONG
PspDispatchCommand(
	IN PPSP_COMMAND Command
	)
{
	ULONG SessionId;
	ULONG Status;
	ULONG Counter;
	PPSP_PROCESS Process;

	Process = Command->Process;
	SessionId = PspGetSession(Process->ProcessId);

	if (SessionId == 0 && Command->Type == StopProbing) {
		if (Command->hWndBar) {
			PostMessage(Command->hWndBar, WM_PROBE_ACK, 0, 0);
		}
		return S_OK;
	}

	if (SessionId == 0) {
		Status = PspCreateSession(Process->ProcessId, &SessionId);
		if (Status != ERROR_SUCCESS) {
			Command->Status = Status;
			PostMessage(Command->hWndBar, WM_PROBE_ACK, 0, (LPARAM)Command);
			return Status;
		}		
	}

	Process->SessionId = SessionId;
	Counter = 0;

	do {
		
		Status = PostThreadMessage(SessionId, WM_PROBE, 0, (LPARAM)Command);

		if (Status != TRUE) {
			Counter += 1;	
			Sleep(1000);
		}
	}
	while (Status != TRUE && Counter < 3);

	if (Status != TRUE) {

		PostMessage(Command->hWndBar, WM_PROBE_ACK, 0, 0);
		return GetLastError();
	}

	return S_OK;
}

PPSP_COMMAND
PspCreateCommandObject(
	IN PSP_COMMAND_TYPE Type,
	IN PPSP_PROCESS Process
	)
{
	PPSP_COMMAND Command;
	
	Command = (PPSP_COMMAND)BspMalloc(sizeof(PSP_COMMAND));
	Command->Type = Type;
	Command->Process = Process;
	Command->Status = S_OK;
	Command->Information = 0;

	return Command;
}
		
ULONG
PspGetSession(
	IN ULONG ProcessId
	)
{
	int i;

	for(i = 0; i < PSP_PORT_NUMBER; i++) {
		if (PspPortTable[i].ProcessId == ProcessId) {
			return PspPortTable[i].ThreadId;
		}		
	}

	return 0;
}

ULONG
PspGetFilePath(
	IN PWSTR Buffer,
	IN ULONG Length
	)
{
	HMODULE ModuleHandle;
	PWCHAR Slash;

	ModuleHandle = GetModuleHandle(NULL);
	GetModuleFileName(ModuleHandle, Buffer, Length);
	Slash = wcsrchr(Buffer, L'\\');

	if (Slash != NULL) {
		*Slash = 0;
		return S_OK;
	}
	
	return S_FALSE;
}


ULONG
PspGetRuntimeFullPathName(
	IN PWCHAR Buffer,
	IN USHORT Length,
	OUT PUSHORT ActualLength
	)
{
	HMODULE ModuleHandle;
	PWCHAR Slash;

	ModuleHandle = GetModuleHandle(NULL);
	GetModuleFileName(ModuleHandle, Buffer, Length);
	Slash = wcsrchr(Buffer, L'\\');
	wcscpy(Slash + 1, L"dprobe.btr.dll");
	*ActualLength = (USHORT)wcslen(Buffer);

	return ERROR_SUCCESS;
}

ULONG
PspCreateThread(
	IN PPSP_THREAD_CONTEXT Context,
	OUT PULONG ThreadId
	)
{
	HANDLE ThreadHandle;
	ULONG Status;
	
	ASSERT(Context != NULL);
	
	ThreadHandle = CreateThread(NULL, 0, PspSessionThread, Context, 0, ThreadId);
	if (ThreadHandle != NULL) {
		CloseHandle(ThreadHandle);
		return S_OK;
	}

	Status = GetLastError();
	DebugTrace("PspCreateThread() failed, error=%d", Status);

	return Status;
}

ULONG 
PspCreateSession(
	IN ULONG ProcessId,
	OUT PULONG ThreadId 
	)
{
	ULONG Status;
	HANDLE EventHandle;
	PSP_THREAD_CONTEXT Context;
	BOOLEAN Complete;
	WCHAR Buffer[MAX_PATH];
	MSG Msg;

	//Status = PspGetRuntimeFullPathName(FullPath, MAX_PATH, &Length);
	//if (Status != ERROR_SUCCESS) {
	//	Buffer = (PWCHAR)BspMalloc(Length * sizeof(WCHAR));
	//	PspGetRuntimeFullPathName(Buffer, Length, &Length);
	//}

	Status = PspLoadProbeRuntime(ProcessId);
	//ThreadHandle = CreateThread(NULL, 0, PspDebugProcedure, ProcessId, 0, NULL);
	//WaitForSingleObject(ThreadHandle, INFINITE);
	//CloseHandle(ThreadHandle);

	//Status = BspLoadLibraryUnsafe(ProcessId, PspRuntimeFullPath);

	//
	// If path is too long, the path buffer is allocated from heap,
	// required to be freed here.
	// 
	//if (Buffer != FullPath) {
	//	BspFree(Buffer);
	//}

	if (Status == S_OK) {
		StringCchPrintf(Buffer, MAX_PATH, L"Succeed inject runtime into PID %d", ProcessId);
		BspWriteLogEntry(LOG_LEVEL_INFORMATION,
			             LOG_SOURCE_PSP, Buffer);

	} else {
		StringCchPrintf(Buffer, MAX_PATH, L"Failed to inject runtime into PID %d, Status=0x%08x", 
			            ProcessId, Status);
		BspWriteLogEntry(LOG_LEVEL_ERROR,
			             LOG_SOURCE_PSP, Buffer);
	}

	EventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
	Context.ProcessId = ProcessId;
	Context.CompleteEvent = EventHandle;

	Status = PspCreateThread(&Context, ThreadId);
	if (Status != ERROR_SUCCESS) {
		return Status;
	}

	//
	// N.B. MsgWaitForMultipleObjects is required here,
	// because this routine is called in main thread,
	// typically with a progress bar sitting above us,
	// which set a timer, and need periodically update 
	// UI to indicate responsiveness.
	//
	
	Complete = FALSE;

	while (Complete != TRUE) {

		Status = MsgWaitForMultipleObjects(1, 
			                               &EventHandle, 
										   FALSE, 
										   INFINITE, 
										   QS_ALLINPUT);

		if (Status == WAIT_OBJECT_0) {
			Complete = TRUE;
			Status = ERROR_SUCCESS;
			break;
		} 
		
		else if (Status == WAIT_OBJECT_0 + 1) {

			if (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}
		} 
		
		else {
			
			Status = GetLastError();
			break;
		} 
	}

	CloseHandle(EventHandle);
	return Status;
}

PPSP_PROBE
PspIsRegisteredProbe(
	IN PPSP_PROCESS Process,
	IN PWSTR ModuleName,
	IN PWSTR ApiName,
	IN PSP_COMMAND_TYPE Type
	)
{
	PLIST_ENTRY Entry;
	PLIST_ENTRY ListHead;
	PSP_PROBE *Define;

	switch (Type) {

		case ManualProbe:
			Entry = Process->ExpressList.Flink;
			ListHead = &Process->ExpressList;
			break;

		case AddOnProbe:
			Entry = Process->AddOnList.Flink;
			ListHead = &Process->AddOnList;
			break;

		default:
			ASSERT(0);
	}

	while (Entry != ListHead) {
		Define = CONTAINING_RECORD(Entry, PSP_PROBE, Entry);
		if (wcscmp(Define->MethodName, ApiName) == 0) {
			if (wcscmp(Define->ModuleName, ModuleName) == 0) {
				return Define;
			}
		}
		Entry = Entry->Flink;
	}

	return NULL;
}

PPSP_PROCESS
PspCreateProcessObject(
	VOID
	)
{
	PPSP_PROCESS Object;

	Object = (PPSP_PROCESS)BspMalloc(sizeof(PSP_PROCESS));
	Object->State = ProcessCreating;
	
	InitializeListHead(&Object->ExpressList);
	InitializeListHead(&Object->AddOnList);

	//
	// N.B. AddOn list is copied from PspAddOnList, so every new process 
	// object has a copy of all found AddOn, however, initially they share 
	// the BTR_FILTER object for each AddOn module, after specific 
	// AddOn is registered, the registration object is replaced with returned 
	// copy sent by target process.
	//

	PspCopyAddOnList(Object);
	return Object;
}

BOOLEAN
PspRemoveProcess(
	IN ULONG ProcessId
	)
{
	PPSP_PROCESS Process;
	PLIST_ENTRY ListEntry;

	PspLockProcessList();

	ListEntry = PspProcessList.Flink;
	while (ListEntry != &PspProcessList) {
		Process = CONTAINING_RECORD(ListEntry, PSP_PROCESS, Entry);
		if (Process->ProcessId == ProcessId) {
			RemoveEntryList(&Process->Entry);
			PspUnlockProcessList();
			PspFreeProcess(Process);
			return TRUE;
		}
		ListEntry = ListEntry->Flink;
	}

	PspUnlockProcessList();
	return FALSE;
}

ULONG
PspActiveProcessCount(
	VOID
	)
{
	ULONG Count;
	PLIST_ENTRY ListEntry;

	Count = 0;

	PspLockProcessList();
	ListEntry = PspProcessList.Flink;
	
	while (ListEntry != &PspProcessList) {
		Count += 1;
		ListEntry = ListEntry->Flink;
	}

	PspUnlockProcessList();
	return Count;
}

VOID
PspCopyAddOnList(
	IN PPSP_PROCESS Process
	)
{
	PLIST_ENTRY ListEntry;
	PPSP_FILTER AddOnObject, Copy;

	ASSERT(Process != NULL);
	ASSERT(Process->State == ProcessCreating);

	ListEntry = PspAddOnList.Flink;

	while (ListEntry != &PspAddOnList) {

		AddOnObject = CONTAINING_RECORD(ListEntry, PSP_FILTER, ListEntry);
		ASSERT(AddOnObject->State == ProcessCreating);

		Copy = (PPSP_FILTER)BspMalloc(sizeof(PSP_FILTER));
		memcpy(Copy, AddOnObject, sizeof(PSP_FILTER));	

		InsertTailList(&Process->AddOnList, &Copy->ListEntry);
		ListEntry = ListEntry->Flink;
	}

	return;
}

ULONG
PspLookupProcessName(
	IN ULONG ProcessId,
	IN PWCHAR Buffer,
	IN ULONG BufferLength
	)
{
	PPSP_PROCESS Process;
	ULONG Length;

	Process = PspQueryProcessObject(ProcessId);
	if (Process == NULL) {
		wcscpy(Buffer, L"Unknown");
		return ERROR_SUCCESS;
	}

	Length = (ULONG)wcslen(Process->Name);
	if (Length + sizeof(WCHAR) > BufferLength) {
		return ERROR_INVALID_PARAMETER;
	}
	
	wcscpy(Buffer, Process->Name);
	return ERROR_SUCCESS;
}

ULONG
PspLookupProbeByAddress(
	IN ULONG ProcessId,
	IN ULONG_PTR Address,
	IN PWCHAR Buffer,
	IN ULONG BufferLength
	)
{
	PPSP_PROCESS Process;
	PPSP_PROBE Probe;
	PLIST_ENTRY Entry;

	Process = PspQueryProcessObject(ProcessId);
	if (Process == NULL) {
		return ERROR_INVALID_PARAMETER;
	}	

	Entry = Process->ExpressList.Flink;
	while (Entry != &Process->ExpressList) {
		
		Probe = CONTAINING_RECORD(Entry, PSP_PROBE, Entry);
		
		if (Probe->Address == Address) {
			wsprintf(Buffer, L"%ws - %ws", Probe->ModuleName, Probe->MethodName);
			ASSERT(wcslen(Buffer) < BufferLength);
			return ERROR_SUCCESS;
		}

		Entry = Entry->Flink;
	}
	
	return ERROR_NOT_FOUND;
}

VOID
PspFreeProbe(
	IN PPSP_PROBE Probe
	)
{
	ASSERT(Probe != NULL);
	BspFree(Probe);
}

PPSP_PROBE
PspCreateProbe(
	IN PWSTR ModuleName,
	IN PWSTR MethodName,
	IN PWSTR UndMethodName
	)
{
	PPSP_PROBE Object;

	Object = (PPSP_PROBE)BspMalloc(sizeof(PSP_PROBE));
	Object->ReturnType = ReturnUIntPtr;
	Object->Current = ProbeCreating;

	StringCchCopy(Object->ModuleName, MAX_MODULE_NAME_LENGTH, ModuleName);
	StringCchCopy(Object->MethodName, MAX_FUNCTION_NAME_LENGTH, MethodName);
	StringCchCopy(Object->UndMethodName, MAX_FUNCTION_NAME_LENGTH, UndMethodName);

	return Object;
}

VOID
PspFreeFilter(
	IN PPSP_FILTER Filter
	)
{
	if (Filter->State != FilterCreating) {

		ASSERT(Filter->State != FilterCreating);
		ASSERT(Filter->FilterObject != NULL);
		ASSERT(Filter->FilterObject->Probes != NULL);
		ASSERT(Filter->PendingBitMap != NULL);
		ASSERT(Filter->CommitBitMap != NULL);

		BspFree(Filter->FilterObject->Probes);
		BspFree(Filter->FilterObject);

		BspFree(Filter->PendingBitMap->Buffer);
		BspFree(Filter->CommitBitMap->Buffer);
	}

	BspFree(Filter);
}

VOID
PspFreeProcess(
	IN PPSP_PROCESS Process
	)
{
	PPSP_PROBE Probe;
	PPSP_FILTER AddOn;
	PLIST_ENTRY ListEntry;

	while (IsListEmpty(&Process->ExpressList) != TRUE) {
		ListEntry = RemoveHeadList(&Process->ExpressList);
		Probe = CONTAINING_RECORD(ListEntry, PSP_PROBE, Entry);
		PspFreeProbe(Probe);
	}

	while (IsListEmpty(&Process->AddOnList) != TRUE) {
		ListEntry = RemoveHeadList(&Process->AddOnList);
		AddOn = CONTAINING_RECORD(ListEntry, PSP_FILTER, ListEntry);
		PspFreeFilter(AddOn);
	}
}

//
// This routine is called to launch dprobe process in analysis mode,
// analysis mode only provide analysis features like view data record,
// compute statistics etc, can't do any probe action.
//

ULONG
PspLaunchQueryMode(
	IN PWSTR FullPath
	)
{
	BOOL Status;
	STARTUPINFO si;
    PROCESS_INFORMATION pi;
	HANDLE ModuleHandle;
	WCHAR Module[MAX_PATH];
	WCHAR CommandLine[MAX_PATH * 2];

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	ModuleHandle = GetModuleHandle(NULL);
	GetModuleFileName(ModuleHandle, Module, MAX_PATH);

	wsprintf(CommandLine, L"%ws -analysis %ws", Module, FullPath);
	Status = CreateProcess(NULL,
		                   CommandLine, 
						   NULL, 
						   NULL, 
						   FALSE, 
						   0, 
						   NULL, 
						   NULL, 
						   &si, 
						   &pi);

	if (Status != TRUE) {
		MessageBox(NULL, 
			       L"Failed to launch dprobe (analysis mode).", 
				   L"D Probe", 
				   MB_OK | MB_ICONERROR);

		return GetLastError();
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return ERROR_SUCCESS;
}

PBTR_RECORD_HEADER
PspGetRecordBase(
	IN PPSP_RECORD Record
	)
{
	return (PBTR_RECORD_HEADER)((PCHAR)Record + FIELD_OFFSET(PSP_RECORD, Data));
}

PBTR_FILTER
PspGetFilterRegistration(
	IN PWSTR FilterName
	)
{
	return NULL;
}

PPSP_FILTER
PspCreateAddOnObject(
	VOID
	)
{
	PPSP_FILTER Object;

	Object = (PPSP_FILTER)BspMalloc(sizeof(PSP_FILTER));
	Object->Registered = FALSE;
	Object->FilterObject = NULL;
	Object->PendingBitMap = NULL;
	Object->CommitBitMap = NULL;
	Object->State = FilterCreating;

	InitializeListHead(&Object->ListEntry);
	return Object;
}

VOID
PspCopyAddOnBitMap(
	IN PVOID Destine,
	IN PVOID Source
	)
{
	RtlCopyMemory(Destine, Source, PSP_BITMAP_SIZE);
}

PPSP_FILTER
PspLoadAddOn(
	IN PWSTR Path
	)
{
	HMODULE ModuleHandle;
	PBTR_FILTER BfrObject;
	BTR_FLT_GETOBJECT ExportApi;
	PPSP_FILTER FltObject;

	ModuleHandle = LoadLibrary(Path);
	if (ModuleHandle == NULL) {
		return NULL;
	}

	ExportApi = (BTR_FLT_GETOBJECT)GetProcAddress(ModuleHandle, 
		                                          "BtrFltGetObject");
	if (ExportApi == NULL) {
		FreeLibrary(ModuleHandle);
		return NULL;
	}
	
	__try {
		
		BfrObject = (*ExportApi)();
		if (BfrObject == NULL) {
			return NULL;
		}
	} 
	__except(EXCEPTION_EXECUTE_HANDLER) {
		return NULL;
	}

	//
	// N.B. Because we only use global addon list for decoding purpose,
	// we don't care other fields of filter objects, only the DecodeCallback
	// is required.
	//

	FltObject = PspCreateAddOnObject();
	FltObject->FilterObject = BfrObject;
	
	return FltObject;
}

ULONG
PspBuildAddOnList(
	IN PWSTR Path	
	)
{
	ULONG Status;
	WIN32_FIND_DATA Data;
	HANDLE FindHandle;
	WCHAR Buffer[MAX_PATH];
	WCHAR FullPathName[MAX_PATH];
	PPSP_FILTER FilterObject;
	
	Status = FALSE;
	InitializeListHead(&PspAddOnList);

	wcsncpy(Buffer, Path, MAX_PATH);
	wcscat(Buffer, L"\\*.flt");

	FindHandle = FindFirstFile(Buffer, &Data);
	if (FindHandle != INVALID_HANDLE_VALUE) {
		Status = TRUE;
	}

	while (Status) {
		
		wcsncpy(FullPathName, Path, MAX_PATH);
		wcsncat(FullPathName, L"\\", MAX_PATH);
		wcsncat(FullPathName, Data.cFileName, MAX_PATH);

		FilterObject = PspLoadAddOn(FullPathName);
		if (FilterObject != NULL) {
			InsertTailList(&PspAddOnList, &FilterObject->ListEntry);
		}

		Status = (ULONG)FindNextFile(FindHandle, &Data);
	}

	return 0;
}

PPSP_FILTER
PspQueryGlobalAddOnObject(
	IN PWSTR Path
	)
{
	PPSP_FILTER AddOn;
	PLIST_ENTRY ListEntry;

	ListEntry = PspAddOnList.Flink;
	while (ListEntry != &PspAddOnList) {
		AddOn = CONTAINING_RECORD(ListEntry, PSP_FILTER, ListEntry);
		if (_wcsicmp(AddOn->FilterObject->FilterFullPath, Path) == 0) {
			return AddOn;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

PBTR_FILTER
PspLookupAddOnByTag(
	IN PBTR_FILTER_RECORD Record
	)
{
	PLIST_ENTRY ListEntry;
	PPSP_FILTER AddOn;

	ASSERT(Record != NULL);
	ASSERT(Record->Base.RecordType == RECORD_FILTER);

		
	ListEntry = PspAddOnList.Flink;

	while (ListEntry != &PspAddOnList) {

		GUID Guid;
		AddOn = CONTAINING_RECORD(ListEntry, PSP_FILTER, ListEntry);
		ASSERT(AddOn->FilterObject != NULL);

		Guid = AddOn->FilterObject->FilterGuid;
		if (InlineIsEqualGUID(&Record->FilterGuid, &Guid)) {
		//if (Record->Base.RecordTag == AddOn->FilterObject->FilterTag) {
			return AddOn->FilterObject;
		}

		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

BOOLEAN
PspIsProbing(
	VOID
	)
{
	BOOLEAN Status;

	PspLockProcessList();
	Status = IsListEmpty(&PspProcessList);
	PspUnlockProcessList();

	return !Status;
}

ULONG
PspGetRunningManualProbeCount(
	IN PPSP_PROCESS Process
	)
{
	ULONG Count;
	PLIST_ENTRY ListEntry;
	PPSP_PROBE Probe;

	Count = 0;
	ListEntry = Process->ExpressList.Flink;

	while (ListEntry != &Process->ExpressList) {

		Probe = CONTAINING_RECORD(ListEntry, PSP_PROBE, Entry);
		if (Probe->Current == ProbeRunning) {
			Count += 1;
		}

		ListEntry = ListEntry->Flink;
	}

	return Count;
}

ULONG
PspStopAllSession(
	VOID
	)
{
	return S_FALSE;
}

ULONG
PspGetRunningAddOnCount(
	IN PPSP_PROCESS Process
	)
{
	ULONG Count;
	PLIST_ENTRY ListEntry;
	PPSP_FILTER AddOn;
	BOOLEAN Clear;

	Count = 0;
	ListEntry = Process->AddOnList.Flink;

	while (ListEntry != &Process->AddOnList) {

		AddOn = CONTAINING_RECORD(ListEntry, PSP_FILTER, ListEntry);
		if (AddOn->State == FilterRunning) {
			Clear = BtrAreAllBitsClear(AddOn->CommitBitMap);
			if (Clear != TRUE) {
				Count += 1;
			}
		}

		ListEntry = ListEntry->Flink;
	}

	return Count;
}

BOOLEAN
PspQueryUserModeRuntime(
	IN ULONG ProcessId
	)
{
	LIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;

	InitializeListHead(&ListHead);
	BspQueryModule(ProcessId, FALSE, &ListHead);

	while (IsListEmpty(&ListHead) != TRUE) {

		ListEntry = RemoveHeadList(&ListHead);
	}
	
	return TRUE;
}

PDTL_OBJECT
PspGetDtlObject(
	IN PPSP_PORT Port
	)
{
	return PspDtlObject;
}

VOID
PspSetDtlObject(
	IN PDTL_OBJECT Object
	)
{
	if (PspDtlObject) {
		DtlDestroyObject(PspDtlObject);
	}
	PspDtlObject = Object;
}

VOID
PspClearDtlObject(
	VOID
	)
{
	if (PspDtlObject) {
		DtlDestroyObject(PspDtlObject);
		PspDtlObject = NULL;
	}
}

ULONG
PspCreateDtlObject(
	VOID
	)
{
	WCHAR Path[MAX_PATH];

	PspGetFilePath(Path, MAX_PATH);
	PspDtlObject = DtlCreateObject(Path);
	if (PspDtlObject != NULL) {
		return S_OK;
	}

	return S_FALSE;
}

ULONG
PspCreateProcessWithDpc(
	IN PWSTR ImagePath,
	IN PWSTR Argument,
	IN PWSTR DpcPath
	)
{
	BOOL Status;
	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi = {0};
	WCHAR Buffer[MAX_PATH * 2];
	WCHAR Command[MAX_PATH * 2];
	WCHAR Folder[MAX_PATH];
	WCHAR Directory[MAX_PATH];
	WCHAR ImageName[MAX_PATH];
	WCHAR ExtendedName[MAX_PATH];
	HANDLE CompleteEvent;
	HANDLE SuccessEvent;
	HANDLE ErrorEvent;
	HANDLE Handle[2];
	ULONG Count;

	si.cb = sizeof(si);

	if (wcslen(Argument)) {
		wsprintf(Command, L"%ws %ws", ImagePath, Argument);
	} else {
		wsprintf(Command, L"%ws", ImagePath);
	}

	//
	// Set work directory as image's path
	//

	CompleteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	SuccessEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	ErrorEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	Handle[0] = SuccessEvent;
	Handle[1] = ErrorEvent;

	_wsplitpath(ImagePath, Directory, Folder, NULL, NULL);
	StringCchCat(Directory, MAX_PATH, Folder);

	Status = CreateProcess(NULL,
		                   Command, 
						   NULL, 
						   NULL, 
						   FALSE, 
						   DEBUG_ONLY_THIS_PROCESS | CREATE_SUSPENDED, 
						   NULL, 
						   Directory, 
						   &si, 
						   &pi);

	if (Status != TRUE) {
		Status = GetLastError();
		StringCchPrintf(Buffer, MAX_PATH * 2, L"Failed to run %ws %ws, error = 0x%08x.",
			            ImagePath, Argument, Status);
		return Status;
	}

	ResumeThread(pi.hThread);
	BspInjectPreExecute(pi.dwProcessId, pi.hProcess, pi.hThread, ImagePath, 
		                CompleteEvent, SuccessEvent, ErrorEvent);

	Status = S_OK;
	Count = 0;

	//
	// Wait until it's either success or failed
	//

	Status = WaitForMultipleObjects(2, Handle, FALSE, INFINITE);
	if (Status == (WAIT_OBJECT_0 + 1)) {

		//
		// ErrorEvent is signaled, terminate the process
		//

		TerminateProcess(pi.hProcess, 0);
		CloseHandle(Handle[0]);
		CloseHandle(Handle[1]);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return S_FALSE;
	}

	//
	// Failed to call BtrFltStartRuntime, terminate it
	//

	Status = PspStartProbeRuntime(pi.dwProcessId);
	if (Status != S_OK) {
		TerminateProcess(pi.hProcess, 0);
		CloseHandle(Handle[0]);
		CloseHandle(Handle[1]);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return S_FALSE;
	}

	//
	// Import dpc without pop up dialog box
	//

	_wsplitpath(ImagePath, NULL, NULL, ImageName, ExtendedName);
	StringCchCat(ImageName, MAX_PATH, ExtendedName);
	Status = PspImportDpc(pi.dwProcessId, ImageName, DpcPath);

	if (!Status) {
		TerminateProcess(pi.hProcess, 0);
	} else {
		SetEvent(CompleteEvent);
	}

	CloseHandle(Handle[0]);
	CloseHandle(Handle[1]);
	CloseHandle(CompleteEvent);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return ERROR_SUCCESS;
}

ULONG
PspImportDpc(
	IN ULONG ProcessId,
	IN PWSTR ImageName,
	IN PWSTR DpcPath 
	)
{
	BOOLEAN Found = FALSE;
	return Found;
}

PDTL_OBJECT PspDtlObject;
PBSP_QUEUE PspQueueObject;