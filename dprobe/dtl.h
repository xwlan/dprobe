//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#pragma once

#ifndef _DTL_H_
#define _DTL_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include <windows.h>
#include <tchar.h>
#include "btr.h"
#include "bsp.h"
#include "psp.h"


typedef enum _DTL_STATUS {
	STATUS_DTL_DISASTER                = 0xE0003001,
	STATUS_DTL_STORAGE_FULL            = 0xE0003002,
	STATUS_DTL_INDEX_FULL              = 0xE0003003,
	STATUS_DTL_WRITE_FAILED	           = 0xE0003004,
	STATUS_DTL_VIEW_FAILED             = 0xE0003005,
	STATUS_DTL_INDEX_COMMIT_FAILED     = 0xE0003006,
	STATUS_DTL_INVALID_INDEX           = 0xE0003007,
	STATUS_DTL_BUFFER_LIMIT            = 0xE0003008,
	STATUS_DTL_EXCEPTION               = 0xE0003009,
	STATUS_DTL_INIT_FAILED             = 0xE000300A,
	STATUS_DTL_STOP_WORKER             = 0xE000300B,
	STATUS_DTL_INVALID_PATH            = 0xE000300C,
	STATUS_DTL_CREATEFILE_FAILED       = 0xE000300D,
	STATUS_DTL_WRITEFILE_FAILED        = 0xE000300E,
	STATUS_DTL_OUTOFMEMORY             = 0xE000300F,
	STATUS_DTL_INVALID_PARAMETER       = 0xE0003010,
	STATUS_DTL_CREATEMAPPING_FAILED    = 0xE0003011,
	STATUS_DTL_BAD_CHECKSUM            = 0xE0003012,
	STATUS_DTL_BUFFER_LIMITED          = 0xE0003013,
} DTL_STATUS, *PDTL_STATUS;


#define DTL_FILE_SIGNATURE        ((ULONG)'ltd')
#define DTL_INVALID_INDEX_VALUE   ((ULONG64)(-1))
#define	DTL_ALIGNMENT_UNIT        ((ULONG)(1024 * 64))
#define DTL_COPY_UNIT             ((ULONG)(1024 * 1024 * 16))
#define	DTL_INCREMENT_UNIT        ((ULONG)(1024 * 1024 * 64))


typedef enum _DTL_FILE_MODE {
	DTL_FILE_CREATE,
	DTL_FILE_OPEN,
} DTL_FILE_MODE;

typedef enum _DTL_FILE_TYPE {
	DTL_FILE_DATA,
	DTL_FILE_INDEX,
	DTL_FILE_QUERY,
} DTL_FILE_TYPE;

//
// N.B. DTL_WORK_MODE can hold multiple bits
//
 
typedef enum _DTL_WORK_MODE {
    DTL_WORK_CAPTURE = 1,
    DTL_WORK_REPORT  = 2,
    DTL_WORK_FILTER  = 4,
} DTL_WORK_MODE;

typedef enum _DTL_INDEX_FLAG {
	DTL_INDEX_NULL,
	DTL_INDEX_FILTER,
} DTL_INDEX_FLAG;

typedef enum _DTL_FLUSH_MODE {
	DTL_FLUSH_DROP,
	DTL_FLUSH_QUERY,
	DTL_FLUSH_COMMIT,
} DTL_FLUSH_MODE;
	
typedef enum DTL_COMMIT_MODE {
	DTL_COMMIT_DROP,
	DTL_COMMIT_ONLY, 
	DTL_COMMIT_CLEAN,
} DTL_COMMIT_MODE;

typedef enum _DTL_SAVE_TYPE {
	DTL_SAVE_DTL = 1,
	DTL_SAVE_CSV = 2,
} DTL_SAVE_TYPE;

typedef struct _DTL_INDEX{
	ULONG64 Offset;
	ULONG Length;
	ULONG Flag;
} DTL_INDEX, *PDTL_INDEX;

