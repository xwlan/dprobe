//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _WEBFLT_H_
#define _WEBFLT_H_

#ifdef __cplusplus 
extern "C" {
#endif

#define _BTR_SDK_ 
#include "btrsdk.h"
#include <strsafe.h>

#ifndef ASSERT
 #include <assert.h>
 #define ASSERT assert
#endif

//
// WININET API
//

#include <wininet.h>

#define WebMajorVersion 1
#define WebMinorVersion 0

extern GUID WebGuid;

//
// Web Probe Ordinals
//

typedef enum _WEB_PROBE_ORDINAL {
	_InternetOpen,
	_InternetConnect,
	_InternetOpenUrl,
	_InternetCloseHandle,
	_InternetGetCookie,
	_InternetSetCookie,
	_InternetReadFile,
	_InternetWriteFile,
	_HttpOpenRequest,
	_HttpSendRequest,
	_HttpQueryInfo,
	_HttpAddRequestHeaders,
	_FtpCommand,
	_FtpFindFirstFile,
	_FtpGetCurrentDirectory,
	_FtpSetCurrentDirectory,
	_FtpCreateDirectory,
	_FtpRemoveDirectory,
	_FtpGetFile,
	_FtpOpenFile,
	_FtpPutFile,
	_FtpDeleteFile,
	_FtpRenameFile,
	WEB_PROBE_NUMBER,
} WEB_PROBE_ORDINAL, *PWEB_PROBE_ORDINAL;

//
// Filter export routine
//

PBTR_FILTER WINAPI
BtrFltGetObject(
	IN BTR_FILTER_MODE Mode	
	);

//
// Filter unregister callback routine
//

ULONG WINAPI
WebUnregisterCallback(
	VOID
	);

//
// Filter control callback routine
//

ULONG WINAPI
WebControlCallback(
	IN PBTR_FILTER_CONTROL Control,
	OUT PBTR_FILTER_CONTROL *Ack
	);

//
// InternetOpen
//

typedef struct _WEB_INTERNETOPEN {
	CHAR lpszAgent[32];
	DWORD dwAccessType;
	CHAR lpszProxyName[32];
	CHAR lpszProxyBypass[32];
	DWORD dwFlags;
	HINTERNET hResult;
	DWORD LastErrorValue;
} WEB_INTERNETOPEN, *PWEB_INTERNETOPEN;

HINTERNET WINAPI
InternetOpenEnter(
	__in PSTR lpszAgent,
	__in DWORD dwAccessType,
	__in PSTR lpszProxyName,
	__in PSTR lpszProxyBypass,
	__in DWORD dwFlags
	);

typedef HINTERNET 
(WINAPI *INTERNETOPEN_ROUTINE)(
	__in PSTR lpszAgent,
	__in DWORD dwAccessType,
	__in PSTR lpszProxyName,
	__in PSTR lpszProxyBypass,
	__in DWORD dwFlags
	);

ULONG
InternetOpenDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// InternetConnect
//

typedef struct _WEB_INTERNETCONNECT {
	HINTERNET hInternet;
	CHAR lpszServerName[32];
	INTERNET_PORT nServerPort;
	CHAR lpszUsername[32];
	CHAR lpszPassword[32];
	DWORD dwService;
	DWORD dwFlags;
	DWORD_PTR dwContext;
	HINTERNET hResult;
	DWORD LastErrorValue;
} WEB_INTERNETCONNECT, *PWEB_INTERNETCONNECT;

HINTERNET WINAPI 
InternetConnectEnter(
	__in  HINTERNET hInternet,
	__in  PSTR lpszServerName,
	__in  INTERNET_PORT nServerPort,
	__in  PSTR lpszUsername,
	__in  PSTR lpszPassword,
	__in  DWORD dwService,
	__in  DWORD dwFlags,
	__in  DWORD_PTR dwContext
	);

typedef HINTERNET 
(WINAPI *INTERNETCONNECT_ROUTINE)(
	__in  HINTERNET hInternet,
	__in  PSTR lpszServerName,
	__in  INTERNET_PORT nServerPort,
	__in  PSTR lpszUsername,
	__in  PSTR lpszPassword,
	__in  DWORD dwService,
	__in  DWORD dwFlags,
	__in  DWORD_PTR dwContext
	);

ULONG
InternetConnectDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// InternetOpenUrl
//

typedef struct _WEB_INTERNETOPENURL {
	HINTERNET hInternet;
	CHAR lpszUrl[64];
	CHAR lpszHeaders[64];
	DWORD dwHeadersLength;
	DWORD dwFlags;
	DWORD_PTR dwContext;
	HINTERNET hResult;
	DWORD LastErrorValue;
} WEB_INTERNETOPENURL, *PWEB_INTERNETOPENURL;

HINTERNET WINAPI
InternetOpenUrlEnter(
	__in  HINTERNET hInternet,
	__in  PSTR lpszUrl,
	__in  PSTR lpszHeaders,
	__in  DWORD dwHeadersLength,
	__in  DWORD dwFlags,
	__in  DWORD_PTR dwContext
	);

typedef HINTERNET 
(WINAPI *INTERNETOPENURL_ROUTINE)(
	__in  HINTERNET hInternet,
	__in  PSTR lpszUrl,
	__in  PSTR lpszHeaders,
	__in  DWORD dwHeadersLength,
	__in  DWORD dwFlags,
	__in  DWORD_PTR dwContext
	);

ULONG
InternetOpenUrlDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// InternetCloseHandle
//

typedef struct _WEB_INTERNETCLOSEHANDLE {
	HINTERNET hInternet;
	BOOL Result;
	DWORD LastErrorValue;
} WEB_INTERNETCLOSEHANDLE, *PWEB_INTERNETCLOSEHANDLE;

BOOL WINAPI
InternetCloseHandleEnter(
	__in HINTERNET hInternet
	);

typedef BOOL
(WINAPI *INTERNETCLOSEHANDLE_ROUTINE)(
	__in HINTERNET hInternet
	);

ULONG
InternetCloseHandleDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// InternetGetCookie
//

typedef struct _WEB_INTERNETGETCOOKIE {
	CHAR lpszUrl[64];
	CHAR lpszCookieName[64];
	CHAR lpszCookieData[64];
	LPDWORD lpdwSize;
	DWORD dwSize;
	BOOL Status;
	ULONG LastErrorValue;
} WEB_INTERNETGETCOOKIE, *PWEB_INTERNETGETCOOKIE;

BOOL WINAPI
InternetGetCookieEnter(
	__in   PSTR lpszUrl,
	__in   PSTR lpszCookieName,
	__out  PSTR lpszCookieData,
	__out  LPDWORD lpdwSize
	);

typedef BOOL
(WINAPI *INTERNETGETCOOKIE_ROUTINE)(
	__in   PSTR lpszUrl,
	__in   PSTR lpszCookieName,
	__out  PSTR lpszCookieData,
	__out  LPDWORD lpdwSize
	);

ULONG
InternetGetCookieDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// InternetSetCookie
//

typedef struct _WEB_INTERNETSETCOOKIE {
	CHAR lpszUrl[64];
	CHAR lpszCookieName[64];
	CHAR lpszCookieData[64];
	BOOL Status;
	ULONG LastErrorValue;
} WEB_INTERNETSETCOOKIE, *PWEB_INTERNETSETCOOKIE;

BOOL WINAPI
InternetSetCookieEnter(
	__in  PSTR lpszUrl,
	__in  PSTR lpszCookieName,
	__in  PSTR lpszCookieData
	);

typedef BOOL
(WINAPI *INTERNETSETCOOKIE_ROUTINE)(
	__in  PSTR lpszUrl,
	__in  PSTR lpszCookieName,
	__in  PSTR lpszCookieData
	);

ULONG
InternetSetCookieDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// InternetReadFile
//

typedef struct _WEB_INTERNETREADFILE {
	HINTERNET hFile;
	PVOID lpBuffer;
	DWORD dwNumberOfBytesToRead;
	LPDWORD lpdwNumberOfBytesRead;
	DWORD CompleteBytes; 
	BOOL Status;
	ULONG LastErrorValue;
} WEB_INTERNETREADFILE, *PWEB_INTERNETREADFILE;

BOOL WINAPI
InternetReadFileEnter(
	__in   HINTERNET hFile,
	__out  LPVOID lpBuffer,
	__in   DWORD dwNumberOfBytesToRead,
	__out  LPDWORD lpdwNumberOfBytesRead
	);

typedef BOOL
(WINAPI *INTERNETREADFILE_ROUTINE)(
	__in   HINTERNET hFile,
	__out  LPVOID lpBuffer,
	__in   DWORD dwNumberOfBytesToRead,
	__out  LPDWORD lpdwNumberOfBytesRead
	);

ULONG
InternetReadFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// InternetWriteFile
//

typedef struct _WEB_INTERNETWRITEFILE {
	HINTERNET hFile;
	PVOID lpBuffer;
	DWORD dwNumberOfBytesToWrite;
	LPDWORD lpdwNumberOfBytesWritten;
	DWORD CompleteBytes; 
	BOOL Status;
	ULONG LastErrorValue;
} WEB_INTERNETWRITEFILE, *PWEB_INTERNETWRITEFILE;

BOOL WINAPI
InternetWriteFileEnter(
	__in   HINTERNET hFile,
	__in   LPVOID lpBuffer,
	__in   DWORD dwNumberOfBytesToWrite,
	__out  LPDWORD lpdwNumberOfBytesWritten
	);

typedef BOOL 
(WINAPI *INTERNETWRITEFILE_ROUTINE)(
	__in   HINTERNET hFile,
	__in   LPVOID lpBuffer,
	__in   DWORD dwNumberOfBytesToWrite,
	__out  LPDWORD lpdwNumberOfBytesWritten
	);

ULONG
InternetWriteFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// HttpOpenRequest
//

typedef struct _WEB_HTTPOPENREQUEST {
	HINTERNET hConnect;
	CHAR lpszVerb[32];
	CHAR lpszObjectName[32];
	CHAR lpszVersion[32];
	CHAR lpszReferer[32];
	PSTR lplpszAcceptTypes;
	DWORD dwFlags;
	DWORD_PTR dwContext;
	HINTERNET hResult;
	ULONG LastErrorValue;
} WEB_HTTPOPENREQUEST, *PWEB_HTTPOPENREQUEST;

HINTERNET WINAPI
HttpOpenRequestEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszVerb,
	__in  PSTR lpszObjectName,
	__in  PSTR lpszVersion,
	__in  PSTR lpszReferer,
	__in  PSTR* lplpszAcceptTypes,
	__in  DWORD dwFlags,
	__in  DWORD_PTR dwContext
	);

