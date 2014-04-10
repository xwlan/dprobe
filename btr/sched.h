//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _SCHED_H_
#define _SCHED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include "thread.h"

typedef struct _BTR_NOTIFICATION_PACKET {
	PVOID Packet;
	HANDLE CompleteEvent;
	BTR_THREAD_TYPE Requestor;
	LIST_ENTRY ListEntry;
} BTR_NOTIFICATION_PACKET, *PBTR_NOTIFICATION_PACKET;

typedef enum _BTR_CONTROLCODE {
	SCHED_START_TRACE       = 0x00000001,
	SCHED_STOP_TRACE        = 0x00000002,
	SCHED_REGISTER_FILTER   = 0x00000004,
	SCHED_UNREGISTER_FILTER = 0x00000008,
	SCHED_PROBE_ON          = 0x00000010,
	SCHED_PROBE_OFF         = 0x00000020,
	SCHED_SCAN_RUNDOWN      = 0x00000040,
	SCHED_WRITE_RECORD      = 0x00000080,
	SCHED_SCAN_LOOKASIDE    = 0x00000100,
	SCHED_TRIM_WORKINGSET   = 0x00000200,
	SCHED_SCAN_STACKTRACE   = 0x00000400,
	SCHED_FLUSH_STACKTRACE  = 0x00000800,
	SCHED_STOP_RUNTIME      = 0xffffffff,
} BTR_CONTROLCODE, *PBTR_CONTROLCODE;

typedef struct _BTR_CONTROL {

	BTR_CONTROLCODE ControlCode;
	BOOLEAN AckRequired;
	BOOLEAN FreeRequired;
	USHORT Spare0;
	ULONG Complete;
	ULONG Result;
	ULONG Posted;

	union {
		
		struct {
			ULONGLONG Alignment;	
		} Start;

		struct {
			ULONGLONG Alignment;
		} Stop;

		struct {
			WCHAR FilterName[MAX_PATH];
			ULONG ResultBits[16];
		} Filter;

		struct {
			WCHAR FilterName[MAX_PATH];
			ULONG Ordinal;
		} Probe;

	} u;

} BTR_CONTROL, *PBTR_CONTROL;

ULONG
BtrCreateQueue(
	IN PBTR_QUEUE Object,
	IN BTR_THREAD_TYPE Type
	);

ULONG
BtrDestroyQueue(
	IN PBTR_QUEUE Object
	);

ULONG
BtrQueueNotification(
	IN PBTR_QUEUE Object,
	IN PBTR_NOTIFICATION_PACKET Notification
	);

ULONG
BtrDeQueueNotification(
	IN PBTR_QUEUE Object,
	IN ULONG Milliseconds,
	OUT PBTR_NOTIFICATION_PACKET *Notification
	);

ULONG
BtrDeQueueNotificationList(
	IN PBTR_QUEUE Object,
	OUT PLIST_ENTRY ListHead
	);

VOID
BtrAllocateCachedPacket(
	IN PBTR_CONTROL ControlCache,
	IN PBTR_NOTIFICATION_PACKET NotificationCache,
	IN LONG CachedNumber,
	OUT PBTR_CONTROL *Control,
	OUT PBTR_NOTIFICATION_PACKET *Notification
	);

ULONG
BtrScheduleScanThread(
	IN PBTR_CONTROL Control,
	IN PBTR_NOTIFICATION_PACKET Notification
	);

ULONG
BtrScheduleWrite(
	IN PBTR_CONTROL Control,
	IN PBTR_NOTIFICATION_PACKET Notification
	);

ULONG
BtrScheduleScanLookaside(
	IN PBTR_CONTROL Control,
	IN PBTR_NOTIFICATION_PACKET Notification
	);

ULONG
BtrScheduleScanStackTrace(
	IN PBTR_CONTROL Control,
	IN PBTR_NOTIFICATION_PACKET Notification
	);

ULONG
BtrScheduleStop(
	IN BTR_THREAD_TYPE Requestor	
	);

ULONG
BtrRequestStop(
	IN BTR_THREAD_TYPE Requestor	
	);

#ifdef __cplusplus
}
#endif

#endif
