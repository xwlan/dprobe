//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "sdk.h"
#include "dprobe.h"
#include "crashdlg.h"
#include "main.h"

//
// Previous exception filter
//

PTOP_LEVEL_EXCEPTION_FILTER SdkPreviousExceptionFilter;

//
// Unhandled exception filter of runtime, note that
// we need to dispatch to previous registered filter
// if there's one
//

LONG WINAPI 
SdkUnhandledExceptionFilter(
	__in struct _EXCEPTION_POINTERS* Pointers 
	)
{
	WCHAR Path[MAX_PATH];
	WCHAR Dump[MAX_PATH];
	SYSTEMTIME Time;
	BOOLEAN Full;
	CRASH_ACTION Action;
	
	GetCurrentDirectory(MAX_PATH, Path);

	GetLocalTime(&Time);
	StringCchPrintf(Dump, MAX_PATH, L"%s\\Date_%04u-%02u-%02u_Time_%02u-%02u-%02u_%s.dmp", 
					Path, Time.wYear, Time.wMonth, Time.wDay, Time.wHour, Time.wMinute, Time.wSecond, L"dprobe");

	Action = (CRASH_ACTION)CrashDialog(hWndMain);

	if (Action == CrashNoDump) {
		return EXCEPTION_EXECUTE_HANDLER;
	}
	
	if (Action == CrashMiniDump) {
		Full = FALSE;
	} else {
		Full = TRUE;
	}

	BspCaptureMemoryDump(GetCurrentProcessId(), Full, Dump, 
		                 Pointers, TRUE);
	return EXCEPTION_EXECUTE_HANDLER;
}

VOID
SdkSetUnhandledExceptionFilter(
	VOID
	)
{
	SdkPreviousExceptionFilter = SetUnhandledExceptionFilter(SdkUnhandledExceptionFilter);
	return;
}