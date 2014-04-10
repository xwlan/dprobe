//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _MMFLT_H_
#define _MMFLT_H_

#ifdef __cplusplus 
extern "C" {
#endif

#define _BTR_SDK_ 
#include "btrsdk.h"
#include <strsafe.h>

#ifndef ASSERT
 #include <assert.h>
 #define ASSERT assert
#endif

#define MmMajorVersion 1
#define MmMinorVersion 0

extern GUID MmGuid;

//
// Mm Probe Ordinals
//

typedef enum _MM_PROBE_ORDINAL {
	_HeapAlloc,
	_HeapFree,
	_HeapReAlloc,
	_HeapCreate,
	_HeapDestroy,
	_VirtualAllocEx,
	_VirtualFreeEx,
	_VirtualProtectEx,
	_VirtualQueryEx,
	_CreateFileMapping,
	_MapViewOfFile,
	_UnmapViewOfFile,
	_OpenFileMapping,
	_FlushViewOfFile,
	MM_PROBE_NUMBER,
} MM_PROBE_ORDINAL, *PMM_PROBE_ORDINAL;

//
// Filter export routine
//

PBTR_FILTER WINAPI
BtrFltGetObject(
	IN BTR_FILTER_MODE Mode	
	);

//
// Filter unregister callback routine
//

ULONG WINAPI
MmUnregisterCallback(
	VOID
	);

//
// Filter control callback routine
//

ULONG WINAPI
MmControlCallback(
	IN PBTR_FILTER_CONTROL Control,
	OUT PBTR_FILTER_CONTROL *Ack
	);

typedef struct _MM_HEAPALLOC {
	PVOID hHeap;
	DWORD dwFlags;
	SIZE_T dwBytes;
	PVOID Block;
} MM_HEAPALLOC, *PMM_HEAPALLOC;

typedef struct _MM_HEAPREALLOC {
	PVOID hHeap;
	DWORD dwFlags;
	PVOID lpMem;
	SIZE_T dwBytes;
	PVOID Block;
} MM_HEAPREALLOC, *PMM_HEAPREALLOC;

typedef struct _MM_HEAPFREE {
	PVOID hHeap;
	DWORD dwFlags;
	PVOID lpMem;
} MM_HEAPFREE, *PMM_HEAPFREE;

typedef struct _MM_HEAPCREATE {
	DWORD  flOptions;
    SIZE_T dwInitialSize;
    SIZE_T dwMaximumSize;
	PVOID  Handle;
} MM_HEAPCREATE, *PMM_HEAPCREATE;

typedef struct _MM_HEAPDESTROY {
	PVOID hHeap;
} MM_HEAPDESTROY, *PMM_HEAPDESTROY;

typedef struct _MM_VIRTUALALLOCEX {
	HANDLE ProcessHandle;
	PVOID  Address;
	SIZE_T Size;
	ULONG AllocationType;
	ULONG ProtectType;
	PVOID ReturnedAddress;
	ULONG LastErrorStatus;
} MM_VIRTUALALLOCEX, *PMM_VIRTUALALLOCEX;

typedef struct _MM_VIRTUALFREEEX{
	HANDLE ProcessHandle;
	PVOID  Address;
	SIZE_T Size;
	ULONG FreeType;
	BOOL Status;
	ULONG LastErrorStatus;
} MM_VIRTUALFREEEX, *PMM_VIRTUALFREEEX;

typedef struct _MM_VIRTUALPROTECTEX {
	HANDLE ProcessHandle;
	PVOID Address;
	SIZE_T Size;
	ULONG NewProtect;
	ULONG_PTR OldProtect;
	BOOL Status;
	ULONG LastErrorStatus;
} MM_VIRTUALPROTECTEX, *PMM_VIRTUALPROTECTEX;

typedef struct _MM_VIRTUALQUERYEX {
	HANDLE hProcess;
	PVOID lpAddress;
	PMEMORY_BASIC_INFORMATION lpBuffer;
	SIZE_T dwLength;
	MEMORY_BASIC_INFORMATION Mbi;
} MM_VIRTUALQUERYEX, *PMM_VIRTUALQUERYEX;

typedef struct _MM_CREATEFILEMAPPING {
	HANDLE FileHandle;
	SECURITY_ATTRIBUTES *Security;
	ULONG Protect;
	ULONG MaximumSizeHigh;
	ULONG MaximumSizeLow;
	WCHAR Name[MAX_PATH];
	HANDLE FileMappingHandle;
	ULONG LastErrorStatus;
} MM_CREATEFILEMAPPING, *PMM_CREATEFILEMAPPING;

typedef struct _MM_MAPVIEWOFFILE {
	HANDLE FileMappingHandle;
	ULONG DesiredAccess;
	ULONG FileOffsetHigh;
	ULONG FileOffsetLow;
	SIZE_T NumberOfBytesToMap;
	PVOID MappedAddress;
	ULONG LastErrorStatus;
} MM_MAPVIEWOFFILE, *PMM_MAPVIEWOFFILE;

typedef struct _MM_UNMAPVIEWOFFILE {
	PVOID MappedAddress;
	BOOL Status;
	ULONG LastErrorStatus;
} MM_UNMAPVIEWOFFILE, *PMM_UNMAPVIEWOFFILE;

typedef struct _MM_OPENFILEMAPPING {
	ULONG DesiredAccess;
	BOOL  InheritHandle;
	WCHAR Name[MAX_PATH];
	HANDLE FileMappingHandle;
	ULONG LastErrorStatus;
} MM_OPENFILEMAPPING, *PMM_OPENFILEMAPPING;

typedef struct _MM_FLUSHVIEWOFFILE {
	PVOID BaseAddress;
	SIZE_T NumberOfBytesToFlush;
	BOOL Status;
	ULONG LastErrorStatus;
} MM_FLUSHVIEWOFFILE, *PMM_FLUSHVIEWOFFILE;

typedef struct _MM_GETMAPPEDFILENAME {
	HANDLE ProcessHandle;
	PVOID MappedAddress;
	WCHAR FileName[MAX_PATH];
	PWCHAR FileNamePointer;
	ULONG Length;
	ULONG ActualLength;
	ULONG LastErrorStatus;
} MM_GETMAPPEDFILENAME, *PMM_GETMAPPEDFILENAME;

//
// HeapAlloc
//

PVOID WINAPI
HeapAllocEnter(
	IN HANDLE hHeap,
	IN DWORD dwFlags,
	IN SIZE_T dwBytes 
	);

typedef PVOID 
(WINAPI *HEAPALLOC_ROUTINE)(
	IN HANDLE hHeap,
	IN DWORD dwFlags,
	IN SIZE_T dwBytes 
	);

ULONG
HeapAllocDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// HeapFree
//

BOOL WINAPI
HeapFreeEnter(
	IN HANDLE hHeap,
	IN DWORD dwFlags,
	IN PVOID lpMem 
	);

typedef BOOL
(WINAPI *HEAPFREE_ROUTINE)(
	IN HANDLE hHeap,
	IN DWORD dwFlags,
	IN PVOID lpMem 
	);

ULONG 
HeapFreeDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// HeapReAlloc
//

LPVOID WINAPI 
HeapReAllocEnter(
	__in HANDLE hHeap,
	__in DWORD dwFlags,
	__in LPVOID lpMem,
	__in SIZE_T dwBytes
	);

typedef LPVOID 
(WINAPI *HEAPREALLOC_ROUTINE)(
	__in HANDLE hHeap,
	__in DWORD dwFlags,
	__in LPVOID lpMem,
	__in SIZE_T dwBytes
	);

ULONG 
HeapReAllocDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// HeapCreate
//

PVOID WINAPI
HeapCreateEnter( 
	__in DWORD flOptions,
	__in SIZE_T dwInitialSize,
	__in SIZE_T dwMaximumSize
	);

typedef PVOID 
(WINAPI *HEAPCREATE_ROUTINE)( 
	__in DWORD flOptions,
	__in SIZE_T dwInitialSize,
	__in SIZE_T dwMaximumSize
    ); 

ULONG
HeapCreateDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// HeapDestroy
//

BOOL WINAPI
HeapDestroyEnter( 
    IN HANDLE hHeap 
    ); 

typedef BOOL 
(WINAPI *HEAPDESTROY_ROUTINE)( 
    IN HANDLE hHeap 
    ); 

ULONG 
HeapDestroyDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// VirtualAllocEx
//

LPVOID WINAPI 
VirtualAllocExEnter(
	IN HANDLE hProcess,
	IN LPVOID lpAddress,
	IN SIZE_T dwSize,
	IN DWORD flAllocationType,
	IN DWORD flProtect
	);

typedef LPVOID 
(WINAPI *VIRTUALALLOCEX_ROUTINE)(
	IN HANDLE hProcess,
	IN LPVOID lpAddress,
	IN SIZE_T dwSize,
	IN DWORD flAllocationType,
	IN DWORD flProtect
	);

ULONG
VirtualAllocExDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// VirtualFreeEx
//

BOOL WINAPI 
VirtualFreeExEnter(
	IN HANDLE hProcess,
	IN LPVOID lpAddress,
	IN SIZE_T dwSize,
	IN DWORD dwFreeType
	);

typedef BOOL 
(WINAPI *VIRTUALFREEEX_ROUTINE)(
	IN HANDLE hProcess,
	IN LPVOID lpAddress,
	IN SIZE_T dwSize,
	IN DWORD dwFreeType
	);

ULONG
VirtualFreeExDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// VirtualProtectEx
//

BOOL WINAPI 
VirtualProtectExEnter(
	IN  HANDLE hProcess,
	IN  LPVOID lpAddress,
	IN  SIZE_T dwSize,
	IN  DWORD flNewProtect,
	OUT PDWORD lpflOldProtect
	);

typedef BOOL 
(WINAPI *VIRTUALPROTECTEX_ROUTINE)(
	IN  HANDLE hProcess,
	IN  LPVOID lpAddress,
	IN  SIZE_T dwSize,
	IN  DWORD flNewProtect,
	OUT PDWORD lpflOldProtect
	);

ULONG 
VirtualProtectExDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// VirtualQueryEx
//

SIZE_T WINAPI 
VirtualQueryExEnter(
  __in  HANDLE hProcess,
  __in  LPCVOID lpAddress,
  __out PMEMORY_BASIC_INFORMATION lpBuffer,
  __in  SIZE_T dwLength
);

typedef SIZE_T 
(WINAPI *VIRTUALQUERYEX_ROUTINE)(
  __in  HANDLE hProcess,
  __in  LPCVOID lpAddress,
  __out PMEMORY_BASIC_INFORMATION lpBuffer,
  __in  SIZE_T dwLength
  );

ULONG 
VirtualQueryExDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// CreateFileMapping
//

HANDLE WINAPI 
CreateFileMappingEnter(
	IN HANDLE hFile,
	IN LPSECURITY_ATTRIBUTES lpAttributes,
	IN DWORD flProtect,
	IN DWORD dwMaximumSizeHigh,
	IN DWORD dwMaximumSizeLow,
	IN LPCWSTR lpName
	);

typedef HANDLE 
(WINAPI *CREATEFILEMAPPING_ROUTINE)(
	IN HANDLE hFile,
	IN LPSECURITY_ATTRIBUTES lpAttributes,
	IN DWORD flProtect,
	IN DWORD dwMaximumSizeHigh,
	IN DWORD dwMaximumSizeLow,
	IN LPCWSTR lpName
	);

ULONG 
CreateFileMappingDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// MapViewOfFile
//

LPVOID WINAPI 
MapViewOfFileEnter(
	IN HANDLE hFileMappingObject,
	IN DWORD dwDesiredAccess,
	IN DWORD dwFileOffsetHigh,
	IN DWORD dwFileOffsetLow,
	IN SIZE_T dwNumberOfBytesToMap
	);

typedef LPVOID 
(WINAPI *MAPVIEWOFFILE_ROUTINE)(
	IN HANDLE hFileMappingObject,
	IN DWORD dwDesiredAccess,
	IN DWORD dwFileOffsetHigh,
	IN DWORD dwFileOffsetLow,
	IN SIZE_T dwNumberOfBytesToMap
	);

ULONG 
MapViewOfFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// UnmapViewOfFile
//

BOOL WINAPI 
UnmapViewOfFileEnter(
	IN LPCVOID lpBaseAddress
	);

typedef BOOL 
(WINAPI *UNMAPVIEWOFFILE_ROUTINE)(
	IN LPCVOID lpBaseAddress
	);

ULONG
UnmapViewOfFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// OpenFileMapping
//

HANDLE WINAPI 
OpenFileMappingEnter(
	IN DWORD dwDesiredAccess,
	IN BOOL bInheritHandle,
	IN LPCWSTR lpName
	);

typedef HANDLE 
(WINAPI *OPENFILEMAPPING_ROUTINE)(
	IN DWORD dwDesiredAccess,
	IN BOOL bInheritHandle,
	IN LPCWSTR lpName
	);

ULONG 
OpenFileMappingDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FlushViewOfFile
//

BOOL WINAPI 
FlushViewOfFileEnter(
	IN LPCVOID lpBaseAddress,
	IN SIZE_T dwNumberOfBytesToFlush
	);

typedef BOOL
(WINAPI *FLUSHVIEWOFFILE_ROUTINE)(
	IN LPCVOID lpBaseAddress,
	IN SIZE_T dwNumberOfBytesToFlush
	);

ULONG
FlushViewOfFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
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
MmConvertBitsToString(
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