typedef HINTERNET 
(WINAPI *HTTPOPENREQUEST_ROUTINE)(
	__in  HINTERNET hConnect,
	__in  PSTR lpszVerb,
	__in  PSTR lpszObjectName,
	__in  PSTR lpszVersion,
	__in  PSTR lpszReferer,
	__in  PSTR* lplpszAcceptTypes,
	__in  DWORD dwFlags,
	__in  DWORD_PTR dwContext
	);

ULONG
HttpOpenRequestDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// HttpSendRequest
//

typedef struct _WEB_HTTPSENDREQUEST {
	HINTERNET hRequest;
	CHAR lpszHeaders[64];
	DWORD dwHeadersLength;
	LPVOID lpOptional;
	DWORD dwOptionalLength;
	BOOL Status;
	ULONG LastErrorValue;
} WEB_HTTPSENDREQUEST, *PWEB_HTTPSENDREQUEST;

BOOL WINAPI
HttpSendRequestEnter(
	__in  HINTERNET hRequest,
	__in  PSTR lpszHeaders,
	__in  DWORD dwHeadersLength,
	__in  LPVOID lpOptional,
	__in  DWORD dwOptionalLength
	);

typedef BOOL 
(WINAPI *HTTPSENDREQUEST_ROUTINE)(
	__in  HINTERNET hRequest,
	__in  PSTR lpszHeaders,
	__in  DWORD dwHeadersLength,
	__in  LPVOID lpOptional,
	__in  DWORD dwOptionalLength
	);

