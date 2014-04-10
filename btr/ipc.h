//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _IPC_H_
#define _IPC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "btr.h"
#include "filter.h"
#include "bitmap.h"

ULONG
BtrCreatePort(
	IN PBTR_PORT Port	
	);

ULONG
BtrAcceptPort(
	IN PBTR_PORT Port	
	);

ULONG
BtrDisconnectPort(
	IN PBTR_PORT Port	
	);

ULONG
BtrClosePort(
	IN PBTR_PORT Port	
	);

ULONG
BtrWaitPortIoComplete(
	IN PBTR_PORT Port,
	IN ULONG Milliseconds,
	OUT PULONG CompleteBytes
	);

ULONG
BtrGetMessage(
	IN PBTR_PORT Port,
	OUT PBTR_MESSAGE_HEADER Packet,
	IN ULONG BufferLength,
	OUT PULONG CompleteBytes
	);

ULONG
BtrSendMessage(
	IN PBTR_PORT Port,
	IN PBTR_MESSAGE_HEADER PacketHeader,
	OUT PULONG CompleteBytes 
	);

ULONG
BtrOnRequest(
	IN PBTR_MESSAGE_HEADER Packet
	);

ULONG
BtrOnCacheRequest(
	IN PBTR_MESSAGE_CACHE Packet
	);

ULONG
BtrOnRecordRequest(
	IN PBTR_MESSAGE_RECORD Packet 
	);

ULONG
BtrOnFastRequest(
	IN PBTR_MESSAGE_FAST Packet 
	);

ULONG
BtrOnStackTraceRequest(
	IN PBTR_MESSAGE_STACKTRACE Packet
	);

ULONG
BtrOnFilterRequest(
	IN PBTR_MESSAGE_FILTER Packet 
	);

ULONG
BtrOnFilterControl(
	IN PBTR_MESSAGE_FILTER_CONTROL Packet
	);

ULONG
BtrOnStopRequest(
	IN PBTR_MESSAGE_STOP Packet
	);

ULONG
BtrOnUserCommand(
	IN PBTR_MESSAGE_USER_COMMAND Packet
	);

ULONG CALLBACK
BtrMessageProcedure(
	IN PVOID Context
	);

ULONG
BtrInitializeIpc(
	VOID
	);

ULONG
BtrSerializeRecords(
	IN PVOID Buffer,
	IN ULONG BufferLength,
	OUT PULONG Length,
	OUT PULONG Count 
	);

ULONG
BtrFilterControl(
	IN HWND hWndOption,
	IN PWSTR FilterFullPath,
	IN PVOID InputBuffer,
	IN ULONG InputBufferLength,
	OUT PVOID *OutputBuffer,
	OUT ULONG *OutputBufferLength
	);

extern HANDLE BtrMessageProcedureHandle;
extern BTR_PORT BtrPort;

#ifdef __cplusplus
}
#endif
#endif  // _IPC_H_

