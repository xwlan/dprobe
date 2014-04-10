//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "mspdefs.h"
#include <tlhelp32.h>
#include <psapi.h>
#include "mspport.h"
#include "msputility.h"
#include "mspprocess.h"

WCHAR MspRuntimePath[MAX_PATH];
WCHAR MspRuntimeName[MAX_PATH];

HANDLE MspPrivateHeap;
ULONG MspMajorVersion;
ULONG MspMinorVersion;

ULONG MspPageSize;
ULONG MspProcessorNumber;
RTL_CREATE_USER_THREAD MspRtlCreateUserThread;

#if defined (_M_IX86)
BOOLEAN MspIs64Bits = FALSE;

#elif defined (_M_X64)
BOOLEAN MspIs64Bits = TRUE;

#endif

//
// Runtime process type flag
//

BOOLEAN MspIsWow64;

typedef BOOL 
(WINAPI *ISWOW64PROCESS)(
	IN HANDLE, 
	OUT PBOOL
	);

ISWOW64PROCESS MspIsWow64ProcessPtr;

#define MSP_OPEN_PROCESS_FLAG (PROCESS_QUERY_INFORMATION |  \
                               PROCESS_CREATE_THREAD     |  \
							   PROCESS_VM_OPERATION      |  \
							   PROCESS_VM_WRITE) 

//
// Performance Counter Frequency (second per tick)
//

double MspSecondPerHardwareTick;


BOOLEAN
MspGetModuleInformation(
	IN ULONG ProcessId,
	IN PWSTR DllName,
	IN BOOLEAN FullPath,
	OUT HMODULE *DllHandle,
	OUT PULONG_PTR Address,
	OUT SIZE_T *Size
	)
{ 
	BOOLEAN Found = FALSE;
	MODULEENTRY32 Module; 
	HANDLE Handle = INVALID_HANDLE_VALUE; 

	Handle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessId); 
	if (Handle == INVALID_HANDLE_VALUE) { 
		return FALSE;
	} 

	Module.dwSize = sizeof(MODULEENTRY32); 

	if (!Module32First(Handle, &Module)) { 
		CloseHandle(Handle);
		return FALSE; 
	} 

	do { 

		if (FullPath) {
			Found = !_wcsicmp(DllName, Module.szExePath);
		} else {
			Found = !_wcsicmp(DllName, Module.szModule);
		}

		if (Found) {

			if (DllHandle) {
				*DllHandle = Module.hModule;
			}

			if (Address) {
				*Address = (ULONG_PTR)Module.modBaseAddr;	
			}

			if (Size) {
				*Size = Module.modBaseSize;
			}

			CloseHandle(Handle);
			return TRUE;
		}

	} while (Module32Next(Handle, &Module));

	CloseHandle(Handle); 
	return FALSE; 
} 


PVOID
MspGetRemoteApiAddress(
	IN ULONG ProcessId,
	IN PWSTR DllName,
	IN PSTR ApiName
	)
{ 
	HMODULE DllHandle;
	ULONG_PTR Address;
	HMODULE LocalDllHandle;
	ULONG_PTR LocalAddress;
	SIZE_T Size;
	ULONG Status;

	ASSERT(GetCurrentProcessId() != ProcessId);
	
	LocalDllHandle = GetModuleHandle(DllName);
	if (!LocalDllHandle) {
		return NULL;
	}
	
	LocalAddress = (ULONG_PTR)GetProcAddress(LocalDllHandle, ApiName);
	if (!LocalAddress) {
		return NULL;
	}

	//
	// N.B. It's only valid to a few system dll (non side by side), we call this
	// routine primarily to handle ASLR case.
	//

	Status = MspGetModuleInformation(ProcessId, DllName, FALSE, &DllHandle, &Address, &Size);
	if (!Status) {
		return NULL;
	}

	return (PVOID)((ULONG_PTR)DllHandle + (LocalAddress - (ULONG_PTR)LocalDllHandle));	
}


