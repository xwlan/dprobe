//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _MSPDTL_H_
#define _MSPDTL_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include "btr.h"
#include "conditiondlg.h"
#include "mspqueue.h"
#include <rpc.h>


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
#define MSP_STATUS_NO_DECODE            0xE1000012
#define MSP_STATUS_BAD_RECORD           0xE1000013 
#define MSP_STATUS_OUTOFMEMORY          0xE1000014
#define MSP_STATUS_SETFILEPOINTER       0xE1000015
#define MSP_STATUS_NO_RECORD            0xE1000016
#define MSP_STATUS_BUFFER_TOO_SMALL     0xE1000017
#define MSP_STATUS_NO_SEQUENCE          0xE1000018
#define MSP_STATUS_MAPVIEWOFFILE        0xE1000019
#define MSP_STATUS_CREATEFILEMAPPING    0xE1000020
#define MSP_STATUS_CREATEFILE           0xE1000021
#define MSP_STATUS_NOT_IMPLEMENTED      0xE1000022
#define MSP_STATUS_NO_STACKTRACE        0xE1000023
#define	MSP_STATUS_ERROR				0xFFFFFFFF

typedef enum _MSP_FILE_TYPE {
	FileGlobalIndex,
	FileGlobalData,
	FileStackEntry,
	FileStackText,
	FileFilterIndex,
	FileDtlReport,
} MSP_FILE_TYPE, *PMSP_FILE_TYPE;

typedef enum _MSP_FILE_FLAG {
	FLAG_FILE_PINNED = 0x00000001,
} MSP_FILE_FLAG, *PMSP_FILE_FLAG;

typedef BTR_SPINLOCK  MSP_SPINLOCK;
typedef PBTR_SPINLOCK PMSP_SPINLOCK;
typedef CRITICAL_SECTION MSP_CS_LOCK;
typedef CRITICAL_SECTION *PMSP_CS_LOCK;

typedef struct _MSP_SECURITY_ATTRIBUTE {
	SECURITY_ATTRIBUTES Attribute;
	PSECURITY_DESCRIPTOR Descriptor;
	PACL Acl;
	PSID Sid;
} MSP_SECURITY_ATTRIBUTE, *PMSP_SECURITY_ATTRIBUTE;

typedef struct _MSP_FILE_OBJECT {
	MSP_SPINLOCK Lock;
	LIST_ENTRY ListEntry;
	HANDLE FileObject;
	ULONG64 FileLength;
	ULONG64 ValidLength;
	ULONG NumberOfRecords;
	MSP_FILE_TYPE Type;
	ULONG ProcessId;
	ULONG ThreadId;
	USHORT FileFlag;
	struct _MSP_MAPPING_OBJECT *Mapping;
	LARGE_INTEGER LastAccessTime;
	SYSTEMTIME StartTime;
	SYSTEMTIME EndTime;
	WCHAR Path[MAX_PATH];
} MSP_FILE_OBJECT, *PMSP_FILE_OBJECT;

typedef struct _MSP_FILTER_FILE_OBJECT {

	LIST_ENTRY ListEntry;
	HANDLE FileObject;
	ULONG64 FileLength;
	ULONG64 ValidLength;
	struct _MSP_MAPPING_OBJECT *MappingObject;
	struct _MSP_MAPPING_OBJECT *MappingObject2;

	ULONG NumberOfRecords;
	ULONG FilterFlag;
	ULONG LastScannedSequence;

	struct _CONDITION_OBJECT *FilterObject;
	struct _MSP_DTL_OBJECT *DtlObject;
	HANDLE WorkItemEvent;

	WCHAR Path[MAX_PATH];

} MSP_FILTER_FILE_OBJECT, *PMSP_FILTER_FILE_OBJECT;

typedef enum _MSP_MAPPING_TYPE {
	DiskFileBacked,
	PageFileBacked,
} MSP_MAPPING_TYPE, *PMSP_MAPPING_TYPE;

