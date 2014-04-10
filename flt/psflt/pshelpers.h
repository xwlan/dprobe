//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _PSHELPERS_H_
#define _PSHELPERS_H_

#include <windows.h>

#ifdef __cplusplus 
extern "C" {
#endif

typedef struct _UNICODE_STRING {
  USHORT  Length;
  USHORT  MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

//
// N.B. Carefully check against Windows version of this structure
//

typedef struct _PEB_LDR_DATA {
   ULONG Length;
   BOOLEAN Initialized;
   PVOID SsHandle;
   LIST_ENTRY InLoadOrderModuleList;
   LIST_ENTRY InMemoryOrderModuleList;
   LIST_ENTRY InInitializationOrderModuleList;
   PVOID EntryInProgress;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

//
// N.B. The following is only small part of PEB, we don't use others,
// so define a small PEB structure, we only use Ldr field.
//

typedef struct _PEB {
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    union {
        BOOLEAN BitField;
        struct {
            BOOLEAN ImageUsesLargePages : 1;
            BOOLEAN IsProtectedProcess : 1;
            BOOLEAN IsLegacyProcess : 1;
            BOOLEAN IsImageDynamicallyRelocated : 1;
            BOOLEAN SkipPatchingUser32Forwarders : 1;
            BOOLEAN SpareBits : 3;
        };
    };
    HANDLE Mutant;
    PVOID ImageBaseAddress;
    PPEB_LDR_DATA Ldr;
} PEB, *PPEB;


//
// N.B. The following is only small part of LDR_DATA_TABLE_ENTRY, 
// we only use DllBase, SizeOfImage, LoadCount field.
//

typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef PPEB
(WINAPI *RTL_GET_CURRENT_PEB)(
	VOID
	);

//
// from NDK
//
//
// LdrLockLoaderLock Flags
//
#define LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS  0x00000001
#define LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY      0x00000002

typedef NTSTATUS
(WINAPI *LDR_LOCK_LOADER_LOCK)(
    IN ULONG Flags,
    OUT PULONG Disposition OPTIONAL,
    OUT PULONG Cookie OPTIONAL
	);

typedef NTSTATUS
(WINAPI *LDR_UNLOCK_LOADER_LOCK)(
    IN ULONG Flags,
    IN ULONG Cookie OPTIONAL
	);

extern HMODULE PsNtDllHandle;
extern RTL_GET_CURRENT_PEB PsRtlGetCurrentPeb;

ULONG
PsInitializeHelpers(
	VOID
	);

ULONG
PsGetDllRefCountUnsafe(
	IN PVOID DllBase 
	);

LDR_DATA_TABLE_ENTRY *
PsGetDllLdrDataEntry(
	IN PVOID DllBase
	);

ULONG
PsGetDllNameUnsafe(
	IN PVOID DllBase,
	OUT PWCHAR Buffer,
	IN ULONG Size,
	OUT PULONG ResultSize
	);

BOOLEAN
PsAcquireLoaderLock(
	VOID
	);

VOID
PsReleaseLoaderLock(
	VOID
	);

//
// Decode Helpers
//

typedef struct _BITS_DECODE {
	ULONG Bit;
	PSTR  Text;
	ULONG Count;
} BITS_DECODE, *PBITS_DECODE;


ULONG
PsConvertBitsToString(
	IN ULONG Flags,
	IN BITS_DECODE Bits[],
	IN ULONG BitsSize,
	IN PSTR Buffer,
	IN ULONG BufferSize,
	OUT PULONG ResultSize
	);

#ifdef __cplusplus 
}
#endif

#endif