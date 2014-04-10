//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "wininetflt.h"

VOID 
DebugTrace(
	IN PSTR Format,
	...
	)
{

#ifdef _DEBUG
	
	va_list arg;
	size_t length;
	char buffer[512];

	va_start(arg, Format);

	StringCchCopyA(buffer, 512, "[wininetflt] : ");
	StringCchVPrintfA(buffer + 15, 400, Format, arg);
	StringCchLengthA(buffer, 512, &length);
	
	if (length < 512) {
		StringCchCatA(buffer, 512, "\n");
	}

	va_end(arg);

	OutputDebugStringA(buffer);	

#endif

}

//
// InternetOpen
//

HINTERNET WINAPI
InternetOpenEnter(
	__in LPSTR lpszAgent,
	__in DWORD dwAccessType,
	__in LPSTR lpszProxyName,
	__in LPSTR lpszProxyBypass,
	__in DWORD dwFlags
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_INTERNETOPEN Filter;
	INTERNETOPEN_ROUTINE Routine;
	HINTERNET hResult = NULL;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (INTERNETOPEN_ROUTINE)Context.Destine;
	hResult = (*Routine)(lpszAgent, dwAccessType, lpszProxyName, lpszProxyBypass, dwFlags);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_INTERNETOPEN),
		                          WebGuid, _InternetOpen);

	Filter = FILTER_RECORD_POINTER(Record, WEB_INTERNETOPEN);
	Filter->LastErrorValue = LastError;
	Filter->hResult = hResult;
	Filter->dwFlags = dwFlags;
	Filter->dwAccessType = dwAccessType;

	if (lpszAgent) {
		StringCchCopyA(Filter->lpszAgent, 32, lpszAgent);
	}

	if (lpszProxyName) {
		StringCchCopyA(Filter->lpszProxyName, 32, lpszProxyName);
	} 

	if (lpszProxyBypass) {
		StringCchCopyA(Filter->lpszProxyBypass, 32, lpszProxyBypass);
	} 

	BtrFltQueueRecord(Record);
	return hResult;
}

ULONG
InternetOpenDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_INTERNETOPEN Data;

	if (Record->ProbeOrdinal != _InternetOpen) {
		return S_FALSE;
	}	

	Data = (PWEB_INTERNETOPEN)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"lpszAgent='%S',dwAccessType=%d,lpszProxyName='%S',lpszProxyBypass='%S', dwFlags=0x%x, Return=0x%p", 
		Data->lpszAgent, Data->dwAccessType, Data->lpszProxyName, Data->lpszProxyBypass, Data->dwFlags, Data->hResult);

	return S_OK;
}

//
// InternetConnect
//

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
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_INTERNETCONNECT Filter;
	INTERNETCONNECT_ROUTINE Routine;
	HINTERNET hResult;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (INTERNETCONNECT_ROUTINE)Context.Destine;
	hResult = (*Routine)(hInternet, lpszServerName, nServerPort, lpszUsername, 
						 lpszPassword, dwService, dwFlags, dwContext);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_INTERNETCONNECT),
		                          WebGuid, _InternetConnect);

	Filter = FILTER_RECORD_POINTER(Record, WEB_INTERNETCONNECT);
	Filter->hInternet = hInternet;

	if (lpszServerName) {
		StringCchCopyA(Filter->lpszServerName, 32, lpszServerName);
	} 

	Filter->nServerPort = nServerPort;

	if (lpszUsername) {
		StringCchCopyA(Filter->lpszUsername, 32, lpszUsername);
	} 

	if (lpszPassword) {
		StringCchCopyA(Filter->lpszPassword, 32, lpszPassword);
	} 

	Filter->dwService = dwService;
	Filter->dwFlags = dwFlags;
	Filter->dwContext = dwContext;
	Filter->hResult = hResult;
	Filter->LastErrorValue = LastError;

	BtrFltQueueRecord(Record);
	return hResult;
}

ULONG
InternetConnectDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_INTERNETCONNECT Data;

	if (Record->ProbeOrdinal != _InternetConnect) {
		return S_FALSE;
	}	

	Data = (PWEB_INTERNETCONNECT)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hInternet=0x%p,lpszServerName='%S', nServerPort=%d, lpszUsername='%S', lpszPassword='%S', dwService=0x%x, dwFlag=0x%x, dwContext=0x%p", 
		Data->hInternet, Data->lpszServerName, Data->nServerPort, Data->lpszUsername, Data->lpszPassword, Data->dwService, Data->dwFlags, Data->dwContext);

	return S_OK;
}

//
// InternetOpenUrl
//

