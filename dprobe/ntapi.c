//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "ntapi.h"
#include "bsp.h"

NT_QUERY_SYSTEM_INFORMATION  NtQuerySystemInformation;
NT_QUERY_INFORMATION_PROCESS NtQueryInformationProcess;
NT_QUERY_INFORMATION_THREAD  NtQueryInformationThread;

NT_SUSPEND_PROCESS NtSuspendProcess;
NT_RESUME_PROCESS  NtResumeProcess;

RTL_REMOTE_CALL RtlRemoteCall;
RTL_CREATE_USER_THREAD RtlCreateUserThread;

BOOLEAN
BspGetSystemRoutine(
	VOID
	)
{
	HMODULE DllHandle;

	//
	// Get system routine address
	//

	DllHandle = LoadLibrary(L"ntdll.dll");
	if (!DllHandle) {
		return FALSE;
	}

	NtQuerySystemInformation = (NT_QUERY_SYSTEM_INFORMATION)GetProcAddress(DllHandle, "NtQuerySystemInformation");
	if (!NtQuerySystemInformation) {
		return FALSE;
	}

	NtQueryInformationProcess = (NT_QUERY_INFORMATION_PROCESS)GetProcAddress(DllHandle, "NtQueryInformationProcess");
	if (!NtQueryInformationProcess) {
		return FALSE;
	}

	NtQueryInformationThread = (NT_QUERY_INFORMATION_THREAD)GetProcAddress(DllHandle, "NtQueryInformationThread");
	if (!NtQueryInformationThread) {
		return FALSE;
	}

	NtSuspendProcess = (NT_SUSPEND_PROCESS)GetProcAddress(DllHandle, "NtSuspendProcess");
	if (!NtSuspendProcess) {
		return FALSE;
	}

	NtResumeProcess = (NT_RESUME_PROCESS)GetProcAddress(DllHandle, "NtResumeProcess");
	if (!NtResumeProcess) {
		return FALSE;
	}

	RtlRemoteCall = (RTL_REMOTE_CALL)GetProcAddress(DllHandle, "RtlRemoteCall");
	if (!RtlRemoteCall) {
		return FALSE;
	}
	
	if (BspIsWindowsXPAbove()) {
		RtlCreateUserThread = (RTL_CREATE_USER_THREAD)GetProcAddress(DllHandle, "RtlCreateUserThread");
		if (!RtlCreateUserThread) {
			return FALSE;
		}
	}
	
	return S_OK;
}