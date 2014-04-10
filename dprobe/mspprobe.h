//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _MSPPROBE_H_
#define _MSPPROBE_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include "btr.h"
#include "conditiondlg.h"
#include "mspqueue.h"
#include "mspprocess.h"

#if defined (_M_IX86)
#define HASH_GOLDEN_RATIO 0x9E3779B9

#elif defined (_M_X64)
#define HASH_GOLDEN_RATIO 0x9E3779B97F4A7C13
#endif

#define MSP_PROBE_TABLE_SIZE  (16381)

typedef enum _MSP_PROBE_STATE {
	InvalidProbe,
	ActivatedProbe,
	DeactivatedProbe,
} MSP_PROBE_STATE, *PMSP_PROBE_STATE;

typedef struct _MSP_PROBE {

	LIST_ENTRY ListEntry;
	LIST_ENTRY TypeListEntry;

	BTR_PROBE_TYPE Type;
	MSP_PROBE_STATE State;
	ULONG ObjectId;

	struct _MSP_COUNTER_ENTRY *Counter;
	PVOID Address;
	PVOID Callback;
	BTR_RETURN_TYPE Return;

	struct _BTR_FILTER *Filter;
	ULONG FilterBit;

	WCHAR ApiName[MAX_PATH];
	WCHAR DllName[MAX_PATH];

} MSP_PROBE, *PMSP_PROBE;

typedef BTR_SPINLOCK  MSP_SPINLOCK;
typedef PBTR_SPINLOCK PMSP_SPINLOCK;
typedef CRITICAL_SECTION MSP_CS_LOCK;
typedef CRITICAL_SECTION *PMSP_CS_LOCK;

typedef struct _MSP_PROBE_TABLE {

	MSP_CS_LOCK Lock;
	struct _MSP_PROCESS *Process;
	HANDLE Object;

	BOOLEAN NoCounters;
	BOOLEAN Spare[3];

	ULONG TotalBuckets;
	ULONG FilledBuckets;

	ULONG FastProbeCount;
	ULONG ActiveFastCount;
	ULONG FilterProbeCount;
	ULONG ActiveFilterCount;

	LIST_ENTRY FastListHead;
	LIST_ENTRY FilterListHead;

	LIST_ENTRY ListHead[MSP_PROBE_TABLE_SIZE];

} MSP_PROBE_TABLE, *PMSP_PROBE_TABLE;

struct _MSP_DTL_OBJECT;
struct _MSP_PROCESS;

ULONG
MspCreateProbeTable(
	IN struct _MSP_PROCESS *Process,
	IN ULONG NumberOfBuckets
	);

VOID
MspDestroyProbeTable(
	IN PMSP_PROBE_TABLE Table
	);

PMSP_PROBE_TABLE
MspQueryProbeTable(
	IN struct _MSP_DTL_OBJECT *DtlObject,
	IN ULONG ProcessId
	);

ULONG 
MspComputeProbeHash(
	IN PMSP_PROBE_TABLE Table,
	IN PVOID Address
	);

PMSP_PROBE
MspLookupProbe(
	IN struct _MSP_PROCESS *Process,
	IN BTR_PROBE_TYPE Type,
	IN PVOID Address,
	IN PWSTR DllName,
	IN PWSTR ApiName,
	IN PBTR_FILTER Filter
	);

PMSP_PROBE
MspLookupProbeEx(
	IN struct _MSP_DTL_OBJECT *DtlObject,
	IN ULONG ProcessId,
	IN BTR_PROBE_TYPE Type,
	IN PVOID Address,
	IN PWSTR DllName,
	IN PWSTR ApiName,
	IN PBTR_FILTER Filter,
	OUT PMSP_PROBE_TABLE *Return
	);

ULONG
MspInsertProbe(
	IN struct _MSP_PROCESS *Process,
	IN BTR_PROBE_TYPE Type,
	IN BTR_RETURN_TYPE Return,
	IN PVOID Address,
	IN ULONG ObjectId,
	IN ULONG Counter,
	IN PWSTR DllName,
	IN PWSTR ApiName,
	IN PBTR_FILTER Filter,
	IN ULONG FilterBit
	);

PMSP_PROBE
MspAllocateProbe(
	IN PMSP_PROBE_TABLE Table,
	IN PVOID Address
	);

BOOLEAN
MspRemoveProbe(
	IN struct _MSP_PROCESS *Process,
	IN BTR_PROBE_TYPE Type,
	IN PVOID Address,
	IN PWSTR DllName,
	IN PWSTR ApiName,
	IN PBTR_FILTER Filter,
	IN ULONG FilterBit,
	OUT PMSP_PROBE Clone
	);

ULONG
MspRemoveAllProbes(
	IN struct _MSP_PROCESS *Process
	);

PBTR_FILTER
MspLookupFilter(
	IN struct _MSP_DTL_OBJECT *DtlObject,
	IN ULONG ProcessId,
	IN GUID *FilterGuid
	);

struct _MSP_PROCESS*
MspQueryProcess(
	IN ULONG ProcessId,
	IN PWSTR ImageName,
	IN PWSTR FullPath
	);

PMSP_PROBE
MspLookupProbeForDecode(
	IN PMSP_DTL_OBJECT DtlObject,
	IN ULONG ProcessId,
	IN BTR_PROBE_TYPE Type,
	IN PVOID Address 
	);

PMSP_PROBE
MspLookupFastProbe(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record
	);

PBTR_FILTER_PROBE
MspLookupFilterProbe(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record
	);

PMSP_PROBE
MspLookupProbeByObjectId(
	IN PMSP_DTL_OBJECT DtlObject,
	IN PBTR_RECORD_HEADER Record
	);

#ifdef __cplusplus
}
#endif
#endif