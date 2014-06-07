//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#include "dbgflt.h"

BOOLEAN DbgInitializedOk = FALSE;
HMODULE DbgModuleHandle = NULL;

//
// {D29C2340-11F9-4acc-90B8-B3F009982E9A}
//

GUID DbgUuid = 
{ 0xd29c2340, 0x11f9, 0x4acc, { 0x90, 0xb8, 0xb3, 0xf0, 0x9, 0x98, 0x2e, 0x9a } };

BTR_FILTER_PROBE DbgProbes[] = {

	{ _OutputDebugStringW, 0, ReturnVoid, NULL, OutputDebugStringWEntry, DbgStringWDecode, L"kernel32.dll", L"OutputDebugStringW" },
	{ _OutputDebugStringA, 0, ReturnVoid, NULL, OutputDebugStringAEntry, DbgStringADecode, L"kernel32.dll", L"OutputDebugStringA" },

	{ _DbgPrint, 0, ReturnUInt32, NULL, DbgPrintEntry, DbgStringADecode, L"ntdll.dll", L"DbgPrint" },
	{ _DbgPrintEx, 0, ReturnUInt32, NULL, DbgPrintExEntry, DbgStringADecode, L"ntdll.dll", L"DbgPrintEx" },

	{ _fprintf, 0, ReturnInt32, NULL, fprintfEntry, DbgStringADecode, L"msvcrt.dll", L"fprintf" },
	{ _fwprintf, 0, ReturnInt32, NULL, fwprintfEntry, DbgStringWDecode, L"msvcrt.dll", L"fwprintf" },
	{ _vfprintf, 0, ReturnInt32, NULL, vfprintfEntry, DbgStringADecode, L"msvcrt.dll", L"vfprintf" },
	{ _vfwprintf, 0, ReturnInt32, NULL, vfwprintfEntry, DbgStringWDecode, L"msvcrt.dll", L"vfwprintf" },

	//
	// vc9
	//

	{ _fprintf9, 0, ReturnInt32, NULL, fprintfEntry, DbgStringADecode, L"msvcr90.dll", L"fprintf" },
	{ _fwprintf9, 0, ReturnInt32, NULL, fwprintfEntry, DbgStringWDecode, L"msvcr90.dll", L"fwprintf" },
	{ _vfprintf9, 0, ReturnInt32, NULL, vfprintfEntry, DbgStringADecode, L"msvcr90.dll", L"vfprintf" },
	{ _vfwprintf9, 0, ReturnInt32, NULL, vfwprintfEntry, DbgStringWDecode, L"msvcr90.dll", L"vfwprintf" },
	{ _fprintf9d, 0, ReturnInt32, NULL, fprintfEntry, DbgStringADecode, L"msvcr90d.dll", L"fprintf" },
	{ _fwprintf9d, 0, ReturnInt32, NULL, fwprintfEntry, DbgStringWDecode, L"msvcr90d.dll", L"fwprintf" },
	{ _vfprintf9d, 0, ReturnInt32, NULL, vfprintfEntry, DbgStringADecode, L"msvcr90d.dll", L"vfprintf" },
	{ _vfwprintf9d, 0, ReturnInt32, NULL, vfwprintfEntry, DbgStringWDecode, L"msvcr90d.dll", L"vfwprintf" },
	
	//
	// vc10
	//

	{ _fprintf10, 0, ReturnInt32, NULL, fprintfEntry, DbgStringADecode, L"msvcr100.dll", L"fprintf" },
	{ _fwprintf10, 0, ReturnInt32, NULL, fwprintfEntry, DbgStringWDecode, L"msvcr100.dll", L"fwprintf" },
	{ _vfprintf10, 0, ReturnInt32, NULL, vfprintfEntry, DbgStringADecode, L"msvcr100.dll", L"vfprintf" },
	{ _vfwprintf10, 0, ReturnInt32, NULL, vfwprintfEntry, DbgStringWDecode, L"msvcr100.dll", L"vfwprintf" },
	{ _fprintf10d, 0, ReturnInt32, NULL, fprintfEntry, DbgStringADecode, L"msvcr100d.dll", L"fprintf" },
	{ _fwprintf10d, 0, ReturnInt32, NULL, fwprintfEntry, DbgStringWDecode, L"msvcr100d.dll", L"fwprintf" },
	{ _vfprintf10d, 0, ReturnInt32, NULL, vfprintfEntry, DbgStringADecode, L"msvcr100d.dll", L"vfprintf" },
	{ _vfwprintf10d, 0, ReturnInt32, NULL, vfwprintfEntry, DbgStringWDecode, L"msvcr100d.dll", L"vfwprintf" },
	
	//
	// vc11
	//

	{ _fprintf11, 0, ReturnInt32, NULL, fprintfEntry, DbgStringADecode, L"msvcr110.dll", L"fprintf" },
	{ _fwprintf11, 0, ReturnInt32, NULL, fwprintfEntry, DbgStringWDecode, L"msvcr110.dll", L"fwprintf" },
	{ _vfprintf11, 0, ReturnInt32, NULL, vfprintfEntry, DbgStringADecode, L"msvcr110.dll", L"vfprintf" },
	{ _vfwprintf11, 0, ReturnInt32, NULL, vfwprintfEntry, DbgStringWDecode, L"msvcr110.dll", L"vfwprintf" },
	{ _fprintf11d, 0, ReturnInt32, NULL, fprintfEntry, DbgStringADecode, L"msvcr110d.dll", L"fprintf" },
	{ _fwprintf11d, 0, ReturnInt32, NULL, fwprintfEntry, DbgStringWDecode, L"msvcr110d.dll", L"fwprintf" },
	{ _vfprintf11d, 0, ReturnInt32, NULL, vfprintfEntry, DbgStringADecode, L"msvcr110d.dll", L"vfprintf" },
	{ _vfwprintf11d, 0, ReturnInt32, NULL, vfwprintfEntry, DbgStringWDecode, L"msvcr110d.dll", L"vfwprintf" },
	
	//
	// vc12
	//

	{ _fprintf12, 0, ReturnInt32, NULL, fprintfEntry, DbgStringADecode, L"msvcr120.dll", L"fprintf" },
	{ _fwprintf12, 0, ReturnInt32, NULL, fwprintfEntry, DbgStringWDecode, L"msvcr120.dll", L"fwprintf" },
	{ _vfprintf12, 0, ReturnInt32, NULL, vfprintfEntry, DbgStringADecode, L"msvcr120.dll", L"vfprintf" },
	{ _vfwprintf12, 0, ReturnInt32, NULL, vfwprintfEntry, DbgStringWDecode, L"msvcr120.dll", L"vfwprintf" },
	{ _fprintf12d, 0, ReturnInt32, NULL, fprintfEntry, DbgStringADecode, L"msvcr120d.dll", L"fprintf" },
	{ _fwprintf12d, 0, ReturnInt32, NULL, fwprintfEntry, DbgStringWDecode, L"msvcr120d.dll", L"fwprintf" },
	{ _vfprintf12d, 0, ReturnInt32, NULL, vfprintfEntry, DbgStringADecode, L"msvcr120d.dll", L"vfprintf" },
	{ _vfwprintf12d, 0, ReturnInt32, NULL, vfwprintfEntry, DbgStringWDecode, L"msvcr120d.dll", L"vfwprintf" },

};