HINTERNET WINAPI
InternetOpenUrlEnter(
	__in  HINTERNET hInternet,
	__in  PSTR lpszUrl,
	__in  PSTR lpszHeaders,
	__in  DWORD dwHeadersLength,
	__in  DWORD dwFlags,
	__in  DWORD_PTR dwContext
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_INTERNETOPENURL Filter;
	INTERNETOPENURL_ROUTINE Routine;
	HINTERNET hResult;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (INTERNETOPENURL_ROUTINE)Context.Destine;
	hResult = (*Routine)(hInternet, lpszUrl, lpszHeaders, dwHeadersLength, dwFlags, dwContext);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_INTERNETOPENURL),
		                          WebGuid, _InternetOpenUrl);

	Filter = FILTER_RECORD_POINTER(Record, WEB_INTERNETOPENURL);
	Filter->hInternet = hInternet;

	if (lpszUrl) {
		StringCchCopyA(Filter->lpszUrl, 64, lpszUrl);
	} 

	if (lpszHeaders) {
		StringCchCopyA(Filter->lpszHeaders, 64, lpszHeaders);
	}

	Filter->dwHeadersLength = dwHeadersLength;
	Filter->dwFlags = dwFlags;
	Filter->dwContext = dwContext;
	Filter->hResult = hResult;
	Filter->LastErrorValue = LastError;

	BtrFltQueueRecord(Record);
	return hResult;
}

ULONG
InternetOpenUrlDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_INTERNETOPENURL Data;

	if (Record->ProbeOrdinal != _InternetOpenUrl) {
		return S_FALSE;
	}	

	Data = (PWEB_INTERNETOPENURL)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hInternet=0x%p,lpszUrl='%S', lpszHeaders='%S', dwHeadersLength=%d, dwFlag=%d, dwContext=0x%p, Return=0x%p", 
		Data->hInternet, Data->lpszUrl, Data->lpszHeaders, Data->dwHeadersLength, Data->dwFlags, Data->dwContext, Data->hResult);

	return S_OK;
}

//
// InternetCloseHandle
//

BOOL WINAPI
InternetCloseHandleEnter(
	__in HINTERNET hInternet
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_INTERNETCLOSEHANDLE Filter;
	INTERNETCLOSEHANDLE_ROUTINE Routine;
	BOOL Status;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (INTERNETCLOSEHANDLE_ROUTINE)Context.Destine;
	Status = (*Routine)(hInternet);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_INTERNETCLOSEHANDLE),
		                          WebGuid, _InternetCloseHandle);

	Filter = FILTER_RECORD_POINTER(Record, WEB_INTERNETCLOSEHANDLE);

	Filter->hInternet = hInternet;
	Filter->Result = Status;
	Filter->LastErrorValue = LastError;

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
InternetCloseHandleDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_INTERNETCLOSEHANDLE Data;

	if (Record->ProbeOrdinal != _InternetCloseHandle) {
		return S_FALSE;
	}	

	Data = (PWEB_INTERNETCLOSEHANDLE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
					L"hInternet=0x%p, Return=%d", Data->hInternet, Data->Result);

	return S_OK;
}

//
// InternetGetCookie
//

BOOL WINAPI
InternetGetCookieEnter(
	__in   PSTR lpszUrl,
	__in   PSTR lpszCookieName,
	__out  PSTR lpszCookieData,
	__out  LPDWORD lpdwSize
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_INTERNETGETCOOKIE Filter;
	INTERNETGETCOOKIE_ROUTINE Routine;
	BOOL Status;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (INTERNETGETCOOKIE_ROUTINE)Context.Destine;
	Status = (*Routine)(lpszUrl, lpszCookieName, lpszCookieData, lpdwSize);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_INTERNETGETCOOKIE),
		                          WebGuid, _InternetGetCookie);

	Filter = FILTER_RECORD_POINTER(Record, WEB_INTERNETGETCOOKIE);

	if (lpszUrl != NULL) {
		StringCchCopyA(Filter->lpszUrl, 64, lpszUrl);
	}

	if (lpszCookieName != NULL) {
		StringCchCopyA(Filter->lpszCookieName, 64, lpszCookieName);
	}

	if (lpszCookieData != NULL) {
		StringCchCopyA(Filter->lpszCookieData, 64, lpszCookieData);
	}

	if (lpdwSize != NULL) {
		Filter->dwSize = *lpdwSize;
	}

	Filter->lpdwSize = lpdwSize;
	Filter->LastErrorValue = LastError;

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
InternetGetCookieDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_INTERNETGETCOOKIE Data;

	if (Record->ProbeOrdinal != _InternetGetCookie) {
		return S_FALSE;
	}	

	Data = (PWEB_INTERNETGETCOOKIE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"lpszUrl='%S', lpszCookieName='%S', lpszCookieData='%S', *lpdwSize=%d", 
		Data->lpszUrl, Data->lpszCookieName, Data->lpszCookieData, Data->dwSize);

	return S_OK;
}

//
// InternetSetCookie
//

