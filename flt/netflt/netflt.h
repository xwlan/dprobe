//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _NETFLT_H_
#define _NETFLT_H_

#ifdef __cplusplus 
extern "C" {
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#define _BTR_SDK_
#include <winsock2.h>
#include "btrsdk.h"
#include <strsafe.h>

#ifndef ASSERT
 #include <assert.h>
 #define ASSERT assert
#endif

#define SkMajorVersion 1
#define SkMinorVersion 0

//
// GUID identify socket filter
//

extern GUID SkGuid;

//
// Socket probe ordinals
//

typedef enum _SK_PROBE_ORDINAL {
	_socket,
	_bind,
	_connect,
	_listen,
	_accept,
	_send,
	_recv,
	_sendto,
	_recvfrom,
	_select,
	_setsockopt,
	_getsockopt,
	_ioctlsocket,
	_shutdown,
	_closesocket,
	_gethostbyname,
	_gethostbyaddr,
	_gethostname,
	_getsockname,
	_WSASocket,
	_WSAConnect,
	_WSAAccept,
	_WSAEventSelect,
	_WSAAsyncSelect,
	_WSAIoctl,
	_WSARecv,
	_WSARecvFrom,
	_WSASend,
	_WSASendTo,
	SK_PROBE_NUMBER,
} SK_PROBE_ORDINAL, *PSK_PROBE_ORDINAL;

//
// socket
//

typedef struct _SK_SOCKET {
	int af;
	int type;
	int protocal;
	SOCKET sk;
	ULONG LastErrorStatus;
} SK_SOCKET, *PSK_SOCKET;

SOCKET WINAPI
SocketEnter(
	IN int af,
	IN int type,
	IN int protocal
	);

typedef SOCKET 
(WINAPI *SOCKET_ROUTINE)(
	IN int af,
	IN int type,
	IN int protocal
	);

ULONG
SocketDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// bind
//

typedef struct _SK_BIND {
	SOCKET sk;
	SOCKADDR *name;
	int namelen;
	SOCKADDR_IN addr;
	int ReturnValue;
	ULONG LastErrorStatus;
} SK_BIND, *PSK_BIND;

int WINAPI
BindEnter(
	IN SOCKET s,
	IN const struct sockaddr* name,
	IN int namelen
	);

typedef int 
(WINAPI *BIND_ROUTINE)(
	IN SOCKET s,
	IN const struct sockaddr* name,
	IN int namelen
	);

ULONG
BindDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// accept
//

typedef struct _SK_ACCEPT {

	SOCKET sk;
	SOCKADDR *addr;
	int *addrlen;

	SOCKADDR_IN Address;
	int AddressLength;
	SOCKET Result;
	ULONG LastErrorStatus;

} SK_ACCEPT, *PSK_ACCEPT;

SOCKET WINAPI
AcceptEnter(
	IN  SOCKET s,
	OUT struct sockaddr* addr,
	OUT int* addrlen
	);

typedef SOCKET 
(WINAPI *ACCEPT_ROUTINE)(
	IN  SOCKET s,
	OUT struct sockaddr* addr,
	OUT int* addrlen
	);

ULONG
AcceptDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// connect
//

typedef struct _SK_CONNECT {

	SOCKET sk;
	SOCKADDR *name;
	int namelen;

	SOCKADDR_IN Address;
	int Result;
	ULONG LastErrorStatus;

} SK_CONNECT, *PSK_CONNECT;

int WINAPI
ConnectEnter(
	IN SOCKET s,
	IN const struct sockaddr* name,
	IN int namelen
	);

typedef int 
(WINAPI *CONNECT_ROUTINE)(
	IN SOCKET s,
	IN const struct sockaddr* name,
	IN int namelen
	);

ULONG
ConnectDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// listen
//

typedef struct _SK_LISTEN {
	SOCKET sk;
	int backlog;
	int ReturnValue;
	ULONG LastErrorStatus;
} SK_LISTEN, *PSK_LISTEN;

int WINAPI
ListenEnter(
	IN SOCKET s,
	IN int backlog
	);

typedef int 
(WINAPI *LISTEN_ROUTINE)(
	IN SOCKET s,
	IN int backlog
	);

ULONG
ListenDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// select
//

typedef struct _SK_SELECT {

	int nfds;
	fd_set *readfds;
	fd_set *writefds;
	fd_set *exceptfds;
	struct timeval *timeout;

	fd_set Read;
	fd_set Write;
	fd_set Except;
	struct timeval Timeout;
	int ReturnValue;
	ULONG Status;

} SK_SELECT, *PSK_SELECT;

int WINAPI
SelectEnter(
	IN  int nfds,
	OUT fd_set* readfds,
	OUT fd_set* writefds,
	OUT fd_set* exceptfds,
	IN  const struct timeval* timeout
	);

typedef int 
(WINAPI *SELECT_ROUTINE)(
	IN  int nfds,
	OUT fd_set* readfds,
	OUT fd_set* writefds,
	OUT fd_set* exceptfds,
	IN  const struct timeval* timeout
	);

ULONG
SelectDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// setsockopt
//

typedef struct _SK_SETSOCKOPT {

	SOCKET sk;
	int level;
	int optname;
	const char *optval;
	int optlen;

	CHAR Value[16];
	int ReturnValue;
	ULONG Status;

} SK_SETSOCKOPT, *PSK_SETSOCKOPT;

int WINAPI 
SetSockOptEnter(
	IN SOCKET s,
	IN int level,
	IN int optname,
	IN const char* optval,
	IN int optlen
	);

typedef int 
(WINAPI *SETSOCKOPT_ROUTINE)(
	IN SOCKET s,
	IN int level,
	IN int optname,
	IN const char* optval,
	IN int optlen
	);

ULONG
SetSockOptDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// getsockopt
//

typedef struct _SK_GETSOCKOPT {

	SOCKET sk;
	int level;
	int optname;
	char *optval;
	int *optlen;

	CHAR OptionValue[16];
	int OptionLength;
	int ReturnValue;
	ULONG Status;

} SK_GETSOCKOPT, *PSK_GETSOCKOPT;

int WINAPI
GetSockOptEnter(
	IN  SOCKET s,
	IN  int level,
	IN  int optname,
	OUT char* optval,
	OUT int* optlen
	);

typedef int 
(WINAPI *GETSOCKOPT_ROUTINE)(
	IN  SOCKET s,
	IN  int level,
	IN  int optname,
	OUT char* optval,
	OUT int* optlen
	);

ULONG
GetSockOptDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// ioctlsocket
//

typedef struct _SK_IOCTLSOCKET {

	SOCKET sk;
	long cmd;
	u_long *argp;
	
	ULONG Argument;
	int ReturnValue;
	ULONG Status;

} SK_IOCTLSOCKET, *PSK_IOCTLSOCKET;

int WINAPI
IoCtlSocketEnter(
	IN  SOCKET s,
	IN  long cmd,
	OUT u_long* argp
	);

typedef int 
(WINAPI *IOCTLSOCKET_ROUTINE)(
	IN  SOCKET s,
	IN  long cmd,
	OUT u_long* argp
	);

ULONG
IoCtlSocketDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// shutdown
//

typedef struct _SK_SHUTDWON {
	SOCKET sk;
	int how;
	int ReturnValue;
	ULONG Status;
} SK_SHUTDOWN, *PSK_SHUTDOWN;

int WINAPI
ShutdownEnter(
	IN SOCKET s,
	IN int how
	);

typedef int 
(WINAPI *SHUTDOWN_ROUTINE)(
	IN SOCKET s,
	IN int how
	);

ULONG
ShutdownDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// closesocket 
//

typedef struct _SK_CLOSESOCKET{
	SOCKET sk;
	int ReturnValue;
	ULONG Status;
} SK_CLOSESOCKET, *PSK_CLOSESOCKET;

int WINAPI
CloseSocketEnter(
	IN SOCKET s
	);

typedef int 
(WINAPI *CLOSESOCKET_ROUTINE)(
	IN SOCKET s
	);

ULONG
CloseSocketDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// send
//

typedef struct _SK_SEND {
	SOCKET sk;
	const char *buf;
	int len;
	int flags;
	int ReturnValue;
	ULONG Status;
} SK_SEND, *PSK_SEND;

int WINAPI
SendEnter(
	IN SOCKET s,
	IN const char* buf,
	IN int len,
	IN int flags
	);

typedef int 
(WINAPI *SEND_ROUTINE)(
	IN SOCKET s,
	IN const char* buf,
	IN int len,
	IN int flags
	);

ULONG
SendDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// sendto
//

typedef struct _SK_SENDTO {
	SOCKET sk;
	const char *buf;
	int len;
	int flags;
	const struct sockaddr *to;
	int tolen;
	SOCKADDR_IN Address;
	int ReturnValue;
	ULONG Status;
} SK_SENDTO, *PSK_SENDTO;

int WINAPI
SendToEnter(
	IN SOCKET s,
	IN const char* buf,
	IN int len,
	IN int flags,
	IN const struct sockaddr* to,
	IN int tolen
	);

typedef int 
(WINAPI *SENDTO_ROUTINE)(
	IN SOCKET s,
	IN const char* buf,
	IN int len,
	IN int flags,
	IN const struct sockaddr* to,
	IN int tolen
	);

ULONG
SendToDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// recv
//

typedef struct _SK_RECV {
	SOCKET sk;
	char *buf;
	int len;
	int flags;
	int ReturnValue;
	ULONG Status;
} SK_RECV, *PSK_RECV;

int WINAPI
RecvEnter(
	IN  SOCKET s,
	OUT char* buf,
	IN  int len,
	IN  int flags
	);

typedef int 
(WINAPI *RECV_ROUTINE)(
	IN  SOCKET s,
	OUT char* buf,
	IN  int len,
	IN  int flags
	);

ULONG
RecvDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// recvfrom
//

typedef struct _SK_RECVFROM {

	SOCKET sk;
	char *buf;
	int len;
	int flags;
	const struct sockaddr *from;
	int *fromlen;

	SOCKADDR_IN Address;
	int FromLength;
	int ReturnValue;
	ULONG Status;

} SK_RECVFROM, *PSK_RECVFROM;

int WINAPI
RecvFromEnter(
	IN  SOCKET s,
	OUT char* buf,
	IN  int len,
	IN  int flags,
	OUT struct sockaddr* from,
	OUT int* fromlen
	);

typedef int 
(WINAPI *RECVFROM_ROUTINE)(
	IN  SOCKET s,
	OUT char* buf,
	IN  int len,
	IN  int flags,
	OUT struct sockaddr* from,
	OUT int* fromlen
	);

ULONG
RecvFromDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// gethostbyname
//

typedef struct _SK_GETHOSTBYNAME {
	const char *name;
	CHAR Name[MAX_PATH];
	struct hostent *ReturnValue;
	struct hostent Host;
	ULONG Status;
} SK_GETHOSTBYNAME, *PSK_GETHOSTBYNAME;

struct hostent* WINAPI
GetHostByNameEnter(
	IN const char* name
	);

typedef struct hostent* 
(WINAPI *GETHOSTBYNAME_ROUTINE)(
	IN const char* name
	);

ULONG
GetHostByNameDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// gethostbyaddr
//

typedef struct _SK_GETHOSTBYADDR {
	const char *addr;
	int len;
	int type;
	CHAR Address[MAX_PATH];
	struct hostent *ReturnValue;
	struct hostent Host;
	ULONG Status;
} SK_GETHOSTBYADDR, *PSK_GETHOSTBYADDR;

struct hostent* WINAPI
GetHostByAddrEnter(
	IN const char* addr,
	IN int len,
	IN int type
	);

typedef struct hostent* 
(WINAPI *GETHOSTBYADDR_ROUTINE)(
	IN const char* addr,
	IN int len,
	IN int type
	);

ULONG
GetHostByAddrDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// gethostname
//

typedef struct _SK_GETHOSTNAME{
	char *name;
	int namelen;
	CHAR Name[MAX_PATH];
	int ReturnValue;
	ULONG Status;
} SK_GETHOSTNAME, *PSK_GETHOSTNAME;

int WINAPI
GetHostNameEnter(
	OUT char* name,
	IN int namelen
	);

typedef int 
(WINAPI *GETHOSTNAME_ROUTINE)(
	OUT char* name,
	IN int namelen
	);

ULONG
GetHostNameDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// getsockname
//

typedef struct _SK_GETSOCKNAME {

	SOCKET sk;
	struct sockaddr *name;
	int *namelen;

	SOCKADDR_IN Name;
	int NameLength;
	int ReturnValue;
	ULONG Status;

} SK_GETSOCKNAME, *PSK_GETSOCKNAME;

int WINAPI
GetSockNameEnter(
	IN  SOCKET s,
	OUT struct sockaddr* name,
	OUT int* namelen
	);

typedef int 
(WINAPI *GETSOCKNAME_ROUTINE)(
	IN  SOCKET s,
	OUT struct sockaddr* name,
	OUT int* namelen
	);

ULONG
GetSockNameDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// WSASocket
//

typedef struct _SK_WSASOCKET {
	int af;
	int type;
	int protocol;
	LPWSAPROTOCOL_INFO lpProtocalInfo;
	GROUP g;
	DWORD dwFlags;
	SOCKET sk;
	ULONG Status;
} SK_WSASOCKET, *PSK_WSASOCKET;

SOCKET WINAPI
WSASocketEnter(
	IN int af,
	IN int type,
	IN int protocol,
	IN LPWSAPROTOCOL_INFO lpProtocolInfo,
	IN GROUP g,
	IN DWORD dwFlags
	);

typedef SOCKET 
(WINAPI *WSASOCKET_ROUTINE)(
	IN int af,
	IN int type,
	IN int protocol,
	IN LPWSAPROTOCOL_INFO lpProtocolInfo,
	IN GROUP g,
	IN DWORD dwFlags
	);

ULONG
WSASocketDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// WSAConnect
//

typedef struct _SK_WSACONNECT {
	SOCKET sk;
	struct sockaddr *name;
	int namelen;
	LPWSABUF lpCallerData;
	LPWSABUF lpCalleeData;
	LPQOS lpSQOS;
	LPQOS lpGQOS;
	SOCKADDR_IN Address;
	int ReturnValue;
	ULONG Status;
} SK_WSACONNECT, *PSK_WSACONNECT;

int WINAPI
WSAConnectEnter(
	IN SOCKET s,
	IN const struct sockaddr* name,
	IN int namelen,
	IN LPWSABUF lpCallerData,
	OUT LPWSABUF lpCalleeData,
	IN  LPQOS lpSQOS,
	IN  LPQOS lpGQOS
	);

typedef int 
(WINAPI *WSACONNECT_ROUTINE)(
	IN SOCKET s,
	IN const struct sockaddr* name,
	IN int namelen,
	IN LPWSABUF lpCallerData,
	OUT LPWSABUF lpCalleeData,
	IN  LPQOS lpSQOS,
	IN  LPQOS lpGQOS
	);

ULONG
WSAConnectDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// WSAAccept
//

typedef struct _SK_WSAACCEPT {

	SOCKET sk;
	struct sockaddr *addr;
	int *addrlen;
	LPCONDITIONPROC lpfnCondition;
	DWORD dwCallbackData;

	SOCKADDR_IN Address;
	int AddressLength;

	SOCKET ReturnValue;
	ULONG Status;

} SK_WSAACCEPT, *PSK_WSAACCEPT;

SOCKET WINAPI
WSAAcceptEnter(
	IN  SOCKET s,
	OUT struct sockaddr* addr,
	OUT LPINT addrlen,
	IN  LPCONDITIONPROC lpfnCondition,
	IN  DWORD dwCallbackData
	);

typedef SOCKET 
(WINAPI *WSAACCEPT_ROUTINE)(
	IN  SOCKET s,
	OUT struct sockaddr* addr,
	OUT LPINT addrlen,
	IN  LPCONDITIONPROC lpfnCondition,
	IN  DWORD dwCallbackData
	);

ULONG
WSAAcceptDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// WSAEventSelect
//

typedef struct _SK_WSAEVENTSELECT {
	SOCKET sk;
	WSAEVENT hEventObject;
	long lNetworkEvents;
	int ReturnValue;
	ULONG Status;
} SK_WSAEVENTSELECT, *PSK_WSAEVENTSELECT;

int WINAPI
WSAEventSelectEnter(
	IN SOCKET s,
	IN WSAEVENT hEventObject,	
	IN long lNetworkEvents
	);

typedef int 
(WINAPI *WSAEVENTSELECT_ROUTINE)(
	IN SOCKET s,
	IN WSAEVENT hEventObject,	
	IN long lNetworkEvents
	);

ULONG
WSAEventSelectDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// WSAAsyncSelect
//

typedef struct _SK_WSAASYNCSELECT {
	SOCKET sk;
	HWND hWnd;
	unsigned int wMsg;
	long lEvent;
	int ReturnValue;
	ULONG Status;
} SK_WSAASYNCSELECT, *PSK_WSAASYNCSELECT;
 
int WINAPI
WSAAsyncSelectEnter(
	IN SOCKET s,
	IN HWND hWnd,
	IN unsigned int wMsg,
	IN long lEvent
	);

typedef int 
(WINAPI *WSAASYNCSELECT_ROUTINE)(
	IN SOCKET s,
	IN HWND hWnd,
	IN unsigned int wMsg,
	IN long lEvent
	);

ULONG
WSAAsyncSelectDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// WSAIoctl
//

typedef struct _SK_WSAIOCTL {

	SOCKET s;
	DWORD dwIoControlCode;
	LPVOID lpvInBuffer;
	DWORD cbInBuffer;
	LPVOID lpvOutBuffer;
	DWORD cbOutBuffer;
	LPDWORD lpcbBytesReturned;
	LPWSAOVERLAPPED lpOverlapped;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine;

	DWORD BytesReturned;
	int ReturnValue;
	ULONG Status;

} SK_WSAIOCTL, *PSK_WSAIOCTL;

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
	);

