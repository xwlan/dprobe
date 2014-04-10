//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "wininetflt.h"

BOOLEAN WebInitializedOk = FALSE;
HMODULE WebModuleHandle = NULL;

//
// WININET GUID
//

// {477F04F0-9168-4d49-8D33-11A25B046DD6}
GUID WebGuid = { 0x477f04f0, 0x9168, 0x4d49, { 0x8d, 0x33, 0x11, 0xa2, 0x5b, 0x4, 0x6d, 0xd6 } };

BTR_FILTER_PROBE WebProbes[WEB_PROBE_NUMBER] = {
	{ _InternetOpen, 0, ReturnPtr, NULL, InternetOpenEnter, InternetOpenDecode, L"wininet.dll", L"InternetOpenA" },
	{ _InternetConnect, 0, ReturnPtr, NULL, InternetConnectEnter, InternetConnectDecode, L"wininet.dll", L"InternetConnectA" },
	{ _InternetOpenUrl, 0, ReturnPtr, NULL, InternetOpenUrlEnter, InternetOpenUrlDecode, L"wininet.dll", L"InternetOpenUrlA" },
	{ _InternetCloseHandle, 0, ReturnInt32, NULL, InternetCloseHandleEnter, InternetCloseHandleDecode, L"wininet.dll", L"InternetCloseHandle" },
	{ _InternetGetCookie, 0, ReturnInt32, NULL, InternetGetCookieEnter, InternetGetCookieDecode, L"wininet.dll", L"InternetGetCookieA" },
	{ _InternetSetCookie, 0, ReturnInt32, NULL, InternetSetCookieEnter, InternetSetCookieDecode, L"wininet.dll", L"InternetSetCookieA" },
	{ _InternetReadFile, 0, ReturnInt32, NULL, InternetReadFileEnter, InternetReadFileDecode, L"wininet.dll", L"InternetReadFile" },
	{ _InternetWriteFile, 0, ReturnInt32, NULL, InternetWriteFileEnter, InternetWriteFileDecode, L"wininet.dll", L"InternetWriteFile" },
	{ _HttpOpenRequest, 0, ReturnPtr, NULL, HttpOpenRequestEnter, HttpOpenRequestDecode, L"wininet.dll", L"HttpOpenRequestA" },
	{ _HttpSendRequest, 0, ReturnInt32, NULL, HttpSendRequestEnter, HttpSendRequestDecode, L"wininet.dll", L"HttpSendRequestA" },
	{ _HttpQueryInfo, 0, ReturnInt32, NULL, HttpQueryInfoEnter, HttpQueryInfoDecode, L"wininet.dll", L"HttpQueryInfoA" },
	{ _HttpAddRequestHeaders, 0, ReturnInt32, NULL, HttpAddRequestHeadersEnter, HttpAddRequestHeadersDecode, L"wininet.dll", L"HttpAddRequestHeadersA" },
	{ _FtpCommand, 0, ReturnInt32, NULL, FtpCommandEnter, FtpCommandDecode, L"wininet.dll", L"FtpCommandA" },
	{ _FtpFindFirstFile, 0, ReturnPtr, NULL, FtpFindFirstFileEnter, FtpFindFirstFileDecode, L"wininet.dll", L"FtpFindFirstFileA" },
	{ _FtpGetCurrentDirectory, 0, ReturnInt32, NULL, FtpGetCurrentDirectoryEnter, FtpGetCurrentDirectoryDecode, L"wininet.dll", L"FtpGetCurrentDirectoryA" },
	{ _FtpSetCurrentDirectory, 0, ReturnInt32, NULL, FtpSetCurrentDirectoryEnter, FtpSetCurrentDirectoryDecode, L"wininet.dll", L"FtpSetCurrentDirectoryA" },
	{ _FtpCreateDirectory, 0, ReturnInt32, NULL, FtpCreateDirectoryEnter, FtpCreateDirectoryDecode, L"wininet.dll", L"FtpCreateDirectoryA" },
	{ _FtpRemoveDirectory, 0, ReturnInt32, NULL, FtpRemoveDirectoryEnter, FtpRemoveDirectoryDecode, L"wininet.dll", L"FtpRemoveDirectoryA" },
	{ _FtpGetFile, 0, ReturnUInt32, NULL, FtpGetFileEnter, FtpGetFileDecode, L"wininet.dll", L"FtpGetFileA" },
	{ _FtpOpenFile, 0, ReturnPtr, NULL, FtpOpenFileEnter, FtpOpenFileDecode, L"wininet.dll", L"FtpOpenFileA" },
	{ _FtpPutFile, 0, ReturnInt32, NULL, FtpPutFileEnter, FtpPutFileDecode, L"wininet.dll", L"FtpPutFileA" },
	{ _FtpDeleteFile, 0, ReturnInt32, NULL, FtpDeleteFileEnter, FtpDeleteFileDecode, L"wininet.dll", L"FtpDeleteFileA" },
	{ _FtpRenameFile, 0, ReturnInt32, NULL, FtpRenameFileEnter, FtpRenameFileDecode, L"wininet.dll", L"FtpRenameFileA" },
};