BOOL WINAPI
InternetSetCookieEnter(
	__in  PSTR lpszUrl,
	__in  PSTR lpszCookieName,
	__in  PSTR lpszCookieData
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_INTERNETSETCOOKIE Filter;
	INTERNETSETCOOKIE_ROUTINE Routine;
	BOOL Status;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (INTERNETSETCOOKIE_ROUTINE)Context.Destine;
	Status = (*Routine)(lpszUrl, lpszCookieName, lpszCookieData);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_INTERNETSETCOOKIE),
		                          WebGuid, _InternetSetCookie);

	Filter = FILTER_RECORD_POINTER(Record, WEB_INTERNETSETCOOKIE);

	if (lpszUrl != NULL) {
		StringCchCopyA(Filter->lpszUrl, 64, lpszUrl);
	}

	if (lpszCookieName != NULL) {
		StringCchCopyA(Filter->lpszCookieName, 64, lpszCookieName);
	}

	if (lpszCookieData != NULL) {
		StringCchCopyA(Filter->lpszCookieData, 64, lpszCookieData);
	}

	Filter->LastErrorValue = LastError;
	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
InternetSetCookieDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_INTERNETSETCOOKIE Data;

	if (Record->ProbeOrdinal != _InternetSetCookie) {
		return S_FALSE;
	}	

	Data = (PWEB_INTERNETSETCOOKIE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"lpszUrl='%S', lpszCookieName='%S', lpszCookieData='%S'", 
		Data->lpszUrl, Data->lpszCookieName, Data->lpszCookieData);

	return S_OK;
}

//
// InternetReadFile
//

BOOL WINAPI
InternetReadFileEnter(
	__in   HINTERNET hFile,
	__out  LPVOID lpBuffer,
	__in   DWORD dwNumberOfBytesToRead,
	__out  LPDWORD lpdwNumberOfBytesRead
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_INTERNETREADFILE Filter;
	INTERNETREADFILE_ROUTINE Routine;
	BOOL Status;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (INTERNETREADFILE_ROUTINE)Context.Destine;
	Status = (*Routine)(hFile, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_INTERNETREADFILE),
		                          WebGuid, _InternetReadFile);

	Filter = FILTER_RECORD_POINTER(Record, WEB_INTERNETREADFILE);
	Filter->hFile = hFile;
	Filter->lpBuffer = lpBuffer;
	Filter->dwNumberOfBytesToRead = dwNumberOfBytesToRead;
	Filter->lpdwNumberOfBytesRead = lpdwNumberOfBytesRead;

	if (lpdwNumberOfBytesRead != NULL) {
		Filter->CompleteBytes = *lpdwNumberOfBytesRead;
	}

	Filter->LastErrorValue = LastError;
	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
InternetReadFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_INTERNETREADFILE Data;

	if (Record->ProbeOrdinal != _InternetReadFile) {
		return S_FALSE;
	}	

	Data = (PWEB_INTERNETREADFILE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hFile=0x%p, lpBuffer=0x%p, dwNumberOfBytesToRead=%d, *lpdwNumberOfBytesRead=%d", 
		Data->hFile, Data->lpBuffer, Data->dwNumberOfBytesToRead, Data->CompleteBytes);

	return S_OK;
}

//
// InternetWriteFile
//

BOOL WINAPI
InternetWriteFileEnter(
	__in   HINTERNET hFile,
	__in   LPVOID lpBuffer,
	__in   DWORD dwNumberOfBytesToWrite,
	__out  LPDWORD lpdwNumberOfBytesWritten
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_INTERNETWRITEFILE Filter;
	INTERNETWRITEFILE_ROUTINE Routine;
	BOOL Status;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (INTERNETWRITEFILE_ROUTINE)Context.Destine;
	Status = (*Routine)(hFile, lpBuffer, dwNumberOfBytesToWrite, lpdwNumberOfBytesWritten);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_INTERNETWRITEFILE),
		                          WebGuid, _InternetWriteFile);

	Filter = FILTER_RECORD_POINTER(Record, WEB_INTERNETWRITEFILE);
	Filter->hFile = hFile;
	Filter->lpBuffer = lpBuffer;
	Filter->dwNumberOfBytesToWrite = dwNumberOfBytesToWrite;
	Filter->lpdwNumberOfBytesWritten = lpdwNumberOfBytesWritten;

	if (lpdwNumberOfBytesWritten != NULL) {
		Filter->CompleteBytes = *lpdwNumberOfBytesWritten;
	}

	Filter->LastErrorValue = LastError;
	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
InternetWriteFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_INTERNETWRITEFILE Data;

	if (Record->ProbeOrdinal != _InternetWriteFile) {
		return S_FALSE;
	}	

	Data = (PWEB_INTERNETWRITEFILE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hFile=0x%p, lpBuffer=0x%p, dwNumberOfBytesToWrite=%d, *lpdwNumberOfBytesWritten=%d", 
		Data->hFile, Data->lpBuffer, Data->dwNumberOfBytesToWrite, Data->CompleteBytes);

	return S_OK;
}

