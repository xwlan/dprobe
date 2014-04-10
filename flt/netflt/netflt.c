//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "netflt.h"
#pragma comment(lib, "ws2_32.lib")

SOCKET WINAPI
SocketEnter(
	IN int af,
	IN int type,
	IN int protocal
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_SOCKET Filter;
	SOCKET_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	SOCKET sk;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (SOCKET_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	sk = (*Routine)(af, type, protocal);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	Record = BtrFltAllocateRecord(sizeof(SK_SOCKET),
		                          SkGuid, _socket);

	Filter = FILTER_RECORD_POINTER(Record, SK_SOCKET);
	Filter->af = af;
	Filter->type = type;
	Filter->protocal = protocal;
	Filter->sk = sk;
	Filter->LastErrorStatus = Status;

	BtrFltQueueRecord(Record);
	return sk;
}

ULONG
SocketDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_SOCKET Data;

	if (Record->ProbeOrdinal != _socket) {
		return S_FALSE;
	}	

	Data = (PSK_SOCKET)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"af=%d, type=%d, protocal=%d, socket=%d, LastErrorStatus=0x%08x",
		Data->af, Data->type, Data->protocal, Data->sk, Data->LastErrorStatus);

	return S_OK;
}

//
// N.B. SOCKADDR and SOCKADDR_IN are compatible
//

int WINAPI
BindEnter(
	IN SOCKET s,
	IN const struct sockaddr* name,
	IN int namelen
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_BIND Filter;
	BIND_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (BIND_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, name, namelen);
	BtrFltSetContext(&Context);

	Status = WSAGetLastError();
	Record = BtrFltAllocateRecord(sizeof(SK_BIND),
		                          SkGuid, _bind);

	Filter = FILTER_RECORD_POINTER(Record, SK_BIND);
	Filter->sk = s;
	Filter->name = (SOCKADDR *)name;
	Filter->namelen = namelen;

	if (name)
		memcpy(&Filter->addr, name, sizeof(*name));	

	Filter->ReturnValue = ReturnValue;
	Filter->LastErrorStatus = Status;

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
BindDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_BIND Data;
	WCHAR Address[MAX_PATH];

	if (Record->ProbeOrdinal != _bind) {
		return S_FALSE;
	}	

	Data = (PSK_BIND)Record->Data;

	//
	// Format the Address:Port pair
	//

	StringCchPrintf(Address, MAX_PATH - 1, L"%S:%d", 
		inet_ntoa(Data->addr.sin_addr), ntohs(Data->addr.sin_port));

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, name=0x%p (%s), namelen=%d, return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->name, Address, Data->namelen, Data->ReturnValue, Data->LastErrorStatus);

	return S_OK;
}

SOCKET WINAPI
AcceptEnter(
	IN  SOCKET s,
	OUT struct sockaddr* addr,
	OUT int* addrlen
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_ACCEPT Filter;
	ACCEPT_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	SOCKET Result;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (ACCEPT_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Result = (*Routine)(s, addr, addrlen);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	Record = BtrFltAllocateRecord(sizeof(SK_ACCEPT),
		                          SkGuid, _accept);

	Filter = FILTER_RECORD_POINTER(Record, SK_ACCEPT);
	Filter->sk = s;
	Filter->addrlen = addrlen;
	Filter->addr = addr;
	Filter->Result = Result;
	Filter->LastErrorStatus = Status;

	if (addr) {
		Filter->Address = *(SOCKADDR_IN *)addr;
	}

	if (addrlen) {
		Filter->AddressLength = *addrlen;
	}

	BtrFltQueueRecord(Record);
	return Result;
}

ULONG
AcceptDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_ACCEPT Data;
	WCHAR Address[MAX_PATH];

	if (Record->ProbeOrdinal != _accept) {
		return S_FALSE;
	}	

	Data = (PSK_ACCEPT)Record->Data;

	if (Data->addr) {
		StringCchPrintf(Address, MAX_PATH - 1, L"%S:%d", 
			inet_ntoa(Data->Address.sin_addr), ntohs(Data->Address.sin_port));
	} else {
		Address[0] = 0;
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, addr=0x%p (%s), addrlen=0x%p (%d), return socket=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->addr, Address, Data->addrlen, Data->AddressLength, 
		Data->Result, Data->LastErrorStatus);

	return S_OK;
}

int WINAPI
ConnectEnter(
	IN SOCKET s,
	IN const struct sockaddr* name,
	IN int namelen
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_CONNECT Filter;
	CONNECT_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int Result;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (CONNECT_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Result = (*Routine)(s, name, namelen);
	BtrFltSetContext(&Context);

	Status = WSAGetLastError();
	Record = BtrFltAllocateRecord(sizeof(SK_CONNECT),
		                          SkGuid, _connect);

	Filter = FILTER_RECORD_POINTER(Record, SK_CONNECT);
	Filter->sk = s;
	Filter->name = (struct sockaddr *)name;
	Filter->namelen = namelen;

	if (name) {
		Filter->Address = *(SOCKADDR_IN *)name;
	}

	Filter->Result = Result;
	Filter->LastErrorStatus = Status;

	BtrFltQueueRecord(Record);
	return Result;
}

ULONG
ConnectDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_CONNECT Data;
	WCHAR Address[MAX_PATH];

	if (Record->ProbeOrdinal != _connect) {
		return S_FALSE;
	}	

	Data = (PSK_CONNECT)Record->Data;

	if (Data->name) {
		StringCchPrintf(Address, MAX_PATH - 1, L"%S:%d", 
			inet_ntoa(Data->Address.sin_addr), ntohs(Data->Address.sin_port));
	} else {
		Address[0] = 0;
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, name=0x%p (%s), namelen=%d, return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->name, Address, Data->namelen, Data->Result, Data->LastErrorStatus);

	return S_OK;
}

int WINAPI
ListenEnter(
	IN SOCKET s,
	IN int backlog
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_LISTEN Filter;
	LISTEN_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (LISTEN_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, backlog);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	Record = BtrFltAllocateRecord(sizeof(SK_LISTEN),
		                          SkGuid, _listen);

	Filter = FILTER_RECORD_POINTER(Record, SK_LISTEN);
	Filter->sk = s;
	Filter->backlog = backlog;
	Filter->ReturnValue = ReturnValue;
	Filter->LastErrorStatus = Status;

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
ListenDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_LISTEN Data;

	if (Record->ProbeOrdinal != _listen) {
		return S_FALSE;
	}	

	Data = (PSK_LISTEN)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, backlog=%d, return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->backlog, Data->ReturnValue, Data->LastErrorStatus);

	return S_OK;
}