typedef struct _DTL_SPAN {
	PDTL_INDEX First;
	PDTL_INDEX Last;
} DTL_SPAN, *PDTL_SPAN;

struct _DTL_FILE; 

typedef struct _DTL_MAP {
	LIST_ENTRY ListEntry;
	HANDLE ObjectHandle;
	PVOID Address;
	ULONG64 Offset;
	ULONG64 Length;
	ULONG64 LogicalFirst;
	ULONG64 LogicalLast;
	ULONG64 PhysicalFirst;
	ULONG64 PhysicalLast;
	struct _DTL_FILE *FileObject;
} DTL_MAP, *PDTL_MAP;

typedef struct _DTL_FILE {
	LIST_ENTRY ListEntry;
	DTL_FILE_MODE Mode;
	DTL_FILE_TYPE Type;
	ULONG64 BeginOffset;
	ULONG64 FileLength;
	ULONG64 DataLength;
	ULONG64 RecordCount;
	HANDLE ObjectHandle;
	PDTL_MAP MapObject;
	WCHAR Path[MAX_PATH];
	ULONG64 CacheMiss;
	ULONG64 CacheHit;
} DTL_FILE, *PDTL_FILE;

typedef struct _DTL_FILE_HEADER {
	ULONG Signature;
	ULONG CheckSum;
	USHORT MajorVersion;
	USHORT MinorVersion;
	ULONG  FeatureFlag;
	SYSTEMTIME StartTime;
	SYSTEMTIME EndTime;
	ULONG64 TotalLength;
	ULONG64 RecordLocation;
	ULONG64 RecordLength;
	ULONG64 IndexLocation;
	ULONG64 IndexNumber;
	ULONG64 IndexLength;
	ULONG64 StackLocation;
	ULONG64 StackLength;
	ULONG64 ProbeLocation;
	ULONG64 ProbeLength;
	ULONG64 SummaryLocation;
	ULONG64 SummaryLength;
	ULONG64 Reserved[2];
} DTL_FILE_HEADER, *PDTL_FILE_HEADER;

C_ASSERT(sizeof(DTL_FILE_HEADER) == 160);

//
// dtl probe group adapted from original rsp design
//

typedef enum _DTL_DECODE_FIELD {
	DTL_DECODE_TIME,
	DTL_DECODE_NAME,
	DTL_DECODE_PID,
	DTL_DECODE_TID,
	DTL_DECODE_PROBE,
	DTL_DECODE_PROVIDER,
	DTL_DECODE_RETURN,
	DTL_DECODE_DURATION,
	DTL_DECODE_DETAIL,
	DTL_DECODE_OTHERS,
} DTL_DECODE_FIELD, *PDTL_DECODE_FIELD;

typedef struct _DTL_PROBE {
	PVOID Address;
	ULONG Flag;
	WCHAR ModuleName[64];
	WCHAR ProcedureName[64];
	LIST_ENTRY Entry;
} DTL_PROBE, *PDTL_PROBE;

typedef struct _DTL_MANUAL_GROUP {
	ULONG ProcessId;
	ULONG NumberOfProbes;
	WCHAR ProcessName[64];
	WCHAR FullPathName[MAX_PATH];
	LIST_ENTRY ListEntry;
	LIST_ENTRY ProbeListHead;
} DTL_MANUAL_GROUP, *PDTL_MANUAL_GROUP; 

typedef struct _DTL_ADDON_GROUP {
	ULONG ProcessId;
	WCHAR ProcessName[64];
	WCHAR FullPathName[MAX_PATH];
	LIST_ENTRY ListEntry;
} DTL_ADDON_GROUP, *PDTL_ADDON_GROUP;

struct _PDB_OBJECT;
struct _CONDITION_OBJECT;
struct _DTL_OBJECT;

typedef struct _PDB_OBJECT *PPDB_OBJECT;
typedef struct _CONDITION_OBJECT *PCONDITION_OBJECT;
typedef struct _DTL_OBJECT *PDTL_OBJECT;
typedef struct _BSP_QUEUE *PBSP_QUEUE;

