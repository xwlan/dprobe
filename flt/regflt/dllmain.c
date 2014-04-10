//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "regflt.h"
#include <winreg.h>
#include "resource.h"

BOOLEAN RegInitializedOk = FALSE;
HMODULE RegModuleHandle = NULL;

GUID RegGuid = { 0x1037f53a, 0x4c41, 0x441a, { 0xb0, 0xc1, 0xb5, 0xe3, 0x32, 0x76, 0xb8, 0xd9 } };

BTR_FILTER_PROBE RegProbes[REG_PROBE_NUMBER] = {
	{ _RegCreateKeyA, 0, ReturnInt32, NULL, RegCreateKeyAEnter, RegCreateKeyADecode, L"advapi32.dll", L"RegCreateKeyA" },
	{ _RegCreateKeyW, 0, ReturnInt32, NULL, RegCreateKeyWEnter, RegCreateKeyWDecode, L"advapi32.dll", L"RegCreateKeyW" },
	{ _RegCreateKeyExA, 0, ReturnInt32, NULL, RegCreateKeyExAEnter, RegCreateKeyExADecode, L"advapi32.dll", L"RegCreateKeyExA" },
	{ _RegCreateKeyExW, 0, ReturnInt32, NULL, RegCreateKeyExWEnter, RegCreateKeyExWDecode, L"advapi32.dll", L"RegCreateKeyExW" },
	{ _RegDeleteKeyA, 0, ReturnInt32, NULL, RegDeleteKeyAEnter, RegDeleteKeyADecode, L"advapi32.dll", L"RegDeleteKeyA" },
	{ _RegDeleteKeyW, 0, ReturnInt32, NULL, RegDeleteKeyWEnter, RegDeleteKeyWDecode, L"advapi32.dll", L"RegDeleteKeyW" },
	{ _RegDeleteKeyExA, 0, ReturnInt32, NULL, RegDeleteKeyExAEnter, RegDeleteKeyExADecode, L"advapi32.dll", L"RegDeleteKeyExA" },
	{ _RegDeleteKeyExW, 0, ReturnInt32, NULL, RegDeleteKeyExWEnter, RegDeleteKeyExWDecode, L"advapi32.dll", L"RegDeleteKeyExW" },
	{ _RegDeleteKeyValueA, 0, ReturnInt32, NULL, RegDeleteKeyValueAEnter, RegDeleteKeyValueADecode, L"advapi32.dll", L"RegDeleteKeyValueA" },
	{ _RegDeleteKeyValueW, 0, ReturnInt32, NULL, RegDeleteKeyValueWEnter, RegDeleteKeyValueWDecode, L"advapi32.dll", L"RegDeleteKeyValueW" },
	{ _RegDeleteValueA, 0, ReturnInt32, NULL, RegDeleteValueAEnter, RegDeleteValueADecode, L"advapi32.dll", L"RegDeleteValueA" },
	{ _RegDeleteValueW, 0, ReturnInt32, NULL, RegDeleteValueWEnter, RegDeleteValueWDecode, L"advapi32.dll", L"RegDeleteValueW" },
	{ _RegDeleteTreeW, 0, ReturnInt32, NULL, RegDeleteTreeEnter, RegDeleteTreeDecode, L"advapi32.dll", L"RegDeleteTreeW" },
	{ _RegEnumKeyExA, 0, ReturnInt32, NULL, RegEnumKeyExAEnter, RegEnumKeyExADecode, L"advapi32.dll", L"RegEnumKeyExA" },
	{ _RegEnumKeyExW, 0, ReturnInt32, NULL, RegEnumKeyExWEnter, RegEnumKeyExWDecode, L"advapi32.dll", L"RegEnumKeyExW" },
	{ _RegSetValueExA, 0, ReturnInt32, NULL, RegSetValueExAEnter, RegSetValueExADecode, L"advapi32.dll", L"RegSetValueExA" },
	{ _RegSetValueExW, 0, ReturnInt32, NULL, RegSetValueExWEnter, RegSetValueExWDecode, L"advapi32.dll", L"RegSetValueExW" },
	{ _RegQueryValueExA, 0, ReturnInt32, NULL, RegQueryValueExAEnter, RegQueryValueExADecode, L"advapi32.dll", L"RegQueryValueExA" },
	{ _RegQueryValueExW, 0, ReturnInt32, NULL, RegQueryValueExWEnter, RegQueryValueExWDecode, L"advapi32.dll", L"RegQueryValueExW" },
	{ _RegOpenKeyExA, 0, ReturnInt32, NULL, RegOpenKeyExAEnter, RegOpenKeyExADecode, L"advapi32.dll", L"RegOpenKeyExA" },
	{ _RegOpenKeyExW, 0, ReturnInt32, NULL, RegOpenKeyExWEnter, RegOpenKeyExWDecode, L"advapi32.dll", L"RegOpenKeyExW" },
	{ _RegQueryInfoKeyA, 0, ReturnInt32, NULL, RegQueryInfoKeyAEnter, RegQueryInfoKeyADecode, L"advapi32.dll", L"RegQueryInfoKeyA" },
	{ _RegQueryInfoKeyW, 0, ReturnInt32, NULL, RegQueryInfoKeyWEnter, RegQueryInfoKeyWDecode, L"advapi32.dll", L"RegQueryInfoKeyW" },
};