//
// HttpOpenRequest
//

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
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_HTTPOPENREQUEST Filter;
	HTTPOPENREQUEST_ROUTINE Routine;
	HINTERNET hResult = NULL;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (HTTPOPENREQUEST_ROUTINE)Context.Destine;
	hResult = (*Routine)(hConnect, lpszVerb, lpszObjectName, lpszVersion,
		                 lpszReferer, lplpszAcceptTypes, dwFlags, dwContext);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_HTTPOPENREQUEST),
		                          WebGuid, _HttpOpenRequest);

	Filter = FILTER_RECORD_POINTER(Record, WEB_HTTPOPENREQUEST);
	Filter->hConnect = hConnect;
	Filter->hResult = hResult;

	if (lpszVerb != NULL) {
		StringCchCopyA(Filter->lpszVerb, 32, lpszVerb);
	}

	if (lpszObjectName != NULL) {
		StringCchCopyA(Filter->lpszObjectName, 32, lpszObjectName);
	}

	if (lpszVersion != NULL) {
		StringCchCopyA(Filter->lpszVersion, 32, lpszVersion);
	}

	if (lpszReferer != NULL) {
		StringCchCopyA(Filter->lpszReferer, 32, lpszReferer);
	}

	Filter->lplpszAcceptTypes = (PSTR)lplpszAcceptTypes;
	Filter->dwFlags = dwFlags;
	Filter->dwContext = dwContext;
	Filter->LastErrorValue = LastError;

	BtrFltQueueRecord(Record);
	return hResult;
}

ULONG
HttpOpenRequestDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_HTTPOPENREQUEST Data;

	if (Record->ProbeOrdinal != _HttpOpenRequest) {
		return S_FALSE;
	}	

	Data = (PWEB_HTTPOPENREQUEST)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hConnect=0x%p, lpszVerb='%S', lpszObjectName='%S', lpszVersion='%S', lpszReferer='%S', lplpAcceptTypes=0x%p, dwFlags=0x%x, dwContext=0x%p", 
		Data->hConnect, Data->lpszVerb, Data->lpszObjectName, Data->lpszVersion, Data->lpszReferer,
		Data->lplpszAcceptTypes, Data->dwFlags, Data->dwContext);

	return S_OK;
}

//
// HttpSendRequest
//

BOOL WINAPI
HttpSendRequestEnter(
	__in  HINTERNET hRequest,
	__in  PSTR lpszHeaders,
	__in  DWORD dwHeadersLength,
	__in  LPVOID lpOptional,
	__in  DWORD dwOptionalLength
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_HTTPSENDREQUEST Filter;
	HTTPSENDREQUEST_ROUTINE Routine;
	BOOL Status;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (HTTPSENDREQUEST_ROUTINE)Context.Destine;
	Status = (*Routine)(hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_HTTPSENDREQUEST),
		                          WebGuid, _HttpSendRequest);

	Filter = FILTER_RECORD_POINTER(Record, WEB_HTTPSENDREQUEST);
	Filter->dwHeadersLength = dwHeadersLength;
	Filter->lpOptional = lpOptional;
	Filter->dwOptionalLength = dwOptionalLength;

	if (lpszHeaders != NULL) {
		StringCchCopyA(Filter->lpszHeaders, 64, lpszHeaders);
	}

	Filter->LastErrorValue = LastError;
	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
HttpSendRequestDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_HTTPSENDREQUEST Data;

	if (Record->ProbeOrdinal != _HttpSendRequest) {
		return S_FALSE;
	}	

	Data = (PWEB_HTTPSENDREQUEST)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hRequest=0x%p, lpszHeaders='%S', dwHeadersLength=%d, lpOptional=0x%p, dwOptionalLength=%d", 
		Data->hRequest, Data->lpszHeaders, Data->dwHeadersLength, Data->lpOptional, Data->dwOptionalLength);

	return S_OK;
}

//
// HttpQueryInfo
//

BOOL WINAPI 
HttpQueryInfoEnter(
	__in  HINTERNET hRequest,
	__in  DWORD dwInfoLevel,
	__in  LPVOID lpvBuffer,
	__in  LPDWORD lpdwBufferLength,
	__in  LPDWORD lpdwIndex
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_HTTPQUERYINFO Filter;
	HTTPQUERYINFO_ROUTINE Routine;
	BOOL Status;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (HTTPQUERYINFO_ROUTINE)Context.Destine;
	Status = (*Routine)(hRequest, dwInfoLevel, lpvBuffer, lpdwBufferLength, lpdwIndex);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_HTTPQUERYINFO),
		                          WebGuid, _HttpQueryInfo);

	Filter = FILTER_RECORD_POINTER(Record, WEB_HTTPQUERYINFO);
	Filter->hRequest = hRequest;
	Filter->dwInfoLevel = dwInfoLevel;
	Filter->lpvBuffer = lpvBuffer;

	Filter->lpdwBufferLength = lpdwBufferLength;
	if (lpdwBufferLength) {
		Filter->dwBufferLength = *lpdwBufferLength;
	}

	Filter->lpdwIndex = lpdwIndex;
	if (lpdwIndex) {
		Filter->dwIndex = *lpdwIndex;
	}

	Filter->LastErrorValue = LastError;
	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
HttpQueryInfoDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_HTTPQUERYINFO Data;

	if (Record->ProbeOrdinal != _HttpQueryInfo) {
		return S_FALSE;
	}	

	Data = (PWEB_HTTPQUERYINFO)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hRequest=0x%p, dwInfoLevel=%d, lpvBuffer=0x%p, *lpdwBufferLength=%d, *lpdwIndex=%d", 
		Data->hRequest, Data->dwInfoLevel, Data->lpvBuffer, Data->dwBufferLength, Data->dwIndex);

	return S_OK;
}

//
// HttpAddRequestHeaders
//

BOOL WINAPI 
HttpAddRequestHeadersEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszHeaders,
	__in  DWORD dwHeadersLength,
	__in  DWORD dwModifiers
	)
{
	
	PBTR_FILTER_RECORD Record;
	PWEB_HTTPADDREQUESTHEADERS Filter;
	HTTPADDREQUESTHEADERS_ROUTINE Routine;
	BOOL Status;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (HTTPADDREQUESTHEADERS_ROUTINE)Context.Destine;
	Status = (*Routine)(hConnect, lpszHeaders, dwHeadersLength, dwModifiers);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_HTTPADDREQUESTHEADERS),
		                          WebGuid, _HttpAddRequestHeaders);

	Filter = FILTER_RECORD_POINTER(Record, WEB_HTTPADDREQUESTHEADERS);
	Filter->hConnect = hConnect;

	if (lpszHeaders != NULL) {
		StringCchCopyA(Filter->lpszHeaders, 64, lpszHeaders);
	}

	Filter->dwHeadersLength = dwHeadersLength;
	Filter->dwModifiers = dwModifiers;
	Filter->Status = Status;
	Filter->LastErrorValue = LastError;

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
HttpAddRequestHeadersDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_HTTPADDREQUESTHEADERS Data;
		
	if (Record->ProbeOrdinal != _HttpAddRequestHeaders) {
		return S_FALSE;
	}	

	Data = (PWEB_HTTPADDREQUESTHEADERS)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hConnect=0x%p, lpszHeaders='%S', dwHeadersLength=%d, dwModifiers=%d", 
		Data->hConnect, Data->lpszHeaders, Data->dwHeadersLength, Data->dwModifiers);

	return S_OK;
}

//
// FtpCommand
//

BOOL WINAPI 
FtpCommandEnter(
	__in  HINTERNET hConnect,
	__in  BOOL fExpectResponse,
	__in  DWORD dwFlags,
	__in  PSTR lpszCommand,
	__in  DWORD_PTR dwContext,
	__out HINTERNET* phFtpCommand
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_FTPCOMMAND Filter;
	FTPCOMMAND_ROUTINE Routine;
	BOOL Status;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (FTPCOMMAND_ROUTINE)Context.Destine;
	Status = (*Routine)(hConnect, fExpectResponse, dwFlags, 
		                lpszCommand, dwContext, phFtpCommand);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_FTPCOMMAND),
		                          WebGuid, _FtpCommand);

	Filter = FILTER_RECORD_POINTER(Record, WEB_FTPCOMMAND);
	Filter->hConnect = hConnect;
	Filter->fExpectResponse = fExpectResponse;
	Filter->dwFlags = dwFlags;

	if (lpszCommand != NULL) {
		StringCchCopyA(Filter->lpszCommand, 32, lpszCommand);
	}

	Filter->dwContext = dwContext;
	Filter->LastErrorValue = LastError;
	Filter->Status = Status;

	//
	// N.B. Refers to MSDN 
	//

	if (fExpectResponse && phFtpCommand != NULL) {
		Filter->phFtpCommand = *phFtpCommand;
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
FtpCommandDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_FTPCOMMAND Data;

	if (Record->ProbeOrdinal != _FtpCommand) {
		return S_FALSE;
	}	

	Data = (PWEB_FTPCOMMAND)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hConnect=0x%p, fExpectResponse=%d, dwFlags=0x%x, lpszCommand='%S', dwContext=0x%p, phFtpCommand=0x%p", 
		Data->hConnect, Data->fExpectResponse, Data->dwFlags, Data->lpszCommand, Data->dwContext, Data->phFtpCommand);

	return S_OK;
}

//
// FtpCreateDirectory
//

BOOL WINAPI
FtpCreateDirectoryEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszDirectory
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_FTPCREATEDIRECTORY Filter;
	FTPCREATEDIRECTORY_ROUTINE Routine;
	BOOL Status;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (FTPCREATEDIRECTORY_ROUTINE)Context.Destine;
	Status = (*Routine)(hConnect, lpszDirectory);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_FTPCREATEDIRECTORY),
		                          WebGuid, _FtpCreateDirectory);

	Filter = FILTER_RECORD_POINTER(Record, WEB_FTPCREATEDIRECTORY);
	Filter->hConnect = hConnect;
	Filter->Status = Status;
	Filter->LastErrorValue = LastError;

	if (lpszDirectory != NULL) {
		StringCchCopyA(Filter->lpszDirectory, MAX_PATH, lpszDirectory);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
FtpCreateDirectoryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_FTPCREATEDIRECTORY Data;

	if (Record->ProbeOrdinal != _FtpCreateDirectory) {
		return S_FALSE;
	}	

	Data = (PWEB_FTPCREATEDIRECTORY)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hConnect=0x%p, lpszDirectory='%S'", Data->hConnect, Data->lpszDirectory);

	return S_OK;
}

