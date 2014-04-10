//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#include "bsp.h"
#include "psp.h"
#include "ntapi.h"

BOOLEAN
PspIsRuntimeLoaded(
	IN ULONG ProcessId
	)
{
	BOOLEAN Status;

	Status = BspGetModuleInformation(ProcessId, PspRuntimeFullPath, TRUE, 
		                             NULL, NULL, NULL);
	return Status;
}

ULONG
PspLoadProbeRuntime(
	IN ULONG ProcessId
	)
{
	ULONG Status;
	HANDLE ProcessHandle;
	HANDLE ThreadHandle;
	HANDLE FileHandle;
	PVOID Buffer;
	SIZE_T Length;
	PVOID ThreadRoutine;

	//
	// Test whether the runtime file exists on given path. 
	//

	FileHandle = CreateFile(PspRuntimeFullPath,
	                        GENERIC_READ,
	                        FILE_SHARE_READ,
	                        NULL,
	                        OPEN_EXISTING,
	                        FILE_ATTRIBUTE_NORMAL,
	                        NULL);
 
	if (FileHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(FileHandle);
	} else {
		return GetLastError();
	}

	//
	// N.B. This is a subtle case, if the dll is already loaded,
	// we have no idea of its state, to keep reliability, we don't
	// want crash customer's application. the dll is possibly is in
	// pending unloading e.g.
	//

	if (PspIsRuntimeLoaded(ProcessId)) {
		return S_OK;
	}

	ProcessHandle = OpenProcess(PSP_OPEN_PROCESS_FLAG, FALSE, ProcessId);
	if (!ProcessHandle) {
		return GetLastError();
	}
	
	Buffer = (PWCHAR)VirtualAllocEx(ProcessHandle, NULL, BspPageSize, MEM_COMMIT, PAGE_READWRITE);
	if (!Buffer) { 
		Status = GetLastError();
		CloseHandle(ProcessHandle);
		return Status;
	}

	Length = (wcslen(PspRuntimeFullPath) + 1) * sizeof(WCHAR);
	Status = WriteProcessMemory(ProcessHandle, Buffer, PspRuntimeFullPath, Length, NULL); 
	if (!Status) {
		Status = GetLastError();
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		CloseHandle(ProcessHandle);
		return Status;
	}	
	
	ThreadRoutine = BspGetRemoteApiAddress(ProcessId, L"kernel32.dll", "LoadLibraryW");
	if (!ThreadRoutine) {
		return ERROR_INVALID_PARAMETER;
	}

	//
	// Execute LoadLibraryW(L"dprobe.btr.dll")
	//

	ThreadHandle = 0;
	if (BspMajorVersion >= 6) {
		Status = (ULONG)(*RtlCreateUserThread)(ProcessHandle, NULL, TRUE, 0, 0, 0,
								               (LPTHREAD_START_ROUTINE)ThreadRoutine, 
										       Buffer, &ThreadHandle, NULL);

	} else {
		ThreadHandle = CreateRemoteThread(ProcessHandle, NULL, 0, 
			                              (LPTHREAD_START_ROUTINE)ThreadRoutine, 
			                              Buffer, CREATE_SUSPENDED, NULL);
	}

	if (!ThreadHandle) {
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		CloseHandle(ProcessHandle);
		return Status;				
	}

	ResumeThread(ThreadHandle);
	WaitForSingleObject(ThreadHandle, INFINITE);
	CloseHandle(ThreadHandle);

	//
	// Check whether it's really loaded into target address space
	//

	Status = BspGetModuleInformation(ProcessId, PspRuntimeFullPath, TRUE, 
		                             NULL, NULL, NULL);

	if (!Status) {
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		CloseHandle(ProcessHandle);
		return S_FALSE;
	}

	//
	// Execute BtrFltStartRuntime to start IPC thread 
	//

	ThreadRoutine = BspGetRemoteApiAddress(ProcessId, L"dprobe.btr.dll", "BtrFltStartRuntime");
	if (!ThreadRoutine) {
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		CloseHandle(ProcessHandle);
		return S_FALSE;
	}

	Status = WriteProcessMemory(ProcessHandle, Buffer, &PspRuntimeFeatures, 
		                        sizeof(PspRuntimeFeatures), NULL); 

	if (!Status) {
		Status = GetLastError();
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		CloseHandle(ProcessHandle);
		return Status;
	}	

	ThreadHandle = 0;
	if (BspMajorVersion >= 6) {
		Status = (ULONG)(*RtlCreateUserThread)(ProcessHandle, NULL, TRUE, 0, 0, 0,
								               (LPTHREAD_START_ROUTINE)ThreadRoutine, 
										       Buffer, &ThreadHandle, NULL);

	} else {
		ThreadHandle = CreateRemoteThread(ProcessHandle, NULL, 0, 
			                              (LPTHREAD_START_ROUTINE)ThreadRoutine, 
			                              Buffer, CREATE_SUSPENDED, NULL);
	}

	if (!ThreadHandle) {
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		CloseHandle(ProcessHandle);
		return Status;				
	}

	ResumeThread(ThreadHandle);
	WaitForSingleObject(ThreadHandle, INFINITE);

	//
	// N.B. If BtrFltStartRuntime fails, it will auto clean up and free itself
	//

	VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
	CloseHandle(ProcessHandle);
	GetExitCodeThread(ThreadHandle, &Status);
	CloseHandle(ThreadHandle);
	return Status;
}