int WINAPI 
SetSockOptEnter(
	IN SOCKET s,
	IN int level,
	IN int optname,
	IN const char* optval,
	IN int optlen
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_SETSOCKOPT Filter;
	SETSOCKOPT_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (SETSOCKOPT_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, level, optname, optval, optlen);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	Record = BtrFltAllocateRecord(sizeof(SK_SETSOCKOPT),
		                          SkGuid, _setsockopt);

	Filter = FILTER_RECORD_POINTER(Record, SK_SETSOCKOPT);
	Filter->sk = s;
	Filter->level = level;
	Filter->optname = optname;
	Filter->optval = optval;
	Filter->optlen = optlen;
	Filter->ReturnValue = ReturnValue;
	Filter->Status = Status;

	//
	// N.B. Support maximum 16 bytes length option value, large
	// enough for current all options.
	//

	if (optval)
		memcpy(Filter->Value, optval, min(optlen, 16));

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
SetSockOptDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_SETSOCKOPT Data;
	WCHAR Value[MAX_PATH];

	if (Record->ProbeOrdinal != _setsockopt) {
		return S_FALSE;
	}	

	Data = (PSK_SETSOCKOPT)Record->Data;

	if (Data->optname == SO_LINGER) {
		StringCchPrintf(Value, MAX_PATH, L"%d:%d", Data->Value[0], Data->Value[1]);
	}
	else if (Data->optlen <= sizeof(int)) {
		StringCchPrintf(Value, MAX_PATH, L"%d", *(int *)&Data->Value[0]);
	} 
	else {
		Value[0] = 0;
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, level=%d, optname=%d, optval=0x%p (%s), optlen=0x%d, return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->level, Data->optname, Data->optval, Value, Data->optlen, 
		Data->ReturnValue, Data->Status);

	return S_OK;
}

int WINAPI 
GetSockOptEnter(
	IN  SOCKET s,
	IN  int level,
	IN  int optname,
	OUT char* optval,
	OUT int* optlen
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_GETSOCKOPT Filter;
	GETSOCKOPT_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (GETSOCKOPT_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, level, optname, optval, optlen);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	Record = BtrFltAllocateRecord(sizeof(SK_GETSOCKOPT),
		                          SkGuid, _getsockopt);

	Filter = FILTER_RECORD_POINTER(Record, SK_GETSOCKOPT);
	Filter->sk = s;
	Filter->level = level;
	Filter->optname = optname;
	Filter->optval = optval;
	Filter->optlen = optlen;
	Filter->ReturnValue = ReturnValue;
	Filter->Status = Status;

	if (optval) {
		memcpy(Filter->OptionValue, optval, min(16, *optlen));
	}

	if (optlen) {
		Filter->OptionLength = *optlen;
	}

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
GetSockOptDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_GETSOCKOPT Data;
	CHAR Value[MAX_PATH];

	if (Record->ProbeOrdinal != _getsockopt) {
		return S_FALSE;
	}	

	Data = (PSK_GETSOCKOPT)Record->Data;

	if (Data->optname == SO_LINGER) {
		StringCchPrintfA(Value, MAX_PATH, "%d:%d", Data->OptionValue[0], Data->OptionValue[1]);
	}
	else if (Data->OptionLength <= sizeof(int)) {
		StringCchPrintfA(Value, MAX_PATH, "%d", *(int *)&Data->OptionValue[0]);
	} 
	else {
		Value[0] = 0;
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, level=%d, optname=%d, optval=0x%p (%S), optlen=0x%p (%d), return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->level, Data->optname, Data->optval, Value, Data->optlen, Data->OptionLength, 
		Data->ReturnValue, Data->Status);

	return S_OK;
}

int WINAPI
SelectEnter(
	IN  int nfds,
	OUT fd_set* readfds,
	OUT fd_set* writefds,
	OUT fd_set* exceptfds,
	IN  const struct timeval* timeout
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_SELECT Filter;
	SELECT_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (SELECT_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(nfds, readfds, writefds, exceptfds, timeout);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	Record = BtrFltAllocateRecord(sizeof(SK_SELECT),
		                          SkGuid, _select);

	Filter = FILTER_RECORD_POINTER(Record, SK_SELECT);
	Filter->nfds = nfds;
	Filter->readfds = readfds;
	Filter->writefds = writefds;
	Filter->exceptfds = exceptfds;

	if (timeout != NULL) {
		Filter->Timeout = *timeout;
	}

	Filter->ReturnValue = ReturnValue;
	Filter->Status = Status;

	if (readfds) {
		Filter->Read = *readfds;
	}
	if (writefds) {
		Filter->Write = *writefds;
	}
	if (exceptfds) {
		Filter->Except = *exceptfds;
	}

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
SelectDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_SELECT Data;

	if (Record->ProbeOrdinal != _select) {
		return S_FALSE;
	}	

	Data = (PSK_SELECT)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"readfds=0x%p, writefds=0x%p, exceptfds=0x%p, timeout=0x%p (%d:%d), return=%d, LastErrorStatus=0x%08x",
		Data->readfds, Data->writefds, Data->exceptfds, Data->timeout, Data->Timeout.tv_sec,
		Data->Timeout.tv_usec, Data->ReturnValue, Data->Status);

	return S_OK;
}

int WINAPI
IoCtlSocketEnter(
	IN  SOCKET s,
	IN  long cmd,
	OUT u_long* argp
	)
{

	PBTR_FILTER_RECORD Record;
	PSK_IOCTLSOCKET Filter;
	IOCTLSOCKET_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (IOCTLSOCKET_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, cmd, argp);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	Record = BtrFltAllocateRecord(sizeof(SK_IOCTLSOCKET),
		                          SkGuid, _ioctlsocket);

	Filter = FILTER_RECORD_POINTER(Record, SK_IOCTLSOCKET);

	Filter->sk = s;
	Filter->cmd = cmd;
	Filter->argp = argp;
	Filter->ReturnValue = ReturnValue;
	Filter->Status = Status;

	if (argp) {
		Filter->Argument = *argp;
	}

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
IoCtlSocketDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_IOCTLSOCKET Data;

	if (Record->ProbeOrdinal != _ioctlsocket) {
		return S_FALSE;
	}	

	Data = (PSK_IOCTLSOCKET)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, cmd=0x%08x, argp=0x%p (%u), return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->cmd, Data->argp, Data->Argument, Data->ReturnValue, Data->Status);

	return S_OK;
}