//
// FtpDeleteFile
//

BOOL WINAPI
FtpDeleteFileEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszFileName
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_FTPDELETEFILE Filter;
	FTPDELETEFILE_ROUTINE Routine;
	BOOL Status;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (FTPDELETEFILE_ROUTINE)Context.Destine;
	Status = (*Routine)(hConnect, lpszFileName);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_FTPDELETEFILE),
		                          WebGuid, _FtpDeleteFile);

	Filter = FILTER_RECORD_POINTER(Record, WEB_FTPDELETEFILE);
	Filter->hConnect = hConnect;
	Filter->Status = Status;
	Filter->LastErrorValue = LastError;

	if (lpszFileName != NULL) {
		StringCchCopyA(Filter->lpszFileName, MAX_PATH, lpszFileName);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
FtpDeleteFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_FTPDELETEFILE Data;

	if (Record->ProbeOrdinal != _FtpDeleteFile) {
		return S_FALSE;
	}	

	Data = (PWEB_FTPDELETEFILE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hConnect=0x%p, lpszFileName='%S'", Data->hConnect, Data->lpszFileName);

	return S_OK;
}

//
// FtpFindFirstFile
//

HINTERNET WINAPI
FtpFindFirstFileEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszSearchFile,
	__out LPWIN32_FIND_DATA lpFindFileData,
	__in  DWORD dwFlags,
	__in  DWORD_PTR dwContext
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_FTPFINDFIRSTFILE Filter;
	FTPFINDFIRSTFILE_ROUTINE Routine;
	HINTERNET hResult;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (FTPFINDFIRSTFILE_ROUTINE)Context.Destine;
	hResult = (*Routine)(hConnect, lpszSearchFile, lpFindFileData, dwFlags, dwContext);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_FTPFINDFIRSTFILE),
		                          WebGuid, _FtpFindFirstFile);

	Filter = FILTER_RECORD_POINTER(Record, WEB_FTPFINDFIRSTFILE);
	Filter->hConnect = hConnect;
	Filter->dwFlags = dwFlags;
	Filter->dwContext = dwContext;
	Filter->hResult = hResult;
	Filter->LastErrorValue = LastError;

	if (lpszSearchFile != NULL) {
		StringCchCopyA(Filter->lpszSearchFile, MAX_PATH, lpszSearchFile);
	}

	if (lpFindFileData != NULL) {
		Filter->lpFindFileData = *lpFindFileData;
	}

	BtrFltQueueRecord(Record);
	return hResult;
}

ULONG
FtpFindFirstFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_FTPFINDFIRSTFILE Data;

	if (Record->ProbeOrdinal != _FtpFindFirstFile) {
		return S_FALSE;
	}	

	Data = (PWEB_FTPFINDFIRSTFILE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hConnect=0x%p, lpszSearchFile='%S', lpFindFileData.cFileName='%S', dwFlags=0x%x, dwContext=0x%p", 
		Data->hConnect, Data->lpszSearchFile, Data->lpFindFileData.cFileName, Data->dwFlags, Data->dwContext);

	return S_OK;
}

//
// FtpGetCurrentDirectory 
//

BOOL WINAPI
FtpGetCurrentDirectoryEnter(
	__in   HINTERNET hConnect,
	__out  PSTR lpszCurrentDirectory,
	__out  LPDWORD lpdwCurrentDirectory
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_FTPGETCURRENTDIRECTORY Filter;
	FTPGETCURRENTDIRECTORY_ROUTINE Routine;
	BOOL Status;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (FTPGETCURRENTDIRECTORY_ROUTINE)Context.Destine;
	Status = (*Routine)(hConnect, lpszCurrentDirectory, lpdwCurrentDirectory);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_FTPGETCURRENTDIRECTORY),
		                          WebGuid, _FtpGetCurrentDirectory);

	Filter = FILTER_RECORD_POINTER(Record, WEB_FTPGETCURRENTDIRECTORY);
	Filter->hConnect = hConnect;
	Filter->LastErrorValue = LastError;
	Filter->Status = Status;

	if (lpszCurrentDirectory != NULL) {
		StringCchCopyA(Filter->lpszCurrentDirectory, MAX_PATH, lpszCurrentDirectory);
	}

	if (lpdwCurrentDirectory != NULL) {
		Filter->lpdwCurrentDirectory = *lpdwCurrentDirectory;
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
FtpGetCurrentDirectoryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_FTPGETCURRENTDIRECTORY Data;
		
	if (Record->ProbeOrdinal != _FtpGetCurrentDirectory) {
		return S_FALSE;
	}	

	Data = (PWEB_FTPGETCURRENTDIRECTORY)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hConnect=0x%p, lpszCurrentDirectory='%S', *lpdwCurrentDirectory=%d", Data->hConnect, Data->lpszCurrentDirectory, Data->lpdwCurrentDirectory);

	return S_OK;
}