BTR_FILTER RegRegistration;

BOOLEAN
RegInitialize(
	IN BTR_FILTER_MODE Mode	
	);

PBTR_FILTER WINAPI
BtrFltGetObject(
	IN BTR_FILTER_MODE Mode	
	)
{
	if (RegInitializedOk) {
		return &RegRegistration;
	}

	RegInitializedOk = RegInitialize(Mode);
	if (RegInitializedOk) {
		return &RegRegistration;
	}

	return NULL;
}

//
// This routine is called in context of runtime worker thread
// however it's not guaranteed to be in same thread
//

ULONG WINAPI
RegUnregisterCallback(
	VOID
	)
{
	//
	// N.B. When filter is unregistered, this routine is called
	// by runtime to make filter have a chance to release allocated
	// resources. Typically filter should provide this routine.
	//

	OutputDebugStringA("RegUnregisterCallback");
	return 0;
}

//
// This routine is called in context of runtime worker thread
// however it's not guaranteed to be in same thread
//

ULONG WINAPI
RegControlCallback(
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

	OutputDebugStringA("RegControlCallback");
	return 0;
}

BOOLEAN
RegInitialize(
	IN BTR_FILTER_MODE Mode 
	)
{
	PVOID Address;
	HMODULE DllHandle;
	PCHAR Buffer;
	int i;

	StringCchCopy(RegRegistration.FilterName,  MAX_PATH - 1, L"Registry Filter");
	StringCchCopy(RegRegistration.Author, MAX_PATH - 1, L"lan.john@gmail.com");
	StringCchCopy(RegRegistration.Description, MAX_PATH - 1, L"Win32 Registry API Filter");

	RegRegistration.FilterTag = 0;
	RegRegistration.MajorVersion = 1;
	RegRegistration.MinorVersion = 0;

	GetModuleFileName(RegModuleHandle, RegRegistration.FilterFullPath, MAX_PATH);

	RegRegistration.Probes = RegProbes;
	RegRegistration.ProbesCount = REG_PROBE_NUMBER;
	
	RegRegistration.ControlCallback = RegControlCallback;
	RegRegistration.UnregisterCallback = RegUnregisterCallback;
	RegRegistration.FilterGuid = RegGuid;

	if (Mode == FILTER_MODE_DECODE) {
		return TRUE;
	}

	//
	// Fill target routine address
	//

	for(i = 0; i < REG_PROBE_NUMBER; i++) {

		DllHandle = LoadLibrary(RegProbes[i].DllName);
		if (DllHandle) {

			BtrFltConvertUnicodeToAnsi(RegProbes[i].ApiName, &Buffer);
			Address = (PVOID)GetProcAddress(DllHandle, Buffer);
			BtrFltFree(Buffer);

			if (Address) {
				RegProbes[i].Address = Address;
			} else {
				continue;
			} 

		} else {
			return FALSE;
		}
	}

	return TRUE;
}

BOOL WINAPI 
DllMain(
	IN HMODULE ModuleHandle,
    IN ULONG Reason,
	IN PVOID Reserved
	)
{
	switch (Reason) {

	case DLL_PROCESS_ATTACH:
		RegModuleHandle = ModuleHandle;
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		break;
	}
	
	return TRUE;
}
