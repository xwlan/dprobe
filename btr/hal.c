//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "btr.h"
#include "hal.h"
#include "util.h"

ULONG BtrNtMajorVersion;
ULONG BtrNtMinorVersion;
ULONG BtrNtServicePack;

ULONG_PTR BtrMaximumUserAddress;
ULONG_PTR BtrMinimumUserAddress;
ULONG_PTR BtrAllocationGranularity;
ULONG_PTR BtrPageSize;
ULONG BtrNumberOfProcessors;

RTL_GET_CURRENT_PROCESSOR_NUMBER BtrProcessorRoutine; 
RTL_CAPTURE_STACK_TRACE BtrStackTraceRoutine;
GET_MODULE_INFORMATION BtrGetModuleInformation;
GET_MODULE_BASE_NAME BtrGetModuleBaseName;
DBG_PRINT BtrDbgPrint;

RTL_INIT_ANSISTRING RtlInitAnsiString;
RTL_FREE_ANSISTRING RtlFreeAnsiString;
RTL_INIT_UNICODESTRING RtlInitUnicodeString;
RTL_FREE_UNICODESTRING RtlFreeUnicodeString;
RTL_UNICODESTRING_TO_ANSISTRING RtlUnicodeStringToAnsiString;
RTL_ANSISTRING_TO_UNICODESTRING RtlAnsiStringToUnicodeString;

RTL_CREATE_USER_THREAD RtlCreateUserThread;
RTL_EXIT_USER_THREAD RtlExitUserThread;

//
// N.B. LdrGetProcedureAddress is set by btr initialize context block
// it's assigned by msp side
//

LDR_GET_PROCEDURE_ADDRESS LdrGetProcedureAddress;
LDR_LOCK_LOADER_LOCK LdrLockLoaderLock;
LDR_UNLOCK_LOADER_LOCK LdrUnlockLoaderLock;

ULONG_PTR BtrDllBase;
ULONG_PTR BtrDllSize;
HMODULE BtrPsapiHandle;

//
// Crt Routine
//

WCSNCPY  HalWcsncpy;
STRNCPY  HalStrncpy;
WCSLEN   HalWcslen;
STRLEN   HalStrlen;
VSPRINTF HalVsprintf;
SNPRINTF HalSnprintf;
SNWPRINTF HalSnwprintf;

