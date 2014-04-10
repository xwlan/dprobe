//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _MSPQUEUE_H_
#define _MSPQUEUE_H_

#include "mspdefs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MSP_QUEUE {
	LIST_ENTRY ListEntry;
	HANDLE ObjectHandle;
} MSP_QUEUE, *PMSP_QUEUE;

typedef enum _MSP_PACKET_FLAG {

	MSP_PACKET_TIMER      = 0x00000001,
	MSP_PACKET_CONTROL    = 0x00000002,
	MPS_PACKET_COMMAND    = 0x00000004,
	MSP_PACKET_IO         = 0x00000008,

	MSP_PACKET_FREE       = 0x00000010,
	MSP_PACKET_WAIT       = 0x00000020,
	MSP_PACKET_CALLBACK   = 0x00000040,

} MSP_PACKET_FLAG, *PMSP_PACKET_FLAG;

typedef ULONG 
(CALLBACK *MSP_QUEUE_CALLBACK)(
	IN PVOID Packet,
	IN PVOID Context
	);

typedef struct _MSP_QUEUE_PACKET {
	LIST_ENTRY ListEntry;
	ULONG PacketFlag;
	ULONG ProcessId;
	PVOID Packet;
	HANDLE CompleteEvent;
	MSP_QUEUE_CALLBACK Callback;
	PVOID CallbackContext;
} MSP_QUEUE_PACKET, *PMSP_QUEUE_PACKET;

typedef enum _MSP_CONTROLCODE {
	MSP_START_TRACE      = 1,
	MSP_STOP_TRACE       = 2,
	MSP_BUILDIN_PROBE    = 3,
	MSP_FILTER_PROBE     = 4,
	MSP_FAST_PROBE    = 5,
	MSP_LARGE_PROBE      = 6,
} MSP_CONTROLCODE, *PMSP_CONTROLCODE;

typedef struct _MSP_FAST_ENTRY {
	PVOID Address;
	ULONG Status;
} MSP_FAST_ENTRY, *PMSP_FAST_ENTRY;

typedef struct _MSP_FILTER_ENTRY {
	USHORT Number;
	USHORT Spare;
	ULONG Status;
	PVOID Address;
} MSP_FILTER_ENTRY, *PMSP_FILTER_ENTRY;

typedef struct _MSP_CONTROL {

	MSP_CONTROLCODE Code;
	ULONG Length;
	ULONG Result;
	PVOID Context;

	union {

		struct {
			ULONG Flag;	
		} Start;

		struct {
			ULONG Flag;
		} Stop;

		struct {
			BOOLEAN Activate;
			BOOLEAN Spare[3];
			ULONG Count;
			WCHAR Path[MAX_PATH];
			MSP_FILTER_ENTRY Entry[ANYSIZE_ARRAY];
		} Filter;	

		struct {
			BOOLEAN Activate;
			BOOLEAN Spare[3];
			ULONG Count;
			MSP_FAST_ENTRY Entry[ANYSIZE_ARRAY];
		} Fast;	

		struct {
			HANDLE MappingObject;
			ULONG Length;
			ULONG NumberOfCommands;
		} Large;
	};

} MSP_CONTROL, *PMSP_CONTROL;


ULONG
MspCreateQueue(
	OUT PMSP_QUEUE *Object
	);

ULONG
MspCloseQueue(
	IN PMSP_QUEUE Object
	);

ULONG
MspQueuePacket(
	IN PMSP_QUEUE Object,
	IN PMSP_QUEUE_PACKET Notification
	);

ULONG
MspDeQueuePacket(
	IN PMSP_QUEUE Object,
	OUT PMSP_QUEUE_PACKET *Notification
	);

ULONG
MspDeQueuePacketEx(
	IN PMSP_QUEUE Object,
	IN ULONG Milliseconds,
	OUT PMSP_QUEUE_PACKET *Packet
	);

ULONG
MspDeQueuePacketList(
	IN PMSP_QUEUE Object,
	OUT PLIST_ENTRY ListHead
	);


#ifdef __cplusplus
}
#endif

#endif
