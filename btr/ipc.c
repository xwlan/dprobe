//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "btr.h"
#include "ipc.h"
#include "hal.h"
#include "util.h"
#include "thread.h"
#include "heap.h"
#include "probe.h"
#include "queue.h"
#include "sched.h"
#include "cache.h"
#include "counter.h"

BTR_PORT BtrPort;

VOID 
BtrInitOverlapped(
	VOID
	)
{
	ASSERT(BtrPort.CompleteEvent != NULL);
	RtlZeroMemory(&BtrPort.Overlapped, sizeof(OVERLAPPED));
	BtrPort.Overlapped.hEvent = BtrPort.CompleteEvent;
}

ULONG
BtrCreatePort(
	IN PBTR_PORT Port
	)
{
	PVOID Buffer;
	HANDLE PortObject;
	HANDLE CompleteEventObject;
	WCHAR NameBuffer[MAX_PATH];

	HalSnwprintf(NameBuffer, MAX_PATH, L"\\\\.\\pipe\\btr\\%u", GetCurrentProcessId());
	StringCchCopy(Port->Name, 64, NameBuffer);

	PortObject = CreateNamedPipe(NameBuffer,
							     PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, 
				                 PIPE_READMODE_MESSAGE | PIPE_TYPE_MESSAGE | PIPE_WAIT,
							     PIPE_UNLIMITED_INSTANCES,
				                 Port->BufferLength,
				                 Port->BufferLength,
				                 NMPWAIT_USE_DEFAULT_WAIT,
							     NULL);

	if (PortObject == NULL) {
		return S_FALSE;
	}
    
	CompleteEventObject = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (CompleteEventObject == NULL) {
		CloseHandle(PortObject);
		return S_FALSE;
	}

	Buffer = VirtualAlloc(NULL, Port->BufferLength, MEM_COMMIT, PAGE_READWRITE);
	if (Buffer == NULL) {
		CloseHandle(PortObject);
		CloseHandle(CompleteEventObject);
		return S_FALSE;
	}
	
	BtrPort.Object = PortObject;
	BtrPort.Buffer = Buffer;
	BtrPort.BufferLength = Port->BufferLength;
	BtrPort.CompleteEvent = CompleteEventObject;

	BtrInitOverlapped();
	return S_OK;
}

ULONG
BtrWaitPortIoComplete(
	IN PBTR_PORT Port,
	IN ULONG Milliseconds,
	OUT PULONG CompleteBytes
	)
{
	ULONG Status;

	Status = WaitForSingleObject(BtrPort.Overlapped.hEvent, Milliseconds);

	if (Status != WAIT_OBJECT_0) {
		DebugTrace("BtrWaitForPortIoComplete gle=0x%08x", Status);
		CancelIo(BtrPort.Overlapped.hEvent);
		return Status;
	}

	Status = GetOverlappedResult(BtrPort.Object, 
								 &BtrPort.Overlapped,
                                 CompleteBytes,
						         FALSE);
	if (Status != TRUE) {
		Status = GetLastError();
		DebugTrace("BtrWaitForPortIoComplete GetOverlappedResult gle=0x%08x", Status);
		return Status;
	}
	
	return S_OK;
}

ULONG
BtrAcceptPort(
	IN PBTR_PORT Port
	)
{
	BOOLEAN Status;
	ULONG Result;
	ULONG CompleteBytes;

	BtrInitOverlapped();
	Status = ConnectNamedPipe(BtrPort.Object, &BtrPort.Overlapped);

	if (Status != TRUE) {

		switch (GetLastError()) {
			case ERROR_IO_PENDING:
				Result = BtrWaitPortIoComplete(&BtrPort, INFINITE, &CompleteBytes);
				break;

			case ERROR_PIPE_CONNECTED:
				Result = S_OK;
				break;

			default:
				Result = S_FALSE;
		}
		return Result;
	} 
	
	return S_OK;
}