typedef struct _MSP_MAPPING_OBJECT {

	LIST_ENTRY ListEntry;
	MSP_MAPPING_TYPE Type;
	HANDLE MappedObject;

	union {
		PVOID MappedVa;
		struct _BTR_FILE_INDEX *Index;
		PULONG FilterIndex;
	};

	ULONG64 MappedOffset;
	ULONG MappedLength;
	ULONG Spare0;

	ULONG FirstSequence;
	ULONG LastSequence;

	struct _MSP_FILE_OBJECT *FileObject;
	struct _MSP_MAPPING_OBJECT *Next;
	ULONG64 FileLength;

} MSP_MAPPING_OBJECT, *PMSP_MAPPING_OBJECT;

//
// N.B. Filter index is only a sequence number, we only scan
// filtered sequence number, don't require to track its full index,
// when access filtered record, we directly pass the sequence to
// MspCopyRecord().
//

typedef ULONG MSP_FILTER_INDEX, *PMSP_FILTER_INDEX;

//
// File and mapping adjustment macros
//

#define MSP_INDEX_MAP_INCREMENT  (1024 * 64)
#define MSP_DATA_MAP_INCREMENT   (1024 * 64)
#define MSP_INDEX_FILE_INCREMENT (1024 * 1024 * 4)
#define MSP_DATA_FILE_INCREMENT  (1024 * 1024 * 64)
#define SEQUENCES_PER_INCREMENT         (MSP_INDEX_MAP_INCREMENT/sizeof(BTR_FILE_INDEX))
#define FILTER_SEQUENCES_PER_INCREMENT  (MSP_INDEX_MAP_INCREMENT/sizeof(MSP_FILTER_INDEX))

#define MAXIMUM_MAPPING_OFFSET(_M) \
	(_M->MappedOffset + _M->MappedLength)

#define MINIMUM_MAPPING_OFFSET(_M) \
	(_M->MappedOffset)

#define VA_FROM_OFFSET(_M, _O) \
	(PVOID)(((ULONG_PTR)_M->MappedVa + (ULONG_PTR)(_O - _M->MappedOffset)))

#define OFFSET_FROM_SEQUENCE(_S) \
	(ULONG64)((_S * sizeof(BTR_FILE_INDEX)))

#define IS_SEQUENCE_MAPPED(_M, _S) \
	((_M->FirstSequence <= (ULONG)_S) && ((ULONG)_S <= _M->LastSequence))

#define VA_FROM_SEQUENCE(_M, _S) \
	((PBTR_FILE_INDEX)&(_M->Index[_S - _M->FirstSequence]));

#define IS_MAPPED_OFFSET(_M, _S, _E) \
	(_S >= _M->MappedOffset && _E <= _M->MappedOffset + _M->MappedLength)

#define IS_FILE_OFFSET(_M, _E) \
	(_E <= _M->FileLength)


//
// Scatter Gather Structures
//

typedef struct _MSP_READ_SEGMENT {
	PVOID Buffer;
	ULONG Length;
} MSP_READ_SEGMENT, *PMSP_READ_SEGMENT;

typedef enum _MSP_COPY_MODE {
	CopyMemoryMapped,
	CopyReadFile,
} MSP_COPY_MODE, *PMSP_COPY_MODE;

typedef enum _MSP_COUNTER_LEVEL {
	CounterEntryLevel,
	CounterGroupLevel,
	CounterRootLevel,
} MSP_COUNTER_LEVEL, *PMSP_COUNTER_LEVEL;

typedef struct _MSP_COUNTER_ENTRY {
	MSP_COUNTER_LEVEL Level;
	PBTR_COUNTER_ENTRY Active;
	LIST_ENTRY ListEntry;
	struct _MSP_COUNTER_GROUP *Group;
	BTR_COUNTER_ENTRY Retire;
} MSP_COUNTER_ENTRY, *PMSP_COUNTER_ENTRY;

typedef struct _MSP_COUNTER_GROUP {
	MSP_COUNTER_LEVEL Level;
	PBTR_COUNTER_TABLE Table;
	HANDLE Handle;
	BTR_COUNTER_ENTRY Retire;
	LIST_ENTRY ListEntry;
	struct _MSP_COUNTER_OBJECT *Object;
	struct _MSP_PROCESS *Process;
	WCHAR Name[MAX_PATH];
	ULONG NumberOfEntries;
	LIST_ENTRY ActiveListHead;
	LIST_ENTRY RetireListHead;
} MSP_COUNTER_GROUP, *PMSP_COUNTER_GROUP;

