//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _BTR_H_
#define _BTR_H_

#ifdef __cplusplus
extern "C" {
#endif

#define WIN32_LEAN_AND_MEAN 

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <windows.h> 
#include <tchar.h>
#include <strsafe.h>
#include <assert.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <emmintrin.h>
#include "status.h"
#include "list.h"
#include "bitmap.h"

#pragma warning(disable : 4996)

#define FlagOn(F,SF)  ((F) & (SF))     
#define SetFlag(F,SF)   { (F) |= (SF);  }
#define ClearFlag(F,SF) { (F) &= ~(SF); }

#define BTR_PORT_SIGNATURE        L"dprobe"
#define BTR_PORT_BUFFER_LENGTH    (1024 * 1024)

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

typedef enum _BTR_OBJECT_FLAG {

	//
	// record flag
	//

	BTR_FLAG_EXCEPTION     = 0x00000001,
	BTR_FLAG_FPU_RETURN    = 0x00000002,

	//
	// probe flag
	//

	BTR_FLAG_ON            = 0x00000010,
	BTR_FLAG_GC            = 0x00000020,

	//
	// stack trace      
	//

	BTR_FLAG_DECODED       = 0x00000100,

	//
	// trap flag
	//

	BTR_FLAG_FUSELITE      = 0x00001000,

	//
	// thread flag
	//

	BTR_FLAG_EXEMPTION     = 0x00010000,
	BTR_FLAG_RUNDOWN       = 0x00020000,

} BTR_OBJECT_FLAG, *PBTR_OBJECT_FLAG;

typedef enum _BTR_THRAD_TYPE {
	NullThread,
	SystemThread,
	NormalThread,
} BTR_THREAD_TYPE, *PBTR_THREAD_TYPE;

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

#if defined (_M_IX86)

#define FAST_STACK_ARGUMENT 4

typedef struct _BTR_FAST_ARGUMENT {
	ULONG_PTR Ecx;
	ULONG_PTR Edx;
	ULONG_PTR Stack[FAST_STACK_ARGUMENT];
} BTR_FAST_ARGUMENT, *PBTR_FAST_ARGUMENT;

#elif defined (_M_X64)

#define FAST_STACK_ARGUMENT 2

typedef struct _BTR_FAST_ARGUMENT {
	ULONG64 Rcx;
	ULONG64 Rdx;
	ULONG64 R8;
	ULONG64 R9;
	__m128d Xmm0;
	__m128d Xmm1;
	__m128d Xmm2;
	__m128d Xmm3;
	ULONG64 Stack[FAST_STACK_ARGUMENT];
} BTR_FAST_ARGUMENT, *PBTR_FAST_ARGUMENT;

#endif

typedef struct _BTR_FAST_RECORD {
	BTR_RECORD_HEADER Base;
	BTR_FAST_ARGUMENT Argument;
} BTR_FAST_RECORD, *PBTR_FAST_RECORD;

typedef struct _BTR_FILTER_RECORD {
	BTR_RECORD_HEADER Base;
	GUID FilterGuid;
	ULONG ProbeOrdinal;
	CHAR Data[ANYSIZE_ARRAY];
} BTR_FILTER_RECORD, *PBTR_FILTER_RECORD;

#define MAX_FILTER_PROBE 512

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
// Filter callback routine
//

typedef PVOID BTR_FILTER_CALLBACK;

//
// Filter decode callback routine, required
// if filter is integrated with D Probe
//

typedef ULONG
(*BTR_DECODE_CALLBACK)(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG BufferLength,
	OUT PWCHAR Buffer
	);

//
// Filter unregister callback
//

typedef ULONG 
(WINAPI *BTR_UNREGISTER_CALLBACK)(
	VOID
	);

//
// Filter can transport their own control
// to its target process via MspControlTrace
// when runtime receive the packet, it will
// pass the packet to filter's control callback routine
//

typedef struct _BTR_FILTER_CONTROL {
	ULONG Length;
	CHAR Control[ANYSIZE_ARRAY];
} BTR_FILTER_CONTROL, *PBTR_FILTER_CONTROL;

//
// Filter custom control callback
//

typedef ULONG
(WINAPI *BTR_CONTROL_CALLBACK)(
	IN PBTR_FILTER_CONTROL Control,
	OUT PBTR_FILTER_CONTROL *Ack
	);

//
// UI option callback for D Probe integrated filters.
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
	BTR_OPTION_CALLBACK OptionCallback;

	//
	// N.B. Don't touch the following fields.
	//

	LIST_ENTRY ListEntry;
	BTR_BITMAP BitMap;
	WCHAR FilterFullPath[MAX_PATH];
	PVOID BaseOfDll;
	ULONG SizeOfImage;

} BTR_FILTER, *PBTR_FILTER;