typedef int 
(WINAPI *WSAIOCTL_ROUTINE)(
	IN  SOCKET s,
	IN  DWORD dwIoControlCode,
	IN  LPVOID lpvInBuffer,
	IN  DWORD cbInBuffer,
	OUT LPVOID lpvOutBuffer,
	IN  DWORD cbOutBuffer,
	OUT LPDWORD lpcbBytesReturned,
	IN  LPWSAOVERLAPPED lpOverlapped,
	IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	);

ULONG
WSAIoctlDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// __WSAFDIsSet
//

typedef struct _SK_WSAFDISSET {
	SOCKET fd;
	fd_set set;
	int ReturnValue;
} SK_WSAFDISSET, *PSK_WSAFDISSET;

int WINAPI
WSAFDIsSetEnter(
	IN SOCKET fd,
	IN fd_set* set
	);

typedef int 
(WINAPI *WSAFDISSET_ROUTINE)(
	IN SOCKET fd,
	IN fd_set* set
	);

ULONG
WSAFDIsSetDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// WSARecv
//

typedef struct _SK_WSARECV {

	SOCKET s;
	LPWSABUF lpBuffer;
	DWORD dwBufferCount;
	LPDWORD lpNumberOfBytesRecvd;
	LPDWORD lpFlags;
	LPWSAOVERLAPPED lpOverlapped;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine;

	DWORD NumberOfBytesRecvd;
	DWORD Flags;
	int ReturnValue;
	ULONG Status;

} SK_WSARECV, *PSK_WSARECV;