ULONG
HttpSendRequestDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// HttpQueryInfo
//

typedef struct _WEB_HTTPQUERYINFO {
	HINTERNET hRequest;
	DWORD dwInfoLevel;
	PVOID lpvBuffer;
	CHAR Buffer[64];
	LPDWORD lpdwBufferLength;
	DWORD dwBufferLength;
	LPDWORD lpdwIndex;
	DWORD dwIndex;
	BOOL Status;
	ULONG LastErrorValue;
} WEB_HTTPQUERYINFO, *PWEB_HTTPQUERYINFO;

BOOL WINAPI 
HttpQueryInfoEnter(
	__in  HINTERNET hRequest,
	__in  DWORD dwInfoLevel,
	__in  LPVOID lpvBuffer,
	__in  LPDWORD lpdwBufferLength,
	__in  LPDWORD lpdwIndex
	);

typedef BOOL 
(WINAPI *HTTPQUERYINFO_ROUTINE)(
	__in  HINTERNET hRequest,
	__in  DWORD dwInfoLevel,
	__in  LPVOID lpvBuffer,
	__in  LPDWORD lpdwBufferLength,
	__in  LPDWORD lpdwIndex
	);

ULONG
HttpQueryInfoDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// HttpAddRequestHeaders
//

typedef struct _WEB_HTTPADDREQUESTHEADERS {
	HINTERNET hConnect;
	CHAR lpszHeaders[64];
	DWORD dwHeadersLength;
	DWORD dwModifiers;
	BOOL Status;
	ULONG LastErrorValue;
} WEB_HTTPADDREQUESTHEADERS, *PWEB_HTTPADDREQUESTHEADERS;

