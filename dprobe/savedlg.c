//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "dprobe.h"
#include "savedlg.h"
#include "mspdtl.h"

#define WM_USER_SAVE  WM_USER + 2

INT_PTR
SaveDialog(
	IN HWND hWndParent,
	IN struct _MSP_DTL_OBJECT *DtlObject,
	IN PWSTR Path,
	IN MSP_SAVE_TYPE Type
	)
{
	DIALOG_OBJECT Object = {0};
	PSAVE_CONTEXT Context;
	INT_PTR Return;
	
	//
	// N.B. We allocate context from heap because the dialog
	// can be destroyed before the context is actually freed,
	// if user aborts, the dialog is destroyed immediately,
	// however, the saving callback may still be called by
	// dtl saving thread, callback will examine it's state,
	// if dialog is destroyed, the context is freed in callback
	// routine.
	//

	Context = (PSAVE_CONTEXT)SdkMalloc(sizeof(SAVE_CONTEXT));
	ZeroMemory(Context, sizeof(SAVE_CONTEXT));

	Context->Type = Type;
	Context->DtlObject = DtlObject;
	StringCchCopy(Context->Path, MAX_PATH, Path);

	Object.Context = Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_SAVE;
	Object.Procedure = SaveProcedure;

	Return = DialogCreate(&Object);
	return Return;
}

LRESULT
SaveOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LRESULT Status = 0;

	switch (LOWORD(wp)) {
	case IDCANCEL:
		if (HIWORD(wp) == 0) {
			Status = SaveOnCancel(hWnd, uMsg, wp, lp);
		}
	    break;
	}

	return Status;
}

INT_PTR CALLBACK
SaveProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	INT_PTR Status = 0;

	switch (uMsg) {
		
		case WM_INITDIALOG:
			return SaveOnInitDialog(hWnd, uMsg, wp, lp);
		
		case WM_COMMAND:
			return SaveOnCommand(hWnd, uMsg, wp, lp);
		
		case WM_USER_SAVE:
			return SaveOnUserSave(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
SaveOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndBar;
	HWND hWndText;
	ULONG Count;
	PDIALOG_OBJECT Object;
	PSAVE_CONTEXT Context;
	PMSP_DTL_OBJECT DtlObject;
	PMSP_FILTER_FILE_OBJECT FileObject;
	WCHAR Buffer[MAX_PATH];

	hWndBar = GetDlgItem(hWnd, IDC_PROGRESS);
	SendMessage(hWndBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	SendMessage(hWndBar, PBM_SETSTEP, 10, 0); 
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, SAVE_CONTEXT);
	Context->hWnd = hWnd;
	Context->State = SaveStarted;

	DtlObject = Context->DtlObject;	
	FileObject = MspGetFilterFileObject(DtlObject);

	if (FileObject != NULL) {

		StringCchCopy(Context->Text, MAX_PATH, L"Saving %%%u of %u filtered records ...");
		Count = MspGetFilteredRecordCount(FileObject);
		Context->Count = Count;

		MspSaveFile(Context->DtlObject, Context->Path, Context->Type, 
		            Count, TRUE, SaveCallback, (PVOID)Context);


	} else {

		StringCchCopy(Context->Text, MAX_PATH, L"Saving %%%u of %u records ...");
		Count = MspGetRecordCount(DtlObject);
		Context->Count = Count;

		MspSaveFile(Context->DtlObject, Context->Path, Context->Type, 
		            Count, FALSE, SaveCallback, (PVOID)Context);

	}

	StringCchPrintf(Buffer, MAX_PATH, Context->Text, 0, Count);
	hWndText = GetDlgItem(hWnd, IDC_STATIC);
	SetWindowText(hWndText, Buffer);

	SetWindowText(hWnd, L"D Probe");
	SdkCenterWindow(hWnd);
	return TRUE;
}

LRESULT
SaveOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PSAVE_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PSAVE_CONTEXT)Object->Context;

	if (Context->State == SaveMspAborted) {

		//
		// This is error case, MSP aborted, user
		// click CANCEL button.
		//

		SdkFree(Context);

	} else {

		//
		// User click CANCEL, but MSP is still working,
		// leave context object to be freed by callback
		//

		Context->State = SaveUserAborted;
	}

	EndDialog(hWnd, IDCANCEL);
	return TRUE;
}


LRESULT
SaveOnUserSave(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PSAVE_CONTEXT Context;
	PSAVE_NOTIFICATION Notification;
	HWND hWndBar;
	HWND hWndText;
	WCHAR Buffer[MAX_PATH];

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PSAVE_CONTEXT)Object->Context;
	hWndBar = GetDlgItem(hWnd, IDC_PROGRESS);
	hWndText = GetDlgItem(hWnd, IDC_STATIC);

	Notification = (PSAVE_NOTIFICATION)wp;

	if (!Notification->Aborted) {

		SendMessage(hWndBar, PBM_SETPOS, Notification->Percent, 0);

		StringCchPrintf(Buffer, MAX_PATH, Context->Text, 
			            Notification->Percent, Context->Count);
		SetWindowText(hWndText, Buffer);

		if (Notification->Percent == 100) {

			//
			// If it's complete, callback won't touch context object
			// anymore.
			//

			EndDialog(hWnd, IDCANCEL);
			return TRUE;
		}

		return TRUE;
	}

	Context->State = SaveMspAborted;
	StringCchPrintf(Buffer, MAX_PATH, L"Saving aborted on error 0x%08x", 
		            Notification->Error);
	SetWindowText(hWndText, Buffer);

	return TRUE;
}

BOOLEAN CALLBACK
SaveCallback(
	IN PVOID Context,
	IN ULONG Percent,
	IN BOOLEAN Abort,
	IN ULONG Error
	)
{
	PSAVE_CONTEXT SaveContext;
	SAVE_NOTIFICATION Notification = {0};

	SaveContext = (PSAVE_CONTEXT)Context;

	if (SaveContext->State == SaveUserAborted) {

		//
		// User aborted, we return FALSE to discontinue the saving
		//

		SdkFree(Context);
		return TRUE;
	}

	Notification.Percent = Percent;
	Notification.Aborted = Abort;
	Notification.Error = Error;

	SendMessage(SaveContext->hWnd, WM_USER_SAVE, (WPARAM)&Notification, 0);

	if (SaveContext->State == SaveUserAborted || Percent == 100) {
		SdkFree(Context);
		return TRUE;
	}

	if (Abort) {

		//
		// UI is responsible for free context
		//

		return TRUE;
	}

	//
	// Continue to write
	//

	return FALSE;
}