typedef struct _BTR_PROBE_CONTEXT {
	PVOID Destine;
	PVOID Frame;
} BTR_PROBE_CONTEXT, *PBTR_PROBE_CONTEXT;

//
// Filter export routine protoptype
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

typedef struct _BTR_CONTEXT {
	LIST_ENTRY ListEntry;
	ULONG ThreadId;
	HANDLE ThreadHandle;
	CONTEXT Registers;
} BTR_CONTEXT, *PBTR_CONTEXT;

//
// Forward declarations
//

typedef struct _BTR_TRAP *PBTR_TRAP;
typedef struct _BTR_FILTER *PBTR_FILTER;
typedef struct _BTR_TRAPFRAME *PBTR_TRAPFRAME;
typedef struct _BTR_REGISTER *PBTR_REGISTER;
typedef struct _BTR_THREAD *PBTR_THREAD;
typedef struct _BTR_FAST_RECORD *PBTR_FAST_RECORD;

#pragma pack(push, 16)

typedef struct _BTR_SPINLOCK {
	DECLSPEC_ALIGN(16) volatile ULONG Acquired;
	ULONG ThreadId;
	ULONG SpinCount;
	USHORT Flag;
	USHORT Spare0;
} BTR_SPINLOCK, *PBTR_SPINLOCK;

#pragma pack(pop)

typedef struct _BTR_LOCK {
	CRITICAL_SECTION Lock;
	ULONG OwnerThreadId;
	ULONG ContentCount;
	SLIST_ENTRY ListEntry;
} BTR_LOCK, *PBTR_LOCK;


//
// Bucket number of stack trace database
//

#define MAX_STACK_DEPTH   58 

//
// This bucket number is enough for most common cases,
// however, it's not enough for some special cases, e.g.
// explorer.exe can easily fill the buckets in a few 
// operation browsing a folder containing a large number of
// files if RtlAllocateHeap is being probed, it can reach up
// to 64K stack traces, that means each bucket has a queue length
// of 15 averagely. for such case, we can use an extensible memory
// file, with a bitmap allocation/deallocation structure to manage
// it, it's unlimited, however, it's more complicated than current
// design, furthermore, it should be lockless to scale up to hundreds
// of threads although explorer has limited concurrency in this case.
//

#define BTR_STACKTRACE_BUCKET_NUMBER ((ULONG)4093)

#define BtrStackTraceBucket(_H) \
	((ULONG)((ULONG)_H % BTR_STACKTRACE_BUCKET_NUMBER))


//
// N.B. SINGLE_LIST_ENTRY is not idential to SLIST_ENTRY on x64.
// don't use system provided InterlockedXxx to manipulate it.
//

#pragma pack(push, 16)

typedef struct _BTR_STACKTRACE_ENTRY {

	SLIST_ENTRY ListEntry;

	union {
		BTR_SPINLOCK SpinLock;
		struct {
			ULONG ProcessId;
			ULONG NumberOfEntries;
		} s1;
	};

	PVOID Probe;
	ULONG Hash;
	ULONG Count;
	ULONG Length;
	ULONG Depth;

	PVOID Frame[MAX_STACK_DEPTH];	

} BTR_STACKTRACE_ENTRY, *PBTR_STACKTRACE_ENTRY;

