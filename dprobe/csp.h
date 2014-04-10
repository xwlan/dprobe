//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _CSP_H_
#define _CSP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dprobe.h"
#include "btr.h"

struct _MSP_PROCESS;

#define CSP_FILE_64BITS            0x00000001
#define CSP_FILE_PROCESS_STRIPPED  0x00000002
#define CSP_FILE_SIGNATURE  ((ULONG)'cpd')

//
// N.B. The same *.dpc file can be used for both
// 32 and 64 bits, so all on disk structures of
// dpc file can not contain any CPU architecture
// dependent field like pointer. otherwise it can
// cause dprobe to crash, the on disk structure are
// all packed by 1 byte alignment.
//

#pragma pack(push, 1)

typedef struct _CSP_BUILDIN {
	ULONG Bits[16];
} CSP_BUILDIN, *PCSP_BUILDIN;

typedef struct _CSP_FILTER{

	union {
		LIST_ENTRY ListEntry;
		ULONG64 Padding[2];
	};

	GUID FilterGuid;
	ULONG Count;
	ULONG Bits[16];
	WCHAR FilterName[MAX_PATH];

} CSP_FILTER, *PCSP_FILTER;

typedef struct _CSP_FAST {

	union {
		LIST_ENTRY ListEntry;
		ULONG64 Padding[2];
	};

	WCHAR DllName[MAX_PATH];
	ULONG ApiCount;
	CHAR ApiName[ANYSIZE_ARRAY][MAX_PATH];

} CSP_FAST, *PCSP_FAST;

typedef struct _CSP_FILE_HEADER {
	ULONG Signature; 
	ULONG Flag;
	ULONG Size;   
	USHORT MajorVersion;
	USHORT MinorVersion;
	BTR_PROBE_TYPE Type;
	ULONG DllCount;
} CSP_FILE_HEADER, *PCSP_FILE_HEADER;

#pragma pack(pop)

typedef struct _CSP_CONTEXT {
	HANDLE FileObject;
	HANDLE MapObject;
	PVOID MapAddress;
	WCHAR FullPath[MAX_PATH];
} CSP_CONTEXT, *PCSP_CONTEXT;

ULONG
CspInitialize(
	VOID
	);

ULONG
CspWriteFile(
	IN PWSTR Path,
	IN struct _MSP_PROCESS *Process
	);

ULONG
CspWriteBuildinProbe(
	IN HANDLE FileHandle,
	IN ULONG Offset,
	IN struct _MSP_PROCESS *Process
	);

ULONG
CspWriteFilterProbe(
	IN HANDLE FileHandle,
	IN ULONG Offset,
	IN struct _MSP_PROCESS *Process,
	OUT PULONG Count
	);

ULONG
CspWriteFastProbe(
	IN HANDLE FileHandle,
	IN ULONG Offset,
	IN struct _MSP_PROCESS *Process,
	OUT PULONG Count
	);

ULONG
CspOpenFile(
	IN PCSP_CONTEXT Context
	);

ULONG
CspCloseFile(
	IN PCSP_CONTEXT Context
	);

ULONG
CspReadFileHead(
	IN PWSTR Path,
	OUT PCSP_FILE_HEADER Head
	);

ULONG
CspExportCommand(
	IN PCSP_CONTEXT Context,
	OUT struct _MSP_USER_COMMAND **Command
	);

//
// N.B. *.dpc file must be classified as either 32 or 64 bits
// because we the probe object is essentially a serialized copy
// of in memory structure.
//


typedef struct _CSP_PROBE_GROUP {
	WCHAR ProcessName[64];
	WCHAR CommandLine[MAX_PATH];
	ULONG ManualCount;
	ULONG FilterCount;
	ULONG StartOffset;
} CSP_PROBE_GROUP, *PCSP_PROBE_GROUP;

typedef struct _CSP_PROBE_TABLE {
	ULONG Signature;   // 'cpd'
	ULONG CheckSum;
	ULONG Size;        // total size of the dpc file. 
	ULONG Flags;
	USHORT MajorVersion;
	USHORT MinorVersion;
	ULONG GroupCount;
	CSP_PROBE_GROUP ProbeGroup[ANYSIZE_ARRAY];
} CSP_PROBE_TABLE, *PCSP_PROBE_TABLE;

#ifdef __cplusplus
}
#endif

#endif