int WINAPI
ShutdownEnter(
	IN SOCKET s,
	IN int how
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_SHUTDOWN Filter;
	SHUTDOWN_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (SHUTDOWN_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, how);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	Record = BtrFltAllocateRecord(sizeof(SK_SHUTDOWN),
		                          SkGuid, _shutdown);

	Filter = FILTER_RECORD_POINTER(Record, SK_SHUTDOWN);

	Filter->sk = s;
	Filter->how = how;
	Filter->ReturnValue = ReturnValue;
	Filter->Status = Status;

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
ShutdownDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_SHUTDOWN Data;

	if (Record->ProbeOrdinal != _shutdown) {
		return S_FALSE;
	}	

	Data = (PSK_SHUTDOWN)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, how=%d, return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->how, Data->ReturnValue, Data->Status);

	return S_OK;
}

int WINAPI
CloseSocketEnter(
	IN SOCKET s
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_CLOSESOCKET Filter;
	CLOSESOCKET_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (CLOSESOCKET_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	Record = BtrFltAllocateRecord(sizeof(SK_CLOSESOCKET),
		                          SkGuid, _closesocket);

	Filter = FILTER_RECORD_POINTER(Record, SK_CLOSESOCKET);
	Filter->sk = s;
	Filter->ReturnValue = ReturnValue;
	Filter->Status = Status;

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
CloseSocketDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_CLOSESOCKET Data;

	if (Record->ProbeOrdinal != _closesocket) {
		return S_FALSE;
	}	

	Data = (PSK_CLOSESOCKET)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->ReturnValue, Data->Status);

	return S_OK;
}

int WINAPI
SendEnter(
	IN SOCKET s,
	IN const char* buf,
	IN int len,
	IN int flags
	)
{

	PBTR_FILTER_RECORD Record;
	PSK_SEND Filter;
	SEND_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (SEND_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, buf, len, flags);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	Record = BtrFltAllocateRecord(sizeof(SK_SEND),
		                          SkGuid, _send);

	Filter = FILTER_RECORD_POINTER(Record, SK_SEND);
	Filter->sk = s;
	Filter->buf = buf;
	Filter->len = len;
	Filter->flags = flags;
	Filter->ReturnValue = ReturnValue;
	Filter->Status = Status;

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
SendDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_SEND Data;

	if (Record->ProbeOrdinal != _send) {
		return S_FALSE;
	}	

	Data = (PSK_SEND)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, buf=0x%p, len=%d, flags=%d, return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->buf, Data->len, Data->flags, Data->ReturnValue, Data->Status);

	return S_OK;
}

int WINAPI
SendToEnter(
	IN SOCKET s,
	IN const char* buf,
	IN int len,
	IN int flags,
	IN const struct sockaddr* to,
	IN int tolen
	)
{

	PBTR_FILTER_RECORD Record;
	PSK_SENDTO Filter;
	SENDTO_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (SENDTO_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, buf, len, flags, to, tolen);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	Record = BtrFltAllocateRecord(sizeof(SK_SENDTO),
		                          SkGuid, _sendto);

	Filter = FILTER_RECORD_POINTER(Record, SK_SENDTO);
	Filter->sk = s;
	Filter->buf = buf;
	Filter->len = len;
	Filter->flags = flags;
	Filter->to = to;
	Filter->tolen = tolen;
	
	if (to != NULL) {
		memcpy(&Filter->Address, to, sizeof(*to));
	}

	Filter->ReturnValue = ReturnValue;
	Filter->Status = Status;

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
SendToDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_SENDTO Data;
	WCHAR Address[MAX_PATH];

	if (Record->ProbeOrdinal != _sendto) {
		return S_FALSE;
	}	

	Data = (PSK_SENDTO)Record->Data;
	StringCchPrintf(Address, MAX_PATH - 1, L"%S:%d", 
		inet_ntoa(Data->Address.sin_addr), ntohs(Data->Address.sin_port));

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, buf=0x%p, len=%d, flags=%d, to=0x%p (%s), tolen=%d, return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->buf, Data->len, Data->flags, Data->to, Address, Data->tolen, 
		Data->ReturnValue, Data->Status);

	return S_OK;
}

int WINAPI
RecvFromEnter(
	IN  SOCKET s,
	OUT char* buf,
	IN  int len,
	IN  int flags,
	OUT struct sockaddr* from,
	OUT int* fromlen
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_RECVFROM Filter;
	RECVFROM_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (RECVFROM_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, buf, len, flags, from, fromlen);
	BtrFltSetContext(&Context);

	Status = WSAGetLastError();
	Record = BtrFltAllocateRecord(sizeof(SK_RECVFROM),
		                          SkGuid, _recvfrom);

	Filter = FILTER_RECORD_POINTER(Record, SK_RECVFROM);
	Filter->sk = s;
	Filter->buf = buf;
	Filter->len = len;
	Filter->flags = flags;
	Filter->from = from;
	Filter->fromlen = fromlen;
	Filter->Status = Status;
	
	if (from) {
		memcpy(&Filter->Address, from, sizeof(*from));
	}

	Filter->ReturnValue = ReturnValue;

	if (fromlen) {
		Filter->FromLength = *fromlen;
	}

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
RecvFromDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_RECVFROM Data;
	WCHAR Address[MAX_PATH];

	if (Record->ProbeOrdinal != _recvfrom) {
		return S_FALSE;
	}	

	Data = (PSK_RECVFROM)Record->Data;
	if (Data->from) {
		StringCchPrintf(Address, MAX_PATH - 1, L"%S:%d", 
			inet_ntoa(Data->Address.sin_addr), ntohs(Data->Address.sin_port));
	} else {
		Address[0] = 0;
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, buf=0x%p, len=%d, flags=%d, from=0x%p (%s), fromlen=0x%p (%d), return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->buf, Data->len, Data->flags, Data->from, Address, Data->fromlen, Data->FromLength, 
		Data->ReturnValue, Data->Status);

	return S_OK;
}