BOOLEAN
MspIsWindowsXPAbove(
	VOID
	)
{
	if (MspMajorVersion > 5) {
		return TRUE;
	}

	if (MspMajorVersion == 5 &&MspMinorVersion >= 1) {
		return TRUE;
	}

	return FALSE;
}


BOOLEAN 
MspIsWow64Process(
	IN HANDLE ProcessHandle	
	)
{
    BOOL IsWow64 = FALSE;
   
    if (MspIsWow64ProcessPtr) {
        MspIsWow64ProcessPtr(ProcessHandle, &IsWow64);
    }
    return (BOOLEAN)IsWow64;
}

BOOLEAN
MspIsLegitimateProcess(
	IN ULONG ProcessId
	)
{
	BOOLEAN Status;
	HANDLE ProcessHandle;

	//
	// If can't open process, we can do nothing.
	//

	ProcessHandle = OpenProcess(MSP_OPEN_PROCESS_FLAG, FALSE, ProcessId);
	if (!ProcessHandle) {
		return FALSE;
	}

	if (MspIs64Bits) {

		//
		// Native 64bits MSP only handle 64 bits process.
		//

		Status = MspIsWow64Process(ProcessHandle);
		CloseHandle(ProcessHandle);
		return !Status;
	}
	else {
		
		//
		// MSP runs on WOW64
		//

		if (MspIsWow64) {
			Status = MspIsWow64Process(ProcessHandle);
			return Status;
		}

		//
		// MSP runs on native 32 bits system
		//

		return TRUE;
	}
}

BOOLEAN
MspIsRuntimeLoaded(
	IN ULONG ProcessId
	)
{
	BOOLEAN Status;

	Status = MspGetModuleInformation(ProcessId, MspRuntimePath, TRUE, 
		                             NULL, NULL, NULL);
	return Status;
}

ULONG
MspAdjustPrivilege(
    IN PWSTR PrivilegeName, 
    IN BOOLEAN Enable
    ) 
{
    HANDLE Handle;
    TOKEN_PRIVILEGES Privileges;
    BOOLEAN Status;
	
	Status = OpenProcessToken(GetCurrentProcess(),  
		                      TOKEN_ADJUST_PRIVILEGES, 
							  &Handle);
    if (Status != TRUE){
        return GetLastError();
    }
    
    Privileges.PrivilegeCount = 1;

    Status = LookupPrivilegeValue(NULL, PrivilegeName, &Privileges.Privileges[0].Luid);

	if (Status != TRUE) {
		CloseHandle(Handle);
		return GetLastError();
	}

	//
	// N.B. SE_PRIVILEGE_REMOVED must be avoided since this will remove privilege from
	// primary token and never get recovered again for current process. For disable case
	// we should use 0.
	//

    Privileges.Privileges[0].Attributes = (Enable ? SE_PRIVILEGE_ENABLED : 0);
    Status = AdjustTokenPrivileges(Handle, 
		                           FALSE, 
								   &Privileges, 
								   sizeof(Privileges), 
								   NULL, 
								   NULL);
	if (Status != TRUE) {
		CloseHandle(Handle);
		return GetLastError();
	}

    CloseHandle(Handle);
    return MSP_STATUS_OK;
}

