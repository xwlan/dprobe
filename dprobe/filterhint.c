//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "dprobe.h"
#include "filterhint.h"
#include "mspdtl.h"

BOOLEAN
FilterHintDialog(
	IN HWND hWndOwner,
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	DIALOG_OBJECT Object = {0};
	FILTERHINT_CONTEXT Context = {0};

	Context.DtlObject = DtlObject;
	Context.hWndOwner = hWndOwner;
	Object.Context = &Context;
	Object.hWndParent = hWndOwner;
	Object.ResourceId = IDD_DIALOG_PROGRESS;
	Object.Procedure = FilterHintProcedure;
	Object.Scaler = NULL;

	DialogCreate(&Object);
	return TRUE;
}

INT_PTR CALLBACK
FilterHintProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	switch (uMsg) {
		case WM_INITDIALOG: 
			return FilterHintOnInitDialog(hWnd, uMsg, wp, lp);
		case WM_COMMAND:
			return FilterHintOnCommand(hWnd, uMsg, wp, lp);
		case WM_TIMER:
			return FilterHintOnTimer(hWnd, uMsg, wp, lp);
	}

	return 0;
}

LRESULT
FilterHintOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	switch (LOWORD(wp)) {

	case IDCANCEL:
		FilterHintOnCancel(hWnd, uMsg, wp, lp);
	    break;
	}

	return 0;
}

LRESULT
FilterHintOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndBar;
    PDIALOG_OBJECT Object;
	WCHAR Buffer[MAX_PATH];
	PFILTERHINT_CONTEXT Context;
	HANDLE ThreadHandle;

	hWndBar = GetDlgItem(hWnd, IDC_PROGRESS);
	SendMessage(hWndBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	SendMessage(hWndBar, PBM_SETSTEP, 10, 0); 
	
	SetWindowText(hWnd, L"D Probe");

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PFILTERHINT_CONTEXT)Object->Context;
	Context->NumberToScan = MspGetRecordCount(Context->DtlObject);
	Context->ScannedNumber = 0;

	ThreadHandle = CreateThread(NULL, 0, FilterScanWorkItem, Context, 
								0, NULL);
	Context->ThreadHandle = ThreadHandle;

	StringCchPrintf(Buffer, MAX_PATH, L"Filtering %u records ...", 
		            Context->NumberToScan);

	SetDlgItemText(hWnd, IDC_STATIC, Buffer);
	
    SetTimer(hWnd, 1, 1000, NULL);
	SdkCenterWindow(hWnd);
	return 0;
}

LRESULT
FilterHintOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PFILTERHINT_CONTEXT Context;
	
    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PFILTERHINT_CONTEXT)Object->Context;
	Context->Cancel = TRUE;

	return 0;
}

LRESULT
FilterHintOnTimer(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndBar;
    ULONG Percent;
    WCHAR Buffer[MAX_PATH];
    PDIALOG_OBJECT Object;
	PMSP_FILTER_FILE_OBJECT FileObject;
	PFILTERHINT_CONTEXT Context;
	PMSP_DTL_OBJECT DtlObject;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PFILTERHINT_CONTEXT)Object->Context;
	DtlObject = Context->DtlObject;
	FileObject = DtlObject->FilterFileObject;

    hWndBar = GetDlgItem(hWnd, IDC_PROGRESS);

	Percent = Context->ScannedNumber * 100 / Context->NumberToScan;
	StringCchPrintf(Buffer, MAX_PATH, L"%%%u of records are scanned ...", Percent);

    SetDlgItemText(hWnd, IDC_STATIC, Buffer);
	SendMessage(hWndBar, PBM_SETPOS, Percent, 0);

	if (Context->ScannedNumber >= Context->NumberToScan ||
		Context->Cancel ) {

		KillTimer(hWnd, 1);
		
		//
		// Wait scan thread terminate and set filter record count
		//

		MsgWaitForMultipleObjects(1, &Context->ThreadHandle, TRUE, INFINITE, QS_ALLEVENTS);
		CloseHandle(Context->ThreadHandle);

		ListView_SetItemCount(Context->hWndOwner, FileObject->NumberOfRecords);
		EndDialog(hWnd, IDOK);
	}

	return 0;
}

#define SCAN_STEP 10000

ULONG CALLBACK
FilterScanWorkItem(
	IN PVOID Context
	)
{
	PFILTERHINT_CONTEXT Scan;
	PMSP_DTL_OBJECT DtlObject;
	PMSP_FILTER_FILE_OBJECT FileObject;
	ULONG Round;
	ULONG Remain;
	ULONG Number;

	Scan = (PFILTERHINT_CONTEXT)Context;
	DtlObject = Scan->DtlObject;
	FileObject = MspGetFilterFileObject(DtlObject);
	
	Round = Scan->NumberToScan / SCAN_STEP;
	Remain = Scan->NumberToScan % SCAN_STEP;

	for(Number = 0; Number < Round; Number += 1) {

		MspScanFilterRecord(FileObject, SCAN_STEP);
		Scan->ScannedNumber += SCAN_STEP;

		//
		// If cancelled, abort here
		//

		if (Scan->Cancel) {
			return 0;
		}
	}
	
	MspScanFilterRecord(FileObject, Remain);
	Scan->ScannedNumber += Remain;

	return 0;
}