typedef struct _DTL_QUEUE_THREAD_CONTEXT {
	PDTL_OBJECT DtlObject;
	PBSP_QUEUE QueueObject;
} DTL_QUEUE_THREAD_CONTEXT, *PDTL_QUEUE_THREAD_CONTEXT;

typedef ULONG
(CALLBACK *DTL_COUNTER_CALLBACK)(
	IN HWND hWnd,
	IN ULONG RecordCount
	);

typedef ULONG
(CALLBACK *DTL_CLEAR_CALLBACK)(
	IN HWND hWnd
	);

typedef struct _DTL_PROBE_COUNTER {
	LIST_ENTRY ListEntry;
	PVOID Address;
	WCHAR Name[MAX_PATH];
	WCHAR Module[MAX_PATH];
	WCHAR Provider[MAX_PATH];
	ULONG ProcessId;
	ULONG RecordPercent;
	ULONG StackSampleCount;
	ULONG64 RecordCount;
	ULONG AverageDuration;
	ULONG MaximumDuration;
} DTL_PROBE_COUNTER, *PDTL_PROBE_COUNTER;

typedef struct _DTL_COUNTER_SET {
	LIST_ENTRY ListEntry;
	LIST_ENTRY CounterList;
	WCHAR ProcessName[MAX_PATH];
	ULONG ProcessId;
	ULONG NumberOfCounter;
	ULONG RecordPercent;
	ULONG64 RecordCount;
} DTL_COUNTER_SET, *PDTL_COUNTER_SET;

typedef struct _DTL_COUNTER_OBJECT {
	LIST_ENTRY CounterSetList;
	ULONG NumberOfCounterSet;
	PDTL_OBJECT DtlObject;
	ULONG64 LastRecord;
} DTL_COUNTER_OBJECT, *PDTL_COUNTER_OBJECT;

//
// DTL_COUNTER_SUMMARY is the format of summary report
// written to dtl file
//

//
// ULONG NumberOfCounterSet;
// DTL_COUNTER_SET Set1;
// DTL_PROBE_COUNTER Counter1[N1];
// DTL_COUNTER_SET Set2;
// DTL_PROBE_COUNTER Counter2[N2];
// ...
//

//
// DtlSharedData consumer must register a callback
// to get called when dtl update its internal status
//

struct _DTL_SHARED_DATA;

typedef VOID
(*DTL_SHARED_DATA_CALLBACK)(
	IN struct _DTL_SHARED_DATA *SharedData,
	IN PVOID Context
	);

typedef struct _DTL_SHARED_DATA {
	ULONG NumberOfProcesses;
	ULONG NumberOfProbes;
	ULONG NumberOfRecords;
	ULONG NumberOfFilterRecords;
	ULONG64 DtlCacheSize;
	WCHAR DtlPath[MAX_PATH];
	PVOID Context;
	DTL_SHARED_DATA_CALLBACK Callback;
} DTL_SHARED_DATA, *PDTL_SHARED_DATA;

struct _MSP_FILTER_FILE_OBJECT;

typedef struct _DTL_OBJECT{
	LIST_ENTRY ListEntry;
	DTL_FILE DataObject;
	DTL_FILE IndexObject;
	BSP_LOCK ObjectLock;
	BSP_LOCK QueueLock;
	HANDLE StopEvent;
	LIST_ENTRY Queue;
	LIST_ENTRY ManualGroupList;
	LIST_ENTRY AddOnGroupList;
	ULONG NumberOfManualGroup;
	ULONG NumberOfAddOnGroup;
	SYSTEMTIME StartTime;
	SYSTEMTIME EndTime;
	PDTL_FILE_HEADER Header;
	PPDB_OBJECT PdbObject;
    PCONDITION_OBJECT ConditionObject;
    PDTL_FILE QueryIndexObject;
	PDTL_COUNTER_OBJECT CounterObject;
	PDTL_COUNTER_OBJECT FilterCounter;
	struct _MSP_FILTER_FILE_OBJECT *FileObject;
} DTL_OBJECT, *PDTL_OBJECT;

