//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "mmflt.h"
#include "resource.h"

BOOLEAN MmInitializedOk = FALSE;
HMODULE MmModuleHandle = NULL;

//
// Memory Management Filter GUID
//

GUID MmGuid = { 0xcab384a0, 0x7c44, 0x4b14, { 0xa8, 0x94, 0x2, 0x73, 0x83, 0x95, 0x25, 0xb9 } };

BTR_FILTER_PROBE MmProbes[MM_PROBE_NUMBER] = {
	{ _HeapAlloc, 0, ReturnPtr, NULL, HeapAllocEnter, HeapAllocDecode, L"kernel32.dll", L"HeapAlloc" },
	{ _HeapFree,  0, ReturnInt32, NULL, HeapFreeEnter, HeapFreeDecode, L"kernel32.dll", L"HeapFree" },
	{ _HeapReAlloc,  0, ReturnPtr, NULL, HeapReAllocEnter, HeapReAllocDecode, L"kernel32.dll", L"HeapReAlloc" },
	{ _HeapCreate, 0, ReturnPtr, NULL, HeapCreateEnter, HeapCreateDecode, L"kernel32.dll", L"HeapCreate" },
	{ _HeapDestroy, 0, ReturnInt32, NULL, HeapDestroyEnter, HeapDestroyDecode, L"kernel32.dll", L"HeapDestroy" },
	{ _VirtualAllocEx, 0, ReturnPtr, NULL, VirtualAllocExEnter, VirtualAllocExDecode, L"kernel32.dll", L"VirtualAllocEx" },
	{ _VirtualFreeEx, 0, ReturnInt32, NULL, VirtualFreeExEnter, VirtualFreeExDecode, L"kernel32.dll", L"VirtualFreeEx" },
	{ _VirtualProtectEx, 0, ReturnInt32, NULL, VirtualProtectExEnter, VirtualProtectExDecode, L"kernel32.dll", L"VirtualProtectEx" },
	{ _VirtualQueryEx, 0, ReturnUIntPtr, NULL, VirtualQueryExEnter, VirtualQueryExDecode, L"kernel32.dll", L"VirtualQueryEx" },
	{ _CreateFileMapping, 0, ReturnPtr, NULL, CreateFileMappingEnter, CreateFileMappingDecode, L"kernel32.dll", L"CreateFileMappingW" },
	{ _MapViewOfFile, 0, ReturnPtr, NULL, MapViewOfFileEnter, MapViewOfFileDecode, L"kernel32.dll", L"MapViewOfFile" },
	{ _UnmapViewOfFile, 0, ReturnInt32, NULL, UnmapViewOfFileEnter, UnmapViewOfFileDecode, L"kernel32.dll", L"UnmapViewOfFile" },
	{ _OpenFileMapping, 0, ReturnPtr, NULL, OpenFileMappingEnter, OpenFileMappingDecode, L"kernel32.dll", L"OpenFileMappingW" },
	{ _FlushViewOfFile, 0, ReturnInt32, NULL, FlushViewOfFileEnter, FlushViewOfFileDecode, L"kernel32.dll", L"FlushViewOfFile" },
};

BTR_FILTER MmRegistration;

BOOLEAN
MmInitialize(
	IN BTR_FILTER_MODE Mode
	);

PBTR_FILTER WINAPI
BtrFltGetObject(
	IN BTR_FILTER_MODE Mode	
	)
{
	if (MmInitializedOk) {
		return &MmRegistration;
	}

	MmInitializedOk = MmInitialize(Mode);
	if (MmInitializedOk) {
		return &MmRegistration;
	}

	return NULL;
}

//
// This routine is called in context of runtime worker thread
// however it's not guaranteed to be in same thread
//

ULONG WINAPI
MmUnregisterCallback(
	VOID
	)
{
	//
	// N.B. When filter is unregistered, this routine is called
	// by runtime to make filter have a chance to release allocated
	// resources. Typically filter should provide this routine.
	//

	OutputDebugStringA("MmUnregisterCallback");
	return 0;
}

//
// This routine is called in context of runtime worker thread
// however it's not guaranteed to be in same thread
//

ULONG WINAPI
MmControlCallback(
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
	// it, runtime will free the buffer later. Note the the BTR_FILTER_CONTROL
	// structure must be allocated with BtrFltMalloc.
	//

	Packet = (PBTR_FILTER_CONTROL)BtrFltMalloc(sizeof(BTR_FILTER_CONTROL));
	Packet->Length = sizeof(BTR_FILTER_CONTROL);
	Packet->Control[0] = 0;

	*Ack = Packet;

	OutputDebugStringA("MmControlCallback");
	return 0;
}

BOOLEAN
MmInitialize(
	IN BTR_FILTER_MODE Mode
	)
{
	PVOID Address;
	HMODULE DllHandle;
	PCHAR Buffer;
	int i;

	StringCchCopy(MmRegistration.FilterName,  MAX_PATH - 1, L"Memory Management Filter");
	StringCchCopy(MmRegistration.Author,  MAX_PATH - 1, L"lan.john@gmail.com");
	StringCchCopy(MmRegistration.Description, MAX_PATH - 1, L"Win32 Memory Management API Filter");

	MmRegistration.FilterTag = 0;
	MmRegistration.MajorVersion = 1;
	MmRegistration.MinorVersion = 0;

	GetModuleFileName(MmModuleHandle, MmRegistration.FilterFullPath, MAX_PATH);

	MmRegistration.Probes = MmProbes;
	MmRegistration.ProbesCount = MM_PROBE_NUMBER;
	
	MmRegistration.ControlCallback = MmControlCallback;
	MmRegistration.UnregisterCallback = MmUnregisterCallback;
	MmRegistration.FilterGuid = MmGuid;

	if (Mode == FILTER_MODE_DECODE) {
		return TRUE;
	}

	//
	// Fill target routine address
	//

	for(i = 0; i < MM_PROBE_NUMBER; i++) {

		DllHandle = LoadLibrary(MmProbes[i].DllName);
		if (DllHandle) {

			BtrFltConvertUnicodeToAnsi(MmProbes[i].ApiName, &Buffer);
			Address = (PVOID)GetProcAddress(DllHandle, Buffer);
			BtrFltFree(Buffer);

			if (Address) {
				MmProbes[i].Address = Address;
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
		MmModuleHandle = ModuleHandle;
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