C_ASSERT(FIELD_OFFSET(BTR_STACKTRACE_ENTRY, ListEntry) == 0);
C_ASSERT(sizeof(BTR_STACKTRACE_ENTRY) % 16 == 0);

#pragma pack(pop)

typedef struct _BTR_STACKTRACE_DATABASE {
	
	CRITICAL_SECTION Lock;
	HANDLE ProcessHandle;
	ULONG ProcessId;
	ULONG Flag;
	ULONG NumberOfTotalBuckets;
    ULONG NumberOfFixedBuckets;
	ULONG NumberOfFirstHitBuckets;
	ULONG NumberOfChainedBuckets;
	ULONG ChainedBucketsThreshold;

	PVOID Allocator;

	union {
		PBTR_STACKTRACE_ENTRY Buckets;
		ULONG64 Offset;
	} u1;

	union {
		LIST_ENTRY ListEntry;
		ULONG64 NextOffset;
	} u2;

} BTR_STACKTRACE_DATABASE, *PBTR_STACKTRACE_DATABASE;

typedef enum _BTR_MESSAGE_TYPE {
	MESSAGE_ACK,
	MESSAGE_CACHE,
	MESSAGE_CACHE_ACK,
	MESSAGE_FILTER,
	MESSAGE_FILTER_ACK,
	MESSAGE_FAST,
	MESSAGE_FAST_ACK,
	MESSAGE_USER_COMMAND,
	MESSAGE_USER_COMMAND_ACK,
	MESSAGE_STOP_REQUEST,
	MESSAGE_STOP_ACK,
	MESSAGE_RECORD,
	MESSAGE_RECORD_ACK,
	MESSAGE_FILTER_CONTROL,
	MESSAGE_FILTER_CONTROL_ACK,
	MESSAGE_STACKTRACE,
	MESSAGE_STACKTRACE_ACK,
	MESSAGE_LAST,
} BTR_MESSAGE_TYPE, *PBTR_MESSAGE_TYPE;

typedef struct _BTR_MESSAGE_HEADER {
	ULONG Length;
	BTR_MESSAGE_TYPE PacketType;
} BTR_MESSAGE_HEADER, *PBTR_MESSAGE_HEADER;

typedef struct _BTR_MESSAGE_ACK {
	BTR_MESSAGE_HEADER Header;
	ULONG Status;
	PVOID Information;
} BTR_MESSAGE_ACK, *PBTR_MESSAGE_ACK;

typedef struct _BTR_FAST_ENTRY {
	PVOID Address;
	ULONG Status;
	ULONG ObjectId;
	ULONG Counter;
} BTR_FAST_ENTRY, *PBTR_FAST_ENTRY;

typedef struct _BTR_MESSAGE_FAST { 
	BTR_MESSAGE_HEADER Header;
	BOOLEAN Activate;
	BOOLEAN Spare[3];
	ULONG Count;
	BTR_FAST_ENTRY Entry[ANYSIZE_ARRAY];
} BTR_MESSAGE_FAST, *PBTR_MESSAGE_FAST;

typedef struct _BTR_MESSAGE_FAST_ACK { 
	BTR_MESSAGE_HEADER Header;
	BOOLEAN Activate;
	BOOLEAN Spare[3];
	ULONG Count;
	BTR_FAST_ENTRY Entry[ANYSIZE_ARRAY];
} BTR_MESSAGE_FAST_ACK, *PBTR_MESSAGE_FAST_ACK;

typedef struct _BTR_FILTER_ENTRY {
	ULONG Number;
	ULONG Status;
	ULONG ObjectId;
	ULONG Counter;
	PVOID Address;
} BTR_FILTER_ENTRY, *PBTR_FILTER_ENTRY;

