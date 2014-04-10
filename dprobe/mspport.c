//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "mspdefs.h"
#include "mspport.h"
#include "msputility.h"
#include "mspprocess.h"
#include "mspdtl.h"

PMSP_MESSAGE_PORT
MspCreatePort(
	IN PMSP_PROCESS Process,
	IN ULONG BufferLength
	)
{
	PMSP_MESSAGE_PORT Port;
	HANDLE Handle;
	PVOID Buffer;

	Handle = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!Handle) {
		return NULL;
	}

	Buffer = VirtualAlloc(NULL, BufferLength, MEM_COMMIT, PAGE_READWRITE);
	if (!Buffer) {
		CloseHandle(Handle);
		return NULL;
	}

	Port = (PMSP_MESSAGE_PORT)MspMalloc(sizeof(MSP_MESSAGE_PORT));
	RtlZeroMemory(Port, sizeof(MSP_MESSAGE_PORT));

	Port->ProcessId = Process->ProcessId;
	StringCchPrintf(&Port->Name[0], 64, L"\\\\.\\pipe\\btr\\%u", Port->ProcessId);

	Port->Buffer = Buffer;
	Port->BufferLength = BufferLength;
	Port->CompleteEvent = Handle;
	Port->Overlapped.hEvent = Port->CompleteEvent;
	Port->Process = Process;

	return Port;
}

ULONG
MspConnectPort(
	IN PMSP_MESSAGE_PORT Port
	)
{
	HANDLE FileHandle;
	ULONG Status;

	while (TRUE) {

		FileHandle = CreateFile(Port->Name,
								GENERIC_READ|GENERIC_WRITE,
								0,
								NULL,
								OPEN_EXISTING,
								FILE_FLAG_OVERLAPPED,
								NULL);

		if (FileHandle != INVALID_HANDLE_VALUE){
			DebugTrace("MspConnectPort(pid=%u)", Port->ProcessId);
			break;
		}

		Status = GetLastError();
		if (Status != ERROR_PIPE_BUSY) {
			DebugTrace("MspConnectPort(), Error=%u", Status);
			return Status;
		}

		//
		// Timeout set as 10 seconds
		//

		if (!WaitNamedPipe(Port->Name, 1000 * 10)) { 
			Status = GetLastError();
			DebugTrace("MspConnectPort(), Error=%u", Status);
			return Status;
		} 
	}

	Port->Object = FileHandle;
	return MSP_STATUS_OK;
}

ULONG
MspWaitPortIoComplete(
	IN PMSP_MESSAGE_PORT Port,
	IN ULONG Milliseconds,
	OUT PULONG CompleteBytes
	)
{
	ULONG Status;

	Status = WaitForSingleObject(Port->Overlapped.hEvent, Milliseconds);

	if (Status != WAIT_OBJECT_0) {
		Status = GetLastError();
		MspWriteLogEntry(Port->ProcessId, LogLevelError, "WaitForSingleObject() failed, status=%u", Status);
		Status = MSP_STATUS_ERROR;
		return Status;
	}

	Status = GetOverlappedResult(Port->Object, 
								 &Port->Overlapped,
                                 CompleteBytes,
						         FALSE);
	if (Status != TRUE) {
		Status = GetLastError();
		MspWriteLogEntry(Port->ProcessId, LogLevelError, "GetOverlappedResult() failed, status=%u", Status);
		Status = MSP_STATUS_ERROR;
		return Status;
	}
	
	return MSP_STATUS_OK;
}

ULONG
MspClosePort(
	IN PMSP_MESSAGE_PORT Port	
	)
{
	CloseHandle(Port->Object);
	CloseHandle(Port->CompleteEvent);
	VirtualFree(Port->Buffer, 0, MEM_RELEASE);
	return MSP_STATUS_OK;
}

ULONG
MspGetMessage(
	IN PMSP_MESSAGE_PORT Port,
	OUT PBTR_MESSAGE_HEADER Packet,
	IN ULONG BufferLength,
	OUT PULONG CompleteBytes
	)
{
	ULONG Status;

	if (BufferLength > Port->BufferLength) {
		return MSP_STATUS_ERROR;
	}

	Status = (ULONG)ReadFile(Port->Object, Packet, BufferLength, CompleteBytes, &Port->Overlapped);
	if (Status != TRUE) {

		Status = GetLastError();
		if (Status != ERROR_IO_PENDING) {

			if (Status == ERROR_NO_DATA || Status == ERROR_PIPE_NOT_CONNECTED) {
				Status = MSP_STATUS_IPC_BROKEN;
			}

			MspWriteLogEntry(Port->ProcessId, LogLevelError, "ReadFile() failed, status=%u", GetLastError());
			return Status;
		}

		Status = MspWaitPortIoComplete(Port, INFINITE, CompleteBytes);
		return Status;
	} 
	
	return MSP_STATUS_OK;
}

ULONG
MspSendMessage(
	IN PMSP_MESSAGE_PORT Port,
	IN PBTR_MESSAGE_HEADER Packet,
	OUT PULONG CompleteBytes 
	)
{
	ULONG Status;

	if (Packet->Length > Port->BufferLength) {
		return MSP_STATUS_ERROR;
	}

	Status = (ULONG)WriteFile(Port->Object, Packet, Packet->Length, CompleteBytes, &Port->Overlapped);
	if (Status != TRUE) {
		
		Status = GetLastError();
		if (Status != ERROR_IO_PENDING) {
			if (Status == ERROR_NO_DATA || Status == ERROR_PIPE_NOT_CONNECTED) {
				Status = MSP_STATUS_IPC_BROKEN;
			}
			MspWriteLogEntry(Port->ProcessId, LogLevelError, "WriteFile() failed, status=%u", 
				             GetLastError());
			return Status;
		}

	    Status = MspWaitPortIoComplete(Port, INFINITE, CompleteBytes);
		return Status;
	} 
	
	return MSP_STATUS_OK;
}