typedef struct _DTL_INFORMATION {
	ULONG64 NumberOfRecords;
	ULONG64 IndexFileLength;
	ULONG64 DataFileLength;
} DTL_INFORMATION, *PDTL_INFORMATION;

typedef enum _DTL_QUEUE_NOTIFICATION {
	DTL_QUEUE_IO,
	DTL_QUEUE_STOP,
} DTL_QUEUE_NOTIFICATION;

typedef struct _DTL_IO_BUFFER {
	LIST_ENTRY ListEntry;
	PVOID Buffer;
	ULONG Length;
	ULONG NumberOfRecords;
} DTL_IO_BUFFER, *PDTL_IO_BUFFER;

typedef struct _DTL_SAVE_CONTEXT {
	DTL_SAVE_TYPE Type;
	BOOLEAN Filtered;
	PDTL_OBJECT Object;
	WCHAR Path[MAX_PATH];
} DTL_SAVE_CONTEXT, *PDTL_SAVE_CONTEXT;

typedef struct _DTL_COPY_CONTEXT {
    HWND hWnd;
    ULONG64 NumberOfRecords;
} DTL_COPY_CONTEXT, *PDTL_COPY_CONTEXT;

typedef struct _DTL_FILTER_CONTEXT {
    HWND hWndList;
    PDTL_OBJECT DtlObject;
    struct _CONDITION_OBJECT *FilterObject;
} DTL_FILTER_CONTEXT, *PDTL_FILTER_CONTEXT;

struct _PSP_PROBE;
struct _PSP_COMMAND;
struct _PSP_PROCESS;
struct _CONDITION_OBJECT;

ULONG
DtlInitialize(
	VOID	
	);

PDTL_OBJECT
DtlCreateObject(
	IN PWSTR Path
	);

PDTL_OBJECT
DtlOpenObject(
	IN PWSTR Path
	);

ULONG
DtlCreateFile(
	IN PDTL_OBJECT Object,
	IN PDTL_FILE FileObject,
	IN DTL_FILE_MODE Mode,
	IN DTL_FILE_TYPE Type
	);

ULONG
DtlDestroyFile(
	IN PDTL_FILE Object
	);

ULONG
DtlCreateMap(
	IN PDTL_FILE Object
	);

ULONG
DtlDestroyMap(
	IN PDTL_FILE Object
	);

ULONG64 
DtlRoundUpLength(
	IN ULONG64 Length,
	IN ULONG64 Unit 
	);

ULONG64 
DtlRoundDownLength(
	IN ULONG64 Length,
	IN ULONG64 Unit
	);

BOOLEAN
DtlAssertFileLength(
	IN HANDLE FileHandle,
	IN PDTL_FILE Object
	);

ULONG
DtlCopyRecord(
	IN PDTL_OBJECT Object,
	IN ULONG64 Number,
	IN PVOID Buffer,
	IN ULONG Length
	);

ULONG
DtlCopyFilteredRecord(
	IN PDTL_OBJECT Object,
	IN ULONG64 Number,
	IN PVOID Buffer,
	IN ULONG Length
	);

ULONG
DtlGetRecordPointer(
	IN PDTL_OBJECT Object,
	IN ULONG64 Number,
	OUT PVOID *Buffer,
	OUT PBOOLEAN NeedFree 
	);

ULONG
DtlWriteRecord(
	IN PDTL_OBJECT Object,
	IN PDTL_IO_BUFFER Buffer 
	);

ULONG
DtlWriteFile(
	IN PDTL_FILE DataObject,
	IN PDTL_FILE IndexObject,
	IN PDTL_INDEX Index,
	IN PDTL_IO_BUFFER
	);

BOOLEAN
DtlIsFileRequireIncrement(
	IN PDTL_FILE Object
	);

BOOLEAN
DtlIsLogicalCached(
	IN PDTL_FILE IndexObject,
	IN ULONG64 First,
	IN ULONG64 Last
	);