//
// Pack on disk structure
//

#pragma pack(push, 1)

typedef struct _MSP_DTL_COUNTER_GROUP {
	ULONG ProcessId;
	ULONG NumberOfEntries;
	ULONG NumberOfCalls;
	ULONG NumberOfStackTraces;
	ULONG NumberOfExceptions;
	ULONG MaximumDuration;
	ULONG MinimumDuration;
	MSP_COUNTER_ENTRY Entry[ANYSIZE_ARRAY];
} MSP_DTL_COUNTER_GROUP, *PMSP_DTL_COUNTER_GROUP;

#pragma pack(pop)

typedef struct _MSP_COUNTER_OBJECT {
	MSP_COUNTER_LEVEL Level;
	ULONG NumberOfCalls;
	ULONG NumberOfStackTraces;
	ULONG NumberOfExceptions;
	ULONG MaximumDuration;
	ULONG MinimumDuration;
	ULONG NumberOfGroups;
	LIST_ENTRY GroupListHead;
	WCHAR Name[MAX_PATH];
	struct _MSP_DTL_OBJECT *DtlObject;
} MSP_COUNTER_OBJECT, *PMSP_COUNTER_OBJECT;

//
// N.B. Ensure MSP_STACKTRACE_ENTRY, MSP_STACKTRACE_TEXT
// are appropriately aligned to make access them easier.
//

typedef struct _MSP_STACKTRACE_ENTRY {
	ULONG ProcessId;
	ULONG Hash;
	ULONG Count;
	USHORT Depth;
	USHORT Flag;
	PVOID Probe;
	ULONG Next;
#if defined(_M_X64)
	ULONG Spare0[5];
#endif
	union {
		PVOID Frame[MAX_STACK_DEPTH];	
		ULONG Text[MAX_STACK_DEPTH];
	};
} MSP_STACKTRACE_ENTRY, *PMSP_STACKTRACE_ENTRY;

#if defined(_M_IX86)
C_ASSERT(sizeof(MSP_STACKTRACE_ENTRY) == 256);
#elif defined (_M_X64)
C_ASSERT(sizeof(MSP_STACKTRACE_ENTRY) == 512);
#endif

#define MAX_STACKTRACE_TEXT 56

typedef struct _MSP_STACKTRACE_TEXT {
	ULONG Hash;
	ULONG Next;
	CHAR Text[MAX_STACKTRACE_TEXT];
} MSP_STACKTRACE_TEXT, *PMSP_STACKTRACE_TEXT;

C_ASSERT(sizeof(MSP_STACKTRACE_TEXT) == 64);

typedef struct _MSP_STACKTRACE_DECODED {
	ULONG Depth;
	CHAR Text[MAX_STACK_DEPTH][MAX_STACKTRACE_TEXT];
} MSP_STACKTRACE_DECODED, *PMSP_STACKTRACE_DECODED;

typedef struct _MSP_STACKTRACE_CONTEXT {
	LIST_ENTRY ListEntry;
	ULONG ProcessId;
	HANDLE ProcessHandle;
} MSP_STACKTRACE_CONTEXT, *PMSP_STACKTRACE_CONTEXT;

