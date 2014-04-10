//
// Apsaras Labs
// lan.john@gmail.com
// Copyright(C) 2009 - 2010
//

#ifndef _RSP_H_ 
#define _RSP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include "bsp.h"
#include "psp.h"

typedef enum _RSP_STATUS {
	STATUS_RSP_DISASTER                = 0xE0003001,
	STATUS_RSP_STORAGE_FULL            = 0xE0003002,
	STATUS_RSP_INDEX_FULL              = 0xE0003003,
	STATUS_RSP_WRITE_FAILED	           = 0xE0003004,
	STATUS_RSP_VIEW_FAILED             = 0xE0003005,
	STATUS_RSP_INDEX_COMMIT_FAILED     = 0xE0003006,
	STATUS_RSP_INVALID_INDEX           = 0xE0003007,
	STATUS_RSP_BUFFER_LIMIT            = 0xE0003008,
	STATUS_RSP_EXCEPTION               = 0xE0003009,
	STATUS_RSP_INIT_FAILED             = 0xE000300A,
	STATUS_RSP_STOP_WORKER             = 0xE000300B,
	STATUS_RSP_INVALID_PATH            = 0xE000300C,
	STATUS_RSP_CREATEFILE_FAILED       = 0xE000300D,
	STATUS_RSP_WRITEFILE_FAILED        = 0xE000300E,
	STATUS_RSP_OUTOFMEMORY             = 0xE000300F,
	STATUS_RSP_INVALID_PARAMETER       = 0xE0003010,
	STATUS_RSP_CREATEMAPPING_FAILED    = 0xE0003011,

} RSP_STATUS, *PRSP_STATUS;

typedef enum _RSP_STORAGE_MODE {
	RspNullMode,
	RspCreateMode,
	RspOpenMode
} RSP_STORAGE_MODE, *PRSP_STORAGE_MODE;

typedef enum _RSP_COMMIT_MODE {
	RspCommitNull,
	RspCommitOnly,
	RspCommitFree,
	RspFreeOnly,
} RSP_COMMIT_MODE, *PRSP_COMMIT_MODE;

typedef enum _RSP_DECODE_FIELD {
	DecodeStartTime,
	DecodeProcessName,
	DecodeProcessId,
	DecodeThreadId,
	DecodeProbeId,
	DecodeReturn,
	DecodeParameters,
	DecodeOthers,
	DecodeQuery = -1
} RSP_DECODE_FIELD, *PRSP_DECODE_FIELD;


typedef PWSTR 
(*RSP_DECODE_CALLBACK)(
	IN PVOID Record,
	IN RSP_DECODE_FIELD Field
	); 

typedef enum _RSP_INDEX_FLAG {
	RSP_INDEX_FREE      = 0,
	RSP_INDEX_ALLOCATED = 1
} RSP_INDEX_FLAG, *PRSP_INDEX_FLAG;

typedef struct _RSP_INDEX_ENTRY {
	ULONG64 FileOffset;
	ULONG Length;
	ULONG FlagMask; 
} RSP_INDEX_ENTRY, *PRSP_INDEX_ENTRY;

typedef struct _RSP_INDEX {
	PRSP_INDEX_ENTRY Entry;
	ULONG LastValidIndex;
} RSP_INDEX, *PRSP_INDEX;

//
// Each RSP_VIEW_RANGE can desribe 1MB space
//

typedef struct _RSP_VIEW_RANGE {
	ULONG FirstIndex;
	ULONG LastIndex;
} RSP_VIEW_RANGE, *PRSP_VIEW_RANGE;

//
// RSP STRUCTURES
//

typedef struct _RSP_VIEW {
	HANDLE ViewHandle;
	PVOID MappedVa;
	ULONG CurrentRange;
	ULONG RangeCount;
	ULONG ValidCount;
	PRSP_VIEW_RANGE Range;
} RSP_VIEW, *PRSP_VIEW;

typedef struct _RSP_STORAGE {
	ULONG64 FileLength;
	ULONG64 ValidLength;
	RSP_STORAGE_MODE Mode;
	HANDLE FileHandle;
	WCHAR FileName[MAX_PATH];
} RSP_STORAGE, *PRSP_STORAGE;

typedef struct _RSP_SIGNATURE { 
	WCHAR Magic[7]; // "dprobe"
	WCHAR Version;
} RSP_SIGNATURE, *PRSP_SIGNATURE;

typedef struct _RSP_PROBE {
	PVOID Address;
	ULONG Version;
	ULONG Flag;
	ULONG_PTR ReturnValue;
	ULONG ArgumentCount;
	BTR_ARGUMENT Parameters[16];
	WCHAR ParameterName[16][16];
	WCHAR ModuleName[64];
	WCHAR ProcedureName[64];
	LIST_ENTRY Entry;
	LIST_ENTRY VariantList;
} RSP_PROBE, *PRSP_PROBE;

typedef struct _RSP_PROBE_GROUP {
	ULONG ProcessId;
	ULONG NumberOfProbes;
	WCHAR ProcessName[64];
	WCHAR FullPathName[MAX_PATH];
	LIST_ENTRY ListEntry;
	LIST_ENTRY ProbeListHead;
} RSP_PROBE_GROUP, *PRSP_PROBE_GROUP; 

typedef struct _RSP_ADDON_GROUP {
	ULONG ProcessId;
	WCHAR ProcessName[64];
	WCHAR FullPathName[MAX_PATH];
	LIST_ENTRY ListEntry;
} RSP_ADDON_GROUP, *PRSP_ADDON_GROUP;