ULONG
MspLoadRuntime(
	IN ULONG ProcessId,
	IN HANDLE ProcessHandle
	)
{
	ULONG Status;
	HANDLE ThreadHandle;
	PVOID Buffer;
	SIZE_T Length;
	PVOID ThreadRoutine;
	ULONG Features;

	//
	// N.B. This is a subtle case, if the dll is already loaded,
	// we have no idea of its state, to keep reliability, we don't
	// want crash customer's application. the dll is possibly is in
	// pending unloading e.g.
	//
	
	Buffer = (PWCHAR)VirtualAllocEx(ProcessHandle, NULL, MspPageSize, MEM_COMMIT, PAGE_READWRITE);
	if (!Buffer) { 
		Status = GetLastError();
		return Status;
	}

	Length = (wcslen(MspRuntimePath) + 1) * sizeof(WCHAR);
	Status = WriteProcessMemory(ProcessHandle, Buffer, MspRuntimePath, Length, NULL); 
	if (!Status) {
		Status = GetLastError();
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		return Status;
	}	
	
	ThreadRoutine = MspGetRemoteApiAddress(ProcessId, L"kernel32.dll", "LoadLibraryW");
	if (!ThreadRoutine) {
		return MSP_STATUS_INVALID_PARAMETER;
	}

	//
	// Runtime initialization stage 0 to do fundamental initialization
	//

	ThreadHandle = 0;
	if (MspMajorVersion >= 6) {
		Status = (ULONG)(*MspRtlCreateUserThread)(ProcessHandle, NULL, TRUE, 0, 0, 0,
								                  (LPTHREAD_START_ROUTINE)ThreadRoutine, 
										          Buffer, &ThreadHandle, NULL);

	} else {
		ThreadHandle = CreateRemoteThread(ProcessHandle, NULL, 0, 
			                              (LPTHREAD_START_ROUTINE)ThreadRoutine, 
			                              Buffer, CREATE_SUSPENDED, NULL);
	}

	if (!ThreadHandle) {
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		return Status;				
	}

	ResumeThread(ThreadHandle);
	WaitForSingleObject(ThreadHandle, INFINITE);
	CloseHandle(ThreadHandle);

	//
	// Check whether it's really loaded into target address space
	//

	Status = MspGetModuleInformation(ProcessId, MspRuntimePath, TRUE, 
		                             NULL, NULL, NULL);

	if (!Status) {
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		return MSP_STATUS_ERROR;
	}

	//
	// Runtime initialization stage 1 to start runtime threads
	//

	ThreadRoutine = MspGetRemoteApiAddress(ProcessId, MspRuntimeName, "BtrFltStartRuntime");
	if (!ThreadRoutine) {
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		CloseHandle(ProcessHandle);
		return MSP_STATUS_ERROR;
	}

	//
	// FeatureRemote must be set, otherwise btr will not start work as expected
	//

	Features = FeatureRemote;
	Status = WriteProcessMemory(ProcessHandle, Buffer, &Features, sizeof(Features), NULL); 
	if (!Status) {
		Status = GetLastError();
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		return Status;
	}	

	ThreadHandle = 0;
	if (MspMajorVersion >= 6) {
		Status = (ULONG)(*MspRtlCreateUserThread)(ProcessHandle, NULL, TRUE, 0, 0, 0,
								               (LPTHREAD_START_ROUTINE)ThreadRoutine, 
										       Buffer, &ThreadHandle, NULL);

	} else {
		ThreadHandle = CreateRemoteThread(ProcessHandle, NULL, 0, 
			                              (LPTHREAD_START_ROUTINE)ThreadRoutine, 
			                              Buffer, CREATE_SUSPENDED, NULL);
	}

	if (!ThreadHandle) {
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		return Status;				
	}

	ResumeThread(ThreadHandle);
	WaitForSingleObject(ThreadHandle, INFINITE);

	VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
	GetExitCodeThread(ThreadHandle, &Status);
	CloseHandle(ThreadHandle);
	return Status;
}

ULONG
MspUnloadRuntime(
	IN ULONG ProcessId
	)
{
	return MSP_STATUS_OK;
}

PVOID
MspMalloc(
	IN SIZE_T Length
	)
{
	return HeapAlloc(MspPrivateHeap, HEAP_ZERO_MEMORY, Length);
}

VOID
MspFree(
	IN PVOID Address
	)
{
	HeapFree(MspPrivateHeap, 0, Address);
}