BTR_FILTER WebRegistration;

BOOLEAN
WebInitialize(
	IN BTR_FILTER_MODE Mode
	);

PBTR_FILTER WINAPI
BtrFltGetObject(
	IN BTR_FILTER_MODE Mode
	)
{
	if (WebInitializedOk) {
		return &WebRegistration;
	}

	WebInitializedOk = WebInitialize(Mode);
	if (WebInitializedOk) {
		return &WebRegistration;
	}

	return NULL;
}

//
// This routine is called in context of runtime worker thread
// however it's not guaranteed to be in same thread
//

ULONG WINAPI
WebUnregisterCallback(
	VOID
	)
{
	//
	// N.B. When filter is unregistered, this routine is called
	// by runtime to make filter have a chance to release allocated
	// resources. Typically filter should provide this routine.
	//

	OutputDebugStringA("WebUnregisterCallback");
	return 0;
}

//
// This routine is called in context of runtime worker thread
// however it's not guaranteed to be in same thread
//

ULONG WINAPI
WebControlCallback(
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

	OutputDebugStringA("WebControlCallback");
	return 0;
}

BOOLEAN
WebInitialize(
	IN BTR_FILTER_MODE Mode
	)
{
	PVOID Address;
	HMODULE DllHandle;
	PCHAR Buffer;
	int i;

	StringCchCopy(WebRegistration.FilterName,  MAX_PATH - 1, L"Internet Filter");
	StringCchCopy(WebRegistration.Author,  MAX_PATH - 1, L"lan.john@gmail.com");
	StringCchCopy(WebRegistration.Description, MAX_PATH - 1, L"Internet API Filter");

	WebRegistration.FilterTag = 0;
	WebRegistration.MajorVersion = 1;
	WebRegistration.MinorVersion = 0;

	GetModuleFileName(WebModuleHandle, WebRegistration.FilterFullPath, MAX_PATH);

	WebRegistration.Probes = WebProbes;
	WebRegistration.ProbesCount = WEB_PROBE_NUMBER;
	
	WebRegistration.ControlCallback = WebControlCallback;
	WebRegistration.UnregisterCallback = WebUnregisterCallback;
	WebRegistration.FilterGuid = WebGuid;

	if (Mode == FILTER_MODE_DECODE) {
		return TRUE;
	}

	//
	// Fill target routine address
	//

	for(i = 0; i < WEB_PROBE_NUMBER; i++) {

		DllHandle = LoadLibrary(WebProbes[i].DllName);
		if (DllHandle) {

			BtrFltConvertUnicodeToAnsi(WebProbes[i].ApiName, &Buffer);
			Address = (PVOID)GetProcAddress(DllHandle, Buffer);
			BtrFltFree(Buffer);

			if (Address) {
				WebProbes[i].Address = Address;
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
		WebModuleHandle = ModuleHandle;
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