int WINAPI
RecvEnter(
	IN  SOCKET s,
	OUT char* buf,
	IN  int len,
	IN  int flags
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_RECV Filter;
	RECV_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (RECV_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, buf, len, flags);
	BtrFltSetContext(&Context);

	Status = WSAGetLastError();
	Record = BtrFltAllocateRecord(sizeof(SK_RECV),
		                          SkGuid, _recv);

	Filter = FILTER_RECORD_POINTER(Record, SK_RECV);
	Filter->sk = s;
	Filter->buf = buf;
	Filter->len = len;
	Filter->flags = flags;
	Filter->ReturnValue = ReturnValue;
	Filter->Status = Status;

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
RecvDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_RECV Data;

	if (Record->ProbeOrdinal != _recv) {
		return S_FALSE;
	}	

	Data = (PSK_RECV)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, buf=0x%p, len=%d, flags=%d, return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->buf, Data->len, Data->flags, Data->ReturnValue, Data->Status);

	return S_OK;
}

struct hostent* WINAPI
GetHostByNameEnter(
	IN const char* name
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_GETHOSTBYNAME Filter;
	GETHOSTBYNAME_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	struct hostent* ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (GETHOSTBYNAME_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(name);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	Record = BtrFltAllocateRecord(sizeof(SK_GETHOSTBYNAME),
		                          SkGuid, _gethostbyname);

	Filter = FILTER_RECORD_POINTER(Record, SK_GETHOSTBYNAME);

	if (name != NULL) {
		StringCchCopyA(Filter->Name, MAX_PATH, name); 
	} 

	Filter->ReturnValue = ReturnValue;
	Filter->Status = Status;

	if (ReturnValue) {
		Filter->Host = *Filter->ReturnValue;
	}

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
GetHostByNameDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_GETHOSTBYNAME Data;
	CHAR Host[MAX_PATH];
	IN_ADDR addr;

	if (Record->ProbeOrdinal != _gethostbyname) {
		return S_FALSE;
	}	

	Data = (PSK_GETHOSTBYNAME)Record->Data;

	addr.s_addr = *(u_long *)Data->Host.h_addr_list[0];
	StringCchPrintfA(Host, MAX_PATH, "%s:%s", Data->Host.h_name, inet_ntoa(addr));

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"name=0x%p (%S), return=0x%p (%S), LastErrorStatus=0x%08x",
		Data->name, Data->Name, Data->ReturnValue, Host, Data->Status);

	return S_OK;
}

struct hostent* WINAPI
GetHostByAddrEnter(
	IN const char* addr,
	IN int len,
	IN int type
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_GETHOSTBYADDR Filter;
	GETHOSTBYADDR_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	struct hostent* ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (GETHOSTBYADDR_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(addr, len, type);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	Record = BtrFltAllocateRecord(sizeof(SK_GETHOSTBYADDR),
		                          SkGuid, _gethostbyaddr);

	Filter = FILTER_RECORD_POINTER(Record, SK_GETHOSTBYADDR);
	Filter->addr = addr;
	Filter->len = len;
	Filter->type = type;
	Filter->Status = Status;

	if (addr != NULL) {
		StringCchCopyA(Filter->Address, MAX_PATH, addr);
	}
	
	Filter->ReturnValue = ReturnValue;
	Filter->Status = WSAGetLastError();

	if (ReturnValue) {
		Filter->Host = *Filter->ReturnValue;
	}

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
GetHostByAddrDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_GETHOSTBYADDR Data;
	CHAR Host[MAX_PATH];
	IN_ADDR addr;

	if (Record->ProbeOrdinal != _gethostbyaddr) {
		return S_FALSE;
	}	

	Data = (PSK_GETHOSTBYADDR)Record->Data;

	addr.s_addr = *(u_long *)Data->Host.h_addr_list[0];
	StringCchPrintfA(Host, MAX_PATH, "%s:%s", Data->Host.h_name, inet_ntoa(addr));

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"addr=0x%p (%S), len=%d, type=%d, return=0x%p (%S), LastErrorStatus=0x%08x",
		Data->addr, Data->Address, Data->len, Data->type, Data->ReturnValue, Host, Data->Status);

	return S_OK;
}

int WINAPI
GetHostNameEnter(
	OUT char* name,
	IN int namelen
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_GETHOSTNAME Filter;
	GETHOSTNAME_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (GETHOSTNAME_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(name, namelen);
	BtrFltSetContext(&Context);

	Status = WSAGetLastError();
	Record = BtrFltAllocateRecord(sizeof(SK_GETHOSTNAME),
		                          SkGuid, _gethostname);

	Filter = FILTER_RECORD_POINTER(Record, SK_GETHOSTNAME);
	Filter->name = name;
	Filter->namelen = namelen;
	Filter->ReturnValue = ReturnValue;	
	Filter->Status = Status;

	if (name) {
		StringCchCopyA(Filter->Name, MAX_PATH, name);
	}

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
GetHostNameDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_GETHOSTNAME Data;

	if (Record->ProbeOrdinal != _gethostname) {
		return S_FALSE;
	}	

	Data = (PSK_GETHOSTNAME)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"name=0x%p (%S), namelen=%d, return=%d, LastErrorStatus=0x%08x",
		Data->name, Data->Name, Data->namelen, Data->ReturnValue, Data->Status);

	return S_OK;
}

int WINAPI
GetSockNameEnter(
	IN  SOCKET s,
	OUT struct sockaddr* name,
	OUT int* namelen
	)
{

	PBTR_FILTER_RECORD Record;
	PSK_GETSOCKNAME Filter;
	GETSOCKNAME_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (GETSOCKNAME_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, name, namelen);
	BtrFltSetContext(&Context);

	Status = WSAGetLastError();
	Record = BtrFltAllocateRecord(sizeof(SK_GETSOCKNAME),
		                          SkGuid, _getsockname);

	Filter = FILTER_RECORD_POINTER(Record, SK_GETSOCKNAME);
	Filter->sk = s;
	Filter->name = name;
	Filter->namelen = namelen;
	Filter->ReturnValue = ReturnValue;
	Filter->Status = Status;

	if (name) {
		Filter->Name = *(SOCKADDR_IN *)name;
	}
	if (namelen) {
		Filter->NameLength = *namelen;
	}

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
GetSockNameDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_GETSOCKNAME Data;
	CHAR Address[MAX_PATH];

	if (Record->ProbeOrdinal != _getsockname) {
		return S_FALSE;
	}	

	Data = (PSK_GETSOCKNAME)Record->Data;

	if (Data->name) {
		StringCchPrintfA(Address, MAX_PATH - 1, "%S:%d", 
			inet_ntoa(Data->Name.sin_addr), ntohs(Data->Name.sin_port));
	} else {
		Address[0] = 0;
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, name=0x%p (%S), namelen=0x%p (%d), return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->name, Address, Data->namelen, Data->NameLength, Data->ReturnValue, Data->Status);

	return S_OK;
}