BOOL WINAPI 
HttpAddRequestHeadersEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszHeaders,
	__in  DWORD dwHeadersLength,
	__in  DWORD dwModifiers
	);

typedef BOOL 
(WINAPI *HTTPADDREQUESTHEADERS_ROUTINE)(
	__in  HINTERNET hConnect,
	__in  PSTR lpszHeaders,
	__in  DWORD dwHeadersLength,
	__in  DWORD dwModifiers
	);

ULONG
HttpAddRequestHeadersDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FtpCommand
//

typedef struct _WEB_FTPCOMMAND {
	HINTERNET hConnect;
	BOOL fExpectResponse;
	DWORD dwFlags;
	CHAR lpszCommand[32];
	DWORD_PTR dwContext;
	HINTERNET phFtpCommand;
	BOOL Status;
	ULONG LastErrorValue;
} WEB_FTPCOMMAND, *PWEB_FTPCOMMAND;

BOOL WINAPI 
FtpCommandEnter(
	__in  HINTERNET hConnect,
	__in  BOOL fExpectResponse,
	__in  DWORD dwFlags,
	__in  PSTR lpszCommand,
	__in  DWORD_PTR dwContext,
	__out HINTERNET* phFtpCommand
	);

typedef BOOL
(WINAPI *FTPCOMMAND_ROUTINE)(
	__in  HINTERNET hConnect,
	__in  BOOL fExpectResponse,
	__in  DWORD dwFlags,
	__in  PSTR lpszCommand,
	__in  DWORD_PTR dwContext,
	__out HINTERNET* phFtpCommand
	);

ULONG
FtpCommandDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FtpCreateDirectory
//

typedef struct _WEB_FTPCREATEDIRECTORY {
	HINTERNET hConnect;
	CHAR lpszDirectory[MAX_PATH];
	BOOL Status;
	ULONG LastErrorValue;
} WEB_FTPCREATEDIRECTORY, *PWEB_FTPCREATEDIRECTORY;

BOOL WINAPI
FtpCreateDirectoryEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszDirectory
	);

typedef BOOL
(WINAPI *FTPCREATEDIRECTORY_ROUTINE)(
	__in  HINTERNET hConnect,
	__in  PSTR lpszDirectory
	);