typedef struct _MSP_STACKTRACE_OBJECT {
	
	DECLSPEC_CACHEALIGN MSP_CS_LOCK Lock;
	LIST_ENTRY ContextListHead;
	PMSP_STACKTRACE_CONTEXT Context;

	ULONG EntryTotalBuckets;
    ULONG EntryFixedBuckets;
	ULONG EntryFilledFixedBuckets;
	ULONG EntryFirstHitBuckets;
	ULONG EntryChainedBuckets;

	ULONG TextTotalBuckets;
	ULONG TextFixedBuckets;
	ULONG TextFilledFixedBuckets;
	ULONG TextFirstHitBuckets;
	ULONG TextChainedBuckets;

	BOOLEAN EntryFixedBucketsFull;
	BOOLEAN TextFixedBucketsFull;

	BTR_BITMAP EntryBitmap;
	BTR_BITMAP TextBitmap;

	PMSP_FILE_OBJECT EntryObject;
	PMSP_FILE_OBJECT TextObject;
	PMSP_MAPPING_OBJECT EntryExtendMapping;
	PMSP_MAPPING_OBJECT TextExtendMapping;

	union {
		PMSP_STACKTRACE_ENTRY Buckets;
	} u1;

	union {
		PMSP_STACKTRACE_TEXT Buckets;
	} u2;

	LARGE_INTEGER EntryOffset;
	LARGE_INTEGER EntrySize;
	LARGE_INTEGER TextOffset;
	LARGE_INTEGER TextSize;

} MSP_STACKTRACE_OBJECT, *PMSP_STACKTRACE_OBJECT;

//
// DTL Structures
//

struct _MSP_PROBE;
struct _MSP_PROBE_GROUP;
struct _MSP_MESSAGE_PORT;
struct _MSP_FILTER;
struct _MSP_PROCESS;
struct _MSP_COMMAND;
struct _MSP_DTL_OBJECT;

typedef enum _MSP_GROUP_TYPE {
	FastGroup,
	FilterGroup,
} MSP_GROUP_TYPE, *PMSP_GROUP_TYPE;


#define MSP_DTL_SIGNATURE   'ltd'
#define DTL_COPY_UNIT       (1024*1024*4)

#pragma pack(push, 1)
typedef struct _MSP_DTL_HEADER {
	ULONG Signature;
	ULONG CheckSum;
	USHORT MajorVersion;
	USHORT MinorVersion;
	BOOLEAN Is64Bits;
	BOOLEAN Spare0[3];
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
} MSP_DTL_HEADER, *PMSP_DTL_HEADER;
#pragma pack(pop)

typedef enum _MSP_DTL_TYPE {
	DTL_LIVE,
	DTL_REPORT,
} MSP_DTL_TYPE, *PMSP_DTL_TYPE;

typedef struct _MSP_DTL_OBJECT {

	LIST_ENTRY ListEntry;

	MSP_DTL_TYPE Type;
	BOOLEAN IndexerStarted;
	BOOLEAN StackTraceStarted;
	BOOLEAN Spare0[2];

	LIST_ENTRY ProcessListHead;
	MSP_CS_LOCK ProcessListLock;
	ULONG ProcessCount;
	ULONG ActiveProcessCount;

	MSP_DTL_HEADER Header;
	MSP_FILE_OBJECT IndexObject;
	MSP_FILE_OBJECT DataObject;
	MSP_STACKTRACE_OBJECT StackTrace;
	MSP_COUNTER_OBJECT CounterObject;

	struct _MSP_FILTER_FILE_OBJECT *FilterFileObject;

	SYSTEMTIME StartTime;
	SYSTEMTIME EndTime;

	WCHAR Path[MAX_PATH];

} MSP_DTL_OBJECT, *PMSP_DTL_OBJECT;

typedef enum _MSP_SAVE_TYPE {
	MSP_SAVE_DTL    = 0,
	MSP_SAVE_CSV    = 1,
	MSP_SAVE_TXT    = 2,
	MSP_SAVE_SQLITE = 3,
} MSP_SAVE_TYPE, *PMSP_SAVE_TYPE;


typedef BOOLEAN
(CALLBACK *MSP_WRITE_CALLBACK)(
	IN PVOID Context,
	IN ULONG Percent,
	IN BOOLEAN Abort,
	IN ULONG Error
	);

typedef struct _MSP_SAVE_CONTEXT {
	PMSP_DTL_OBJECT Object;
	MSP_SAVE_TYPE Type;
	WCHAR Path[MAX_PATH];
	ULONG Count;
	BOOLEAN Filtered;
	BOOLEAN AdjustFilterIndex;
	MSP_WRITE_CALLBACK Callback;
	PVOID Context;
} MSP_SAVE_CONTEXT, *PMSP_SAVE_CONTEXT;

//
// MSP Private 
//