ULONG
BtrClosePort(
	IN PBTR_PORT Port
	)
{
	FlushFileBuffers(BtrPort.Object);
	DisconnectNamedPipe(BtrPort.Object);
	CloseHandle(BtrPort.Object);
	CloseHandle(BtrPort.CompleteEvent);
	VirtualFree(BtrPort.Buffer, 0, MEM_RELEASE);
	return S_OK;
}

ULONG
BtrDisconnectPort(
	IN PBTR_PORT Port
	)
{
	DisconnectNamedPipe(BtrPort.Object);
	return S_OK;
}

ULONG
BtrGetMessage(
	IN PBTR_PORT Port,
	OUT PBTR_MESSAGE_HEADER Packet,
	IN ULONG BufferLength,
	OUT PULONG CompleteBytes
	)
{
	BOOLEAN Status;
	ULONG ErrorCode;
	ULONG Result;

	if (BufferLength > BtrPort.BufferLength) {
		return S_FALSE;
	}

	BtrInitOverlapped();

	Status = ReadFile(BtrPort.Object, Packet, BufferLength, CompleteBytes, &BtrPort.Overlapped);
	if (Status != TRUE) {

		ErrorCode = GetLastError();
		if (ErrorCode != ERROR_IO_PENDING) {
			DebugTrace("BtrGetMessage gle=0x%08x", ErrorCode);
			return S_FALSE;
		}

		Result = BtrWaitPortIoComplete(&BtrPort, INFINITE, CompleteBytes);
		return Result;
	} 
	
	return S_OK;
}

ULONG
BtrSendMessage(
	IN PBTR_PORT Port,
	IN PBTR_MESSAGE_HEADER Packet,
	OUT PULONG CompleteBytes 
	)
{
	BOOLEAN Status;
	ULONG Result;
	ULONG ErrorCode;

	if (Packet->Length > BtrPort.BufferLength) {
		return S_FALSE;
	}

	BtrInitOverlapped();

	Status = WriteFile(BtrPort.Object, Packet, Packet->Length, CompleteBytes, &BtrPort.Overlapped);
	if (Status != TRUE) {
		
		ErrorCode = GetLastError();
		if (ErrorCode != ERROR_IO_PENDING) {
			DebugTrace("BtrSendMessage gle=0x%08x", ErrorCode);
			return ErrorCode;
		}

	    Result = BtrWaitPortIoComplete(&BtrPort, INFINITE, CompleteBytes);
		return Result;
	} 
	
	return S_OK;
}

ULONG
BtrOnRequest(
	IN PBTR_MESSAGE_HEADER Packet
	)
{
	ULONG Status;	

	switch (Packet->PacketType) {

		case MESSAGE_CACHE:
			Status = BtrOnCacheRequest((PBTR_MESSAGE_CACHE)Packet);
			break;

		case MESSAGE_FAST:
			Status = BtrOnFastRequest((PBTR_MESSAGE_FAST)Packet);
			break;

		case MESSAGE_FILTER:
			Status = BtrOnFilterRequest((PBTR_MESSAGE_FILTER)Packet);
			break;
		
		case MESSAGE_RECORD:
			Status = BtrOnRecordRequest((PBTR_MESSAGE_RECORD)Packet);
			break;
	
		case MESSAGE_FILTER_CONTROL:
			Status = BtrOnFilterControl((PBTR_MESSAGE_FILTER_CONTROL)Packet);
			break;

		case MESSAGE_STACKTRACE:
			Status = BtrOnStackTraceRequest((PBTR_MESSAGE_STACKTRACE)Packet);
			break;

		case MESSAGE_USER_COMMAND:
			Status = BtrOnUserCommand((PBTR_MESSAGE_USER_COMMAND)Packet);
			break;

		default:
			Status = S_OK;
	}

	return Status;	
}

ULONG
BtrOnCacheRequest(
	IN PBTR_MESSAGE_CACHE Packet
	)
{
	BTR_MESSAGE_ACK Ack;
	ULONG CompleteBytes;
	ULONG Status;

	Status = BtrConfigureCache(Packet);

	Ack.Header.PacketType = MESSAGE_CACHE_ACK;
	Ack.Header.Length = sizeof(BTR_MESSAGE_ACK);
	Ack.Status = Status;
	Ack.Information = 0;

	Status = BtrSendMessage(&BtrPort, &Ack.Header, &CompleteBytes);	
	return Status;
}

