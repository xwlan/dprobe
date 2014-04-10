//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _MSPPORT_H_
#define _MSPPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

struct _MSP_PROCESS;

typedef struct _MSP_MESSAGE_PORT {
	LIST_ENTRY ListEntry;
	WCHAR Name[64];
	ULONG ProcessId;
	ULONG ThreadId;
	HANDLE Object;
	PVOID Buffer;
	ULONG BufferLength;
	HANDLE CompleteEvent; 
	OVERLAPPED Overlapped;
	struct _MSP_PROCESS *Process;
} MSP_MESSAGE_PORT, *PMSP_MESSAGE_PORT;

PMSP_MESSAGE_PORT
MspCreatePort(
	IN struct _MSP_PROCESS *Process,
	IN ULONG BufferLength
	);

ULONG
MspConnectPort(
	IN PMSP_MESSAGE_PORT Port
	);

ULONG
MspClosePort(
	IN PMSP_MESSAGE_PORT Port	
	);

ULONG
MspGetMessage(
	IN PMSP_MESSAGE_PORT Port,
	OUT PBTR_MESSAGE_HEADER Packet,
	IN ULONG BufferLength,
	OUT PULONG CompleteBytes
	);

ULONG
MspSendMessage(
	IN PMSP_MESSAGE_PORT Port,
	IN PBTR_MESSAGE_HEADER Packet,
	OUT PULONG CompleteBytes 
	);

ULONG
MspWaitPortIoComplete(
	IN PMSP_MESSAGE_PORT Port,
	IN ULONG Milliseconds,
	OUT PULONG CompleteBytes
	);

#ifdef __cplusplus
}
#endif
#endif