ULONG
MspInitialize(
	IN MSP_MODE Mode,
	IN ULONG Flag,
	IN PWSTR RuntimeName,
	IN PWSTR RuntimePath,
	IN ULONG RuntimeFeature,
	IN PWSTR FilterPath,
	IN PWSTR CachePath
	);

ULONG
MspUninitialize(
	VOID
	);

ULONG
MspInitializeDtl(
	IN PWSTR CachePath	
	);

ULONG
MspValidateCachePath(
	IN PWSTR Path
	);

VOID
MspGetCachePath(
	OUT PWCHAR Buffer,
	IN ULONG Length
	);

ULONG
MspConfigureCache(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PWSTR Path
	);

ULONG
MspCreateSharedDataMap(
	VOID
	);

ULONG
MspStartIndexProcedure(
	IN PMSP_FILE_OBJECT FileObject
	);

PBTR_RECORD_HEADER
MspGetRecordPointer(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG Sequence
	);

ULONG
MspGetRecordPointerScatter(
	IN ULONG StartSequence,
	IN ULONG MaximumCount,
	OUT MSP_READ_SEGMENT Segment[],
	OUT ULONG CompleteCount
	);

ULONG WINAPI
MspCopyRecord(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG Sequence,
	IN PVOID Buffer,
	IN ULONG Size
	);

ULONG
MspCopyRecordEx(
	IN ULONG Sequence,
	IN PVOID Buffer,
	IN ULONG Size,
	OUT PBTR_FILE_INDEX Index
	);

ULONG
MspQueueFilterWorkItem(
	IN PMSP_FILTER_FILE_OBJECT FileObject
	);

ULONG CALLBACK
MspFilterWorkRoutine(
	IN PVOID Context
	);

PMSP_FILTER_FILE_OBJECT 
WINAPI
MspGetFilterFileObject(
	IN PMSP_DTL_OBJECT DtlObject
	);

PCONDITION_OBJECT
MspGetFilterCondition(
	IN PMSP_DTL_OBJECT DtlObject
	);

PCONDITION_OBJECT
MspAttachFilterCondition(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PCONDITION_OBJECT Condition
	);

PCONDITION_OBJECT
MspDetachFilterCondition(
	IN PMSP_DTL_OBJECT DtlObject
	);

ULONG WINAPI
MspGetFilteredRecordCount(
	IN PMSP_FILTER_FILE_OBJECT FileObject
	);

ULONG
MspComputeFilterScanStep(
	IN PMSP_FILTER_FILE_OBJECT FileObject
	);

ULONG WINAPI
MspCopyRecordFiltered(
	IN PMSP_FILTER_FILE_OBJECT FileObject,
	IN ULONG Sequence,
	IN PVOID Buffer,
	IN ULONG Size
	);

ULONG
MspWriteFilterIndex(
	IN PMSP_FILTER_FILE_OBJECT FileObject,
	IN PBTR_RECORD_HEADER Record,
	IN ULONG Sequence,
	IN ULONG FilterSequence
	);

PMSP_FILTER_FILE_OBJECT
MspCreateFilterFileObject(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PCONDITION_OBJECT Filter
	);

PMSP_FILTER_FILE_OBJECT
MspCreateFilterFileObjectForDtl(
	IN PCONDITION_OBJECT Filter,
	IN PMSP_FILE_OBJECT Dtl 
	);

ULONG
MspScanFilterRecord(
	IN PMSP_FILTER_FILE_OBJECT FileObject,
	IN ULONG ScanStep
	);

ULONG
MspCloseFileMapping(
	IN PMSP_MAPPING_OBJECT Mapping
	);

PCONDITION_OBJECT
MspDestroyFilterFileObject(
	IN PMSP_FILTER_FILE_OBJECT FileObject
	);

BOOLEAN
MspIsFilteredRecord(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PCONDITION_OBJECT FilterObject,
	IN PBTR_RECORD_HEADER Record
	);

ULONG WINAPI
MspReadRecord(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG Sequence,
	IN PVOID Buffer,
	IN ULONG Size
	);

ULONG
MspCopyMemory(
	IN PVOID Destine,
	IN PVOID Source,
	IN ULONG Size
	);