typedef struct _RSP_ADDON_TABLE {
	ULONG NumberOfAddOn;
	RSP_ADDON_GROUP AddOnGroup[ANYSIZE_ARRAY];
} RSP_ADDON_TABLE, *PRSP_ADDON_TABLE;


ULONG
RspInitialize(
	IN PWSTR StoragePath	
	);

ULONG
RspCreateCache(
	IN PWSTR Path
	);

ULONG
RspLoadCache(
	IN PWSTR Path
	);

ULONG 
RspCommitCache(
	IN PWSTR Path
	);

ULONG
RspCommitFreeCache(
	IN RSP_COMMIT_MODE Mode,
	IN PWSTR Path
	);

ULONG
RspGetStorageBaseName(
	IN PWCHAR Buffer,
	IN ULONG Length
	);

BOOLEAN
RspIsValidIndex(
	IN ULONG Index
	);

BOOLEAN
RspIsCachedIndex(
	IN ULONG Index
	);

ULONG
RspComputeRange(
	IN ULONG Index
	);

ULONG
RspCacheRange(
	IN ULONG FirstIndex,
	IN ULONG LastIndex
	);

BOOLEAN
RspRegisterCallback(
	IN RSP_DECODE_CALLBACK Callback
	);

BOOLEAN
RspRemoveCallback(
	IN RSP_DECODE_CALLBACK Callback
	);

RSP_DECODE_CALLBACK
RspQueryDecode(
	IN PVOID Record
	);

ULONG
RspReadRecord(
	IN ULONG Index,
	IN PVOID Buffer,
	IN ULONG BufferLength
	);

ULONG
RspCopyRecord(
	IN ULONG Index,
	IN PVOID Buffer,
	IN ULONG BufferLength
	);

ULONG
RspWriteRecord(
	IN PVOID Record,
	IN ULONG Length
	);

ULONG
RspGetRecordCount(
	VOID
	);

BOOLEAN
RspIsCurrentStorage(
	IN PWSTR Path
	);

ULONG
RspExpandStorage(
	VOID
	);

ULONG
RspMapView(
	IN ULONG RangeNumber 
	);

ULONG
RspUnmapView(
	OUT PULONG RangeNumber 
	);

ULONG
RspAdjustView(
	IN PRSP_VIEW ViewObject,
	IN PRSP_INDEX IndexObject,
	IN ULONG RangeNumber,
	OUT PULONG OldRangeNumber
	);

LONG CALLBACK 
RspExceptionFilter(
	IN EXCEPTION_POINTERS *Pointers
	);

PRSP_PROBE
RspQueryProbeByName(
	IN ULONG ProcessId,
	IN PWSTR ModuleName,
	IN PWSTR ProcedureName,
	IN ULONG TypeFlag
	);

PRSP_PROBE
RspQueryProbeByAddress(
	IN ULONG ProcessId,
	IN PVOID Address,
	IN ULONG TypeFlag
	);

PRSP_PROBE_GROUP
RspQueryProbeGroup(
	IN ULONG ProcessId
	);

PRSP_PROBE_GROUP 
RspCreateProbeGroup(
	IN PPSP_PROCESS PspProcess
	);

ULONG
RspCommitProbe(
	IN PRSP_PROBE_GROUP Group,
	IN PPSP_PROBE Probe
	);

PRSP_PROBE
RspQueryManualProbe(
	IN ULONG ProcessId,
	IN PVOID Address
	);

ULONG
RspChainMultiVersionProbe(
	IN PRSP_PROBE RootProbe,
	IN PRSP_PROBE Subversion
	);

PRSP_PROBE
RspCreateProbe(
	IN PPSP_PROBE PspProbe
	);

ULONG
RspLookupProcessName(
	IN ULONG ProcessId,
	IN PWCHAR Buffer,
	IN ULONG BufferLength
	);

ULONG
RspLookupProbeByAddress(
	IN ULONG ProcessId,
	IN PVOID Address,
	IN PWCHAR Buffer, 
	IN ULONG BufferLength
	);

ULONG
RspWriteProbeGroup(
	IN HANDLE FileHandle
	);

ULONG
RspLoadProbeGroup(
	IN HANDLE FileHandle
	);

ULONG
RspDecodeRecord(
	IN PBTR_RECORD_BASE Record,
	IN RSP_DECODE_FIELD Field,
	IN PWCHAR Buffer,
	IN ULONG BufferLength
	);

PRSP_PROBE
RspQueryProbeByRecord(
	IN PBTR_RECORD_BASE Record
	);

ULONG
RspDecodeParameters(
	IN PBTR_MANUAL_RECORD Record,
	IN PRSP_PROBE Probe,
	IN PWCHAR Buffer,
	IN ULONG Length,
	OUT PULONG ActualLength
	);

ULONG
RspDecodeReturn(
	IN PBTR_RECORD_BASE Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
RspLookupAddOnProbeByOrdinal(
	IN PBTR_FILTER_RECORD Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
RspDecodeAddOnParameters(
	IN PBTR_FILTER_RECORD Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
RspCommitAddOnGroup(
	IN PPSP_COMMAND Command
	);

PRSP_ADDON_GROUP
RspLookupAddOnGroup(
	IN ULONG ProcessId
	);

ULONG
RspWriteAddOnGroup(
	IN HANDLE FileHandle
	);

ULONG
RspLoadAddOnGroup(
	IN HANDLE FileHandle
	);

//
// PspWriteProbeGroup
//

ULONG
PspWriteProbeGroup(
	IN HANDLE FileHandle
	);

extern ULONG RspHostThreadId;
extern ULONG RspProbeGroupCount;

extern LIST_ENTRY RspProbeGroupList;

#ifdef __cplusplus
}
#endif

#endif