ULONG
BtrInitializeHal(
	VOID
	)
{
	HMODULE DllHandle;
	HANDLE ProcessHandle;
	OSVERSIONINFOEX Information;
	SYSTEM_INFO SystemInformation = {0};
	ULONG Status;
	MODULEINFO Module = {0};
	WCHAR Buffer[MAX_PATH];

	Information.dwOSVersionInfoSize = sizeof(Information);
	GetVersionEx((LPOSVERSIONINFO)&Information);

	BtrNtMajorVersion = Information.dwMajorVersion;
	BtrNtMinorVersion = Information.dwMinorVersion;
	BtrNtServicePack = Information.wServicePackMajor;

	GetSystemInfo(&SystemInformation);
	BtrMinimumUserAddress = (ULONG_PTR)SystemInformation.lpMinimumApplicationAddress;
	BtrMaximumUserAddress = (ULONG_PTR)SystemInformation.lpMaximumApplicationAddress;
	BtrAllocationGranularity = (ULONG_PTR)SystemInformation.dwAllocationGranularity;
	BtrPageSize = (ULONG_PTR)SystemInformation.dwPageSize;
	BtrNumberOfProcessors = SystemInformation.dwNumberOfProcessors;

	//
	// Get system routine address
	//

	DllHandle = GetModuleHandle(L"ntdll.dll");
	if (!DllHandle) {
		BtrProcessorRoutine = NULL;
		return S_FALSE;
	}

	RtlCreateUserThread = (RTL_CREATE_USER_THREAD)GetProcAddress(DllHandle, "RtlCreateUserThread");
	RtlExitUserThread = (RTL_EXIT_USER_THREAD)GetProcAddress(DllHandle, "RtlExitUserThread");

	if (BtrNtMajorVersion > 5 || (BtrNtMajorVersion == 5 && BtrNtMinorVersion == 2)) {
		BtrProcessorRoutine = (RTL_GET_CURRENT_PROCESSOR_NUMBER)GetProcAddress(DllHandle, "RtlGetCurrentProcessorNumber");
	}
	else {
		BtrProcessorRoutine = NULL;
	}

	BtrStackTraceRoutine = (RTL_CAPTURE_STACK_TRACE)GetProcAddress(DllHandle, "RtlCaptureStackBackTrace");
	if (!BtrStackTraceRoutine) {
		return S_FALSE;
	}
	
	BtrDbgPrint = (DBG_PRINT)GetProcAddress(DllHandle, "DbgPrint");
	if (!BtrDbgPrint) {
		return S_FALSE;
	}

	LdrLockLoaderLock = (LDR_LOCK_LOADER_LOCK)GetProcAddress(DllHandle, "LdrLockLoaderLock");
	LdrUnlockLoaderLock = (LDR_UNLOCK_LOADER_LOCK)GetProcAddress(DllHandle, "LdrUnlockLoaderLock");

	//
	// Get NT STRING routines
	//

	RtlInitUnicodeString = (RTL_INIT_UNICODESTRING)GetProcAddress(DllHandle, "RtlInitUnicodeString");
	RtlFreeUnicodeString = (RTL_FREE_UNICODESTRING)GetProcAddress(DllHandle, "RtlFreeUnicodeString");
	RtlInitAnsiString = (RTL_INIT_ANSISTRING)GetProcAddress(DllHandle, "RtlInitAnsiString");
	RtlFreeAnsiString = (RTL_FREE_ANSISTRING)GetProcAddress(DllHandle, "RtlFreeAnsiString");

	RtlUnicodeStringToAnsiString = (RTL_UNICODESTRING_TO_ANSISTRING)
		                            GetProcAddress(DllHandle, "RtlUnicodeStringToAnsiString");

	RtlAnsiStringToUnicodeString = (RTL_ANSISTRING_TO_UNICODESTRING)
		                            GetProcAddress(DllHandle, "RtlAnsiStringToUnicodeString");
	
	//
	// Crt Routines
	//

	HalVsprintf = (VSPRINTF)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "vsprintf");
	HalSnprintf = (SNPRINTF)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "_snprintf");
	HalSnwprintf = (SNWPRINTF)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "_snwprintf");
	HalWcsncpy = (WCSNCPY)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "wcsncpy");
	HalStrncpy = (STRNCPY)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "strncpy");
	HalWcslen = (WCSLEN)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "wcslen");
	HalStrlen = (STRLEN)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "strlen");
	
	BtrPsapiHandle = LoadLibrary(L"psapi.dll");
	if (!BtrPsapiHandle) {
		return S_FALSE;
	}

	BtrGetModuleInformation = (GET_MODULE_INFORMATION)GetProcAddress(BtrPsapiHandle, "GetModuleInformation");
	if (!BtrGetModuleInformation) {
		return S_FALSE;
	}

	BtrGetModuleBaseName = (GET_MODULE_BASE_NAME)GetProcAddress(BtrPsapiHandle, "GetModuleBaseNameW");
	if (!BtrGetModuleBaseName) {
		return S_FALSE;
	}

	ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 
		                        FALSE, GetCurrentProcessId());
	if (!ProcessHandle) {
		return S_FALSE;
	}

	//
	// Get runtime base and its size
	//

	Status = (ULONG)(*BtrGetModuleInformation)(ProcessHandle, (HMODULE)BtrDllBase, 
			                                   &Module, sizeof(Module));
	if (!Status) {
		Status = GetLastError();
		CloseHandle(ProcessHandle);
		return Status;
	}
	
	BtrDllBase = (ULONG_PTR)Module.lpBaseOfDll;
	BtrDllSize = Module.SizeOfImage;

	(*BtrGetModuleBaseName)(ProcessHandle, NULL, Buffer, MAX_PATH);

	if (!_wcsicmp(L"csrss.exe", Buffer) || !_wcsicmp(L"smss.exe", Buffer)) {
		BtrIsNativeHost = TRUE;
	}
	
	CloseHandle(ProcessHandle);
	return S_OK;
}