HANDLE
MspCreatePrivateHeap(
	VOID
	)
{
	HANDLE Handle;
	ULONG HeapFragValue = 2;

	Handle = HeapCreate(0, 0, 0);
	HeapSetInformation(Handle, HeapCompatibilityInformation, 
		               &HeapFragValue, sizeof(ULONG));

	return Handle;
}

VOID
MspDestroyPrivateHeap(
	IN HANDLE NormalHeap
	)
{
	HeapDestroy(MspPrivateHeap);
}

BOOLEAN
MspIsValidFilterPath(
	IN PWSTR Path
	)
{
	HMODULE Handle;

	Handle = LoadLibraryEx(Path, NULL, LOAD_LIBRARY_AS_DATAFILE);
	if (Handle != NULL) {
		FreeLibrary(Handle);
		return TRUE;
	}

	return FALSE;
}

ULONG
MspQueryProcessName(
	IN PMSP_PROCESS Process,
	IN ULONG ProcessId
	)
{
	HANDLE Handle;
	ULONG Length;
	WCHAR Buffer[MAX_PATH];

	Handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
	if (!Handle) {
		return MSP_STATUS_ERROR;
	}
	
	Length = GetModuleFileNameEx(Handle, NULL, Process->FullName, MAX_PATH);
	CloseHandle(Handle);

	if (!Length) {
		return MSP_STATUS_ERROR;
	}

	Buffer[0] = 0;
	_wsplitpath(Process->FullName, NULL, NULL, Process->BaseName, Buffer);

	if (Buffer[0] != 0) {
		wcscat_s(Process->BaseName, MAX_PATH, Buffer);
	}

	return MSP_STATUS_OK;
}

ULONG
MspQueryProcessFullPath(
	IN ULONG ProcessId,
	IN PWSTR Buffer,
	IN ULONG Length,
	OUT PULONG Result
	)
{
	HANDLE Handle;

	Handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
	if (!Handle) {
		return MSP_STATUS_ERROR;
	}
	
	*Result = GetModuleFileNameEx(Handle, NULL, Buffer, Length);
	CloseHandle(Handle);

	if (*Result != 0) {
		return MSP_STATUS_OK;
	}

	return MSP_STATUS_ERROR;
}

ULONG_PTR
MspUlongPtrRoundDown(
	IN ULONG_PTR Value,
	IN ULONG_PTR Align
	)
{
	return Value & ~(Align - 1);
}

ULONG
MspUlongRoundDown(
	IN ULONG Value,
	IN ULONG Align
	)
{
	return Value & ~(Align - 1);
}

ULONG64
MspUlong64RoundDown(
	IN ULONG64 Value,
	IN ULONG64 Align
	)
{
	return Value & ~(Align - 1);
}

ULONG_PTR
MspUlongPtrRoundUp(
	IN ULONG_PTR Value,
	IN ULONG_PTR Align
	)
{
	return (Value + Align - 1) & ~(Align - 1);
}

ULONG
MspUlongRoundUp(
	IN ULONG Value,
	IN ULONG Align
	)
{
	return (Value + Align - 1) & ~(Align - 1);
}

ULONG64
MspUlong64RoundUp(
	IN ULONG64 Value,
	IN ULONG64 Align
	)
{
	return (Value + Align - 1) & ~(Align - 1);
}

BOOLEAN
MspIsUlongAligned(
	IN ULONG Value,
	IN ULONG Unit
	)
{
	return ((Value & (Unit - 1)) == 0) ? TRUE : FALSE;
}

BOOLEAN
MspIsUlong64Aligned(
	IN ULONG64 Value,
	IN ULONG64 Unit
	)
{
	return ((Value & (Unit - 1)) == 0) ? TRUE : FALSE;
}

double
MspComputeMilliseconds(
	IN ULONG Duration
	)
{
	return (Duration * MspSecondPerHardwareTick) * 1000;
}