BOOLEAN
DtlIsPhysicalCached(
	IN PDTL_FILE IndexObject,
	IN ULONG64 First,
	IN ULONG64 Last,
	OUT PDTL_SPAN Span 
	);

ULONG
DtlAdjustIndexMap(
	IN PDTL_FILE Object,
	IN ULONG64 First,
	IN ULONG64 Last,
	OUT PDTL_SPAN Span
	);

ULONG
DtlAdjustQueryIndexMap(
	IN PDTL_FILE Object,
	IN ULONG64 First,
	IN ULONG64 Last,
	OUT PDTL_SPAN Span
	);

ULONG
DtlUpdateQueryCache(
	IN PDTL_OBJECT Object,
	IN ULONG64 First,
	IN ULONG64 Last
	);

ULONG
DtlAdjustDataMap(
	IN PDTL_FILE DataObject,
	IN PDTL_FILE IndexObject,
	IN ULONG64 MappedOffset,
	IN ULONG MappedLength
	);

ULONG
DtlAdjustDataMap2(
	IN PDTL_FILE DataObject,
	IN ULONG64 MappedOffset,
	IN ULONG MappedLength
	);

ULONG
DtlUpdateCache(
	IN PDTL_OBJECT Object,
	IN ULONG64 First,
	IN ULONG64 Last
	);

ULONG
DtlQueryInformation(
	IN PDTL_OBJECT Object,
	IN PDTL_INFORMATION Information
	);

ULONG
DtlQueueRecord(
	IN PDTL_OBJECT Object,
	IN PDTL_IO_BUFFER Buffer
	);

ULONG
DtlFlushQueue(
	IN PDTL_OBJECT Object,
	IN DTL_FLUSH_MODE Mode,
	OUT PLIST_ENTRY QueryListHead
	);

ULONG CALLBACK
DtlWorkThread(
	IN PVOID
	);

ULONG
DtlWriteDtl(
	IN PDTL_OBJECT Object,
	IN PWSTR Path,
	IN HWND hWnd
	);

ULONG
DtlWriteDtlFiltered(
	IN PDTL_OBJECT Object,
	IN PWSTR Path,
	IN HWND hWnd
	);

HANDLE
DtlCopyDataFile(
    IN PDTL_OBJECT Object,
    IN ULONG64 DataLength,
    IN PWSTR Path,
    IN HWND hWnd
    );

ULONG
DtlGetDiskFreeSpace(
	IN PWSTR Path,
	OUT PULARGE_INTEGER FreeBytes
	);

ULONG
DtlComputeCheckSum(
	IN PDTL_FILE_HEADER Header
	);

ULONG
DtlWriteSummaryReport(
	IN PDTL_OBJECT Object, 
	IN HANDLE FileHandle
	);

ULONG
DtlLoadSummaryReport(
	IN PDTL_OBJECT Object,
	IN HANDLE FileHandle
	);

ULONG 
DtlWriteManualGroup(
	IN PDTL_OBJECT Object,
	IN HANDLE FileHandle
	);

PDTL_PROBE
DtlQueryManualProbe(
	IN PDTL_OBJECT Object,
	IN ULONG ProcessId,
	IN PVOID Address
	);

ULONG
DtlLookupProbeByAddress(
	IN PDTL_OBJECT Object,
	IN ULONG ProcessId,
	IN PVOID Address,
	IN PWCHAR Buffer, 
	IN ULONG BufferLength
	);

PDTL_ADDON_GROUP
DtlLookupAddOnGroup(
	IN PDTL_OBJECT Object,
	IN ULONG ProcessId
	);

ULONG
DtlLookupProcessName(
	IN PDTL_OBJECT Object,
	IN ULONG ProcessId,
	IN PWCHAR Buffer,
	IN ULONG BufferLength
	);

PDTL_MANUAL_GROUP
DtlQueryManualGroup(
	IN PDTL_OBJECT Object,
	IN ULONG ProcessId
	);

PDTL_PROBE
DtlCreateProbe(
	IN struct _PSP_PROBE *Probe
	);