VOID
BtrUninitializeHal(
	VOID
	)
{
	if (BtrPsapiHandle) {
		FreeLibrary(BtrPsapiHandle);
		BtrPsapiHandle = NULL;
	}
}

ULONG
HalCurrentProcessor(
	VOID
	)
{
	if (BtrProcessorRoutine != NULL) {
		return (*BtrProcessorRoutine)();
	
	}
	return (ULONG)-1;
}

ULONG
HalCurrentThreadId(
	VOID
	)
{
	PTEB Teb = NtCurrentTeb();
	return PtrToUlong(Teb->Cid.UniqueThread);
}

ULONG
HalCurrentProcessId(
	VOID
	)
{
	PTEB Teb = NtCurrentTeb();
	return PtrToUlong(Teb->Cid.UniqueProcess);
}

VOID
HalPrepareUnloadDll(
	IN PVOID Address 
	)
{
	PPEB Peb;
	PTEB Teb;
	PPEB_LDR_DATA Ldr; 
	PLDR_DATA_TABLE_ENTRY Module; 

	Teb = NtCurrentTeb();
	Peb = (PPEB)Teb->ProcessEnvironmentBlock;
	Ldr = Peb->Ldr;

	Module = (LDR_DATA_TABLE_ENTRY *)Ldr->InLoadOrderModuleList.Flink;

	while (Module->DllBase != 0) { 

		if ((ULONG_PTR)Module->DllBase <= (ULONG_PTR)Address && 
			(ULONG_PTR)Address < ((ULONG_PTR)Module->DllBase + Module->SizeOfImage)) {
			
			Module->LoadCount = 1;
			break;
		}
	
		Module = (LDR_DATA_TABLE_ENTRY *)Module->InLoadOrderLinks.Flink; 
	} 

	return;
}

ULONG
HalGetProcedureAddress(
	IN PVOID BaseAddress,
	IN PSTR Name,
	IN ULONG Ordinal,
	OUT PVOID *ProcedureAddress
	)
{
	LONG NtStatus;
	ANSI_STRING Ansi;

	(*RtlInitAnsiString)(&Ansi, Name);
	NtStatus = (*LdrGetProcedureAddress)(BaseAddress, &Ansi, Ordinal, ProcedureAddress);
	(*RtlFreeAnsiString)(&Ansi);
	return NtStatus;
}

BOOLEAN
HalLockLoaderLock(
	OUT PULONG Cookie	
	)
{
	LONG Status;

	if (LdrLockLoaderLock) {
		Status = (*LdrLockLoaderLock)(1, NULL, Cookie);
		return (Status == 0) ? TRUE : FALSE;
	}

	return FALSE;
}

VOID
HalUnlockLoaderLock(
	IN ULONG Cookie	
	)
{
	if (LdrUnlockLoaderLock) {
		(*LdrUnlockLoaderLock)(1, Cookie);
	}
}

BOOLEAN
HalIsAcspValid(
	VOID
	)
{
	//
	// N.B. Windows XP embed ACSP in TEB, just return TRUE
	//

	if (BtrNtMajorVersion == 5 && BtrNtMinorVersion == 1) {
		return TRUE;
	}

	return NtCurrentTeb()->ActivationContextStackPointer ? TRUE : FALSE;
}

PULONG_PTR
HalGetCurrentStackBase(
	VOID
	)
{
	return ((PULONG_PTR)((NT_TIB *)NtCurrentTeb())->StackBase) - 1;
}

PULONG_PTR
HalGetCurrentStackLimit(
	VOID
	)
{
	return ((PULONG_PTR)((NT_TIB *)NtCurrentTeb())->StackLimit) - 1;
}