ULONG
BtrOnRecordRequest(
	IN PBTR_MESSAGE_RECORD Record
	)
{
	ULONG Status;
	ULONG CompleteBytes;
	PBTR_MESSAGE_RECORD_ACK Packet;

	Packet = (PBTR_MESSAGE_RECORD_ACK)BtrPort.Buffer;
	Packet->Header.PacketType = MESSAGE_RECORD_ACK;
	Packet->Header.Length = sizeof(BTR_MESSAGE_RECORD_ACK);
	Packet->NumberOfRecords = 0;
	Packet->PacketFlag = 0;

	Status = BtrSendMessage(&BtrPort, &Packet->Header, &CompleteBytes); 
	return Status; 
}

ULONG
BtrOnStackTraceRequest(
	IN PBTR_MESSAGE_STACKTRACE Packet
	)
{
	ULONG Status;
	ULONG CompleteBytes;
	ULONG NumberOfTraces;
	PUCHAR Buffer;
	LONG Length;
	PBTR_MESSAGE_STACKTRACE_ACK Ack;

	Ack = (PBTR_MESSAGE_STACKTRACE_ACK)Packet;
	Ack->Header.PacketType = MESSAGE_STACKTRACE_ACK;

	Length = FIELD_OFFSET(BTR_MESSAGE_STACKTRACE_ACK, Entries);
	Buffer = (PUCHAR)BtrPort.Buffer + Length;
	Length = BtrPort.BufferLength - Length;

	Status = BtrDeQueueStackTrace(Buffer, Length, &Length, &NumberOfTraces);
	Ack->Status = Status;
	Ack->NumberOfTraces = NumberOfTraces;
	Ack->Header.Length = FIELD_OFFSET(BTR_MESSAGE_STACKTRACE_ACK, Entries[NumberOfTraces]);

	Status = BtrSendMessage(&BtrPort, &Ack->Header, &CompleteBytes);	
	if (Status != ERROR_SUCCESS) {
		return Status;
	}

	return Status;	
}

ULONG
BtrOnFastRequest(
	IN PBTR_MESSAGE_FAST Packet
	)
{
	ULONG Status;
	ULONG CompleteBytes;
	ULONG Number;
	ULONG Length;
	ULONG ObjectId;
	ULONG Counter;
	PBTR_MESSAGE_FAST_ACK Ack;
	
	Length = Packet->Header.Length;

	for (Number = 0; Number < Packet->Count; Number += 1) {
		Status = BtrRegisterFastProbe(Packet->Entry[Number].Address, 
			                          Packet->Activate, &ObjectId, &Counter);
		Packet->Entry[Number].Status = Status;
		Packet->Entry[Number].ObjectId = ObjectId;
		Packet->Entry[Number].Counter = Counter;
	}

	Ack = (PBTR_MESSAGE_FAST_ACK)Packet;
	Ack->Header.PacketType = MESSAGE_FAST_ACK;
	Ack->Header.Length = Length;
	Status = BtrSendMessage(&BtrPort, &Ack->Header, &CompleteBytes);
	return Status;	
}

