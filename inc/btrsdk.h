//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _BTRSDK_H_
#define _BTRSDK_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// N.B. Define _BTR_SDK_ to use this file
//

#if defined (_BTR_SDK_)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <windows.h>

typedef enum _BTR_FEATURE_FLAG {
	FeatureLibrary      = 0x00000001,
	FeatureLocal        = 0x00000002,
	FeatureRemote       = 0x00000004,
	FeatureLogging      = 0x00000100,
} BTR_FEATURE_FLAG, *PBTR_FEATURE_FLAG;

typedef enum _BTR_RETURN_TYPE {
	ReturnVoid,
	ReturnInt8,
	ReturnInt16,
	ReturnInt32,
	ReturnInt64,
	ReturnUInt8,
	ReturnUInt16,
	ReturnUInt32,
	ReturnUInt64,
	ReturnPtr,
	ReturnIntPtr,   // ptr sized int
	ReturnUIntPtr,  // ptr sized uint
	ReturnFloat,
	ReturnDouble,
	ReturnNumber,
} BTR_RETURN_TYPE, PBTR_RETURN_TYPE;

typedef enum _BTR_RECORD_TYPE {
	RECORD_BUILDIN,
	RECORD_FAST,
	RECORD_FILTER,
	RECORD_CALLBACK,
} BTR_RECORD_TYPE, *PBTR_RECORD_TYPE;

typedef enum _BTR_PROBE_TYPE {
	PROBE_BUILDIN,
	PROBE_FAST,
	PROBE_FILTER,
	PROBE_CALLBACK,
} BTR_PROBE_TYPE, *PBTR_PROBE_TYPE;

typedef struct _BTR_RECORD_HEADER {

	ULONG TotalLength;

	ULONG RecordFlag : 4;
	ULONG RecordType : 4;
	ULONG ReturnType : 4;
	ULONG Reserved   : 4;
	ULONG StackDepth : 6;
	ULONG CallDepth  : 10;

	PVOID Address;
	PVOID Caller;
	ULONG Sequence;
	ULONG ObjectId;
	ULONG StackHash;
	ULONG ProcessId;
	ULONG ThreadId;
	ULONG LastError;
	ULONG Duration;
	FILETIME FileTime;

	union {
		ULONG64 Return;
		double  FpuReturn;
	};

} BTR_RECORD_HEADER, *PBTR_RECORD_HEADER;

typedef struct _BTR_FILTER_RECORD {
	BTR_RECORD_HEADER Base;
	GUID FilterGuid;
	ULONG ProbeOrdinal;
	CHAR Data[ANYSIZE_ARRAY];
} BTR_FILTER_RECORD, *PBTR_FILTER_RECORD;

//
// _R, Filter record base address
// _T, Type to be converted to
//

#define FILTER_RECORD_POINTER(_R, _T)   ((_T *)(&_R->Data[0]))

typedef struct _BTR_BITMAP {
    ULONG SizeOfBitMap;
    PULONG Buffer; 
} BTR_BITMAP, *PBTR_BITMAP;

typedef struct _BTR_FILTER_PROBE {
	ULONG Ordinal;
	ULONG Flags;
	BTR_RETURN_TYPE ReturnType;
	PVOID Address;
	PVOID FilterCallback;
	PVOID DecodeCallback;
	WCHAR DllName[64];	
	WCHAR ApiName[64];
} BTR_FILTER_PROBE, *PBTR_FILTER_PROBE;

//
// UnregisterCallback is called when filter unloads,
// this provide a chance for filters to do clean up work,
// it's called in arbitrary thread context. it's optional.
//

typedef ULONG 
(WINAPI *BTR_UNREGISTER_CALLBACK)(
	VOID
	);

//
// N.B. BTR_FILTER_CONTROL's size limit is 8KB
//

#define BTR_MAX_FILTER_CONTROL_SIZE  (1024 * 8)

typedef struct _BTR_FILTER_CONTROL {
	ULONG Length;
	CHAR Control[ANYSIZE_ARRAY];
} BTR_FILTER_CONTROL, *PBTR_FILTER_CONTROL;

//
// ControlCallback provide a mechanism to implement custome IPC
// between filter and its control end, it's optional.
//

typedef ULONG
(WINAPI *BTR_CONTROL_CALLBACK)(
	IN PBTR_FILTER_CONTROL Control,
	OUT PBTR_FILTER_CONTROL *Ack
	);

//
// OptionCallback provide a mechanism to integrate filter into D Probe
// console, it's optional, and most filters should implement its own
// configuration file to adjust filter's runtime configuration.
//

typedef ULONG
(WINAPI *BTR_OPTION_CALLBACK)(
	IN HWND hWndParent,
	IN PVOID Buffer,
	IN ULONG BufferLength,
	OUT PULONG ActualLength
	);

