//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2010
//

#ifndef _MSP_H_
#define _MSP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

#include "btr.h"

#define MSP_PORT_SIGNATURE      L"dprobe"
#define MSP_PORT_BUFFER_LENGTH  1024 * 1024 * 4

typedef enum _MSP_PORT_STATE {
	MspPortConnected,
	MspPortClosed
} MSP_PORT_STATE, *PMSP_PORT_STATE;

typedef struct _MSP_PORT {
	ULONG ProcessId;
	ULONG ThreadId;
	HANDLE PortHandle;
	HANDLE CompleteHandle;
	HANDLE TimerHandle;
	ULONG TimerPeriod;
	MSP_PORT_STATE State;
	PVOID Buffer;
	ULONG BufferLength;
	OVERLAPPED Overlapped;
} MSP_PORT, *PMSP_PORT;

ULONG
MspConnectPort(
	IN ULONG ProcessId,
	IN ULONG BufferLength,
	IN ULONG Timeout,
	OUT PMSP_PORT Port
	);

ULONG
MspWaitPortIoComplete(
	IN PMSP_PORT Port,
	IN OVERLAPPED *Overlapped,
	IN ULONG Milliseconds,
	OUT PULONG CompleteBytes
	);

ULONG
MspDisconnectPort(
	IN PMSP_PORT Port
	);

ULONG
MspSendMessage(
	IN PMSP_PORT Port,
	IN PBTR_MESSAGE_HEADER Message,
	IN OVERLAPPED *Overlapped,
	OUT PULONG CompleteBytes 
	);

ULONG
MspWaitMessage(
	IN PMSP_PORT Port,
	OUT PBTR_MESSAGE_HEADER Message,
	IN ULONG BufferLength,
	OUT PULONG CompleteBytes,
	IN OVERLAPPED *Overlapped
	);

ULONG
MspAuditMessage(
	IN PBTR_MESSAGE_HEADER Header,
	IN BTR_MESSAGE_TYPE Type,
	IN ULONG MessageLength
	);

ULONG
MspSendMessageWaitReply(
	IN PMSP_PORT Port,
	IN PBTR_MESSAGE_HEADER Message,
	OUT PBTR_MESSAGE_HEADER Reply
	);

#ifdef __cplusplus
}
#endif

#endif