int WINAPI
WSARecvEnter(
	IN  SOCKET s,
	OUT LPWSABUF lpBuffers,
	IN  DWORD dwBufferCount,
	OUT LPDWORD lpNumberOfBytesRecvd,
	OUT LPDWORD lpFlags,
	IN  LPWSAOVERLAPPED lpOverlapped,
	IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	);

typedef int 
(WINAPI *WSARECV_ROUTINE)(
	IN  SOCKET s,
	OUT LPWSABUF lpBuffers,
	IN  DWORD dwBufferCount,
	OUT LPDWORD lpNumberOfBytesRecvd,
	OUT LPDWORD lpFlags,
	IN  LPWSAOVERLAPPED lpOverlapped,
	IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	);

ULONG
WSARecvDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// WSARecvFrom
//

typedef struct _SK_WSARECVFROM{

	SOCKET sk;
	LPWSABUF lpBuffer;
	DWORD dwBufferCount;
	LPDWORD lpNumberOfBytesRecvd;
	LPDWORD lpFlags;
	struct sockaddr* lpFrom;
	LPINT lpFromLen;
	LPWSAOVERLAPPED lpOverlapped;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine;
	
	DWORD NumberOfBytesRecvd;
	DWORD Flags;
	SOCKADDR_IN Address;
	int AddressLength;
	int ReturnValue;
	ULONG Status;

} SK_WSARECVFROM, *PSK_WSARECVFROM;