SOCKET WINAPI
WSASocketEnter(
	IN int af,
	IN int type,
	IN int protocol,
	IN LPWSAPROTOCOL_INFO lpProtocolInfo,
	IN GROUP g,
	IN DWORD dwFlags
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_WSASOCKET Filter;
	WSASOCKET_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	SOCKET sk;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (WSASOCKET_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	sk = (*Routine)(af, type, protocol, lpProtocolInfo, g, dwFlags);
	BtrFltSetContext(&Context);

	Status = WSAGetLastError();
	Record = BtrFltAllocateRecord(sizeof(SK_WSASOCKET),
		                          SkGuid, _WSASocket);

	Filter = FILTER_RECORD_POINTER(Record, SK_WSASOCKET);
	Filter->af = af;
	Filter->type = type;
	Filter->protocol = protocol;
	Filter->lpProtocalInfo = lpProtocolInfo;
	Filter->g = g;
	Filter->dwFlags = dwFlags;
	Filter->sk = sk;
	Filter->Status = Status;

	BtrFltQueueRecord(Record);
	return sk;
}

ULONG
WSASocketDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_WSASOCKET Packet;
	CHAR Family[32];
	CHAR Type[32];
	CHAR Protocol[32];
	CHAR Flags[64];

	if (Record->ProbeOrdinal != _WSASocket) {
		return S_FALSE;
	}	

	Packet = (PSK_WSASOCKET)Record->Data;

	switch (Packet->af) {

			#define AF_BTH 32

			case AF_UNSPEC:
				StringCchPrintfA(Family, 32, "%s", "AF_UNSPEC");
				break;
			case AF_INET:
				StringCchPrintfA(Family, 32, "%s", "AF_INET");
				break;
			case AF_NETBIOS:
				StringCchPrintfA(Family, 32, "%s", "AF_NETBIOS");
				break;
			case AF_INET6:
				StringCchPrintfA(Family, 32, "%s", "AF_INET6");
				break;
			case AF_IRDA:
				StringCchPrintfA(Family, 32, "%s", "AF_IRDA");
				break;
			case AF_BTH:
				StringCchPrintfA(Family, 32, "%s", "AF_BTH");
				break;
			default:
				StringCchPrintfA(Family, 32, "%d", Packet->af);
	}

	switch (Packet->type) {
			case SOCK_STREAM:
				StringCchPrintfA(Type, 32, "%s", "SOCK_STREAM");
				break;
			case SOCK_DGRAM:
				StringCchPrintfA(Type, 32, "%s", "SOCK_DGRAM");
				break;
			case SOCK_RAW:
				StringCchPrintfA(Type, 32, "%s", "SOCK_RAW");
				break;
			case SOCK_RDM:
				StringCchPrintfA(Type, 32, "%s", "SOCK_RDM");
				break;
			case SOCK_SEQPACKET:
				StringCchPrintfA(Type, 32, "%s", "SOCK_SEQPACKET");
				break;
	}

	switch (Packet->protocol) {
			case SOCK_STREAM:
				StringCchPrintfA(Protocol, 32, "%s", "IPPROTO_TCP");
				break;
			case SOCK_DGRAM:
				StringCchPrintfA(Protocol, 32, "%s", "IPPROTO_UDP");
				break;
			case SOCK_RAW:
				StringCchPrintfA(Protocol, 32, "%s", "IPPROTO_RM");
				break;
			default:
				StringCchPrintfA(Protocol, 32, "%d", Packet->protocol);
				break;
	}

	//
	// Care about only overlapped flag
	//

	if (Packet->dwFlags & WSA_FLAG_OVERLAPPED) {
		StringCchPrintfA(Flags, 64, "0x%x (%s)", Packet->dwFlags, "WSA_FLAG_OVERLAPPED");
	} else {
		StringCchPrintfA(Flags, 64, "0x%x", Packet->dwFlags);
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"af=%S, type=%S, protocal=%S, lpProtocalInfo=0x%p, g=%d, dwFlags=%S, return=%d, LastErrorStatus=0x%08x",
		Family, Type, Protocol, Packet->lpProtocalInfo, Packet->g, Flags, Packet->sk, Packet->Status);

	return S_OK;
}

int WINAPI
WSAConnectEnter(
	IN SOCKET s,
	IN const struct sockaddr* name,
	IN int namelen,
	IN LPWSABUF lpCallerData,
	OUT LPWSABUF lpCalleeData,
	IN  LPQOS lpSQOS,
	IN  LPQOS lpGQOS
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_WSACONNECT Filter;
	WSACONNECT_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (WSACONNECT_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, name, namelen, lpCallerData, lpCalleeData,
		                     lpSQOS, lpGQOS);
	BtrFltSetContext(&Context);

	Status = WSAGetLastError();
	Record = BtrFltAllocateRecord(sizeof(SK_WSACONNECT),
		                          SkGuid, _WSAConnect);

	Filter = FILTER_RECORD_POINTER(Record, SK_WSACONNECT);
	Filter->sk = s;
	Filter->name = (struct sockaddr *)name;
	Filter->namelen = namelen;
	Filter->lpCallerData = lpCallerData;
	Filter->lpCalleeData = lpCalleeData;
	Filter->lpSQOS = lpSQOS;
	Filter->lpGQOS = lpGQOS;
	Filter->ReturnValue = ReturnValue;
	Filter->Status = Status;

	if (name) {
		Filter->Address = *(SOCKADDR_IN *)name;
	}
		
	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG 
WSAConnectDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_WSACONNECT Data;
	CHAR Address[MAX_PATH];

	if (Record->ProbeOrdinal != _WSAConnect) {
		return S_FALSE;
	}	

	Data = (PSK_WSACONNECT)Record->Data;

	if (Data->name) {
		StringCchPrintfA(Address, MAX_PATH - 1, "%S:%d", 
			inet_ntoa(Data->Address.sin_addr), ntohs(Data->Address.sin_port));
	} else {
		Address[0] = 0;
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, name=0x%p (%S), namelen=%d, lpCallerData=0x%p, lpCalleeData=0x%p,"
		L"lpSQOS=0x%p, lpGQOS=0x%p, return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->name, Address, Data->namelen, Data->lpCallerData, 
		Data->lpCalleeData, Data->lpSQOS, Data->lpGQOS, Data->ReturnValue, Data->Status);

	return S_OK;
}