typedef struct _BTR_MESSAGE_FILTER {
	BTR_MESSAGE_HEADER Header;
	BOOLEAN Activate;
	BOOLEAN Spare[3];
	ULONG Status;
	ULONG Count;
	WCHAR Path[MAX_PATH];
	BTR_FILTER_ENTRY Entry[ANYSIZE_ARRAY];
} BTR_MESSAGE_FILTER, *PBTR_MESSAGE_FILTER;

typedef struct _BTR_MESSAGE_FILTER_ACK {
	BTR_MESSAGE_HEADER Header;
	BOOLEAN Activate;
	BOOLEAN Spare[3];
	ULONG Status;
	ULONG Count;
	WCHAR Path[MAX_PATH];
	BTR_FILTER_ENTRY Entry[ANYSIZE_ARRAY];
} BTR_MESSAGE_FILTER_ACK, *PBTR_MESSAGE_FILTER_ACK;

//
// BTR_MESSAGE_USER_COMMAND is used for large user request
//

typedef struct _BTR_USER_PROBE {
	BOOLEAN Activate;
	BOOLEAN Spare[3];
	ULONG Status;
	ULONG ObjectId;
	ULONG Counter;
	PVOID Address;
	union {
		CHAR ApiName[MAX_PATH];
		ULONG Number;
	};
} BTR_USER_PROBE, *PBTR_USER_PROBE;

typedef struct _BTR_USER_COMMAND {
	LIST_ENTRY ListEntry;
	ULONG Length;
	BTR_PROBE_TYPE Type;
	ULONG Status;
	ULONG FailureCount;
	WCHAR DllName[MAX_PATH];
	ULONG Count;
	BTR_USER_PROBE Probe[ANYSIZE_ARRAY];
} BTR_USER_COMMAND, *PBTR_USER_COMMAND;

typedef struct _BTR_MESSAGE_USER_COMMAND {
	BTR_MESSAGE_HEADER Header;
	HANDLE MappingObject;
	ULONG NumberOfCommands;
	ULONG TotalLength;
} BTR_MESSAGE_USER_COMMAND, *PBTR_MESSAGE_USER_COMMAND;

typedef struct _BTR_MESSAGE_USER_COMMAND_ACK {
	BTR_MESSAGE_HEADER Header;
	ULONG Status;
} BTR_MESSAGE_USER_COMMAND_ACK, *PBTR_MESSAGE_USER_COMMAND_ACK;

typedef struct _BTR_MESSAGE_CACHE {
	BTR_MESSAGE_HEADER Header;
	HANDLE IndexFileHandle;
	HANDLE DataFileHandle;
	HANDLE SharedDataHandle;
	HANDLE PerfHandle;
} BTR_MESSAGE_CACHE, *PBTR_MESSAGE_CACHE;

typedef struct _BTR_MESSAGE_STACKTRACE {
	BTR_MESSAGE_HEADER Header;
} BTR_MESSAGE_STACKTRACE, *PBTR_MESSAGE_STACKTRACE;

typedef struct _BTR_MESSAGE_STACKTRACE_ACK {
	BTR_MESSAGE_HEADER Header;
	ULONG Status;
	ULONG NumberOfTraces;
	BTR_STACKTRACE_ENTRY Entries[ANYSIZE_ARRAY];
} BTR_MESSAGE_STACKTRACE_ACK, *PBTR_MESSAGE_STACKTRACE_ACK;

typedef struct _BTR_MESSAGE_STOP {
	BTR_MESSAGE_HEADER Header;
} BTR_MESSAGE_STOP, *PBTR_MESSAGE_STOP;

typedef struct _BTR_MESSAGE_RECORD {
	BTR_MESSAGE_HEADER Header;
	ULONG BufferLength;
} BTR_MESSAGE_RECORD, *PBTR_MESSAGE_RECORD;