int WINAPI
WSARecvFromEnter(
	IN  SOCKET s,
	OUT LPWSABUF lpBuffers,
	IN  DWORD dwBufferCount,
	OUT LPDWORD lpNumberOfBytesRecvd,
	OUT LPDWORD lpFlags,
	OUT struct sockaddr* lpFrom,
	OUT LPINT lpFromlen,
	IN  LPWSAOVERLAPPED lpOverlapped,
	IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	);

typedef int 
(WINAPI *WSARECVFROM_ROUTINE)(
	IN  SOCKET s,
	OUT LPWSABUF lpBuffers,
	IN  DWORD dwBufferCount,
	OUT LPDWORD lpNumberOfBytesRecvd,
	OUT LPDWORD lpFlags,
	OUT struct sockaddr* lpFrom,
	OUT LPINT lpFromlen,
	IN  LPWSAOVERLAPPED lpOverlapped,
	IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	);

ULONG
WSARecvFromDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// WSASend
//

typedef struct _SK_WSASEND{

	SOCKET s;
	LPWSABUF lpBuffers;
	DWORD dwBufferCount;
	LPDWORD lpNumberOfBytesSent;
	DWORD dwFlags;
	LPWSAOVERLAPPED lpOverlapped;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine;

	DWORD NumberOfBytesSent;
	int ReturnValue;
	ULONG Status;

} SK_WSASEND, *PSK_WSASEND;

