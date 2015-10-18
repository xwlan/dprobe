//
// Apsara Labs 
// lan.john@gmail.com
// Copyright(C) 2009 - 2011
//

#ifndef _NTAPI_H_
#define _NTAPI_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include <windows.h>

typedef LONG NTSTATUS;
#define STATUS_SUCCESS              ((NTSTATUS)0L)
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
#define NT_SUCCESS(Status)          (((NTSTATUS)(Status)) >= 0)

typedef struct _ANSI_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PCHAR  Buffer;
} ANSI_STRING, *PANSI_STRING;

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    PVOID RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

//
// String Routines
//

typedef 
VOID 
(NTAPI *RTL_INIT_UNICODESTRING)(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString
    );

typedef
VOID 
(NTAPI *RTL_INIT_ANSISTRING)(
    IN OUT PANSI_STRING  DestinationString,
    IN PSTR  SourceString
    );

typedef 
NTSTATUS
(NTAPI *RTL_UNICODESTRING_TO_ANSISTRING)(
    IN PANSI_STRING DestinationString,
    IN PUNICODE_STRING SourceString,
    IN BOOLEAN AllocateDestinationString
    );

typedef 
NTSTATUS 
(NTAPI *RTL_ANSISTRING_TO_UNICODESTRING)(
    IN OUT PUNICODE_STRING  DestinationString,
    IN PANSI_STRING  SourceString,
    IN BOOLEAN  AllocateDestinationString
    );

typedef 
VOID 
(NTAPI *RTL_FREE_UNICODESTRING)(
    IN PUNICODE_STRING  UnicodeString
    );

typedef 
VOID
(NTAPI *RTL_FREE_ANSISTRING)(
    IN PANSI_STRING AnsiString
    );

//
// Process and Thread Routines
//

typedef LONG KPRIORITY;

typedef struct _CLIENT_ID {
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;
//
//typedef enum _THREAD_INFORMATION_CLASS {
//    ThreadBasicInformation,
//} THREAD_INFORMATION_CLASS;
//
//typedef enum _PROCESS_INFORMATION_CLASS {
//    ProcessBasicInformation,
//} PROCESS_INFORMATION_CLASS;

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation,
    SystemProcessorInformation,
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemPathInformation,
    SystemProcessInformation,
    SystemCallCountInformation,
    SystemDeviceInformation,
    SystemProcessorPerformanceInformation,
} SYSTEM_INFORMATION_CLASS;

typedef struct _PEB *PPEB;
typedef struct _TEB *PTEB;

typedef struct _THREAD_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PTEB TebBaseAddress;
    CLIENT_ID ClientId;
    ULONG_PTR AffinityMask;
    KPRIORITY Priority;
    LONG BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;
 
typedef struct _PROCESS_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PPEB PebBaseAddress;
    ULONG_PTR AffinityMask;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

typedef enum _KWAIT_REASON {
    Executive = 0,
    FreePage = 1,
    PageIn = 2,
    PoolAllocation = 3,
    DelayExecution = 4,
    Suspended = 5,
    UserRequest = 6,
    WrExecutive = 7,
    WrFreePage = 8,
    WrPageIn = 9,
    WrPoolAllocation = 10,
    WrDelayExecution = 11,
    WrSuspended = 12,
    WrUserRequest = 13,
    WrEventPair = 14,
    WrQueue = 15,
    WrLpcReceive = 16,
    WrLpcReply = 17,
    WrVirtualMemory = 18,
    WrPageOut = 19,
    WrRendezvous = 20,
    Spare2 = 21,
    Spare3 = 22,
    Spare4 = 23,
    Spare5 = 24,
    WrCalloutStack = 25,
    WrKernel = 26,
    WrResource = 27,
    WrPushLock = 28,
    WrMutex = 29,
    WrQuantumEnd = 30,
    WrDispatchInt = 31,
    WrPreempted = 32,
    WrYieldExecution = 33,
    WrFastMutex = 34,
    WrGuardedMutex = 35,
    WrRundown = 36,
    MaximumWaitReason = 37
} KWAIT_REASON, *PKWAIT_REASON;

