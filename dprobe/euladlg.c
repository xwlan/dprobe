//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#include "dprobe.h"
#include "euladlg.h"
#include "dialog.h"
#include <richedit.h>

INT_PTR
EulaDialog(
	IN HWND hWndParent
	)
{
	DIALOG_OBJECT Object = {0};
	INT_PTR Return;

	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_EULA;
	Object.Procedure = EulaProcedure;
	
	Return = DialogCreate(&Object);
	return Return;
}

INT_PTR CALLBACK
EulaProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LRESULT Status = 0;

	switch (uMsg) {

		case WM_INITDIALOG:
			return EulaOnInitDialog(hWnd, uMsg, wp, lp);			

		case WM_COMMAND:
			if (LOWORD(wp) == IDOK) { 
				EndDialog(hWnd, IDOK);
			}
			else if (LOWORD(wp) == IDCANCEL) { 
				EndDialog(hWnd, IDCANCEL);
			}
			break;
	}
	
	return Status;
}

LRESULT
EulaOnInitDialog(
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

	hrsrc = FindResource(SdkInstance, MAKEINTRESOURCE(IDR_EULA1), L"EULA");
	ASSERT(hrsrc != NULL);

	hEula = LoadResource(SdkInstance, hrsrc);
	ASSERT(hEula != NULL);

	hWndEdit = GetDlgItem(hWnd, IDC_RICHEDIT);

	Stream.dwCookie = (DWORD_PTR)hEula;
	Stream.dwError = 0;
	Stream.pfnCallback = EulaStreamCallback;
	SendMessage(hWndEdit, EM_STREAMIN, SF_RTF, (LPARAM)&Stream);
	
	SetWindowText(hWnd, L"EULA");
	SdkSetMainIcon(hWnd);
	SdkCenterWindow(hWnd);
	return TRUE;
}

DWORD CALLBACK
EulaStreamCallback(
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