//
// FtpGetFile
//

BOOL WINAPI
FtpGetFileEnter(
	__in HINTERNET hConnect,
	__in PSTR lpszRemoteFile,
	__in PSTR lpszNewFile,
	__in BOOL fFailIfExists,
	__in DWORD dwFlagsAndAttributes,
	__in DWORD dwFlags,
	__in DWORD_PTR dwContext
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_FTPGETFILE Filter;
	FTPGETFILE_ROUTINE Routine;
	BOOL Status;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (FTPGETFILE_ROUTINE)Context.Destine;
	Status = (*Routine)(hConnect, lpszRemoteFile, lpszNewFile, fFailIfExists, 
		                dwFlagsAndAttributes, dwFlags, dwContext);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_FTPGETFILE),
		                          WebGuid, _FtpGetFile);

	Filter = FILTER_RECORD_POINTER(Record, WEB_FTPGETFILE);
	Filter->hConnect = hConnect;
	Filter->fFailIfExists = fFailIfExists;
	Filter->dwFlagsAndAttributes = dwFlagsAndAttributes;
	Filter->dwFlags = dwFlags;
	Filter->dwContext = dwContext;
	Filter->Status = Status;
	Filter->LastErrorValue = LastError;

	if (lpszRemoteFile != NULL) {
		StringCchCopyA(Filter->lpszRemoteFile, MAX_PATH, lpszRemoteFile);
	}

	if (lpszNewFile != NULL) {
		StringCchCopyA(Filter->lpszNewFile, MAX_PATH, lpszNewFile);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
FtpGetFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_FTPGETFILE Data;

	if (Record->ProbeOrdinal != _FtpGetFile) {
		return S_FALSE;
	}	

	Data = (PWEB_FTPGETFILE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hConnect=0x%p, lpszRemoteFile='%S', lpszNewFile='%S', fFailIfExists=%d, dwFlagsAndAttributes=0x%x, dwFlag=0x%x, dwContext=0x%p", 
		Data->hConnect, Data->lpszRemoteFile, Data->lpszNewFile, Data->fFailIfExists, Data->dwFlagsAndAttributes, Data->dwFlags, Data->dwContext);

	return S_OK;
}

//
// FtpOpenFile
//

HINTERNET WINAPI
FtpOpenFileEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszFileName,
	__in  DWORD dwAccess,
	__in  DWORD dwFlags,
	__in  DWORD_PTR dwContext
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_FTPOPENFILE Filter;
	FTPOPENFILE_ROUTINE Routine;
	HINTERNET hResult;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (FTPOPENFILE_ROUTINE)Context.Destine;
	hResult = (*Routine)(hConnect, lpszFileName, dwAccess, dwFlags, dwContext);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_FTPOPENFILE),
		                          WebGuid, _FtpOpenFile);

	Filter = FILTER_RECORD_POINTER(Record, WEB_FTPOPENFILE);
	Filter->hConnect = hConnect;
	Filter->dwAccess = dwAccess;
	Filter->dwFlags = dwFlags;
	Filter->dwContext = dwContext;
	Filter->hResult = hResult;
	Filter->LastErrorValue = LastError;

	if (lpszFileName != NULL) {
		StringCchCopyA(Filter->lpszFileName, MAX_PATH, lpszFileName);
	}

	BtrFltQueueRecord(Record);
	return hResult;
}

ULONG
FtpOpenFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_FTPOPENFILE Data;

	if (Record->ProbeOrdinal != _FtpOpenFile) {
		return S_FALSE;
	}	

	Data = (PWEB_FTPOPENFILE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hConnect=0x%p, lpszFileName='%S', dwAccess=0x%x, dwFlag=0x%x, dwContext=0x%p", 
		Data->hConnect, Data->lpszFileName, Data->dwAccess, Data->dwFlags, Data->dwContext);

	return S_OK;
}

//
// FtpPutFile
//

BOOL WINAPI
FtpPutFileEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszLocalFile,
	__in  PSTR lpszNewRemoteFile,
	__in  DWORD dwFlags,
	__in  DWORD_PTR dwContext
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_FTPPUTFILE Filter;
	FTPPUTFILE_ROUTINE Routine;
	BOOL Status;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (FTPPUTFILE_ROUTINE)Context.Destine;
	Status = (*Routine)(hConnect, lpszLocalFile, lpszNewRemoteFile, dwFlags, dwContext);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_FTPPUTFILE),
		                          WebGuid, _FtpPutFile);

	Filter = FILTER_RECORD_POINTER(Record, WEB_FTPPUTFILE);
	Filter->hConnect = hConnect;
	Filter->dwFlags = dwFlags;
	Filter->dwContext = dwContext;
	Filter->LastErrorValue = LastError;
	Filter->Status = Status;

	if (lpszLocalFile != NULL) {
		StringCchCopyA(Filter->lpszLocalFile, MAX_PATH, lpszLocalFile);
	}

	if (lpszNewRemoteFile != NULL) {
		StringCchCopyA(Filter->lpszNewRemoteFile, MAX_PATH, lpszNewRemoteFile);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
FtpPutFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_FTPPUTFILE Data;

	if (Record->ProbeOrdinal != _FtpPutFile) {
		return S_FALSE;
	}	

	Data = (PWEB_FTPPUTFILE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hConnect=0x%p, lpszLocalFile='%S', lpszNewRemoteFile='%S', dwFlag=0x%x, dwContext=0x%p", 
		Data->hConnect, Data->lpszLocalFile, Data->lpszNewRemoteFile, Data->dwFlags, Data->dwContext);

	return S_OK;
}