ULONG
BtrOnUserCommand(
	IN PBTR_MESSAGE_USER_COMMAND Packet
	)
{
	ULONG Status;
	ULONG Number;
	ULONG CompleteBytes;
	PBTR_MESSAGE_USER_COMMAND_ACK Ack;
	PBTR_USER_COMMAND Command;
	PVOID Address;

	//
	// MappingObject is duplicated into current address space by MSP
	//

	Address = MapViewOfFile(Packet->MappingObject, 
		                    FILE_MAP_READ|FILE_MAP_WRITE, 
							0, 0, 0);

	Command = (PBTR_USER_COMMAND)Address;		
	if (!Command) {
		Ack = (PBTR_MESSAGE_USER_COMMAND_ACK)Packet;
		Ack->Header.PacketType = MESSAGE_USER_COMMAND_ACK;
		Ack->Header.Length = sizeof(BTR_MESSAGE_USER_COMMAND_ACK);
		Ack->Status = BTR_E_MAPVIEWOFFILE;
		Status = BtrSendMessage(&BtrPort, &Ack->Header, &CompleteBytes);
		return Status;
	}

	for(Number = 0; Number < Packet->NumberOfCommands; ) {
		
		if (Command->Type == PROBE_FAST) {
			BtrRegisterFastProbeEx(Command);
		}
		
		if (Command->Type == PROBE_FILTER) {
			BtrRegisterFilterProbeEx(Command);
		}

		Command = (PBTR_USER_COMMAND)((PUCHAR)Command + Command->Length);
		Number += 1;
	}

	//
	// Close the duplicated handle
	//

	UnmapViewOfFile(Address);
	CloseHandle(Packet->MappingObject);

	Ack = (PBTR_MESSAGE_USER_COMMAND_ACK)Packet;
	Ack->Header.PacketType = MESSAGE_USER_COMMAND_ACK;
	Ack->Header.Length = sizeof(BTR_MESSAGE_USER_COMMAND_ACK);
	Ack->Status = S_OK;
	Status = BtrSendMessage(&BtrPort, &Ack->Header, &CompleteBytes);
	return Status;
}

ULONG 
BtrRegisterFilterProbeEx(
	IN PBTR_USER_COMMAND Command
	)
{
	ULONG Status;
	ULONG Number;
	PBTR_FILTER Filter;
	PVOID Address;
	ULONG ObjectId;
	ULONG Counter;

	Filter = BtrQueryFilter(Command->DllName);
	if (!Filter) {
		Status = BtrCreateFilter(Command->DllName, &Filter);
		if (Status != S_OK) {
			Command->Status = Status;
			return S_OK;
		} 
	}
	
	for(Number = 0; Number < Command->Count; Number += 1) {

		Status = BtrRegisterFilterProbe(Filter, Command->Probe[Number].Number, 
										Command->Probe[Number].Activate, &Address,
										&ObjectId,
										&Counter);

		Command->Probe[Number].Address = Address;
		Command->Probe[Number].Status = Status;
		Command->Probe[Number].ObjectId = ObjectId;
		Command->Probe[Number].Counter = Counter;

		if (Status == S_OK) {
			ASSERT(Counter != -1);
		}
	}

	Command->Status = S_OK;
	return S_OK;
}

ULONG
BtrRegisterFastProbeEx(
	IN PBTR_USER_COMMAND Command
	)
{
	ULONG Status;
	ULONG Number;
	HMODULE DllHandle;
	PVOID Address;
	ULONG ObjectId;
	ULONG Counter;

	DllHandle = GetModuleHandle(Command->DllName);
	if (!DllHandle) {
		Command->Status = BTR_E_GETMODULEHANDLE;
		return S_OK;
	}

	for(Number = 0; Number < Command->Count; Number += 1) {

		Address = GetProcAddress(DllHandle, Command->Probe[Number].ApiName);

		if (Address != NULL) {

			Status = BtrRegisterFastProbe(Address, 
				                          Command->Probe[Number].Activate,
										  &ObjectId,
										  &Counter);

			Command->Probe[Number].Address = Address;
			Command->Probe[Number].Status = Status;
			Command->Probe[Number].ObjectId = ObjectId;
			Command->Probe[Number].Counter = Counter;

			if (Status != S_OK) {
				ASSERT(Counter == -1);
			}

			if (Status == S_OK) {
				ASSERT(Counter != -1);
			}
		}

		else {
			Command->Probe[Number].Address = NULL;
			Command->Probe[Number].ObjectId = 0;
			Command->Probe[Number].Status = BTR_E_GETPROCADDRESS;
			Command->Probe[Number].Counter = -1;
		}
	}

	Command->Status = S_OK;
	return S_OK;
}