SOCKET WINAPI
WSAAcceptEnter(
	IN  SOCKET s,
	OUT struct sockaddr* addr,
	OUT LPINT addrlen,
	IN  LPCONDITIONPROC lpfnCondition,
	IN  DWORD dwCallbackData
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_WSAACCEPT Filter;
	WSAACCEPT_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	SOCKET sk;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (WSAACCEPT_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	sk = (*Routine)(s, addr, addrlen, lpfnCondition, dwCallbackData);
	BtrFltSetContext(&Context);

	Status = WSAGetLastError();
	Record = BtrFltAllocateRecord(sizeof(SK_WSAACCEPT),
		                          SkGuid, _WSAAccept);

	Filter = FILTER_RECORD_POINTER(Record, SK_WSAACCEPT);
	Filter->sk = s;
	Filter->addr = addr;
	Filter->addrlen = addrlen;
	Filter->lpfnCondition = lpfnCondition;
	Filter->dwCallbackData = dwCallbackData;
	Filter->ReturnValue = sk;
	Filter->Status = Status;

	if (addr) {
		Filter->Address = *(SOCKADDR_IN *)addr;
	}

	if (addrlen) {
		Filter->AddressLength = *addrlen;
	}
		
	BtrFltQueueRecord(Record);
	return sk;
}

ULONG
WSAAcceptDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_WSAACCEPT Data;
	CHAR Address[MAX_PATH];

	if (Record->ProbeOrdinal != _WSAAccept) {
		return S_FALSE;
	}	

	Data = (PSK_WSAACCEPT)Record->Data;

	if (Data->addr) {
		StringCchPrintfA(Address, MAX_PATH - 1, "%S:%d", 
			inet_ntoa(Data->Address.sin_addr), ntohs(Data->Address.sin_port));
	} else {
		Address[0] = 0;
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, addr=0x%p (%S), addrlen=0x%p (%d), lpfnCondition=0x%p, dwCallbackData=0x%d,"
		L"return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->addr, Address, Data->addrlen, Data->AddressLength, 
		Data->ReturnValue, Data->Status);

	return S_OK;
}

int WINAPI
WSAEventSelectEnter(
	IN SOCKET s,
	IN WSAEVENT hEventObject,	
	IN long lNetworkEvents
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_WSAEVENTSELECT Filter;
	WSAEVENTSELECT_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (WSAEVENTSELECT_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, hEventObject, lNetworkEvents);
	BtrFltSetContext(&Context);

	Status = WSAGetLastError();
	Record = BtrFltAllocateRecord(sizeof(SK_WSAEVENTSELECT),
		                          SkGuid, _WSAEventSelect);

	Filter = FILTER_RECORD_POINTER(Record, SK_WSAEVENTSELECT);
	Filter->sk = s;
	Filter->hEventObject = hEventObject;
	Filter->lNetworkEvents = lNetworkEvents;
	Filter->ReturnValue = ReturnValue;
	Filter->Status = Status;

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG 
WSAEventSelectDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_WSAEVENTSELECT Data;

	if (Record->ProbeOrdinal != _WSAEventSelect) {
		return S_FALSE;
	}	

	Data = (PSK_WSAEVENTSELECT)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, hEventObject=0x%p, lNetworkEvents=0x%08x, return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->hEventObject, Data->lNetworkEvents, Data->ReturnValue, Data->Status);

	return S_OK;
}

int WINAPI
WSAAsyncSelectEnter(
	IN SOCKET s,
	IN HWND hWnd,
	IN unsigned int wMsg,
	IN long lEvent
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_WSAASYNCSELECT Filter;
	WSAASYNCSELECT_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (WSAASYNCSELECT_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, hWnd, wMsg, lEvent);
	BtrFltSetContext(&Context);

	Status = WSAGetLastError();
	Record = BtrFltAllocateRecord(sizeof(SK_WSAASYNCSELECT),
		                          SkGuid, _WSAAsyncSelect);

	Filter = FILTER_RECORD_POINTER(Record, SK_WSAASYNCSELECT);
	Filter->sk = s;
	Filter->hWnd = hWnd;
	Filter->wMsg = wMsg;
	Filter->lEvent = lEvent;
	Filter->ReturnValue = ReturnValue;
	Filter->Status = Status;

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
WSAAsyncSelectDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_WSAASYNCSELECT Data;

	if (Record->ProbeOrdinal != _WSAAsyncSelect) {
		return S_FALSE;
	}	

	Data = (PSK_WSAASYNCSELECT)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, hWnd=0x%p, wMsg=0x%x, lEvent=0x%x, return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->hWnd, Data->wMsg, Data->lEvent, Data->ReturnValue, Data->Status);

	return S_OK;
}