ULONG
DtlInsertVariantProbe(
	IN PDTL_PROBE RootObject,
	IN PDTL_PROBE VariantObject 
	);

ULONG
DtlLoadAddOnGroup(
	IN PDTL_OBJECT Object,
	IN HANDLE FileHandle
	);

PDTL_MANUAL_GROUP 
DtlCreateManualGroup(
	IN PDTL_OBJECT Object,
	IN struct _PSP_PROCESS *Process
	);

ULONG
DtlCommitProbe(
	IN PDTL_OBJECT Object,
	IN PDTL_MANUAL_GROUP Group,
	IN struct _PSP_PROBE *Probe 
	);

ULONG
DtlCommitAddOnGroup(
	IN PDTL_OBJECT Object,
	IN struct _PSP_COMMAND *Command
	);

ULONG
DtlWriteAddOnGroup(
	IN PDTL_OBJECT Object,
	IN HANDLE FileHandle
	);

PDTL_PROBE
DtlQueryProbeByAddress(
	IN PDTL_OBJECT Object,
	IN ULONG ProcessId,
	IN PVOID Address,
	IN ULONG TypeFlag 
	);

ULONG
DtlDecodeAddOnParameters(
	IN PBTR_FILTER_RECORD Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
DtlDecodeParameters(
	IN PBTR_EXPRESS_RECORD Record,
	IN PDTL_PROBE Probe,
	IN PWCHAR Buffer,
	IN ULONG Length,
	OUT PULONG ActualLength
	);

PDTL_PROBE
DtlQueryProbeByName(
	IN PDTL_OBJECT Object,
	IN ULONG ProcessId,
	IN PWSTR ModuleName,
	IN PWSTR ProcedureName,
	IN ULONG TypeFlag
	);

PDTL_PROBE
DtlQueryProbeByRecord(
	IN PDTL_OBJECT Object,
	IN PBTR_RECORD_HEADER Record
	);

ULONG
DtlLookupAddOnName(
	IN PDTL_OBJECT Object,
	IN PBTR_FILTER_RECORD Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
DtlDecodeCaller(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

BOOLEAN
DtlIsCachedFile(
	IN PDTL_OBJECT Object,
	IN PWSTR Path
	);

ULONG CALLBACK
DtlQueryFilterCallback(
	IN HWND hWnd,
	IN PVOID Context
	);

ULONG CALLBACK
DtlWriteDtlCallback(
	IN HWND hWnd,
	IN PVOID Context
	);

ULONG CALLBACK
DtlWriteCsvCallback(
	IN HWND hWnd,
	IN PVOID Context
	);

ULONG
DtlWriteCsvRound(
	IN HWND hWnd,
	IN ULONG64 RecordNumber,
	IN ULONG64 TotalNumber,
	IN PDTL_OBJECT Object,
	IN PDTL_INDEX IndexCache,
	IN ULONG NumberOfIndex,
	IN HANDLE DataHandle,
	IN HANDLE CsvHandle
	);

ULONG
DtlWriteCsvRoundFiltered(
	IN HWND hWnd,
	IN ULONG64 RecordNumber,
	IN ULONG64 TotalNumber,
	IN PDTL_OBJECT Object,
	IN PDTL_INDEX IndexCache,
	IN ULONG NumberOfIndex,
	IN HANDLE DataHandle,
	IN HANDLE CsvHandle
	);

ULONG
DtlLoadDtl(
	IN PWSTR Path,
	IN PDTL_OBJECT Object
	);

ULONG 
DtlLoadManualGroup(
	IN PDTL_OBJECT Object,
	IN HANDLE FileHandle
	);

ULONG
DtlDecodeReturn(
	IN PBTR_RECORD_HEADER Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
DtlLookupAddOnProbeByOrdinal(
	IN PDTL_OBJECT Object,
	IN PBTR_FILTER_RECORD Record,
	IN PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
DtlDropCache(
	IN PDTL_OBJECT Object
	);

PDTL_FILE
DtlCreateQueryFile(
    IN PDTL_OBJECT Object 
    );

ULONG
DtlDestroyQueryFile(
	IN PDTL_OBJECT Object
	);

ULONG
DtlApplyQueryFilterLite(
	IN PDTL_OBJECT DtlObject,
	IN PCONDITION_OBJECT FilterObject
	);

ULONG
DtlApplyQueryFilter(
    IN PDTL_OBJECT DtlObject,
    IN PCONDITION_OBJECT FilterObject,
    IN HWND hWndList
    );

BOOLEAN
DtlIsFilteredRecord(
	IN PDTL_OBJECT DtlObject,
    IN PCONDITION_OBJECT FilterObject,
    IN PBTR_RECORD_HEADER Record
    );

ULONG
DtlFreeFilterObject(
	IN PCONDITION_OBJECT Object
	);

ULONG64
DtlCopyFilterIndex(
    IN PDTL_OBJECT DtlObject, 
    IN ULONG64 StorageIndex,
    IN PDTL_FILE QueryObject
    );

ULONG
DtlWriteQueryFile(
    IN PDTL_OBJECT Object,
    IN PDTL_INDEX Index,
    IN ULONG NumberOfIndex
    );

ULONG
DtlDecodeStackTrace(
	IN PDTL_OBJECT Object,
	IN PDTL_IO_BUFFER Buffer 
	);

LONG CALLBACK 
DtlExceptionFilter(
	IN PEXCEPTION_POINTERS Pointers
	);

BOOLEAN
DtlIsAllowEnterDtl(
    PDTL_OBJECT Object
    );

ULONG
DtlDestroyObject(
	IN PDTL_OBJECT Object
	);

VOID
DtlFreeManualGroup(
	IN PLIST_ENTRY ListHead
	);

VOID
DtlFreeAddOnGroup(
	IN PLIST_ENTRY ListHead
	);

ULONG
DtlOpenReport(
	IN PWSTR Path,
	OUT PDTL_OBJECT *DtlObject
	);

ULONG
DtlUpdateCounters(
	IN PDTL_OBJECT DtlObject,
	IN PDTL_IO_BUFFER Buffer
	);

ULONG
DtlUpdateCounter(
	IN PDTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record
	);

ULONG
DtlUpdateFilterCounters(
	IN PDTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record 
	);

ULONG
DtlDestroyFilterCounters(
	IN PDTL_OBJECT Object
	);

PDTL_COUNTER_OBJECT
DtlQueryCounters(
	IN PDTL_OBJECT DtlObject
	);

PDTL_COUNTER_SET
DtlLookupCounterSet(
	IN PDTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record,
	IN BOOLEAN FilterCounter
	);

PDTL_PROBE_COUNTER
DtlLookupProbeCounter(
	IN PDTL_OBJECT DtlObject,
	IN PDTL_COUNTER_SET Set,
	IN PBTR_RECORD_HEADER Record
	);

ULONG
DtlCleanCache(
	IN PDTL_OBJECT Object,
	IN BOOLEAN Save
	);

VOID
DtlRegisterSharedDataCallback(
	IN PDTL_OBJECT Object,
	IN DTL_SHARED_DATA_CALLBACK Callback,
	IN PVOID Context
	);

BOOLEAN
DtlIsReportFile(
	IN PDTL_OBJECT Object
	);

VOID
DtlDeleteGarbageFiles(
	IN PWSTR Path
	);

//
// STATUS_DTL_BAD_CHECKSUM indicates it's a bad report file,
// ignore all other return status.
//

ULONG
DtlIsValidReport(
	IN PWSTR Path
	);

//
// Dtl Version 
//

extern USHORT DtlMajorVersion;
extern USHORT DtlMinorVersion;

//
// Running DtlObject List
//

extern LIST_ENTRY DtlObjectList;
extern PSTR DtlCsvRecordColumn;
extern PSTR DtlCsvRecordFormat;
extern DTL_SHARED_DATA DtlSharedData;

#ifdef __cplusplus 
}
#endif

#endif
