//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _MSPSDK_H_
#define _MSPSDK_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WIN32_WINNT 
#define _WIN32_WINNT  0x0501
#endif

#include <windows.h>

#define _BTR_SDK_
#include "btrsdk.h"

//
// MSP Status
//

#define	MSP_STATUS_OK					0x00000000	
#define	MSP_STATUS_IPC_BROKEN			0xE1000000
#define	MSP_STATUS_BUFFER_LIMIT			0xE1000001
#define	MSP_STATUS_STOPPED				0xE1000002
#define	MSP_STATUS_INIT_FAILED			0xE1000003
#define	MSP_STATUS_EXCEPTION			0xE1000004
#define	MSP_STATUS_WOW64_ERROR			0xE1000005  
#define	MSP_STATUS_INVALID_PARAMETER	0xE1000006
#define MSP_STATUS_NO_FILTER            0xE1000007
#define MSP_STATUS_FILTER_FAILURE       0xE1000008
#define MSP_STATUS_LOG_FAILURE          0xE1000009
#define MSP_STATUS_ACCESS_DENIED        0xE1000010
#define MSP_STATUS_NO_UUID              0xE1000011
#define	MSP_STATUS_ERROR				0xFFFFFFFF

//
// MSP Flag
//

#define MSP_FLAG_LOGGING   0x00000001

//
// MSP Mode
//

typedef enum _MSP_MODE {
	MSP_MODE_LIB,
	MSP_MODE_DLL,
} MSP_MODE, *PMSP_MODE;

//
// Trace callback is required if user need get trace records,
// if this callback is NULL, MSP won't receive any record.
// This callback should copy the buffer as soon as possible and
// decode later in other threads.
// 
// Arguments:
// TraceUuid, UUID identify a trace session.
// Buffer, Base address of record buffer.
// Length, Size of records in byte count.
// 
// Return:
// MSP_STATUS_OK
//

typedef ULONG
(WINAPI *MSP_TRACE_CALLBACK)(
	IN UUID *TraceUuid,
	IN PVOID Buffer,
	IN ULONG Length 
	);

//
// Error callback is required in any case, if ErrorCode is
// MSP_STATUS_IPC_BROKEN, MSP_STATUS_EXCEPTION, MSP_STATUS_ERROR,
// user should call MspStopTrace to stop the trace later, note 
// that MspStopTrace can not be safely called in error callback
// routine, user should mark a variable or signal an event etc,
// and call MspStopTrace outside.
//
// Arguments:
// TraceUuid, UUID identify a trace session
// ErrorCode, MSP status code
//
// Return:
// MSP_STATUS_OK
//

typedef ULONG
(WINAPI *MSP_ERROR_CALLBACK)(
	IN UUID *TraceUuid,
	IN ULONG ErrorCode
	);

//
// Initialize routine, must be called before any other MSP API.
//
//
// MSP API
//

ULONG WINAPI
MspInitialize(
	IN MSP_MODE Mode,
	IN ULONG Flag,
	IN PWSTR BtrPath,
	IN ULONG BtrFeature,
	IN PWSTR CachePath
	);

ULONG WINAPI
MspCreateTrace(
	IN ULONG ProcessId,
	IN MSP_TRACE_CALLBACK TraceCallback,
	IN MSP_ERROR_CALLBACK ErrorCallback,
	OUT UUID *TraceUuid
	);

ULONG WINAPI
MspRegisterFilter(
	IN UUID *TraceUuid,
	IN PWSTR FilterPath
	);

ULONG WINAPI
MspUnregisterFilter(
	IN UUID *TraceUuid,
	IN PWSTR FilterPath
	);

ULONG WINAPI
MspTurnOnProbe(
	IN UUID *TraceUuid,
	IN PWSTR FilterPath,
	IN ULONG Ordinal
	);

ULONG WINAPI
MspTurnOffProbe(
	IN UUID *TraceUuid,
	IN PWSTR FilterPath,
	IN ULONG Ordinal
	);

ULONG WINAPI
MspStartTrace(
	IN UUID *TraceUuid
	);

ULONG WINAPI
MspStopTrace(
	IN UUID *TraceUuid
	);

ULONG WINAPI
MspFilterControl(
	IN UUID *TraceUuid,
	IN PWSTR FilterPath,
	IN PBTR_FILTER_CONTROL Control,
	OUT PBTR_FILTER_CONTROL *Ack
	);

VOID WINAPI
MspUninitialize(
	VOID	
	);

ULONG WINAPI
MspDecodeDetailA(
	IN PBTR_FILTER_RECORD Record,
	IN PSTR Buffer,
	IN ULONG Length
	);

ULONG WINAPI
MspDecodeDetailW(
	IN PBTR_FILTER_RECORD Record,
	IN PWSTR Buffer,
	IN ULONG Length
	);

ULONG 
MspDecodeProbeA(
	IN PBTR_FILTER_RECORD Record,
	IN PSTR NameBuffer,
	IN ULONG NameLength,
	IN PSTR ModuleBuffer,
	IN ULONG ModuleLength
	);

ULONG 
MspDecodeProbeW(
	IN PBTR_FILTER_RECORD Record,
	IN PWSTR NameBuffer,
	IN ULONG NameLength,
	IN PWSTR ModuleBuffer,
	IN ULONG ModuleLength
	);

PBTR_FILTER WINAPI
MspLoadFilter(
	IN PWSTR FilterPath
	);

ULONG WINAPI
MspCopyRecord(
	IN ULONG Sequence,
	IN PVOID Buffer,
	IN ULONG Size
	);

ULONG WINAPI
MspReadRecord(
	IN ULONG Sequence,
	IN PVOID Buffer,
	IN ULONG Size
	);

#ifdef __cplusplus
}
#endif

#endif