int WINAPI
WSAIoctlEnter(
	IN  SOCKET s,
	IN  DWORD dwIoControlCode,
	IN  LPVOID lpvInBuffer,
	IN  DWORD cbInBuffer,
	OUT LPVOID lpvOutBuffer,
	IN  DWORD cbOutBuffer,
	OUT LPDWORD lpcbBytesReturned,
	IN  LPWSAOVERLAPPED lpOverlapped,
	IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_WSAIOCTL Filter;
	WSAIOCTL_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (WSAIOCTL_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, dwIoControlCode, lpvInBuffer, cbInBuffer,
                             lpvOutBuffer, cbOutBuffer, lpcbBytesReturned,
							 lpOverlapped, lpCompletionRoutine);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	Record = BtrFltAllocateRecord(sizeof(SK_WSAIOCTL),
		                          SkGuid, _WSAIoctl);

	Filter = FILTER_RECORD_POINTER(Record, SK_WSAIOCTL);
	Filter->s = s;
	Filter->dwIoControlCode = dwIoControlCode;
	Filter->lpvInBuffer = lpvInBuffer;
	Filter->cbInBuffer = cbInBuffer;
	Filter->lpvOutBuffer = lpvOutBuffer;
	Filter->cbOutBuffer = cbOutBuffer;
	Filter->lpcbBytesReturned = lpcbBytesReturned;
	Filter->lpOverlapped = lpOverlapped;
	Filter->lpCompletionRoutine = lpCompletionRoutine;
	Filter->ReturnValue = ReturnValue;
	Filter->Status = Status;

	if (lpcbBytesReturned) {
		Filter->BytesReturned = *lpcbBytesReturned;
	}

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
WSAIoctlDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_WSAIOCTL Data;

	if (Record->ProbeOrdinal != _WSAIoctl) {
		return S_FALSE;
	}	

	Data = (PSK_WSAIOCTL)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, dwIoControlCode=0x%x, lpvInBuffer=0x%p, cbInBuffer=%d,"
		L"lpvOutBuffer=0x%p, cbOutBuffer=%d, lpcbBytesReturned=0x%p (%d), lpOverlapped=0x%p,"
		L"lpCompletionRoutine=0x%p, return=%d, LastErrorStatus=0x%08x",
		Data->s, Data->dwIoControlCode, Data->lpvInBuffer, Data->cbInBuffer,
		Data->lpvOutBuffer, Data->cbOutBuffer, Data->lpcbBytesReturned, Data->BytesReturned,
		Data->lpOverlapped, Data->lpCompletionRoutine, Data->ReturnValue, Data->Status);

	return S_OK;
}

int WINAPI
WSARecvEnter(
	IN  SOCKET s,
	OUT LPWSABUF lpBuffers,
	IN  DWORD dwBufferCount,
	OUT LPDWORD lpNumberOfBytesRecvd,
	OUT LPDWORD lpFlags,
	IN  LPWSAOVERLAPPED lpOverlapped,
	IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_WSARECV Filter;
	WSARECV_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (WSARECV_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd,
		                     lpFlags, lpOverlapped, lpCompletionRoutine);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	Record = BtrFltAllocateRecord(sizeof(SK_WSARECV),
		                          SkGuid, _WSARecv);

	Filter = FILTER_RECORD_POINTER(Record, SK_WSARECV);
	Filter->s = s;
	Filter->lpBuffer = lpBuffers;
	Filter->dwBufferCount = dwBufferCount;
	Filter->lpNumberOfBytesRecvd = lpNumberOfBytesRecvd;
	Filter->lpFlags = lpFlags;
	Filter->lpOverlapped = lpOverlapped;
	Filter->lpCompletionRoutine = lpCompletionRoutine;
	Filter->ReturnValue = ReturnValue;
	Filter->Status = Status;

	if (lpNumberOfBytesRecvd) {
		Filter->NumberOfBytesRecvd = *lpNumberOfBytesRecvd;
	}
	if (lpFlags) {
		Filter->Flags = *lpFlags;
	}

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG 
WSARecvDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_WSARECV Data;

	if (Record->ProbeOrdinal != _WSARecv) {
		return S_FALSE;
	}	

	Data = (PSK_WSARECV)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, lpBuffers=0x%p, lpNumberOfBytesRecvd=0x%p (%d), lpFlags=0x%p (%d),"
		L"lpOverlapped=0x%p, lpCompletionRoutine=0x%p, return=%d, LastErrorStatus=0x%08x",
		Data->s, Data->lpBuffer, Data->lpNumberOfBytesRecvd, Data->NumberOfBytesRecvd, 
		Data->lpFlags, Data->Flags, Data->lpOverlapped, Data->lpCompletionRoutine, 
		Data->ReturnValue, Data->Status);

	return S_OK;
}

int WINAPI
WSARecvFromEnter(
	IN  SOCKET s,
	OUT LPWSABUF lpBuffers,
	IN  DWORD dwBufferCount,
	OUT LPDWORD lpNumberOfBytesRecvd,
	OUT LPDWORD lpFlags,
	OUT struct sockaddr* lpFrom,
	OUT LPINT lpFromLen,
	IN  LPWSAOVERLAPPED lpOverlapped,
	IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_WSARECVFROM Filter;
	WSARECVFROM_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (WSARECVFROM_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd,
		                     lpFlags, lpFrom, lpFromLen, lpOverlapped, 
							 lpCompletionRoutine);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	Record = BtrFltAllocateRecord(sizeof(SK_WSARECVFROM),
		                          SkGuid, _WSARecvFrom);

	Filter = FILTER_RECORD_POINTER(Record, SK_WSARECVFROM);
	Filter->sk = s;
	Filter->lpBuffer = lpBuffers;
	Filter->dwBufferCount = dwBufferCount;
	Filter->lpNumberOfBytesRecvd = lpNumberOfBytesRecvd;
	Filter->lpFromLen = lpFromLen;
	Filter->lpFlags = lpFlags;
	Filter->lpOverlapped = lpOverlapped;
	Filter->lpCompletionRoutine = lpCompletionRoutine;
	Filter->lpFrom = lpFrom;
	Filter->ReturnValue = ReturnValue;
	Filter->Status = Status;

	if (lpNumberOfBytesRecvd) {
		Filter->NumberOfBytesRecvd = *lpNumberOfBytesRecvd;
	}
	if (lpFlags) {
		Filter->Flags = *lpFlags;
	}
	if (lpFrom) {
		Filter->Address = *(SOCKADDR_IN *)lpFrom;
	}
	if (lpFromLen) {
		Filter->AddressLength = *lpFromLen;
	}

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
WSARecvFromDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_WSARECVFROM Data;
	CHAR Address[MAX_PATH];

	if (Record->ProbeOrdinal != _WSARecvFrom) {
		return S_FALSE;
	}	

	Data = (PSK_WSARECVFROM)Record->Data;

	if (Data->lpFrom) {
		StringCchPrintfA(Address, MAX_PATH - 1, "%S:%d", 
			inet_ntoa(Data->Address.sin_addr), ntohs(Data->Address.sin_port));
	} else {
		Address[0] = 0;
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, lpBuffers=0x%p, lpNumberOfBytesRecvd=0x%p (%d), lpFlags=0x%p (%d),"
		L"lpFrom=0x%p (%S), lpFromLen=0x%p (%d), lpOverlapped=0x%p, lpCompletionRoutine=0x%p, return=%d, LastErrorStatus=0x%08x",
		Data->sk, Data->lpBuffer, Data->lpNumberOfBytesRecvd, Data->NumberOfBytesRecvd, 
		Data->lpFlags, Data->Flags, Data->lpFrom, Address, Data->lpFromLen, Data->AddressLength,
		Data->lpOverlapped, Data->lpCompletionRoutine, Data->ReturnValue, Data->Status);

	return S_OK;
}

