//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#include "ntflt.h"

BOOLEAN NtInitializedOk = FALSE;
HMODULE NtModuleHandle = NULL;

//
// {CDF63043-2C9D-475a-A1D2-F0C70A76E91B}
//

GUID NtUuid = 
{ 0xcdf63043, 0x2c9d, 0x475a, { 0xa1, 0xd2, 0xf0, 0xc7, 0xa, 0x76, 0xe9, 0x1b } };


BTR_FILTER_PROBE NtProbes[] = {
	{ 0, 0, ReturnVoid, NULL, NULL, NULL, L"kernel32.dll", L"OutputDebugStringW" },
};

BTR_FILTER NtRegistration;

//
// This routine is called in context of runtime worker thread
// however it's not guaranteed to be in same thread
//

ULONG WINAPI
NtUnregisterCallback(
	VOID
	)
{
	//
	// N.B. When filter is unregistered, this routine is called
	// by runtime to make filter have a chance to release allocated
	// resources. Typically filter should provide this routine.
	//

	return 0;
}

//
// This routine is called in context of runtime worker thread
// however it's not guaranteed to be in same thread
//

ULONG WINAPI
NtControlCallback(
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

	return 0;
}

BOOLEAN
NtInitialize(
	IN BTR_FILTER_MODE Mode
	)
{
	PVOID Address;
	HMODULE DllHandle;
	PCHAR Buffer;
	int i;

	StringCchCopy(NtRegistration.FilterName,  MAX_PATH - 1, L"NT API Filter");
	StringCchCopy(NtRegistration.Author, MAX_PATH - 1, L"lan.john@gmail.com");
	StringCchCopy(NtRegistration.Description, MAX_PATH - 1, L"NT API Filter");

	NtRegistration.FilterTag = 0;
	NtRegistration.MajorVersion = 1;
	NtRegistration.MinorVersion = 0;

	GetModuleFileName(NtModuleHandle, NtRegistration.FilterFullPath, MAX_PATH);

	NtRegistration.Probes = NtProbes;
	NtRegistration.ProbesCount = NT_PROBE_NUMBER;
	
	NtRegistration.ControlCallback = NtControlCallback;
	NtRegistration.UnregisterCallback = NtUnregisterCallback;
	NtRegistration.FilterGuid = NtUuid;

	if (Mode == FILTER_MODE_DECODE) {
		return TRUE;
	}

	//
	// Fill target routine address
	//

	DllHandle = GetModuleHandle(L"ntdll.dll");
	ASSERT(DllHandle != NULL);

	for(i = 0; i < NT_PROBE_NUMBER; i++) {

		BtrFltConvertUnicodeToAnsi(NtProbes[i].ApiName, &Buffer);
		Address = (PVOID)GetProcAddress(DllHandle, Buffer);
		BtrFltFree(Buffer);

		if (Address) {
			NtProbes[i].Address = Address;
		} else {
			continue;
		} 
	}

	return TRUE;
}

PBTR_FILTER WINAPI
BtrFltGetObject(
	IN BTR_FILTER_MODE Mode	
	)
{
	if (NtInitializedOk) {
		return &NtRegistration;
	}

	NtInitializedOk = NtInitialize(Mode);
	if (NtInitializedOk) {
		return &NtRegistration;
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
		NtModuleHandle = hModule;
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}