ULONG
BtrOnStopRequest(
	IN PBTR_MESSAGE_STOP Packet
	)
{
	ULONG Status;
	ULONG CompleteBytes;
	BTR_MESSAGE_ACK Ack;

	Status = S_OK;
	Ack.Header.PacketType = MESSAGE_ACK;
	Ack.Header.Length = sizeof(Ack);
	Ack.Status = 0;
	
	BtrSendMessage(&BtrPort, &Ack.Header, &CompleteBytes);

	BtrStopRuntime(ThreadUser);
	return Status;
}

ULONG
BtrOnFilterRequest(
	IN PBTR_MESSAGE_FILTER Packet
	)
{
	ULONG Status;
	ULONG CompleteBytes;
	PBTR_MESSAGE_FILTER_ACK Ack;
	PBTR_FILTER FilterObject;
	ULONG Number;
	ULONG ObjectId;
	ULONG Counter;

	Status = S_FALSE;

	FilterObject = BtrQueryFilter(Packet->Path);

	if (!FilterObject) {

		Status = BtrCreateFilter(Packet->Path, &FilterObject);
		if (Status != S_OK) {

			Ack = (PBTR_MESSAGE_FILTER_ACK)BtrPort.Buffer;
			Ack->Header.PacketType = MESSAGE_FILTER_ACK;
			Ack->Status = Status;
			Status = BtrSendMessage(&BtrPort, &Ack->Header, &CompleteBytes);
			return Status;
		} 

	} 

	for(Number = 0; Number < Packet->Count; Number += 1) {
		Status = BtrRegisterFilterProbe(FilterObject, Packet->Entry[Number].Number,
										Packet->Activate, &Packet->Entry[Number].Address,
										&ObjectId, &Counter);
		Packet->Entry[Number].Status = Status;
		Packet->Entry[Number].ObjectId = ObjectId;
		Packet->Entry[Number].Counter = Counter;
		
		if (Status == S_OK) {
			ASSERT(Counter != -1);
		}
	}

	//
	// N.B. Filter request's length does not change, we use same length
	// to ack the requestor.
	//

	Ack = (PBTR_MESSAGE_FILTER_ACK)BtrPort.Buffer;
	Ack->Header.PacketType = MESSAGE_FILTER_ACK;
	Ack->Status = S_OK;

	Status = BtrSendMessage(&BtrPort, &Ack->Header, &CompleteBytes);
	return Status;
}