ULONG CALLBACK
MspIndexProcedure(
	IN PVOID Context
	);

ULONG
MspLookupIndexBySequence(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG Sequence,
	OUT PBTR_FILE_INDEX Index
	);

PMSP_MAPPING_OBJECT
MspLookupMappingObject(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG64 Offset
	);

PMSP_MAPPING_OBJECT
MspCreateMappingObject(
	IN PMSP_FILE_OBJECT FileObject,
	IN ULONG64 Offset,
	IN ULONG Size
	);

VOID
MspCloseMappingObject(
	IN PMSP_FILE_OBJECT FileObject
	);

ULONG
MspUpdateNextSequence(
	IN PMSP_FILE_OBJECT FileObject,
	IN PMSP_MAPPING_OBJECT MappingObject,
	IN PULONG NextSequence
	);

ULONG
MspExtendFileLength(
	IN HANDLE FileObject,
	IN ULONG64 Length
	);

ULONG
MspCloseCache(
	VOID
	);

ULONG
MspCloseFileObject(
	IN PMSP_FILE_OBJECT FileObject,
	IN BOOLEAN FreeRequired
	);

HANDLE
MspDuplicateHandle(
	IN HANDLE DestineProcess,
	IN HANDLE SourceHandle
	);

ULONG 
MspCompareMemoryPointer(
	IN PVOID Source,
	IN PVOID Destine,
	IN ULONG NumberOfPointers
	);

VOID
MspInitSpinLock(
	IN PMSP_SPINLOCK Lock,
	IN ULONG SpinCount
	);

VOID
MspAcquireSpinLock(
	IN PMSP_SPINLOCK Lock
	);

VOID
MspReleaseSpinLock(
	IN PMSP_SPINLOCK Lock
	);

VOID
MspInitCsLock(
	IN PMSP_CS_LOCK Lock,
	IN ULONG SpinCount
	);

VOID
MspAcquireCsLock(
	IN PMSP_CS_LOCK Lock
	);

VOID
MspReleaseCsLock(
	IN PMSP_CS_LOCK Lock
	);

VOID
MspDeleteCsLock(
	IN PMSP_CS_LOCK Lock
	);


ULONG
MspCreateSharedMemory(
	IN PWSTR ObjectName,
	IN SIZE_T Length,
	IN ULONG ViewOffset,
	IN SIZE_T ViewSize,
	OUT PMSP_MAPPING_OBJECT *Object
	);

typedef BOOLEAN 
(*DTL_WRITE_CALLBACK)(
	IN PVOID Context,
	IN ULONG Percent,
	IN PWSTR Text
	);

ULONG
MspSaveFile(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PWSTR Path,
	IN MSP_SAVE_TYPE Type,
	IN ULONG Count,
	IN BOOLEAN Filtered,
	IN MSP_WRITE_CALLBACK Callback,
	IN PVOID Context
	);

ULONG CALLBACK
MspWriteDtlWorkItem(
	IN PVOID Context
	);

ULONG 
MspWriteDtlFiltered(
	IN PMSP_SAVE_CONTEXT Context
	);

ULONG CALLBACK
MspWriteCsvWorkItem(
	IN PVOID Context
	);

ULONG
MspWriteCsvFiltered(
	IN PMSP_SAVE_CONTEXT Context
	);

ULONG
MspWriteCsvRound(
	IN PMSP_DTL_OBJECT Object,
	IN PBTR_FILE_INDEX IndexCache,
	IN ULONG NumberOfIndex,
	IN HANDLE DataHandle,
	IN HANDLE CsvHandle
	);

ULONG
MspWriteDtl(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PWSTR Path,
	IN HWND hWnd
	);

ULONG
MspComputeDtlCheckSum(
	IN PMSP_DTL_HEADER Header
	);

ULONG
MspComputeReplicaLength(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG NumberOfRecords,
	OUT PULONG64 IndexLength,
	OUT PULONG64 DataLength
	);

//
// replica in batch mode
//

