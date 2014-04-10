//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "netflt.h"
#include "resource.h"

BOOLEAN SkInitializedOk = FALSE;
HMODULE SkModuleHandle = NULL;

GUID SkGuid = { 0x364546d0, 0xe5c0, 0x41a0, { 0x95, 0x24, 0x14, 0xb6, 0xfc, 0xf5, 0x35, 0x92 } };

BTR_FILTER_PROBE SkProbes[SK_PROBE_NUMBER] = {
	{ _socket, 0, ReturnUIntPtr, NULL, SocketEnter, SocketDecode, L"ws2_32.dll", L"socket" },
	{ _bind, 0, ReturnInt32, NULL, BindEnter, BindDecode, L"ws2_32.dll", L"bind" },
	{ _connect, 0, ReturnInt32,  NULL, ConnectEnter, ConnectDecode, L"ws2_32.dll", L"connect" },
	{ _listen, 0, ReturnInt32,   NULL, ListenEnter, ListenDecode, L"ws2_32.dll", L"listen" },
	{ _accept, 0, ReturnUIntPtr, NULL, AcceptEnter, AcceptDecode, L"ws2_32.dll", L"accept" },
	{ _send, 0, ReturnInt32, NULL, SendEnter, SendDecode, L"ws2_32.dll", L"send" },
	{ _recv, 0, ReturnInt32, NULL, RecvEnter, RecvDecode, L"ws2_32.dll", L"recv" },
	{ _sendto, 0, ReturnInt32, NULL, SendToEnter, SendToDecode, L"ws2_32.dll", L"sendto" },
	{ _recvfrom, 0, ReturnInt32, NULL, RecvFromEnter, RecvFromDecode, L"ws2_32.dll", L"recvfrom" },
	{ _select, 0, ReturnInt32, NULL, SelectEnter, SelectDecode, L"ws2_32.dll", L"select" },
	{ _setsockopt, 0, ReturnInt32, NULL, SetSockOptEnter, SetSockOptDecode, L"ws2_32.dll", L"setsockopt" },
	{ _getsockopt, 0, ReturnInt32, NULL, GetSockOptEnter, GetSockOptDecode, L"ws2_32.dll", L"getsockopt" },
	{ _ioctlsocket, 0, ReturnInt32, NULL, IoCtlSocketEnter, IoCtlSocketDecode, L"ws2_32.dll", L"ioctlsocket" },
	{ _shutdown, 0, ReturnInt32, NULL, ShutdownEnter, ShutdownDecode, L"ws2_32.dll", L"shutdown" },
	{ _closesocket, 0, ReturnInt32, NULL, CloseSocketEnter, CloseSocketDecode, L"ws2_32.dll", L"closesocket" },
	{ _gethostbyname, 0, ReturnPtr, NULL, GetHostByNameEnter, GetHostByNameDecode, L"ws2_32.dll", L"gethostbyname" },
	{ _gethostbyaddr, 0, ReturnPtr, NULL, GetHostByAddrEnter, GetHostByAddrDecode, L"ws2_32.dll", L"gethostbyaddr" },
	{ _gethostname, 0, ReturnInt32, NULL, GetHostNameEnter, GetHostNameDecode, L"ws2_32.dll", L"gethostname" },
	{ _getsockname, 0, ReturnInt32, NULL, GetSockNameEnter, GetSockNameDecode, L"ws2_32.dll", L"getsockname" },
	{ _WSASocket, 0, ReturnUIntPtr, NULL, WSASocketEnter, WSASocketDecode, L"ws2_32.dll", L"WSASocketW" },
	{ _WSAConnect, 0, ReturnInt32, NULL, WSAConnectEnter, WSAConnectDecode, L"ws2_32.dll", L"WSAConnect" },
	{ _WSAAccept, 0, ReturnUIntPtr, NULL, WSAAcceptEnter, WSAAcceptDecode, L"ws2_32.dll", L"WSAAccept" },
	{ _WSAEventSelect, 0, ReturnInt32, NULL, WSAEventSelectEnter, WSAEventSelectDecode, L"ws2_32.dll", L"WSAEventSelect" },
	{ _WSAAsyncSelect, 0, ReturnInt32, NULL, WSAAsyncSelectEnter, WSAAsyncSelectDecode, L"ws2_32.dll", L"WSAAsyncSelect" },
	{ _WSAIoctl, 0, ReturnInt32, NULL, WSAIoctlEnter, WSAIoctlDecode, L"ws2_32.dll", L"WSAIoctl" },
	{ _WSARecv, 0, ReturnInt32, NULL, WSARecvEnter, WSARecvDecode, L"ws2_32.dll", L"WSARecv" },
	{ _WSARecvFrom, 0, ReturnInt32, NULL, WSARecvFromEnter, WSARecvFromDecode, L"ws2_32.dll", L"WSARecvFrom" },
	{ _WSASend, 0, ReturnInt32, NULL, WSASendEnter, WSASendDecode, L"ws2_32.dll", L"WSASend" } ,
	{ _WSASendTo, 0, ReturnInt32, NULL, WSASendToEnter, WSASendToDecode, L"ws2_32.dll", L"WSASendTo" } ,
};