ULONG
BtrOnFilterControl(
	IN PBTR_MESSAGE_FILTER_CONTROL Packet
	)
{
	ULONG Status;
	ULONG CompleteBytes;
	PBTR_MESSAGE_FILTER_CONTROL_ACK Ack;
	PBTR_FILTER FilterObject;
	WCHAR FilterName[MAX_PATH];
	PBTR_FILTER_CONTROL Control;
	PBTR_FILTER_CONTROL ControlAck;

	Status = S_FALSE;

	StringCchCopy(FilterName, MAX_PATH, Packet->FilterName);

	FilterObject = BtrQueryFilter(FilterName);

	//
	// Filter object does not exist
	//

	if (!FilterObject) {

		Ack = (PBTR_MESSAGE_FILTER_CONTROL_ACK)BtrPort.Buffer;
		RtlZeroMemory(Ack, sizeof(*Ack));
		Ack->Header.PacketType = MESSAGE_FILTER_CONTROL_ACK;
		Ack->Header.Length = sizeof(BTR_MESSAGE_FILTER_CONTROL_ACK);
		Ack->Status = BTR_E_NO_FILTER;
		StringCchCopy(Ack->FilterName, MAX_PATH, FilterName);

		Status = BtrSendMessage(&BtrPort, &Ack->Header, &CompleteBytes);
		return Status;
	}

	//
	// Filter object does not have a valid control callback routine
	//

	if (!FilterObject->ControlCallback) {
		
		Ack = (PBTR_MESSAGE_FILTER_CONTROL_ACK)BtrPort.Buffer;
		RtlZeroMemory(Ack, sizeof(*Ack));
		Ack->Header.PacketType = MESSAGE_FILTER_CONTROL_ACK;
		Ack->Header.Length = sizeof(BTR_MESSAGE_FILTER_CONTROL_ACK);
		Ack->Status = BTR_E_NO_FILTER_CONTROL;
		StringCchCopy(Ack->FilterName, MAX_PATH, FilterName);

		Status = BtrSendMessage(&BtrPort, &Ack->Header, &CompleteBytes);
	}

	//
	// Execute filter control callback
	//

	ControlAck = NULL;

	__try {
		
		Ack = (PBTR_MESSAGE_FILTER_CONTROL_ACK)BtrPort.Buffer;
		Ack->Header.PacketType = MESSAGE_FILTER_CONTROL_ACK;
		StringCchCopy(Ack->FilterName, MAX_PATH, FilterName);
		
		Control = (PBTR_FILTER_CONTROL)&Packet->Data[0];
		(*FilterObject->ControlCallback)(Control, &ControlAck);

		//
		// Check ControlAck packet size
		//

		if (ControlAck->Length > BTR_MAX_FILTER_CONTROL_SIZE) {
			Ack->Status = BTR_E_BAD_MESSAGE;
			Ack->Header.Length = sizeof(BTR_MESSAGE_FILTER_CONTROL_ACK);
		} 
		else {
			Ack->Header.Length = FIELD_OFFSET(BTR_MESSAGE_FILTER_CONTROL_ACK, Data[ControlAck->Length]);
			RtlCopyMemory(&Ack->Data[0], ControlAck, ControlAck->Length);
			Ack->Status = S_OK;
		}

	} 
	__except(EXCEPTION_EXECUTE_HANDLER) {
		Ack->Status = BTR_E_EXCEPTION;
		Ack->Header.Length = sizeof(BTR_MESSAGE_FILTER_CONTROL_ACK);
	}

	if (ControlAck != NULL) {
		BtrFltFree(ControlAck);
	}

	Status = BtrSendMessage(&BtrPort, &Ack->Header, &CompleteBytes);
	return Status;
}

ULONG
BtrFilterControl(
	IN HWND hWndOption,
	IN PWSTR FilterFullPath,
	IN PVOID InputBuffer,
	IN ULONG InputBufferLength,
	OUT PVOID *OutputBuffer,
	OUT ULONG *OutputBufferLength
	)
{
	return S_OK;
}

ULONG CALLBACK 
BtrMessageProcedure(
	IN PVOID Context
	)
{
	ULONG Status;
	BOOLEAN Stop;
	ULONG CompleteBytes;
	PBTR_MESSAGE_HEADER Packet;

	BtrInitializeRuntimeThread();

	RtlZeroMemory(&BtrPort, sizeof(BTR_PORT));
	BtrPort.BufferLength = BTR_PORT_BUFFER_LENGTH;

	Status = BtrCreatePort(&BtrPort);
	if (Status != S_OK) {
		return Status;
	}

	Stop = FALSE;

	while (Stop != TRUE) {

		Status = BtrAcceptPort(&BtrPort);
		if (Status != S_OK) {
			return Status;
		}
		
		Packet = (PBTR_MESSAGE_HEADER)BtrPort.Buffer;

		while (TRUE) {

			Status = BtrGetMessage(&BtrPort, Packet, BtrPort.BufferLength, &CompleteBytes);

			if (Status != S_OK) {
				CancelIo(BtrPort.Object);
				DebugTrace("BtrMessageProcedure: request stop!, status=%d", Status);
				BtrRequestStop(ThreadMessage);
				Stop = TRUE;
				break;
			}

			if (Packet->PacketType == MESSAGE_STOP_REQUEST) {
				BtrOnStopRequest((PBTR_MESSAGE_STOP)Packet);
				Stop = TRUE;
				break;
			}

			Status = BtrOnRequest(Packet);
		}
	}

	return 0;
}