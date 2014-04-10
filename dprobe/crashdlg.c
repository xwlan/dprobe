//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#include "dprobe.h"
#include "crashdlg.h"
#include "dialog.h"
#include <richedit.h>

typedef struct _CRASH_CONTEXT {
	CRASH_ACTION Action;
} CRASH_CONTEXT, *PCRASH_CONTEXT;

ULONG
CrashDialog(
	IN HWND hWndParent
	)
{
	DIALOG_OBJECT Object = {0};
	CRASH_CONTEXT Context = {0};

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_CRASH;
	Object.Procedure = CrashProcedure;
	
	DialogCreate(&Object);
	return Context.Action;
}

INT_PTR CALLBACK
CrashProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LRESULT Status = 0;

	switch (uMsg) {

		case WM_INITDIALOG:
			return CrashOnInitDialog(hWnd, uMsg, wp, lp);			

		case WM_COMMAND:
			return CrashOnCommand(hWnd, uMsg, wp, lp);
	}
	
	return Status;
}

LRESULT
CrashOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndEdit;
	HRSRC hrsrc;
	HGLOBAL hEula;
	EDITSTREAM Stream = {0};

	hrsrc = FindResource(SdkInstance, MAKEINTRESOURCE(IDR_EULA2), L"EULA");
	ASSERT(hrsrc != NULL);

	hEula = LoadResource(SdkInstance, hrsrc);
	ASSERT(hEula != NULL);

	hWndEdit = GetDlgItem(hWnd, IDC_RICHEDIT);

	Stream.dwCookie = (DWORD_PTR)hEula;
	Stream.dwError = 0;
	Stream.pfnCallback = CrashStreamCallback;
	SendMessage(hWndEdit, EM_STREAMIN, SF_RTF, (LPARAM)&Stream);
	
	SetWindowText(hWnd, L"D Probe");
	SdkSetMainIcon(hWnd);
	SdkCenterWindow(hWnd);
	return TRUE;
}

LRESULT
CrashOnCommand(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCRASH_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, CRASH_CONTEXT);

	switch (LOWORD(wp)) {
		case IDC_MINIDUMP:
			Context->Action = CrashMiniDump;
			break;
		case IDC_FULLDUMP:
			Context->Action = CrashFullDump;
			break;
		case IDC_NODUMP:
			Context->Action = CrashNoDump;
			break;
		default:
			return 0;
	}

	EndDialog(hWnd, IDOK);
	return TRUE;
}

DWORD CALLBACK
CrashStreamCallback(
	IN DWORD_PTR dwCookie,
    IN LPBYTE pbBuff,
    IN LONG cb,
    OUT LONG *pcb
	)
{
	PCHAR Eula;

	Eula = (PCHAR)dwCookie;
	memcpy(pbBuff, Eula, cb);

	*pcb = cb;
	return 0;
}
