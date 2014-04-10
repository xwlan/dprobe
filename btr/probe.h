//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _PROBE_H_
#define _PROBE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "btr.h"
#include "filter.h"

#pragma pack(push, 16)

typedef struct _BTR_PROBE {
	LIST_ENTRY ListEntry;
	BTR_PROBE_TYPE Type;
	ULONG ObjectId;
	PVOID Address;
	PVOID PatchAddress;
	PBTR_TRAP Trap;
	ULONG Flags;
	ULONG References;
	BTR_RETURN_TYPE ReturnType;
	GUID FilterGuid;
	PVOID FilterCallback;
	ULONG FilterBit;
	PVOID Callback;
	PBTR_COUNTER_ENTRY Counter;
} BTR_PROBE, *PBTR_PROBE;

#pragma pack(pop)

typedef struct _BTR_PROBE_DATABASE {
	ULONG NumberOfBuckets;
	LIST_ENTRY ListEntry[PROBE_BUCKET_NUMBER];
} BTR_PROBE_DATABASE, *PBTR_PROBE_DATABASE;

//
// Callback routine when call BtrDestroyAddressTable
//

ULONG
BtrComputeProbeHash(
	IN PVOID Address
	);

ULONG
BtrUnregisterProbe(
	IN PBTR_PROBE Probe 
	);

VOID
BtrUnregisterProbes(
	VOID
	);

PBTR_PROBE
BtrLookupProbe(
	IN PVOID Address
	);

PBTR_PROBE
BtrAllocateProbe(
	IN PVOID Address	
	);

VOID
BtrFreeProbe(
	IN PBTR_PROBE Probe
	);

BOOLEAN
BtrVerifyProbe(
	IN PBTR_PROBE Probe
	);

ULONG
BtrRegisterFilterProbe(
	IN PBTR_FILTER Filter,
	IN ULONG Number,
	IN BOOLEAN Enable,
	OUT PVOID *AddressOfProbe,
	OUT PULONG ObjectId,
	OUT PULONG Counter
	);

ULONG
BtrRegisterFastProbe(
	IN PVOID Address,
	IN BOOLEAN Activate,
	OUT PULONG ObjectId,
	OUT PULONG Counter
	);

ULONG
BtrRegisterFastProbeEx(
	IN PBTR_USER_COMMAND Command
	);

ULONG 
BtrRegisterFilterProbeEx(
	IN PBTR_USER_COMMAND Command
	);

ULONG
BtrApplyProbe(
	IN PBTR_PROBE Probe
	);

BOOLEAN
BtrIsExecutingRange(
	IN PLIST_ENTRY ListHead,
	IN PVOID Address,
	IN ULONG Length
	);

VOID
BtrUninitializeProbe(
	VOID
	);

extern ULONG BtrUnloading;

#ifdef __cplusplus
}
#endif

#endif