typedef struct _BTR_FILTER {

	WCHAR FilterName[MAX_PATH];
	WCHAR Author[MAX_PATH];
	WCHAR Description[MAX_PATH];
	WCHAR MajorVersion;
	WCHAR MinorVersion;
	HICON FilterBanner;

	ULONG FilterTag;
	GUID FilterGuid;
	PBTR_FILTER_PROBE Probes;
	ULONG ProbesCount;
	
	BTR_UNREGISTER_CALLBACK UnregisterCallback;
	BTR_CONTROL_CALLBACK ControlCallback;
	BTR_OPTION_CALLBACK  OptionCallback;

	//
	// N.B. Don't touch the following fields.
	//

	LIST_ENTRY ListEntry;
	BTR_BITMAP BitMap;
	WCHAR FilterFullPath[MAX_PATH];
	PVOID BaseOfDll;
	ULONG SizeOfImage;

} BTR_FILTER, *PBTR_FILTER;

//
// BTR_PROBE_CONTEXT is used by filter to get the destine address to call,
// and used to capture call result just in time.
//

typedef struct _BTR_PROBE_CONTEXT {
	PVOID Destine;
	PVOID Frame;
} BTR_PROBE_CONTEXT, *PBTR_PROBE_CONTEXT;

//
// DecodeCallback is called by D Probe console to decode captured records,
// it's required if filters integrate with D Probe, it's DecodeCallback is
// not available, records details won't be displayed in D Probe console.
//

typedef ULONG
(*BTR_DECODE_CALLBACK)(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG BufferLength,
	OUT PWCHAR Buffer
	);

//
// Filter can work under two modes, for FILTER_MODE_DECODE,
// filter must initialize all required data to support record decode.
// for FILTER_MODE_CAPTURE, filter must initialize all required data
// to support capture records in address space of target process.
// D Probe console load filters in FILTER_MODE_DECODE, runtime dll
// load filters in FILTER_MODE_CAPTURE.
//

typedef enum _BTR_FILTER_MODE {
	FILTER_MODE_DECODE,
	FILTER_MODE_CAPTURE,
} BTR_FILTER_MODE;

//
// BtrFltGetObject is required for all filters, runtime and D Probe
// console both call this exported routine to get filter information,
// it's mandatory. note that the exported name must be BtrFltGetObject.
//

typedef PBTR_FILTER
(WINAPI *BTR_FLT_GETOBJECT)(
	IN BTR_FILTER_MODE Mode	
	);

PBTR_FILTER WINAPI
BtrFltGetObject(
	IN BTR_FILTER_MODE Mode	
	);

//
// Allocate memory block from heap
//

PVOID WINAPI
BtrFltMalloc(
	IN ULONG Length
	);

//
// Free heap block allocated by BtrFtlMalloc
//

VOID WINAPI
BtrFltFree(
	IN PVOID Address
	);

//
// After usage of Destine, BtrFltFree must be called
// to free allocated string
//

SIZE_T WINAPI
BtrFltConvertUnicodeToAnsi(
	IN PWSTR Source, 
	OUT PCHAR *Destine
	);

//
// After usage of Destine, BtrFltFree must be called
// to free allocated string
//

VOID WINAPI
BtrFltConvertAnsiToUnicode(
	IN PCHAR Source,
	OUT PWCHAR *Destine
	);

//
// Get call context for current thread
//

VOID WINAPI
BtrFltGetContext(
	IN PBTR_PROBE_CONTEXT Context 
	);

//
// Set call context for current thread
//

VOID WINAPI
BtrFltSetContext(
	IN PBTR_PROBE_CONTEXT Context 
	);

//
// Wrapper of WIN32 GetLastError()
//

VOID WINAPI
BtrFltGetLastError(
	IN PBTR_PROBE_CONTEXT Context 
	);

//
// Allocate record buffer from runtime 
//

PBTR_FILTER_RECORD WINAPI
BtrFltAllocateRecord(
	IN ULONG DataLength, 
	IN GUID FilterGuid, 
	IN ULONG Ordinal
	);

//
// Free record allocated by BtrFltAllocateRecord
//

VOID WINAPI
BtrFltFreeRecord(
	PBTR_FILTER_RECORD Record
	);

//
// Queue captured record to runtime record queue,
// note that after call to BtrFltQueueRecord(),
// the record pointer can not be touched anymore,
// since it's possible that it's already freed.
//

VOID WINAPI
BtrFltQueueRecord(
	IN PVOID Record
	);

//
// Enter exemption region, in exemption region,
// runtime does not capture any records, it's 
// re-entry safe.
//

VOID WINAPI
BtrFltEnterExemptionRegion(
	VOID
	);

//
// Leave exemption region
//

VOID WINAPI
BtrFltLeaveExemptionRegion(
	VOID
	);

//
// Start initialization of runtime, called by remote end
//

ULONG WINAPI
BtrFltStartRuntime(
	IN PULONG Features 
	);

//
// Stop working of runtime, called by remote end
//

ULONG WINAPI
BtrFltStopRuntime(
	IN PVOID Reserved
	);

//
// Format status code into string
//

VOID WINAPI
BtrFormatError(
	IN ULONG ErrorCode,
	IN BOOLEAN SystemError,
	OUT PWCHAR Buffer,
	IN ULONG BufferLength, 
	OUT PULONG ActualLength
	);

#endif

#ifdef __cplusplus
}
#endif

#endif