int WINAPI
WSASendEnter(
	IN  SOCKET s,
	IN  LPWSABUF lpBuffers,
	IN  DWORD dwBufferCount,
	OUT LPDWORD lpNumberOfBytesSent,
	IN  DWORD dwFlags,
	IN  LPWSAOVERLAPPED lpOverlapped,
	IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	);

typedef int 
(WINAPI *WSASEND_ROUTINE)(
	IN  SOCKET s,
	IN  LPWSABUF lpBuffers,
	IN  DWORD dwBufferCount,
	OUT LPDWORD lpNumberOfBytesSent,
	IN  DWORD dwFlags,
	IN  LPWSAOVERLAPPED lpOverlapped,
	IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	);

ULONG
WSASendDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// WSASendTo
//

typedef struct _SK_WSASENDTO{

	SOCKET s;
	LPWSABUF lpBuffers;
	DWORD dwBufferCount;
	LPDWORD lpNumberOfBytesSent;
	DWORD dwFlags;
	const struct sockaddr* lpTo;
	int iToLen;
	LPWSAOVERLAPPED lpOverlapped;
	LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine;

	DWORD NumberOfBytesSent;
	SOCKADDR_IN Address;
	int ReturnValue;
	ULONG Status;

} SK_WSASENDTO, *PSK_WSASENDTO;

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
	);

typedef int 
(WINAPI *WSASENDTO_ROUTINE)(
	IN SOCKET s,
	IN LPWSABUF lpBuffers,
	IN DWORD dwBufferCount,
	OUT LPDWORD lpNumberOfBytesSent,
	IN  DWORD dwFlags,
	IN  const struct sockaddr* lpTo,
	IN  int iToLen,
	IN  LPWSAOVERLAPPED lpOverlapped,
	IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	);

ULONG
WSASendToDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

#ifdef __cplusplus
}
#endif

#endif