typedef struct _BTR_MESSAGE_RECORD_ACK {
	BTR_MESSAGE_HEADER Header;
	ULONG NumberOfRecords;
	ULONG PacketFlag;
	UCHAR Data[ANYSIZE_ARRAY];
} BTR_MESSAGE_RECORD_ACK, *PBTR_MESSAGE_RECORD_ACK;

//
// Filter Control (Filter's Custom IPC)
//

#define BTR_MAX_FILTER_CONTROL_SIZE   (1024 * 8)

typedef struct _BTR_MESSAGE_FILTER_CONTROL {
	BTR_MESSAGE_HEADER Header;
	WCHAR FilterName[MAX_PATH];
	CHAR Data[ANYSIZE_ARRAY];
} BTR_MESSAGE_FILTER_CONTROL, *PBTR_MESSAGE_FILTER_CONTROL;

typedef struct _BTR_MESSAGE_FILTER_CONTROL_ACK {
	BTR_MESSAGE_HEADER Header;
	WCHAR FilterName[MAX_PATH];
	ULONG Status;
	CHAR Data[ANYSIZE_ARRAY];
} BTR_MESSAGE_FILTER_CONTROL_ACK, *PBTR_MESSAGE_FILTER_CONTROL_ACK;

typedef struct _BTR_PORT {
	LIST_ENTRY ListEntry;
	WCHAR Name[64];
	ULONG ProcessId;
	ULONG ThreadId;
	HANDLE Object;
	PVOID Buffer;
	ULONG BufferLength;
	HANDLE CompleteEvent; 
	OVERLAPPED Overlapped;
} BTR_PORT, *PBTR_PORT;

#define BTR_INIT_STAGE_0  0
#define BTR_INIT_STAGE_1  1

#pragma pack(push, 1)

typedef struct _BTR_FILE_INDEX {
	ULONG64 Committed : 1;
	ULONG64 Excluded  : 1;
	ULONG64 Length    : 16;
	ULONG64 Reserved  : 10;
	ULONG64 Offset    : 36;
} BTR_FILE_INDEX, *PBTR_FILE_INDEX;

typedef struct _BTR_WRITER_MAP {
	ULONG64 Length;
	HANDLE PerfHandle;
	ULONG ProcessId;
	ULONG ThreadId;
	PVOID FileObject;
} BTR_WRITER_MAP, *PBTR_WRITER_MAP;

#pragma pack(pop)

C_ASSERT(sizeof(BTR_FILE_INDEX) == 8);

//
// Maximum data file size is 64 GB
// Maximum index file size is 8 GB
//

#define BTR_MAXIMUM_DATA_FILE_SIZE   ((ULONG64)0x1000000000)
#define BTR_MAXIMUM_INDEX_FILE_SIZE  ((ULONG64)0x0800000000)
#define BTR_MAXIMUM_INDEX_NUMBER     ((ULONG)0xffffffff)
#define BTR_MAXIMUM_WRITER_ID        ((USHORT)0xffff)

//
// Data file is per thread based.
//

#define BTR_DATA_FILE_INCREMENT     1024 * 64
#define BTR_DATA_MAP_INCREMENT      1024 * 64 

#define BTR_SHARED_DATA_FILE_INCREMENT   1024 * 1024 * 64
#define BTR_SHARED_DATA_MAP_INCREMENT    1024 * 64 

//
// Index file is shared by all btr targets
//

#define BTR_INDEX_FILE_INCREMENT    1024 * 1024 * 4
#define BTR_INDEX_MAP_INCREMENT     1024 * 64 

//
// Global shared data mapped into all processes
//

