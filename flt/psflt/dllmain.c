//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "psflt.h"
#include "pshelpers.h"

BOOLEAN PsInitializedOk = FALSE;
HMODULE PsModuleHandle = NULL;

//
// {0992C95C-8B41-4d6a-AEAC-828F566DF731}
//

UUID PsUuid = { 0x992c95c, 0x8b41, 0x4d6a, { 0xae, 0xac, 0x82, 0x8f, 0x56, 0x6d, 0xf7, 0x31 } };

BTR_FILTER_PROBE PsProbes[] = {

	//
	// process
	//

	{ _CreateProcess, 0, ReturnInt32, NULL, CreateProcessWEnter, PsProcessDecode, L"kernel32.dll", L"CreateProcessW" },
	{ _OpenProcess, 0, ReturnPtr, NULL, OpenProcessEnter, PsProcessDecode, L"kernel32.dll", L"OpenProcess" },
	{ _ExitProcess, 0, ReturnVoid, NULL, ExitProcessEnter, PsProcessDecode, L"kernel32.dll", L"ExitProcess" },
	{ _TerminateProcess, 0, ReturnInt32, NULL, TerminateProcessEnter, PsProcessDecode, L"kernel32.dll", L"TerminateProcess" },

	//
	// thread
	//

	{ _CreateThread, 0, ReturnPtr, NULL, CreateThreadEnter, PsThreadDecode, L"kernel32.dll", L"CreateThread" },
	{ _CreateRemoteThread, 0, ReturnPtr, NULL, CreateRemoteThreadEnter, PsThreadDecode, L"kernel32.dll", L"CreateRemoteThread" },
	{ _OpenThread, 0, ReturnPtr, NULL, OpenThreadEnter, PsThreadDecode, L"kernel32.dll", L"OpenThread" },
	{ _ExitThread, 0, ReturnVoid, NULL, ExitThreadEnter, PsThreadDecode, L"kernel32.dll", L"ExitThread" },
	{ _TerminateThread, 0, ReturnInt32, NULL, TerminateThreadEnter, PsThreadDecode, L"kernel32.dll", L"TerminateThread" },
	{ _SuspendThread, 0, ReturnUInt32, NULL, SuspendThreadEnter, PsThreadDecode, L"kernel32.dll", L"SuspendThread" },
	{ _ResumeThread, 0, ReturnUInt32, NULL, ResumeThreadEnter, PsThreadDecode, L"kernel32.dll", L"ResumeThread" },
	{ _GetThreadContext, 0, ReturnUInt32, NULL, GetThreadContextEnter, PsThreadDecode, L"kernel32.dll", L"GetThreadContext" },
	{ _SetThreadContext, 0, ReturnUInt32, NULL, SetThreadContextEnter, PsThreadDecode, L"kernel32.dll", L"SetThreadContext" },
	{ _TlsAlloc, 0, ReturnUInt32, NULL, TlsAllocEnter, PsThreadDecode, L"kernel32.dll", L"TlsAlloc" },
	{ _TlsFree, 0, ReturnUInt32, NULL, TlsFreeEnter, PsThreadDecode, L"kernel32.dll", L"TlsFree" },
	{ _TlsSetValue, 0, ReturnUInt32, NULL, TlsSetValueEnter, PsThreadDecode, L"kernel32.dll", L"TlsSetValue" },
	{ _TlsGetValue, 0, ReturnPtr, NULL, TlsGetValueEnter, PsThreadDecode, L"kernel32.dll", L"TlsGetValue" },
	{ _QueueUserWorkItem, 0, ReturnUInt32, NULL, QueueUserWorkItemEnter, PsThreadDecode, L"kernel32.dll", L"QueueUserWorkItem" },
	{ _QueueUserAPC, 0, ReturnUInt32, NULL, QueueUserAPCEnter, PsThreadDecode, L"kernel32.dll", L"QueueUserAPC" },

	//
	// ldr
	//

	{ _LoadLibraryEx, 0, ReturnPtr, NULL, LoadLibraryExEnter, LoadLibraryExDecode, L"kernel32.dll", L"LoadLibraryExW" },
	{ _GetProcAddress, 0, ReturnPtr, NULL, GetProcAddressEnter, GetProcAddressDecode, L"kernel32.dll", L"GetProcAddress" },
	{ _FreeLibrary, 0, ReturnInt32, NULL, FreeLibraryEnter, FreeLibraryDecode, L"kernel32.dll", L"FreeLibrary" },
	{ _GetModuleFileName, 0, ReturnUInt32, NULL, GetModuleFileNameEnter, GetModuleFileNameDecode, L"kernel32.dll", L"GetModuleFileNameW" },
	{ _GetModuleHandle, 0, ReturnPtr, NULL, GetModuleHandleEnter, GetModuleHandleDecode, L"kernel32.dll", L"GetModuleHandleW" },
	{ _GetModuleHandleExA, 0, ReturnInt32, NULL, GetModuleHandleExAEnter, GetModuleHandleExADecode, L"kernel32.dll", L"GetModuleHandleExA" },
	{ _GetModuleHandleExW, 0, ReturnInt32, NULL, GetModuleHandleExWEnter, GetModuleHandleExWDecode, L"kernel32.dll", L"GetModuleHandleExW" },
};