HANDLE
MspReplicateData(
	IN PMSP_FILE_OBJECT IndexObject,
	IN PMSP_FILE_OBJECT DataObject,
	IN PMSP_SAVE_CONTEXT Context,
    IN ULONG64 IndexLength,
    IN ULONG64 DataLength,
	OUT BOOLEAN *Cancel
    );

//
// replica in record mode
//

HANDLE
MspReplicateRecord(
	IN PMSP_FILE_OBJECT IndexObject,
	IN PMSP_FILE_OBJECT DataObject,
	IN PMSP_SAVE_CONTEXT Context,
    IN ULONG64 IndexLength,
    IN ULONG64 DataLength,
	IN ULONG RecordCount,
	OUT BOOLEAN *Cancel
    );

ULONG
MspWriteStackTraceToDtl(
	IN PMSP_DTL_OBJECT DtlObject,
	IN HANDLE FileHandle,
	IN ULONG64 Location,
	PULONG64 CompleteLength 
	);

ULONG
MspLoadStackTraceFromDtl(
	IN PMSP_DTL_OBJECT DtlObject
	);

ULONG
MspWriteProbeToDtl(
	IN PMSP_DTL_OBJECT DtlObject,
	IN HANDLE FileHandle,
	IN ULONG64 Location,
	PULONG64 CompleteLength 
	);

ULONG
MspLoadProbeFromDtl(
	IN PMSP_DTL_OBJECT DtlObject
	);

#define BTR_MAXIMUM_RECORD_SIZE  (4096)
#define MSP_STACKTRACE_FILE_INCREMENT  (1024 * 1024 * 4)
#define MSP_STACKTRACE_MAP_INCREMENT   (1024 * 64)

#define MSP_FIXED_BUCKET_NUMBER  (40939)
#define MSP_HASH_BUCKET(_H) \
	((ULONG)((ULONG)_H % MSP_FIXED_BUCKET_NUMBER))

ULONG
MspCreateStackTrace(
	IN PMSP_STACKTRACE_OBJECT Database
	);

ULONG
MspCloseStackTrace(
	IN PMSP_STACKTRACE_OBJECT Object
	);

ULONG
MspSaveStackTrace(
	IN HANDLE Handle,
	IN PWSTR Path,
	OUT SIZE_T *Size
	);

PMSP_STACKTRACE_CONTEXT
MspLookupStackTraceContext(
	IN PMSP_STACKTRACE_OBJECT Database,
	IN ULONG ProcessId,
	IN BOOLEAN CreateIfNo
	);

BOOLEAN
MspAcquireContext(
	IN PMSP_STACKTRACE_OBJECT Database,
	IN ULONG ProcessId
	);

VOID
MspReleaseContext(
	IN PMSP_STACKTRACE_OBJECT Database,
	IN BOOLEAN Clear	
	);

PMSP_STACKTRACE_ENTRY
MspGetStackTraceEntry(
	IN PMSP_STACKTRACE_OBJECT Database,
	IN ULONG Bucket
	);

PMSP_STACKTRACE_ENTRY	
MspAllocateExtendEntryBucket(
	IN PMSP_STACKTRACE_OBJECT Database,
	OUT PULONG NewBucket
	);

PMSP_STACKTRACE_ENTRY
MspLookupStackTrace(
	IN PMSP_STACKTRACE_OBJECT Database,
	IN PVOID Probe,
	IN ULONG Hash,
	IN ULONG Depth
	);

PMSP_STACKTRACE_ENTRY
MspDecodeStackTrace(
	IN PMSP_STACKTRACE_OBJECT Database,
	IN PMSP_STACKTRACE_ENTRY Trace
	);

ULONG
MspDecodeModuleName(
	IN PMSP_STACKTRACE_OBJECT Database,
	IN ULONG64 Address,
	IN PCHAR NameBuffer,
	IN ULONG BufferLength
	);

BOOLEAN
MspInsertStackTrace(
	IN PMSP_STACKTRACE_OBJECT Database,
	IN PBTR_STACKTRACE_ENTRY Trace,
	IN ULONG ProcessId
	);

ULONG
MspGetStackTrace(
	IN PBTR_RECORD_HEADER Record,
	IN PMSP_STACKTRACE_OBJECT Database,
	OUT PMSP_STACKTRACE_DECODED Decoded
	);