BTR_FILTER DbgRegistration;

//
// This routine is called in context of runtime worker thread
// however it's not guaranteed to be in same thread
//

ULONG WINAPI
DbgUnregisterCallback(
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
DbgControlCallback(
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
DbgInitialize(
	IN BTR_FILTER_MODE Mode
	)
{
	PVOID Address;
	HMODULE DllHandle;
	PCHAR Buffer;
	int i;

	StringCchCopy(DbgRegistration.FilterName,  MAX_PATH - 1, L"Debug Filter");
	StringCchCopy(DbgRegistration.Author, MAX_PATH - 1, L"lan.john@gmail.com");
	StringCchCopy(DbgRegistration.Description, MAX_PATH - 1, L"Win32 Debug Filter");

	DbgRegistration.FilterTag = 0;
	DbgRegistration.MajorVersion = 1;
	DbgRegistration.MinorVersion = 0;

	GetModuleFileName(DbgModuleHandle, DbgRegistration.FilterFullPath, MAX_PATH);

	DbgRegistration.Probes = DbgProbes;
	DbgRegistration.ProbesCount = DBG_PROBE_NUMBER;
	
	DbgRegistration.ControlCallback = DbgControlCallback;
	DbgRegistration.UnregisterCallback = DbgUnregisterCallback;
	DbgRegistration.FilterGuid = DbgUuid;

	if (Mode == FILTER_MODE_DECODE) {
		return TRUE;
	}

	//
	// Fill target routine address
	//

	for(i = 0; i < DBG_PROBE_NUMBER; i++) {

		DllHandle = GetModuleHandle(DbgProbes[i].DllName);
		if (DllHandle) {

			BtrFltConvertUnicodeToAnsi(DbgProbes[i].ApiName, &Buffer);
			Address = (PVOID)GetProcAddress(DllHandle, Buffer);
			BtrFltFree(Buffer);

			if (Address) {
				DbgProbes[i].Address = Address;
			} else {
				continue;
			} 

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
	if (DbgInitializedOk) {
		return &DbgRegistration;
	}

	DbgInitializedOk = DbgInitialize(Mode);
	if (DbgInitializedOk) {
		return &DbgRegistration;
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
		DbgModuleHandle = hModule;
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}