ULONG
PspStartProbeRuntime(
	IN ULONG ProcessId
	)
{
	ULONG Status;
	HANDLE ProcessHandle;
	HANDLE ThreadHandle;
	PVOID Buffer;
	PVOID ThreadRoutine;

	//
	// Ensure the runtime dll is already loaded
	//

	if (!PspIsRuntimeLoaded(ProcessId)) {
		return S_FALSE;
	}

	ProcessHandle = OpenProcess(PSP_OPEN_PROCESS_FLAG, FALSE, ProcessId);
	if (!ProcessHandle) {
		return GetLastError();
	}
	
	Buffer = (PWCHAR)VirtualAllocEx(ProcessHandle, NULL, BspPageSize, MEM_COMMIT, PAGE_READWRITE);
	if (!Buffer) { 
		Status = GetLastError();
		CloseHandle(ProcessHandle);
		return Status;
	}

	//
	// Execute BtrFltStartRuntime to start IPC thread 
	//

	ThreadRoutine = BspGetRemoteApiAddress(ProcessId, L"dprobe.btr.dll", "BtrFltStartRuntime");
	if (!ThreadRoutine) {
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		CloseHandle(ProcessHandle);
		return S_FALSE;
	}

	Status = WriteProcessMemory(ProcessHandle, Buffer, &PspRuntimeFeatures, 
		                        sizeof(PspRuntimeFeatures), NULL); 

	if (!Status) {
		Status = GetLastError();
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		CloseHandle(ProcessHandle);
		return Status;
	}	

	ThreadHandle = 0;
	if (BspMajorVersion >= 6) {
		Status = (ULONG)(*RtlCreateUserThread)(ProcessHandle, NULL, TRUE, 0, 0, 0,
								               (LPTHREAD_START_ROUTINE)ThreadRoutine, 
										       Buffer, &ThreadHandle, NULL);

	} else {
		ThreadHandle = CreateRemoteThread(ProcessHandle, NULL, 0, 
			                              (LPTHREAD_START_ROUTINE)ThreadRoutine, 
			                              Buffer, CREATE_SUSPENDED, NULL);
	}

	if (!ThreadHandle) {
		VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
		CloseHandle(ProcessHandle);
		return Status;				
	}

	ResumeThread(ThreadHandle);
	WaitForSingleObject(ThreadHandle, INFINITE);

	//
	// N.B. If BtrFltStartRuntime fails, it will auto clean up and free itself
	//

	VirtualFreeEx(ProcessHandle, Buffer, 0, MEM_RELEASE);
	CloseHandle(ProcessHandle);
	GetExitCodeThread(ThreadHandle, &Status);
	CloseHandle(ThreadHandle);
	return Status;
}