BTR_FILTER PsRegistration;

//
// This routine is called in context of runtime worker thread
// however it's not guaranteed to be in same thread
//

ULONG WINAPI
PsUnregisterCallback(
	VOID
	)
{
	//
	// N.B. When filter is unregistered, this routine is called
	// by runtime to make filter have a chance to release allocated
	// resources. Typically filter should provide this routine.
	//

	OutputDebugStringA("PsUnregisterCallback");
	return 0;
}

//
// This routine is called in context of runtime worker thread
// however it's not guaranteed to be in same thread
//

ULONG WINAPI
PsControlCallback(
	IN PBTR_FILTER_CONTROL Control,
	OUT PBTR_FILTER_CONTROL *Ack
	)
{
	PBTR_FILTER_CONTROL Packet;

	//
	// Control callback is called when user send a message
	// to current filter via MspSendFilterControl(), filter
	// should send a ack to this control back to user.
	//

	//
	// Allocate and build a dummy ack packet back to user, the packet
	// don't belong to filter anymore once callback return, never touch
	// it, runtime will free the buffer later.
	//

	Packet = (PBTR_FILTER_CONTROL)BtrFltMalloc(sizeof(BTR_FILTER_CONTROL));
	Packet->Length = sizeof(BTR_FILTER_CONTROL);
	Packet->Control[0] = 0;

	*Ack = Packet;

	OutputDebugStringA("PsControlCallback");
	return 0;
}

BOOLEAN
PsInitialize(
	IN BTR_FILTER_MODE Mode
	)
{
	PVOID Address;
	HMODULE DllHandle;
	PCHAR Buffer;
	int i;
	ULONG Status;

	Status = PsInitializeHelpers();
	if (Status != 0) {
		return FALSE;
	}

	StringCchCopy(PsRegistration.FilterName,  MAX_PATH - 1, L"Process Thread Filter");
	StringCchCopy(PsRegistration.Author, MAX_PATH - 1, L"lan.john@gmail.com");
	StringCchCopy(PsRegistration.Description, MAX_PATH - 1, L"Win32 Process Thread Filter");

	PsRegistration.FilterTag = 0;
	PsRegistration.MajorVersion = 1;
	PsRegistration.MinorVersion = 0;

	GetModuleFileName(PsModuleHandle, PsRegistration.FilterFullPath, MAX_PATH);

	PsRegistration.Probes = PsProbes;
	PsRegistration.ProbesCount = PS_PROBE_NUMBER;
	
	PsRegistration.ControlCallback = PsControlCallback;
	PsRegistration.UnregisterCallback = PsUnregisterCallback;
	PsRegistration.FilterGuid = PsUuid;

	if (Mode == FILTER_MODE_DECODE) {
		return TRUE;
	}

	//
	// Fill target routine address
	//

	for(i = 0; i < PS_PROBE_NUMBER; i++) {

		DllHandle = LoadLibrary(PsProbes[i].DllName);
		if (DllHandle) {

			BtrFltConvertUnicodeToAnsi(PsProbes[i].ApiName, &Buffer);
			Address = (PVOID)GetProcAddress(DllHandle, Buffer);
			BtrFltFree(Buffer);

			if (Address) {
				PsProbes[i].Address = Address;
			} else {
				continue;
			} 

		} else {
			return FALSE;
		}
	}

	return TRUE;
}

PBTR_FILTER WINAPI
BtrFltGetObject(
	IN BTR_FILTER_MODE Mode	
	)
{
	if (PsInitializedOk) {
		return &PsRegistration;
	}

	PsInitializedOk = PsInitialize(Mode);
	if (PsInitializedOk) {
		return &PsRegistration;
	}

	return NULL;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		PsModuleHandle = hModule;
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}