ULONG
FtpCreateDirectoryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FtpDeleteFile
//

typedef struct _WEB_FTPDELETEFILE {
	HINTERNET hConnect;
	CHAR lpszFileName[MAX_PATH];
	BOOL Status;
	ULONG LastErrorValue;
} WEB_FTPDELETEFILE, *PWEB_FTPDELETEFILE;

BOOL WINAPI
FtpDeleteFileEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszFileName
	);

typedef BOOL 
(WINAPI *FTPDELETEFILE_ROUTINE)(
	__in  HINTERNET hConnect,
	__in  PSTR lpszFileName
	);

ULONG
FtpDeleteFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FtpFindFirstFile
//

typedef struct _WEB_FTPFINDFIRSTFILE {
	HINTERNET hConnect;
	CHAR lpszSearchFile[MAX_PATH];
	WIN32_FIND_DATA lpFindFileData;
	DWORD dwFlags;
	DWORD_PTR dwContext;
	HINTERNET hResult;
	ULONG LastErrorValue;
} WEB_FTPFINDFIRSTFILE, *PWEB_FTPFINDFIRSTFILE;

HINTERNET WINAPI
FtpFindFirstFileEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszSearchFile,
	__out LPWIN32_FIND_DATA lpFindFileData,
	__in  DWORD dwFlags,
	__in  DWORD_PTR dwContext
	);

typedef HINTERNET 
(WINAPI *FTPFINDFIRSTFILE_ROUTINE)(
	__in  HINTERNET hConnect,
	__in  PSTR lpszSearchFile,
	__out LPWIN32_FIND_DATA lpFindFileData,
	__in  DWORD dwFlags,
	__in  DWORD_PTR dwContext
	);

ULONG
FtpFindFirstFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FtpGetCurrentDirectory
//

typedef struct _WEB_FTPGETCURRENTDIRECTORY {
	HINTERNET hConnect;
	CHAR lpszCurrentDirectory[MAX_PATH];
	DWORD lpdwCurrentDirectory;
	BOOL Status;
	ULONG LastErrorValue;
} WEB_FTPGETCURRENTDIRECTORY, *PWEB_FTPGETCURRENTDIRECTORY;

BOOL WINAPI
FtpGetCurrentDirectoryEnter(
	__in   HINTERNET hConnect,
	__out  PSTR lpszCurrentDirectory,
	__out  LPDWORD lpdwCurrentDirectory
	);

typedef BOOL 
(WINAPI *FTPGETCURRENTDIRECTORY_ROUTINE)(
	__in   HINTERNET hConnect,
	__out  PSTR lpszCurrentDirectory,
	__out  LPDWORD lpdwCurrentDirectory
	);

ULONG
FtpGetCurrentDirectoryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FtpGetFile
//

typedef struct _WEB_FTPGETFILE {
	HINTERNET hConnect;
	CHAR lpszRemoteFile[MAX_PATH];
	CHAR lpszNewFile[MAX_PATH];
	BOOL fFailIfExists;
	DWORD dwFlagsAndAttributes;
	DWORD dwFlags;
	DWORD_PTR dwContext;
	BOOL Status;
	ULONG LastErrorValue;
} WEB_FTPGETFILE, *PWEB_FTPGETFILE;

BOOL WINAPI
FtpGetFileEnter(
	__in HINTERNET hConnect,
	__in PSTR lpszRemoteFile,
	__in PSTR lpszNewFile,
	__in BOOL fFailIfExists,
	__in DWORD dwFlagsAndAttributes,
	__in DWORD dwFlags,
	__in DWORD_PTR dwContext
	);

typedef BOOL 
(WINAPI *FTPGETFILE_ROUTINE)(
	__in HINTERNET hConnect,
	__in PSTR lpszRemoteFile,
	__in PSTR lpszNewFile,
	__in BOOL fFailIfExists,
	__in DWORD dwFlagsAndAttributes,
	__in DWORD dwFlags,
	__in DWORD_PTR dwContext
	);

ULONG
FtpGetFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FtpOpenFile
//

