//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#ifndef _NTFLT_H_
#define _NTFLT_H_

#ifdef __cplusplus 
extern "C" {
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#define _BTR_SDK_
#include "btrsdk.h"
#include <strsafe.h>

//
// NT API Defintion
//

#include "ntifs.h"

#ifndef ASSERT
 #include <assert.h>
 #define ASSERT assert
#endif

#define NtMajorVersion 1
#define NtMinorVersion 0

//
// UUID identify NT filter
//

extern UUID NtUuid;

typedef enum _NT_PROBE_ORDINAL {

	//
	// Security
	//

	_NtAccessCheck,
	_NtPrivilegeCheck,
	_NtAdjustPrivilegesToken,
	_NtAdjustGroupsToken,
	_NtOpenProcessToken,
	_NtOpenThreadToken,
	_NtSetInformationToken,
	_NtQueryInformationToken,

	//
	// Timer
	//

	_NtCreateTimer,
	_NtOpenTimer,
	_NtQueryTimer,
	_NtSetTimer,
	_NtCancelTimer,

	//
	// Symbolic Link
	//

	_NtCreateSymbolicLinkObject,
	_NtOpenSymbolicLinkObject,
	_NtQuerySymbolicLinkObject,

	//
	// Event
	//
	
	_NtCreateEvent,
	_NtOpenEvent,
	_NtSetEvent,
	_NtResetEvent,
	_NtClearEvent,
	_NtPulseEvent,
	_NtQueryEvent,

	//
	// Mutant
	//
	
	_NtCreateMutant,
	_NtOpenMutant,
	_NtQueryMutant,
	_NtReleaseMutant,

	//
	// Semaphore
	//
	
	_NtCreateSemaphore,
	_NtOpenSemaphore,
	_NtQuerySemaphore,
	_NtReleaseSemaphore,

	//
	// I/O Completion Port 
	//

	_NtCreateIoCompletion,
	_NtOpenIoCompletion,
	_NtQueryIoCompletion,
	_NtRemoveIoCompletion,
	_NtSetIoCompletion,

	//
	// Process
	//
	
	_NtCreateProcess,
	_NtCreateProcessEx,
	_NtCreateUserProcess,
	_NtOpenProcess,
	_NtSuspendProcess,
	_NtResumeProcess,
	_NtTerminateProcess,
	_NtQueryInformationProcess,
	_NtSetInformationProcess,

	//
	// Thread
	//
	
	_NtCreateThread,
	_NtCreateThreadEx,
	_NtOpenThread,
	_NtTerminateThread,
	_NtSuspendThread,
	_NtResumeThread,
	_NtGetContextThread,
	_NtSetContextThread,
	_NtQueryInformationThread,
	_NtSetInformationThread,
	_NtQueueApcThread,
	_NtTestAlert,
	_NtAlertThread,
	_NtAlertResumeThread,

	//
	// Memory
	//
	
	_NtAllocateVirtualMemory,
	_NtFreeVirtualMemory,
	_NtReadVirtualMemory,
	_NtWriteVirtualMemory,
	_NtQueryVirtualMemory,
	_NtFlushVirtualMemory,
	_NtLockVirtualMemory,
	_NtProtectVirtualMemory,

	//
	// Section
	//

	_NtQuerySection,
	_NtCreateSection,
	_NtExtendSection,
	_NtMapViewOfSection,
	_NtOpenSection,
	_NtUnmapViewOfSection,

	//
	// Wait 
	//
	
	_NtWaitForSingleObject,
	_NtWaitForMultipleObjects,
	_NtSignalAndWaitForSingleObject,

	//
	// File System
	//
	
	_NtCreateFile,
	_NtOpenFile,
	_NtReadFile,
	_NtWriteFile,
	_NtLockFile,
	_NtUnlockFile,
	_NtDeleteFile,
	_NtDeviceIoControlFile,
	_NtFlushBuffersFile,
	_NtCreateNamedPipeFile,
	_NtCreatePagingFile,
	_NtFsControlFile,
	_NtQueryAttributesFile,
	_NtQueryEaFile,
	_NtQueryFullAttributesFile,
	_NtQueryInformationFile,
	_NtQueryVolumeInformationFile,
	_NtSetInformationFile,
	_NtSetVolumeInformationFile,
	_NtCreateDirectoryObject,
	_NtOpenDirectoryObject,
	_NtQueryDirectoryObject,
	_NtQueryDirectoryFile,
	_NtNotifyChangeDirectoryFile,

	//
	// LPC
	//
	
	_NtConnectPort,
	_NtCreatePort,
	_NtCreateWaitablePort,
	_NtListenPort,
	_NtRequestPort,
	_NtReplyPort,
	_NtReplyWaitReceivePort,
	_NtRequestWaitReplyPort,
	_NtReplyWaitReceivePortEx,
	_NtReplyWaitReplyPort,
	_NtQueryInformationPort,

	//
	// Misc Key Routines
	//

	_NtClose,
	_NtRaiseException,

	_NtLoadDriver,
	_NtUnloadDriver,

	_NtDebugActiveProcess,
	_NtSystemDebugControl,

	_NtQuerySystemInformation,
	_NtSetSystemInformation,

	NT_PROBE_NUMBER,

} NT_PROBE_ORDINAL, *PNT_PROBE_ORDINAL;

#ifdef __cplusplus
}
#endif

#endif