//
// FtpRemoveDirectory
//

BOOL WINAPI
FtpRemoveDirectoryEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszDirectory
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_FTPREMOVEDIRECTORY Filter;
	FTPREMOVEDIRECTORY_ROUTINE Routine;
	BOOL Status;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (FTPREMOVEDIRECTORY_ROUTINE)Context.Destine;
	Status = (*Routine)(hConnect, lpszDirectory);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_FTPREMOVEDIRECTORY),
		                          WebGuid, _FtpRemoveDirectory);

	Filter = FILTER_RECORD_POINTER(Record, WEB_FTPREMOVEDIRECTORY);
	Filter->hConnect = hConnect;
	Filter->LastErrorValue = LastError;
	Filter->Status = Status;

	if (lpszDirectory != NULL) {
		StringCchCopyA(Filter->lpszDirectory, MAX_PATH, lpszDirectory);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
FtpRemoveDirectoryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_FTPREMOVEDIRECTORY Data;

	if (Record->ProbeOrdinal != _FtpRemoveDirectory) {
		return S_FALSE;
	}	

	Data = (PWEB_FTPREMOVEDIRECTORY)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hConnect=0x%p, lpszDirectory='%S'", Data->hConnect, Data->lpszDirectory);

	return S_OK;
}

//
// FtpRenameFile
//

BOOL WINAPI
FtpRenameFileEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszExisting,
	__in  PSTR lpszNew
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_FTPRENAMEFILE Filter;
	FTPRENAMEFILE_ROUTINE Routine;
	BOOL Status;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (FTPRENAMEFILE_ROUTINE)Context.Destine;
	Status = (*Routine)(hConnect, lpszExisting, lpszNew);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_FTPRENAMEFILE),
		                          WebGuid, _FtpRenameFile);

	Filter = FILTER_RECORD_POINTER(Record, WEB_FTPRENAMEFILE);
	Filter->hConnect = hConnect;
	Filter->LastErrorValue = LastError;
	Filter->Status = Status;

	if (lpszExisting != NULL) {
		StringCchCopyA(Filter->lpszExisting, MAX_PATH, lpszExisting);
	}

	if (lpszNew != NULL) {
		StringCchCopyA(Filter->lpszNew, MAX_PATH, lpszNew);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
FtpRenameFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_FTPRENAMEFILE Data;
		
	if (Record->ProbeOrdinal != _FtpRenameFile) {
		return S_FALSE;
	}	

	Data = (PWEB_FTPRENAMEFILE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hConnect=0x%p, lpszExisting='%S', lpszNew='%S'", Data->hConnect, Data->lpszExisting, Data->lpszNew);

	return S_OK;
}

//
// FtpSetCurrentDirectory
//

BOOL WINAPI
FtpSetCurrentDirectoryEnter(
	__in  HINTERNET hConnect,
	__in  PSTR lpszDirectory
	)
{
	PBTR_FILTER_RECORD Record;
	PWEB_FTPSETCURRENTDIRECTORY Filter;
	FTPSETCURRENTDIRECTORY_ROUTINE Routine;
	BOOL Status;
	BTR_PROBE_CONTEXT Context;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (FTPSETCURRENTDIRECTORY_ROUTINE)Context.Destine;
	Status = (*Routine)(hConnect, lpszDirectory);
	LastError = GetLastError();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(WEB_FTPSETCURRENTDIRECTORY),
		                          WebGuid, _FtpSetCurrentDirectory);

	Filter = FILTER_RECORD_POINTER(Record, WEB_FTPSETCURRENTDIRECTORY);
	Filter->hConnect = hConnect;
	Filter->LastErrorValue = LastError;
	Filter->Status = Status;

	if (lpszDirectory != NULL) {
		StringCchCopyA(Filter->lpszDirectory, MAX_PATH, lpszDirectory);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
FtpSetCurrentDirectoryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PWEB_FTPSETCURRENTDIRECTORY Data;

	if (Record->ProbeOrdinal != _FtpSetCurrentDirectory) {
		return S_FALSE;
	}	

	Data = (PWEB_FTPSETCURRENTDIRECTORY)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"hConnect=0x%p, lpszDirectory='%S'", Data->hConnect, Data->lpszDirectory);

	return S_OK;
}