typedef DECLSPEC_CACHEALIGN struct _BTR_SHARED_DATA {
	DECLSPEC_CACHEALIGN BTR_SPINLOCK SpinLock;
	DECLSPEC_CACHEALIGN ULONG Sequence;
	DECLSPEC_CACHEALIGN ULONG SequenceFull;
	DECLSPEC_CACHEALIGN ULONG PendingReset;
	DECLSPEC_CACHEALIGN ULONG ObjectId;
	DECLSPEC_CACHEALIGN ULONG64 IndexFileLength;
	DECLSPEC_CACHEALIGN ULONG64 DataFileLength;
	DECLSPEC_CACHEALIGN ULONG64 DataValidLength;
	WCHAR ParentPath[MAX_PATH];
} BTR_SHARED_DATA, *PBTR_SHARED_DATA;

//
// This value should be round up to align with 64KB boundary.
//

#define BTR_SHARED_DATA_SIZE  sizeof(BTR_SHARED_DATA)

//
// Performance Counters
//

#define PROBE_BUCKET_NUMBER 4093

#define BtrCounterBucket(_ObjectId)  \
	(_ObjectId % PROBE_BUCKET_NUMBER)

typedef struct _BTR_COUNTER_ENTRY {
	BTR_PROBE_TYPE Type;
	PVOID Address;
	ULONG ObjectId;
	ULONG NumberOfCalls;
	ULONG NumberOfStackTraces;
	ULONG NumberOfExceptions;
	ULONG MinimumDuration;
	ULONG MaximumDuration;
	WCHAR Name[MAX_PATH];
	FILETIME StartTime;
	FILETIME EndTime;
} BTR_COUNTER_ENTRY, *PBTR_COUNTER_ENTRY;

typedef struct DECLSPEC_CACHEALIGN _BTR_COUNTER_TABLE {
	BTR_SPINLOCK Lock;
	ULONG ProcessId;
	ULONG NumberOfCalls;
	ULONG NumberOfStackTraces;
	ULONG NumberOfExceptions;
	ULONG NumberOfBuckets;
	BTR_COUNTER_ENTRY Entries[PROBE_BUCKET_NUMBER];
} BTR_COUNTER_TABLE, *PBTR_COUNTER_TABLE;

VOID
BtrVersion(
	OUT PUSHORT MajorVersion,
	OUT PUSHORT MinorVersion
	);

ULONG
BtrInitialize(
	IN HMODULE Runtime
	);

ULONG WINAPI
BtrFltStartRuntime(
	IN PULONG Features 
	);

ULONG WINAPI
BtrFltStopRuntime(
	IN PVOID Reserved
	);

ULONG WINAPI
BtrFltCurrentFeature(
	VOID
	);

ULONG
BtrStopRuntime(
	IN ULONG Reason	
	);

ULONG
BtrUnloadRuntime(
	VOID
	);

ULONG
BtrIsUnloading(
	VOID
	);

LONG CALLBACK
BtrExceptionFilter(
	IN EXCEPTION_POINTERS *Pointers
	);

ULONG
BtrInitializeHal(
	VOID
	);

VOID
BtrUninitializeHal(
	VOID
	);

ULONG
BtrInitializeMm(
	ULONG Stage	
	);

VOID
BtrUninitializeMm(
	VOID
	);

ULONG
BtrInitializeThread(
	VOID
	);

ULONG
BtrInitializeProbe(
	VOID
	);

ULONG
BtrInitializeTrap(
	VOID
	);

PVOID WINAPI
BtrFltMalloc(
	IN ULONG Length
	);

VOID WINAPI
BtrFltFree(
	IN PVOID Address
	);

VOID WINAPI
BtrFltEnterExemptionRegion(
	VOID
	);

VOID WINAPI
BtrFltLeaveExemptionRegion(
	VOID
	);

//
// N.B. This is important 
//

extern ULONG BtrFeatures;

//
// Export defs for MSP
//

#ifdef _MSP_

#endif

#ifndef ASSERT
#ifdef _DEBUG
	#define ASSERT assert
#else
	#define ASSERT
#endif
#endif

#ifdef __cplusplus
} 
#endif 

#endif