typedef struct _WEB_FTPOPENFILE {
	HINTERNET hConnect;
	CHAR lpszFileName[MAX_PATH];
	DWORD dwAccess;
	DWORD dwFlags;
	DWORD_PTR dwContext;
	HINTERNET hResult;
	ULONG LastErrorValue;
} WEB_FTPOPENFILE, *PWEB_FTPOPENFILE;

HINTERNET WINAPI
FtpOpenFileEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszFileName,
	__in  DWORD dwAccess,
	__in  DWORD dwFlags,
	__in  DWORD_PTR dwContext
	);

typedef HINTERNET 
(WINAPI *FTPOPENFILE_ROUTINE)(
	__in  HINTERNET hConnect,
	__in  PSTR lpszFileName,
	__in  DWORD dwAccess,
	__in  DWORD dwFlags,
	__in  DWORD_PTR dwContext
	);

ULONG
FtpOpenFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FtpPutFile
//

typedef struct _WEB_FTPPUTFILE {
	HINTERNET hConnect;
	CHAR lpszLocalFile[MAX_PATH];
	CHAR lpszNewRemoteFile[MAX_PATH];
	DWORD dwFlags;
	DWORD_PTR dwContext;
	BOOL Status;
	ULONG LastErrorValue;
} WEB_FTPPUTFILE, *PWEB_FTPPUTFILE;

BOOL WINAPI
FtpPutFileEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszLocalFile,
	__in  PSTR lpszNewRemoteFile,
	__in  DWORD dwFlags,
	__in  DWORD_PTR dwContext
	);

typedef BOOL 
(WINAPI *FTPPUTFILE_ROUTINE)(
	__in  HINTERNET hConnect,
	__in  PSTR lpszLocalFile,
	__in  PSTR lpszNewRemoteFile,
	__in  DWORD dwFlags,
	__in  DWORD_PTR dwContext
	);

ULONG
FtpPutFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FtpRemoveDirectory
//

typedef struct _WEB_FTPREMOVEDIRECTORY {
	HINTERNET hConnect;
	CHAR lpszDirectory[MAX_PATH];
	BOOL Status;
	ULONG LastErrorValue;
} WEB_FTPREMOVEDIRECTORY, *PWEB_FTPREMOVEDIRECTORY;

BOOL WINAPI
FtpRemoveDirectoryEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszDirectory
	);

typedef BOOL 
(WINAPI *FTPREMOVEDIRECTORY_ROUTINE)(
	__in  HINTERNET hConnect,
	__in  PSTR lpszDirectory
	);

ULONG
FtpRemoveDirectoryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FtpRenameFile
//

typedef struct _WEB_FTPRENAMEFILE {
	HINTERNET hConnect;
	CHAR lpszExisting[MAX_PATH];
	CHAR lpszNew[MAX_PATH];
	BOOL Status;
	ULONG LastErrorValue;
} WEB_FTPRENAMEFILE, *PWEB_FTPRENAMEFILE;

BOOL WINAPI
FtpRenameFileEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszExisting,
	__in  PSTR lpszNew
	);

typedef BOOL 
(WINAPI *FTPRENAMEFILE_ROUTINE)(
	__in  HINTERNET hConnect,
	__in  PSTR lpszExisting,
	__in  PSTR lpszNew
	);

ULONG
FtpRenameFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FtpSetCurrentDirectory
//

typedef struct _WEB_FTPSETCURRENTDIRECTORY {
	HINTERNET hConnect;
	CHAR lpszDirectory[MAX_PATH];
	BOOL Status;
	ULONG LastErrorValue;
} WEB_FTPSETCURRENTDIRECTORY, *PWEB_FTPSETCURRENTDIRECTORY;

BOOL WINAPI
FtpSetCurrentDirectoryEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszDirectory
	);

typedef BOOL 
(WINAPI *FTPSETCURRENTDIRECTORY_ROUTINE)(
	__in  HINTERNET hConnect,
	__in  PSTR lpszDirectory
	);

ULONG
FtpSetCurrentDirectoryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

#ifdef __cplusplus
}
#endif

#endif