VOID
MspComputeHardwareFrequency(
	VOID
	)
{
	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);
	MspSecondPerHardwareTick = 1.0 / Frequency.QuadPart;
}

VOID
MspCleanCache(
	IN PWSTR Path,
	IN ULONG Level
	)
{
	ULONG Status;
	WIN32_FIND_DATA Data;
	HANDLE FindHandle;
	WCHAR Buffer[MAX_PATH];
	WCHAR FilePath[MAX_PATH];
	
	Status = FALSE;

	StringCchCopy(Buffer, MAX_PATH, Path);
	StringCchCat(Buffer, MAX_PATH, L"\\*");

	FindHandle = FindFirstFile(Buffer, &Data);
	if (FindHandle != INVALID_HANDLE_VALUE) {
		Status = TRUE;
	}

	while (Status) {
		
		StringCchCopy(FilePath, MAX_PATH, Path);
		StringCchCat(FilePath, MAX_PATH, L"\\");
		StringCchCat(FilePath, MAX_PATH, Data.cFileName);

		if (!(Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			DeleteFile(FilePath);
			DebugTrace("remove file: %S", FilePath);
		}

		if ((Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			if (wcscmp(Data.cFileName, L".") && wcscmp(Data.cFileName, L"..")) {
				StringCchPrintf(Buffer, MAX_PATH, L"%s\\%s", Path, Data.cFileName);
				MspCleanCache(Buffer, Level + 1);
			}
		}

		Status = (ULONG)FindNextFile(FindHandle, &Data);
	}

	if (Level != 0) {
		RemoveDirectory(Path);
		DebugTrace("remove dir: %S", Path);
	}

	FindClose(FindHandle);
}

ULONG
MspIsManagedModule(
	IN PWSTR Path,
	OUT BOOLEAN *Managed
	)
{
	PVOID Address;
	HANDLE FileHandle;
	HANDLE MappingHandle;
	PIMAGE_SECTION_HEADER Header;
	ULONG Size;
	PVOID Ptr;

	Address = BspMapImageFile(Path, &FileHandle, &MappingHandle);
	if (!Address) {
		return MSP_STATUS_ERROR;
	}

	Ptr = ImageDirectoryEntryToDataEx(Address, FALSE, IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
									  &Size, &Header);
	if (Ptr) {
		*Managed = TRUE;
	} else {
		*Managed = FALSE;
	}

	UnmapViewOfFile(Address);
	CloseHandle(MappingHandle);
	CloseHandle(FileHandle);

	return MSP_STATUS_OK;
}

ULONG
MspGetImageType(
	IN PWSTR Path,
	OUT PULONG Type
	)
{
	PVOID Address;
	HANDLE FileHandle;
	HANDLE MappingHandle;
	PIMAGE_NT_HEADERS Header;

	Address = BspMapImageFile(Path, &FileHandle, &MappingHandle);
	if (!Address) {
		return MSP_STATUS_ERROR;
	}

	Header = ImageNtHeader(Address);
	if (!Header) {
		return MSP_STATUS_ERROR;
	}

	if (Header->Signature != 0x4550) {
		return MSP_STATUS_ERROR;
	}

	//
	// Get machine architecture 
	//

	*Type = Header->FileHeader.Machine;

	UnmapViewOfFile(Address);
	CloseHandle(MappingHandle);
	CloseHandle(FileHandle);

	return MSP_STATUS_OK;
}

BOOLEAN
MspIsRuntimeFilterDll(
	IN PWSTR Path
	)
{
	PBTR_FILTER Filter;

	//
	// Check whether it's btr runtime
	//

	if (!wcsicmp(MspRuntimePath, Path)) {
		return TRUE;
	}

	//
	// Check whether it's filter dll
	//

	Filter = MspQueryDecodeFilterByPath(Path);
	if (Filter != NULL) {
		return TRUE;
	}

	return FALSE;
}

