//
// lan.john@gmail.com 
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "mspdefs.h"
#include "mspqueue.h"
#include "msputility.h"

ULONG
MspCreateQueue(
	OUT PMSP_QUEUE *Object
	)
{
	HANDLE QueueHandle;
	PMSP_QUEUE Queue;

	QueueHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (!QueueHandle) {
		return GetLastError();
	}

	Queue = (PMSP_QUEUE)MspMalloc(sizeof(MSP_QUEUE));
	Queue->ObjectHandle = QueueHandle;
	InitializeListHead(&Queue->ListEntry);

	*Object = Queue;
	return MSP_STATUS_OK;
}

ULONG
MspCloseQueue(
	IN PMSP_QUEUE Object
	)
{
	LIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PMSP_QUEUE_PACKET Packet;

	InitializeListHead(&ListHead);
	MspDeQueuePacketList(Object, &ListHead);

	while (IsListEmpty(&ListHead) != TRUE) {

		ListEntry = RemoveHeadList(&ListHead);
		Packet = CONTAINING_RECORD(ListEntry, MSP_QUEUE_PACKET, ListEntry);

		if (FlagOn(Packet->PacketFlag, MSP_PACKET_FREE)) {
			MspFree(Packet);
		}
	}

	CloseHandle(Object->ObjectHandle);
	Object->ObjectHandle = NULL;
	return MSP_STATUS_OK;
}

ULONG
MspQueuePacket(
	IN PMSP_QUEUE Object,
	IN PMSP_QUEUE_PACKET Packet
	)
{
	ULONG Status;

	Status = PostQueuedCompletionStatus(Object->ObjectHandle, 
		                                0, 
										(ULONG_PTR)Packet, 
										NULL);
	
	if (Status) {
		return MSP_STATUS_OK;
	}

	Status = GetLastError();
	return Status;
}

ULONG
MspDeQueuePacket(
	IN PMSP_QUEUE Object,
	OUT PMSP_QUEUE_PACKET *Packet
	)
{
	ULONG Status;
	ULONG TransferedLength;
	ULONG_PTR CompletionKey;
	OVERLAPPED *Overlapped;

	*Packet = NULL;
	Status = GetQueuedCompletionStatus(Object->ObjectHandle, 
				                       &TransferedLength, 
									   &CompletionKey, 
									   &Overlapped, INFINITE);
	
	if (Status) {
		*Packet = (PMSP_QUEUE_PACKET)CompletionKey;
		return MSP_STATUS_OK;
	}

	Status = GetLastError();
	return Status;
}

ULONG
MspDeQueuePacketEx(
	IN PMSP_QUEUE Object,
	IN ULONG Milliseconds,
	OUT PMSP_QUEUE_PACKET *Packet
	)
{
	ULONG Status;
	ULONG TransferedLength;
	ULONG_PTR CompletionKey;
	OVERLAPPED *Overlapped;

	*Packet = NULL;
	Status = GetQueuedCompletionStatus(Object->ObjectHandle, 
				                       &TransferedLength, 
									   &CompletionKey, 
									   &Overlapped, Milliseconds);
	
	if (Status) {
		*Packet = (PMSP_QUEUE_PACKET)CompletionKey;
		return MSP_STATUS_OK;
	} 
	
	else if (Overlapped == NULL) {
	
		//
		// N.B. If it's timeout, NT null overlapped structure and return FALSE
		//

		return MSP_STATUS_TIMEOUT;

	}

	Status = GetLastError();
	return Status;
}

ULONG
MspDeQueuePacketList(
	IN PMSP_QUEUE Object,
	OUT PLIST_ENTRY ListHead
	)
{
	ULONG Status;
	ULONG TransferedLength;
	ULONG_PTR CompletionKey;
	OVERLAPPED *Overlapped;
	PMSP_QUEUE_PACKET Packet;

	Status = TRUE;
	InitializeListHead(ListHead);

	while (Status) {
		Status = GetQueuedCompletionStatus(Object->ObjectHandle, 
					                       &TransferedLength, 
										   &CompletionKey, 
										   &Overlapped, 0);
		
		if (Status) {
			Packet = (PMSP_QUEUE_PACKET)CompletionKey;
			InsertTailList(ListHead, &Packet->ListEntry);
		}
	}

	Status = GetLastError();
	return Status;
}
