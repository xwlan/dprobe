//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _HAL_H_
#define _HAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <psapi.h>

typedef struct _STRING {
  USHORT  Length;
  USHORT  MaximumLength;
  PCHAR  Buffer;
} ANSI_STRING, *PANSI_STRING;

typedef struct _UNICODE_STRING {
  USHORT  Length;
  USHORT  MaximumLength;
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

//
// N.B. This is only a small fix part of NT TEB, we don't use other
// fields, what we're interested are Cid, ActivationContextStackPointer,
// ExceptionCode CountOfOwnedCriticalSections, ProcessEnvironmentBlock.
//
//

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _TEB {
    NT_TIB Tib;
    PVOID EnvironmentPointer;
    CLIENT_ID Cid;
    PVOID ActiveRpcHandle;
    PVOID ThreadLocalStoragePointer;
    struct _PEB *ProcessEnvironmentBlock;
    ULONG LastErrorValue;
    ULONG CountOfOwnedCriticalSections;
    PVOID CsrClientThread;
    struct _W32THREAD* Win32ThreadInfo;
    ULONG User32Reserved[0x1A];
    ULONG UserReserved[5];
    PVOID WOW32Reserved;
    LCID CurrentLocale;
    ULONG FpSoftwareStatusRegister;
    PVOID SystemReserved1[0x36];
    LONG ExceptionCode;

	//
	// N.B. Windows XP embed a _ACTIVATION_CONTEXT_STACK structure,
	// not a pointer. like the followings:
	//
	// +0x1a8 ActivationContextStack : _ACTIVATION_CONTEXT_STACK
    //  +0x000 Flags            : Uint4B
    //  +0x004 NextCookieSequenceNumber : Uint4B
    //  +0x008 ActiveFrame      : Ptr32 Void
    //  +0x00c FrameListCache   : _LIST_ENTRY
    // +0x1bc SpareBytes1      : [24] UChar
	//

    struct _ACTIVATION_CONTEXT_STACK *ActivationContextStackPointer;

} TEB, *PTEB;

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

typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    union
    {
        LIST_ENTRY HashLinks;
        PVOID SectionPointer;
    };
    ULONG CheckSum;
    union
    {
        ULONG TimeDateStamp;
        PVOID LoadedImports;
    };
    PVOID EntryPointActivationContext;
    PVOID PatchInformation;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

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


typedef BOOL 
(WINAPI *GET_MODULE_INFORMATION)(
	__in HANDLE hProcess,
	__in HMODULE hModule,
	__in LPMODULEINFO lpmodinfo,
	__in DWORD cb
	);

typedef DWORD
(WINAPI *GET_MODULE_BASE_NAME)(
	__in  HANDLE hProcess,
	__in  HMODULE hModule,
	__out LPTSTR lpBaseName,
	__in  DWORD nSize
	);

typedef DWORD 
(WINAPI *RTL_GET_CURRENT_PROCESSOR_NUMBER)(
	VOID
	);

typedef USHORT 
(WINAPI *RTL_CAPTURE_STACK_TRACE)(
	IN ULONG FramesToSkip,
	IN ULONG FramesToCapture,
	OUT PVOID* BackTrace,
	OUT PULONG BackTraceHash
	);

//
// NT STRING API
//

typedef VOID 
(WINAPI *RTL_INIT_UNICODESTRING)(
    IN OUT PUNICODE_STRING  DestinationString,
    IN PCWSTR  SourceString
    );

typedef VOID 
(WINAPI *RTL_FREE_UNICODESTRING)(
    IN PUNICODE_STRING  UnicodeString
    );

typedef VOID 
(WINAPI *RTL_INIT_ANSISTRING)(
    IN OUT PANSI_STRING  DestinationString,
    IN PSTR SourceString
    );

typedef VOID  
(WINAPI *RTL_FREE_ANSISTRING)(
    IN PANSI_STRING  AnsiString
    );

typedef LONG 
(WINAPI *RTL_UNICODESTRING_TO_ANSISTRING)(
    IN OUT PANSI_STRING  DestinationString,
    IN PUNICODE_STRING  SourceString,
    IN BOOLEAN  AllocateDestinationString
    );

typedef LONG 
(WINAPI *RTL_ANSISTRING_TO_UNICODESTRING)(
    IN OUT PUNICODE_STRING  DestinationString,
    IN PANSI_STRING  SourceString,
    IN BOOLEAN  AllocateDestinationString
    );

typedef ULONG
(__cdecl *DBG_PRINT)(
    IN PCHAR Format,
	...
    );

ULONG
BtrInitializeHal(
	VOID
	);

extern ULONG BtrNtMajorVersion;
extern ULONG BtrNtMinorVersion;
extern ULONG_PTR BtrMaximumUserAddress;
extern ULONG_PTR BtrMinimumUserAddress;
extern ULONG_PTR BtrAllocationGranularity;
extern ULONG_PTR BtrPageSize;
extern ULONG BtrNumberOfProcessors;
extern BOOLEAN BtrIsNativeHost;

extern ULONG_PTR BtrDllBase;
extern ULONG_PTR BtrDllSize;
extern HMODULE BtrPsapiHandle;

extern GET_MODULE_INFORMATION BtrGetModuleInformation;
extern RTL_GET_CURRENT_PROCESSOR_NUMBER BtrProcessorRoutine; 
extern RTL_CAPTURE_STACK_TRACE BtrStackTraceRoutine;
extern DBG_PRINT BtrDbgPrint;

extern RTL_INIT_ANSISTRING RtlInitAnsiString;
extern RTL_FREE_ANSISTRING RtlFreeAnsiString;
extern RTL_INIT_UNICODESTRING RtlInitUnicodeString;
extern RTL_FREE_UNICODESTRING RtlFreeUnicodeString;
extern RTL_UNICODESTRING_TO_ANSISTRING RtlUnicodeStringToAnsiString;
extern RTL_ANSISTRING_TO_UNICODESTRING RtlAnsiStringToUnicodeString;

typedef LONG
(NTAPI *RTL_CREATE_USER_THREAD)(
    IN HANDLE Process,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN CreateSuspended,
    IN ULONG ZeroBits,
    IN SIZE_T MaximumStackSize,
    IN SIZE_T CommittedStackSize,
    IN PTHREAD_START_ROUTINE StartAddress,
    IN PVOID Context,
    OUT PHANDLE Thread,
    OUT PCLIENT_ID ClientId
    );

typedef LONG 
(NTAPI *RTL_EXIT_USER_THREAD)(
    IN ULONG Status
	);

extern RTL_CREATE_USER_THREAD RtlCreateUserThread;
extern RTL_EXIT_USER_THREAD RtlExitUserThread;

//
// Loader Routine 
//

typedef LONG 
(NTAPI *LDR_GET_PROCEDURE_ADDRESS)(
    IN PVOID BaseAddress,
    IN PANSI_STRING Name,
    IN ULONG Ordinal,
    OUT PVOID *ProcedureAddress
	);

typedef LONG 
(NTAPI *LDR_LOCK_LOADER_LOCK)(
    IN ULONG Flags,
    OUT PULONG Disposition OPTIONAL,
    OUT PULONG Cookie OPTIONAL
	);

typedef LONG 
(NTAPI *LDR_UNLOCK_LOADER_LOCK)(
    IN ULONG Flags,
    IN ULONG Cookie OPTIONAL
	);

ULONG
HalCurrentProcessor(
	VOID
	);

ULONG
HalCurrentThreadId(
	VOID
	);

ULONG
HalCurrentProcessId(
	VOID
	);

BOOLEAN
HalLockLoaderLock(
	OUT PULONG Cookie	
	);

VOID
HalUnlockLoaderLock(
	IN ULONG Cookie	
	);

VOID
HalPrepareUnloadDll(
	IN PVOID Address 
	);

BOOLEAN
HalIsAcspValid(
	VOID
	);

PULONG_PTR
HalGetCurrentStackBase(
	VOID
	);

PULONG_PTR
HalGetCurrentStackLimit(
	VOID
	);

typedef 
int (__cdecl *VSPRINTF)(
	char *buffer,
	const char *format,
    va_list argptr 
	); 

typedef
int (__cdecl *SNPRINTF)(
	char *buffer,
	size_t count,
	const char *format,
	... 
	);

typedef
int (__cdecl *SNWPRINTF)(
	wchar_t *buffer,
	size_t count,
	const wchar_t *format,
	... 
	);

typedef
char (__cdecl *STRNCPY)(
	char *strDest,
	const char *strSource,
	size_t count 
	);

typedef 
wchar_t (__cdecl *WCSNCPY)(
	wchar_t *strDest,
	const wchar_t *strSource,
	size_t count 
	);

typedef 
size_t (__cdecl *STRLEN)(
	const char *str
	);

typedef 
size_t (__cdecl *WCSLEN)(
	const wchar_t *str 
	);
	
extern WCSNCPY  HalWcsncpy;
extern STRNCPY  HalStrncpy;
extern WCSLEN   HalWcslen;
extern STRLEN   HalStrlen;
extern VSPRINTF HalVsprintf;
extern SNWPRINTF HalSnwprintf;
extern SNPRINTF HalSnprintf;

#ifdef __cplusplus
}
#endif

#endif