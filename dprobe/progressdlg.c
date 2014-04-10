//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "dprobe.h"
#include "progressdlg.h"

BOOLEAN
ProgressDialogCreate(
    IN PPROGRESS_DIALOG_CONTEXT Context
	)
{
	DIALOG_OBJECT Object = {0};

    ASSERT(IsWindow(Context->hWndOwner));

	Object.Context = (PVOID)Context;
	Object.hWndParent = Context->hWndOwner;
	Object.ResourceId = IDD_DIALOG_PROGRESS;
	Object.Procedure = ProgressProcedure;
	Object.Scaler = NULL;

	DialogCreate(&Object);
	return TRUE;
}

INT_PTR CALLBACK
ProgressProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	switch (uMsg) {
		case WM_INITDIALOG: 
			return ProgressOnInitDialog(hWnd, uMsg, wp, lp);
		case WM_COMMAND:
			return ProgressOnCommand(hWnd, uMsg, wp, lp);
		case WM_TIMER:
			return ProgressOnTimer(hWnd, uMsg, wp, lp);
	}

	return 0;
}

LRESULT
ProgressOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	switch (LOWORD(wp)) {

	case IDCANCEL:
		ProgressOnCancel(hWnd, uMsg, wp, lp);
	    break;
	}

	return 0;
}

LRESULT
ProgressOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndBar;
	ULONG ThreadId;
    PDIALOG_OBJECT Object;
    PPROGRESS_DIALOG_CONTEXT Context;

	hWndBar = GetDlgItem(hWnd, IDC_PROGRESS);
	SendMessage(hWndBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	SendMessage(hWndBar, PBM_SETSTEP, 1, 0); 
	
	SetWindowText(hWnd, L"D Probe");

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PPROGRESS_DIALOG_CONTEXT)Object->Context;

    Context->hWndDialog = hWnd;
	SetDlgItemText(hWnd, IDC_STATIC, Context->InitText);
	
    SetTimer(hWnd, 1, 1000, NULL);
    Context->TimerId = 1;

	CreateThread(NULL, 0, ProgressDialogThread, Context, 0, &ThreadId);
	SdkCenterWindow(hWnd);
	return 0;
}

ULONG CALLBACK
ProgressDialogThread(
	IN PVOID Context
	)
{
    HWND hWnd;
	HWND hWndText;
	PPROGRESS_DIALOG_CONTEXT DialogContext;
	DialogContext = (PPROGRESS_DIALOG_CONTEXT)Context;
    hWnd = DialogContext->hWndDialog;

	__try {
		(*DialogContext->Callback)(hWnd, DialogContext->CallbackContext);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		hWndText = GetDlgItem(hWnd, IDC_STATIC);
		SetWindowText(hWndText, L"Unexpected problem occurred, the operation aborted.");
		EndDialog(hWnd, IDCANCEL);
	}

	return 0;
}

LRESULT
ProgressOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
    PDIALOG_OBJECT Object;
    PPROGRESS_DIALOG_CONTEXT Context;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PPROGRESS_DIALOG_CONTEXT)Object->Context;

	BspAcquireLock(&Context->ObjectLock);
	Context->Cancel = TRUE;
	BspReleaseLock(&Context->ObjectLock);

	KillTimer(hWnd, Context->TimerId);
    Context->TimerId = 0;

	return 0;
}

LRESULT
ProgressOnTimer(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndBar;
    BOOL Cancel;
    BOOL Complete;
    ULONG LastPercent;
    ULONG CurrentPercent;
    WCHAR Buffer[MAX_PATH];
    PDIALOG_OBJECT Object;
    PPROGRESS_DIALOG_CONTEXT Context;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PPROGRESS_DIALOG_CONTEXT)Object->Context;

	BspAcquireLock(&Context->ObjectLock);

    CurrentPercent = Context->CurrentPercent; 
    LastPercent = Context->LastPercent;
    Context->LastPercent = CurrentPercent;
    StringCchCopy(Buffer, MAX_PATH - 1, Context->CurrentText);
    Cancel = Context->Cancel;
    Complete = Context->Complete;

	BspReleaseLock(&Context->ObjectLock);

    if (CurrentPercent > LastPercent) {
    	hWndBar = GetDlgItem(hWnd, IDC_PROGRESS);
    	SetDlgItemText(hWnd, IDC_STATIC, Buffer);
    	SendMessage(hWndBar, PBM_SETPOS, CurrentPercent, 0);
    }

    if (Complete) {
        KillTimer(hWnd, Context->TimerId);
        EndDialog(hWnd, IDCANCEL);
    }

    if (Cancel) {
        if (Context->TimerId != 0) {
            KillTimer(hWnd, Context->TimerId);
        }
        EndDialog(hWnd, IDCANCEL);
    }

	return 0;
}

BOOL CALLBACK
ProgressDialogCallback(
	IN HWND hWnd,
    IN ULONG Percent,
    IN PWSTR CurrentText,
    IN BOOL Cancel,
    IN BOOL Complete
	)
{
	BOOL UserCancel;
    PDIALOG_OBJECT Object;
    PPROGRESS_DIALOG_CONTEXT Context;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
    Context = (PPROGRESS_DIALOG_CONTEXT)Object->Context;

	BspAcquireLock(&Context->ObjectLock);

    Context->CurrentPercent = Percent;
    StringCchCopy(Context->CurrentText, MAX_PATH - 1, CurrentText);
	UserCancel = Context->Cancel;
    Context->Cancel = Cancel;
    Context->Complete = Complete;

	BspReleaseLock(&Context->ObjectLock);
	return UserCancel;
}