int WINAPI
WSASendEnter(
	IN  SOCKET s,
	IN  LPWSABUF lpBuffers,
	IN  DWORD dwBufferCount,
	OUT LPDWORD lpNumberOfBytesSent,
	IN  DWORD dwFlags,
	IN  LPWSAOVERLAPPED lpOverlapped,
	IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_WSASEND Filter;
	WSASEND_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	int ReturnValue;
	ULONG Status;

	BtrFltGetContext(&Context);
	Routine = (WSASEND_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnValue = (*Routine)(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent,
		                     dwFlags, lpOverlapped, lpCompletionRoutine);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	Record = BtrFltAllocateRecord(sizeof(SK_WSASEND),
		                          SkGuid, _WSASend);

	Filter = FILTER_RECORD_POINTER(Record, SK_WSASEND);
	Filter->s = s;
	Filter->lpBuffers = lpBuffers;
	Filter->dwBufferCount = dwBufferCount;
	Filter->lpNumberOfBytesSent = lpNumberOfBytesSent;
	Filter->dwFlags = dwFlags;
	Filter->lpOverlapped = lpOverlapped;
	Filter->lpCompletionRoutine = lpCompletionRoutine;
	Filter->ReturnValue;	
	Filter->Status = Status;

	if (lpNumberOfBytesSent) {
		Filter->NumberOfBytesSent = *lpNumberOfBytesSent;
	}

	BtrFltQueueRecord(Record);
	return ReturnValue;
}

ULONG
WSASendDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_WSASEND Data;

	if (Record->ProbeOrdinal != _WSASend) {
		return S_FALSE;
	}	

	Data = (PSK_WSASEND)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, lpBuffers=0x%p, dwBufferCount=%d, lpNumberOfBytesSent=0x%p (%d), dwFlags=0x%08x,"
		L"lpOverlapped=0x%p, lpCompletionRoutine=0x%p, return=%d, LastErrorStatus=0x%08x",
		Data->s, Data->lpBuffers, Data->dwBufferCount, Data->lpNumberOfBytesSent, Data->NumberOfBytesSent, 
		Data->dwFlags, Data->lpOverlapped, Data->lpCompletionRoutine, Data->ReturnValue, Data->Status);

	return S_OK;
}

int WINAPI
WSASendToEnter(
	IN SOCKET s,
	IN LPWSABUF lpBuffers,
	IN DWORD dwBufferCount,
	OUT LPDWORD lpNumberOfBytesSent,
	IN  DWORD dwFlags,
	IN  const struct sockaddr* lpTo,
	IN  int iToLen,
	IN  LPWSAOVERLAPPED lpOverlapped,
	IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	)
{
	PBTR_FILTER_RECORD Record;
	PSK_WSASENDTO Filter;
	WSASENDTO_ROUTINE Routine;
	int Return;
	DWORD Status;
	BTR_PROBE_CONTEXT Context;

	BtrFltGetContext(&Context);
	Routine = (WSASENDTO_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Return = (*Routine)(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent,
		                dwFlags, lpTo, iToLen, lpOverlapped, lpCompletionRoutine);
	BtrFltSetContext(&Context);
	Status = WSAGetLastError();

	//
	// Allocate record and fill captured fields, before this,
	// N.B. a capture filter can be applied, avoid memory allocation. 
	//

	Record = BtrFltAllocateRecord(sizeof(SK_WSASENDTO),
		                          SkGuid, _WSASend);

	Filter = FILTER_RECORD_POINTER(Record, SK_WSASENDTO);
	Filter->s = s;
	Filter->lpBuffers = lpBuffers;
	Filter->dwBufferCount = dwBufferCount;
	Filter->lpNumberOfBytesSent = lpNumberOfBytesSent;
	Filter->dwFlags = dwFlags;
	Filter->iToLen = iToLen;
	Filter->lpOverlapped = lpOverlapped;
	Filter->lpCompletionRoutine = lpCompletionRoutine;
	Filter->lpTo = lpTo;

	if (lpNumberOfBytesSent) {
		Filter->NumberOfBytesSent = *lpNumberOfBytesSent;
	}
	if (lpTo) {
		Filter->Address = *(SOCKADDR_IN *)lpTo;
	}

	Filter->ReturnValue = Return;
	Filter->Status = Status;

	BtrFltQueueRecord(Record);
	return Return;
}

ULONG
WSASendToDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PSK_WSASENDTO Data;
	CHAR Address[MAX_PATH];

	if (Record->ProbeOrdinal != _WSASendTo) {
		return S_FALSE;
	}	

	Data = (PSK_WSASENDTO)Record->Data;

	if (Data->lpTo) {
		StringCchPrintfA(Address, MAX_PATH - 1, "%S:%d", 
			inet_ntoa(Data->Address.sin_addr), ntohs(Data->Address.sin_port));
	} else {
		Address[0] = 0;
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"socket=%d, lpBuffers=0x%p, dwBufferCount=%d, lpNumberOfBytesSent=0x%p (%d), dwFlags=0x%08x,"
		L"lpTo=0x%p (%S), iToLen=%d, lpOverlapped=0x%p, lpCompletionRoutine=0x%p, return=%d, LastErrorStatus=0x%08x",
		Data->s, Data->lpBuffers, Data->dwBufferCount, Data->lpNumberOfBytesSent, Data->NumberOfBytesSent, 
		Data->dwFlags, Data->lpTo, Address, Data->iToLen, Data->lpOverlapped, 
		Data->lpCompletionRoutine, Data->ReturnValue, Data->Status);

	return S_OK;
}