typedef struct _SYSTEM_THREAD_INFORMATION {
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG WaitTime;
    PVOID StartAddress;
    CLIENT_ID ClientId;
    KPRIORITY Priority;
    LONG BasePriority;
    ULONG ContextSwitches;
    ULONG ThreadState;
    KWAIT_REASON WaitReason;
} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;

typedef struct _SYSTEM_PROCESS_INFORMATION {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER Spare[3];
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG_PTR PageDirectoryBase;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
    SYSTEM_THREAD_INFORMATION Threads[1];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

typedef
NTSTATUS 
(NTAPI *NT_QUERY_SYSTEM_INFORMATION)(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength
	);

typedef 
NTSTATUS
(NTAPI *NT_QUERY_INFORMATION_THREAD)(
    IN HANDLE ThreadHandle,
    IN ULONG ThreadInformationClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength
    );

typedef
NTSTATUS
(NTAPI *NT_QUERY_INFORMATION_PROCESS)(
    IN HANDLE ProcessHandle,
    IN ULONG ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength 
	);

typedef
NTSTATUS
(NTAPI *NT_SUSPEND_PROCESS)(
    IN HANDLE ProcessHandle
	);

typedef
NTSTATUS
(NTAPI *NT_RESUME_PROCESS)(
    IN HANDLE ProcessHandle
	);

typedef NTSTATUS
(NTAPI *RTL_CREATE_USER_THREAD)(
    IN HANDLE Process,
    IN PSECURITY_DESCRIPTOR ThreadSecurityDescriptor,
    IN BOOLEAN CreateSuspended,
    IN ULONG ZeroBits,
    IN SIZE_T MaximumStackSize,
    IN SIZE_T CommittedStackSize,
    IN LPTHREAD_START_ROUTINE StartAddress,
    IN PVOID Parameter,
    OUT PHANDLE Thread,
    OUT PCLIENT_ID ClientId
    );

//
// Registry Routines
//

typedef enum _KEY_INFORMATION_CLASS {
	KeyBasicInformation,
	KeyCachedInformation,
	KeyNameInformation,
	KeyNodeInformation,
	KeyFullInformation,
	KeyVirtualizationInformation
} KEY_INFORMATION_CLASS;

typedef 
NTSTATUS 
(NTAPI *NT_QUERY_KEY)(
    IN HANDLE  KeyHandle,
    IN KEY_INFORMATION_CLASS  KeyInformationClass,
    OUT PVOID  KeyInformation,
    IN ULONG  Length,
    OUT PULONG  ResultLength
    );

typedef NTSTATUS 
(NTAPI *PUSER_THREAD_START_ROUTINE)(
    IN PVOID Context
    );

typedef NTSTATUS 
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

typedef NTSTATUS
(NTAPI *LDR_LOAD_DLL)(
	IN PWCHAR PathToFile,
	IN ULONG Flags,
	IN PUNICODE_STRING ModuleFileName, 
	OUT PHANDLE ModuleHandle
	); 

//
// N.B. Maximum 4 arguments supported for both
// x86 and x64
//

typedef NTSTATUS
(NTAPI *RTL_REMOTE_CALL)(
    IN HANDLE Process,
    IN HANDLE Thread,
    IN PVOID CallSite,
    IN ULONG ArgumentCount,
    IN PULONG_PTR Arguments,
    IN BOOLEAN PassContext,
    IN BOOLEAN AlreadySuspended
	);

//
// System Routine Pointers
//

extern NT_QUERY_SYSTEM_INFORMATION  NtQuerySystemInformation;
extern NT_QUERY_INFORMATION_PROCESS NtQueryInformationProcess;
extern NT_QUERY_INFORMATION_THREAD  NtQueryInformationThread;

extern NT_SUSPEND_PROCESS NtSuspendProcess;
extern NT_RESUME_PROCESS  NtResumeProcess;

extern RTL_CREATE_USER_THREAD RtlCreateUserThread;
extern RTL_REMOTE_CALL RtlRemoteCall;

#ifdef __cplusplus 
}
#endif

#endif