ULONG
MspDecodeStackTraceByRecord(
	IN PBTR_RECORD_HEADER Record,
	IN PMSP_STACKTRACE_OBJECT Object
	);

VOID
MspGetStackCounters(
	OUT PULONG EntryBuckets,
	OUT PULONG TextBuckets
	);

//
// Background stack decode procedure
//

ULONG CALLBACK
MspDecodeProcedure(
	IN PVOID Context
	);

VOID
MspSetStackTraceBarrier(
	VOID
	);

VOID
MspClearStackTraceBarrier(
	VOID
	);

VOID
MspClearDbghelp(
	VOID
	);

ULONG
MspInsertStackTraceString(
	IN PMSP_STACKTRACE_OBJECT Database,
	IN PMSP_STACKTRACE_ENTRY Entry
	);

BOOLEAN
MspInsertStackTraceText(
	IN PMSP_STACKTRACE_OBJECT Database,
	IN PSTR String,
	OUT PULONG Bucket
	);

PMSP_STACKTRACE_TEXT
MspGetStackTraceText(
	IN PMSP_STACKTRACE_OBJECT Database,
	IN ULONG Bucket
	);

PMSP_STACKTRACE_TEXT
MspAllocateExtendTextBucket(
	IN PMSP_STACKTRACE_OBJECT Database,
	OUT PULONG Bucket
	);

ULONG
MspCopyStackTraceString(
	IN ULONG Bucket,
	IN PVOID Buffer,
	IN ULONG Size
	);

ULONG
MspLookupStackTraceString(
	IN PVOID String,
	OUT PULONG Bucket
	);

ULONG 
MspHashStringBKDR(
	IN PSTR Buffer
	);

PMSP_DTL_OBJECT
MspCurrentDtlObject(
	VOID
	);

ULONG
MspGetRecordCount(
	IN PMSP_DTL_OBJECT DtlObject
	);

ULONG
MspReferenceCounterObject(
	IN PMSP_DTL_OBJECT DtlObject,
	OUT PMSP_COUNTER_OBJECT *CounterObject 
	);

VOID
MspReleaseCounterObject(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PMSP_COUNTER_OBJECT Object 
	);
 
ULONG
MspIsValidDtlReport(
	IN PWSTR Path
	);

ULONG
MspOpenDtlReport(
	IN PWSTR Path,
	OUT PMSP_DTL_OBJECT *DtlObject
	);

VOID
MspCloseDtlReport(
	IN PMSP_DTL_OBJECT DtlObject
	);

VOID
MspFreeDtlProcess(
	IN PMSP_DTL_OBJECT DtlObject
	);

VOID
MspDumpDtlRecord(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PSTR Path
	);

VOID
MspInsertCounterEntryByRecord(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record
	);

ULONG
MspWriteSummary(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PMSP_COUNTER_OBJECT Counter
	);

ULONG
MspLoadSummaryFromDtl(
	IN PMSP_DTL_OBJECT DtlObject
	);

ULONG
MspInsertCounterEntryByAddress(
	IN struct _MSP_PROCESS *Process, 
	IN PMSP_COUNTER_ENTRY Counter
	);


extern HANDLE MspSharedDataHandle;
extern PBTR_SHARED_DATA MspSharedData;
extern PBTR_WRITER_MAP MspWriterMap;
extern WCHAR MspSessionPath[];

extern WCHAR MspSessionUuidString[];
extern WCHAR MspRuntimeName[];
extern WCHAR MspRuntimePath[];

//
// Decode thread handle
//

extern HANDLE MspDecodeHandle;

//
// Access Macro
//

#define MspGetIndexObject(_D) \
	(PMSP_FILE_OBJECT)(&_D.IndexObject)

#define MspGetDataObject(_D) \
	(PMSP_FILE_OBJECT)(&_D.DataObject)

PMSP_STACKTRACE_OBJECT
MspGetStackTraceObject(
	IN PMSP_DTL_OBJECT DtlObject
	);

extern MSP_DTL_OBJECT MspDtlObject;


#ifdef __cplusplus
}
#endif

#endif