BTR_FILTER SkRegistration;

BOOLEAN
SkInitialize(
	IN BTR_FILTER_MODE Mode
	);

PBTR_FILTER WINAPI
BtrFltGetObject(
	IN BTR_FILTER_MODE Mode	
	)
{
	if (SkInitializedOk) {
		return &SkRegistration;
	}

	SkInitializedOk = SkInitialize(Mode);
	if (SkInitializedOk) {
		return &SkRegistration;
	}

	return NULL;
}

//
// This routine is called in context of runtime worker thread
// however it's not guaranteed to be in same thread
//

ULONG WINAPI
SkUnregisterCallback(
	VOID
	)
{
	//
	// N.B. When filter is unregistered, this routine is called
	// by runtime to make filter have a chance to release allocated
	// resources. Typically filter should provide this routine.
	//

	OutputDebugStringA("SkUnregisterCallback");
	return 0;
}

//
// This routine is called in context of runtime worker thread
// however it's not guaranteed to be in same thread
//

ULONG WINAPI
SkControlCallback(
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

	OutputDebugStringA("SkControlCallback");
	return 0;
}

BOOLEAN
SkInitialize(
	IN BTR_FILTER_MODE Mode 
	)
{
	PVOID Address;
	HMODULE DllHandle;
	PCHAR Buffer;
	int i;

	StringCchCopy(SkRegistration.FilterName,  MAX_PATH - 1, L"Socket Filter");
	StringCchCopy(SkRegistration.Author, MAX_PATH - 1, L"lan.john@gmail.com");
	StringCchCopy(SkRegistration.Description, MAX_PATH - 1, L"Win32 Socket API Filter");

	SkRegistration.FilterTag = 0;
	SkRegistration.MajorVersion = 1;
	SkRegistration.MinorVersion = 0;

	GetModuleFileName(SkModuleHandle, SkRegistration.FilterFullPath, MAX_PATH);

	SkRegistration.Probes = SkProbes;
	SkRegistration.ProbesCount = SK_PROBE_NUMBER;
	
	SkRegistration.ControlCallback = SkControlCallback;
	SkRegistration.UnregisterCallback = SkUnregisterCallback;
	SkRegistration.FilterGuid = SkGuid;

	if (Mode == FILTER_MODE_DECODE) {
		return TRUE;
	}

	//
	// Fill target routine address
	//

	for(i = 0; i < SK_PROBE_NUMBER; i++) {

		DllHandle = LoadLibrary(SkProbes[i].DllName);
		if (DllHandle) {

			BtrFltConvertUnicodeToAnsi(SkProbes[i].ApiName, &Buffer);
			Address = (PVOID)GetProcAddress(DllHandle, Buffer);
			BtrFltFree(Buffer);

			if (Address) {
				SkProbes[i].Address = Address;
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
		SkModuleHandle = ModuleHandle;
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
