//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "btr.h"
#include "sched.h"
#include "heap.h"
#include "thread.h"
#include "probe.h"
#include "ipc.h"

typedef enum _BTR_SCHEDULE_QUANTUM {
	QUANTUM_ZERO             = 0,
	QUANTUM_WRITE_RECORD     = 1,
	QUANTUM_SCAN_LOOKASIDE   = 2,
	QUANTUM_SCAN_STACKTRACE  = 4,
	QUANTUM_SCAN_THREAD      = 16,
	QUANTUM_MAXIMUM = QUANTUM_SCAN_THREAD,
} BTR_SCHEDULE_QUANTUM, *PBTR_SCHEDULE_QUANTUM;

ULONG CALLBACK
BtrScheduleProcedure(
	IN PVOID Context
	)
{
	ULONG Status;
	PBTR_NOTIFICATION_PACKET Notification;
	PBTR_CONTROL Control;
	HANDLE Timer;
	LARGE_INTEGER DueTime;
	BTR_CONTROL ControlCache[10];
	BTR_NOTIFICATION_PACKET NotificationCache[10];
	BOOLEAN StopRequired;
	BOOLEAN HasStopRequestor = FALSE;
	ULONG Quantum;

	BtrInitializeRuntimeThread();

	RtlZeroMemory(&ControlCache, sizeof(ControlCache));
	RtlZeroMemory(&NotificationCache, sizeof(NotificationCache));

	Quantum = QUANTUM_ZERO;
	StopRequired = FALSE; 
	Notification = NULL;
	Control = NULL;

	//
	// Schedule procedure use a quantum of 1000 milliseconds
	//

	DueTime.QuadPart = -10000 * 1000;
	Timer = CreateWaitableTimer(NULL, FALSE, NULL);
	SetWaitableTimer(Timer, &DueTime, 500, NULL, NULL, FALSE);

	__try {

		while (TRUE) {

			Status = WaitForSingleObject(Timer, INFINITE);

			if (Status == WAIT_OBJECT_0) {

				Quantum += 1;

				//
				// Schedule a SCHED_WRITE_RECORD request to write procedure
				//

				BtrAllocateCachedPacket(ControlCache, NotificationCache, 10,
					                    &Control, &Notification);
				BtrScheduleWrite(Control, Notification);

				if ((Quantum + QUANTUM_MAXIMUM) % QUANTUM_SCAN_LOOKASIDE == 0) {
					BtrAllocateCachedPacket(ControlCache, NotificationCache, 10,
							                &Control, &Notification);
					BtrScheduleScanLookaside(Control, Notification);
				}	
				
				if ((Quantum + QUANTUM_MAXIMUM) % QUANTUM_SCAN_STACKTRACE == 0) {
					BtrAllocateCachedPacket(ControlCache, NotificationCache, 10,
									        &Control, &Notification);
					BtrScheduleScanStackTrace(Control, Notification);
				}
				
				//
				// Rest quantum value
				//

				if (Quantum == QUANTUM_MAXIMUM) {
					Quantum = 0;
				}
			}

			//
			// No blocking DeQueue to pick up any work to do
			//

			Status = BtrDeQueueNotification(&BtrSystemQueue[ThreadSchedule], 
				                            0, &Notification);
			ASSERT(Status == S_OK);

			if (Notification == NULL) {
				continue;
			}

			Control = (PBTR_CONTROL)Notification->Packet;

			switch (Control->ControlCode) {
			case SCHED_STOP_RUNTIME:
				HasStopRequestor = TRUE;
				goto Exit;	
			}

			if(Control->FreeRequired) {
				BtrFree(Control);
				BtrFree(Notification);
			}
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		//
		// BtrScheduleStop is always called if exception occurs
		//
	}

Exit:

	CancelWaitableTimer(Timer);
	CloseHandle(Timer);

	if (HasStopRequestor) {
		BtrScheduleStop(Notification->Requestor);
	} else {
		BtrScheduleStop(ThreadSchedule);
	}

	return S_OK;
}

ULONG
BtrScheduleStop(
	IN BTR_THREAD_TYPE Requestor	
	)
{
	ULONG Number;
	PBTR_CONTROL Control;
	PBTR_NOTIFICATION_PACKET Notification;
	HANDLE ThreadHandles[3];

	//
	// N.B. Mark BtrUnloading to make trap procedure ignore all probes and return
	// to call function's original code.
	//

	InterlockedCompareExchange(&BtrUnloading, 1, 0);

	for(Number = 0; Number < ThreadNumber; Number += 1) {

		if (Number != (ULONG)ThreadSchedule && Number != (ULONG)Requestor) {

			Control = (PBTR_CONTROL)BtrMalloc(sizeof(BTR_CONTROL));
			Control->ControlCode = SCHED_STOP_RUNTIME;
			Control->AckRequired = FALSE;

			Notification = (PBTR_NOTIFICATION_PACKET)BtrMalloc(sizeof(BTR_NOTIFICATION_PACKET));
			Notification->CompleteEvent = NULL;
			Notification->Packet = (PVOID)Control;
			Notification->Requestor = ThreadSchedule;

			BtrQueueNotification(&BtrSystemQueue[Number], Notification); 
		}
	}
	
	ThreadHandles[0] = BtrSystemThread[ThreadMessage];
	ThreadHandles[1] = BtrSystemThread[ThreadWrite];
	ThreadHandles[2] = BtrSystemThread[ThreadWatch];

	//
	// Wait all runtime threads to exit
	//

	WaitForMultipleObjects(3, &ThreadHandles[0], TRUE, INFINITE);
	BtrClosePort(&BtrPort);

	for (Number = 0; Number < ThreadNumber; Number += 1) {

		BtrDestroyQueue(&BtrSystemQueue[Number]);

		//
		// The shedule thread's handle will be closed in unload routine
		//

		if (Number != ThreadSchedule) {
			CloseHandle(BtrSystemThread[Number]);
			BtrSystemThread[Number] = NULL;
		}
	}

	BtrUnregisterProbes();

	if (!FlagOn(BtrFeatures, FeatureRemote)) {
		CloseHandle(BtrSystemThread[ThreadSchedule]);
		BtrSystemThread[Number] = NULL;
	}

	BtrUnloadRuntime();
	return S_OK;
}

ULONG
BtrScheduleScanThread(
	IN PBTR_CONTROL Control,
	IN PBTR_NOTIFICATION_PACKET Notification
	)
{
	if (!Control || !Notification) {
		Control = (PBTR_CONTROL)BtrMalloc(sizeof(BTR_CONTROL));
		Control->FreeRequired = TRUE;
		Notification = (PBTR_NOTIFICATION_PACKET)BtrMalloc(sizeof(BTR_NOTIFICATION_PACKET));
	}

	Control->ControlCode = SCHED_SCAN_RUNDOWN;
	Control->AckRequired = FALSE;
	Control->Complete = FALSE;
	Control->Posted = TRUE;

	Notification->CompleteEvent = NULL;
	Notification->Packet = (PVOID)Control;

	//
	// N.B. Post notification to write procedure, note that this is a asynchronous
	// request, we don't expect return from it. write procedure is responsible to
	// free the allocated packets.
	//

	BtrQueueNotification(&BtrSystemQueue[ThreadWatch], Notification); 
	return S_OK;
}

ULONG
BtrScheduleScanLookaside(
	IN PBTR_CONTROL Control,
	IN PBTR_NOTIFICATION_PACKET Notification
	)
{
	if (!Control || !Notification) {
		Control = (PBTR_CONTROL)BtrMalloc(sizeof(BTR_CONTROL));
		Control->FreeRequired = TRUE;
		Notification = (PBTR_NOTIFICATION_PACKET)BtrMalloc(sizeof(BTR_NOTIFICATION_PACKET));
	}

	Control->ControlCode = SCHED_SCAN_LOOKASIDE;
	Control->AckRequired = FALSE;
	Control->Complete = FALSE;
	Control->Posted = TRUE;

	Notification->CompleteEvent = NULL;
	Notification->Packet = (PVOID)Control;

	//
	// N.B. Post notification to write procedure, note that this is a asynchronous
	// request, we don't expect return from it. write procedure is responsible to
	// free the allocated packets.
	//

	BtrQueueNotification(&BtrSystemQueue[ThreadWatch], Notification); 
	return S_OK;
}

ULONG
BtrScheduleScanStackTrace(
	IN PBTR_CONTROL Control,
	IN PBTR_NOTIFICATION_PACKET Notification
	)
{
	if (!Control || !Notification) {
		Control = (PBTR_CONTROL)BtrMalloc(sizeof(BTR_CONTROL));
		Control->FreeRequired = TRUE;
		Notification = (PBTR_NOTIFICATION_PACKET)BtrMalloc(sizeof(BTR_NOTIFICATION_PACKET));
	}

	Control->ControlCode = SCHED_SCAN_STACKTRACE;
	Control->AckRequired = FALSE;
	Control->Complete = FALSE;
	Control->Posted = TRUE;

	Notification->CompleteEvent = NULL;
	Notification->Packet = (PVOID)Control;

	//
	// N.B. Post notification to write procedure, note that this is a asynchronous
	// request, we don't expect return from it. write procedure is responsible to
	// free the allocated packets.
	//

	BtrQueueNotification(&BtrSystemQueue[ThreadWatch], Notification); 
	return S_OK;
}

ULONG
BtrScheduleWrite(
	IN PBTR_CONTROL Control,
	IN PBTR_NOTIFICATION_PACKET Notification
	)
{
	if (!Control || !Notification) {
		Control = (PBTR_CONTROL)BtrMalloc(sizeof(BTR_CONTROL));
		Control->FreeRequired = TRUE;
		Notification = (PBTR_NOTIFICATION_PACKET)BtrMalloc(sizeof(BTR_NOTIFICATION_PACKET));
	}

	Control->ControlCode = SCHED_WRITE_RECORD;
	Control->AckRequired = FALSE;
	Control->Complete = FALSE;
	Control->Posted = TRUE;

	Notification->CompleteEvent = NULL;
	Notification->Packet = (PVOID)Control;

	//
	// N.B. Post notification to write procedure, note that this is a asynchronous
	// request, we don't expect return from it. write procedure is responsible to
	// free the allocated packets.
	//

	BtrQueueNotification(&BtrSystemQueue[ThreadWrite], Notification); 
	return S_OK;
}

ULONG
BtrRequestStop(
	IN BTR_THREAD_TYPE Requestor	
	)
{
	PBTR_CONTROL Control;
	PBTR_NOTIFICATION_PACKET Notification;

	__try {

		Control = (PBTR_CONTROL)BtrMalloc(sizeof(BTR_CONTROL));
		Control->ControlCode = SCHED_STOP_RUNTIME;
		Control->FreeRequired = TRUE;

		Notification = (PBTR_NOTIFICATION_PACKET)BtrMalloc(sizeof(BTR_NOTIFICATION_PACKET));
		Notification->CompleteEvent = NULL;
		Notification->Packet = (PVOID)Control;
		Notification->Requestor = Requestor;

		//
		// N.B. Post notification to write procedure, note that this is a asynchronous
		// request, we don't expect return from it. write procedure is responsible to
		// free the allocated packets.
		//

		BtrQueueNotification(&BtrSystemQueue[ThreadSchedule], Notification); 

	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

		//
		// ignore exception here
		//
	}

	return S_OK;
}

ULONG
BtrCreateQueue(
	IN PBTR_QUEUE Object,
	IN BTR_THREAD_TYPE Type
	)
{
	HANDLE QueueHandle;

	QueueHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (!QueueHandle) {
		return GetLastError();
	}

	Object->Object = QueueHandle;
	Object->Type = Type;
	InitializeListHead(&Object->ListEntry);

	return S_OK;
}

ULONG
BtrDestroyQueue(
	IN PBTR_QUEUE Object
	)
{
	LIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PBTR_NOTIFICATION_PACKET Packet;

	InitializeListHead(&ListHead);
	BtrDeQueueNotificationList(Object, &ListHead);
	CloseHandle(Object->Object);

	while (IsListEmpty(&ListHead) != TRUE) {
		ListEntry = RemoveHeadList(&ListHead);
		Packet = CONTAINING_RECORD(ListEntry, BTR_NOTIFICATION_PACKET, ListEntry);
		BtrFree(Packet);
	}

	return S_OK;
}

ULONG
BtrQueueNotification(
	IN PBTR_QUEUE Object,
	IN PBTR_NOTIFICATION_PACKET Notification
	)
{
	ULONG Status;

	Status = PostQueuedCompletionStatus(Object->Object, 
		                                0, 
										(ULONG_PTR)Notification, 
										NULL);
	
	if (Status) {
		return S_OK;
	}

	Status = GetLastError();
	return Status;
}

ULONG
BtrDeQueueNotification(
	IN PBTR_QUEUE Object,
	IN ULONG Milliseconds,
	OUT PBTR_NOTIFICATION_PACKET *Notification
	)
{
	ULONG Status;
	ULONG TransferedLength;
	ULONG_PTR CompletionKey;
	OVERLAPPED *Overlapped = UlongToPtr(1);

	*Notification = NULL;
	Status = GetQueuedCompletionStatus(Object->Object, 
				                       &TransferedLength, 
									   &CompletionKey, 
									   &Overlapped, Milliseconds);
	
	if (Status) {
		*Notification = (PBTR_NOTIFICATION_PACKET)CompletionKey;
		return S_OK;
	}

	//
	// This is timeout case
	//

	if (!Status && Overlapped == NULL) {
		return S_OK;
	}

	Status = GetLastError();
	return Status;
}

ULONG
BtrDeQueueNotificationList(
	IN PBTR_QUEUE Object,
	OUT PLIST_ENTRY ListHead
	)
{
	ULONG Status;
	ULONG TransferedLength;
	ULONG_PTR CompletionKey;
	OVERLAPPED *Overlapped;
	PBTR_NOTIFICATION_PACKET Notification;

	Status = TRUE;
	InitializeListHead(ListHead);

	while (Status) {
		Status = GetQueuedCompletionStatus(Object->Object, 
					                       &TransferedLength, 
										   &CompletionKey, 
										   &Overlapped, 0);
		
		if (Status) {
			Notification = (PBTR_NOTIFICATION_PACKET)CompletionKey;
			InsertTailList(ListHead, &Notification->ListEntry);
		}
	}

	Status = GetLastError();
	return Status;
}

VOID
BtrAllocateCachedPacket(
	IN PBTR_CONTROL ControlCache,
	IN PBTR_NOTIFICATION_PACKET NotificationCache,
	IN LONG CachedNumber,
	OUT PBTR_CONTROL *Control,
	OUT PBTR_NOTIFICATION_PACKET *Notification
	)
{
	LONG Number;
	LONG Complete;

	__try {

		for(Number = 0; Number < CachedNumber; Number += 1) {

			if (!ControlCache[Number].Posted) {
				*Control = &ControlCache[Number];
				ControlCache[Number].Complete = 0;
				ControlCache[Number].FreeRequired = 0;
				*Notification = &NotificationCache[Number];
				return;
			}

			Complete = ReadForWriteAccess(&ControlCache[Number].Complete);

			if (Complete == TRUE) {
				ControlCache[Number].Posted = 0;
				ControlCache[Number].Complete = 0;
				ControlCache[Number].FreeRequired = 0;
				*Notification = &NotificationCache[Number];
				return;
			}
		}
	}
	__except(1) {

	}

	*Control = NULL;
	*Notification = NULL;
	return;
}