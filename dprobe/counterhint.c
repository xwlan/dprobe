//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "dprobe.h"
#include "counterhint.h"
#include "mspdtl.h"

BOOLEAN
CounterHintDialog(
	IN HWND hWndOwner,
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	DIALOG_OBJECT Object = {0};
	COUNTERHINT_CONTEXT Context = {0};

	Context.DtlObject = DtlObject;
	Context.hWndOwner = hWndOwner;
	Object.Context = &Context;
	Object.hWndParent = hWndOwner;
	Object.ResourceId = IDD_DIALOG_PROGRESS;
	Object.Procedure = CounterHintProcedure;
	Object.Scaler = NULL;

	DialogCreate(&Object);
	return TRUE;
}

INT_PTR CALLBACK
CounterHintProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	switch (uMsg) {
		case WM_INITDIALOG: 
			return CounterHintOnInitDialog(hWnd, uMsg, wp, lp);
		case WM_COMMAND:
			return CounterHintOnCommand(hWnd, uMsg, wp, lp);
		case WM_TIMER:
			return CounterHintOnTimer(hWnd, uMsg, wp, lp);
	}

	return 0;
}

LRESULT
CounterHintOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	switch (LOWORD(wp)) {

	case IDCANCEL:
		CounterHintOnCancel(hWnd, uMsg, wp, lp);
	    break;
	}

	return 0;
}

LRESULT
CounterHintOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndBar;
    PDIALOG_OBJECT Object;
	WCHAR Buffer[MAX_PATH];
	PCOUNTERHINT_CONTEXT Context;

	hWndBar = GetDlgItem(hWnd, IDC_PROGRESS);
	SendMessage(hWndBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	SendMessage(hWndBar, PBM_SETSTEP, 10, 0); 
	
	SetWindowText(hWnd, L"D Probe");

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCOUNTERHINT_CONTEXT)Object->Context;
	Context->NumberToScan = MspGetRecordCount(Context->DtlObject);
	Context->ScannedNumber = 0;

	QueueUserWorkItem(CounterScanWorkItem, Context, 
		              WT_EXECUTEDEFAULT|WT_EXECUTELONGFUNCTION);

	StringCchPrintf(Buffer, MAX_PATH, L"Scanning %u records ...", 
		            Context->NumberToScan);

	SetDlgItemText(hWnd, IDC_STATIC, Buffer);
	
    SetTimer(hWnd, 1, 1000, NULL);
	SdkCenterWindow(hWnd);
	return 0;
}

LRESULT
CounterHintOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	KillTimer(hWnd, 1);
	EndDialog(hWnd, IDCANCEL);
	return 0;
}

LRESULT
CounterHintOnTimer(
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
	PCOUNTERHINT_CONTEXT Context;
	PMSP_DTL_OBJECT DtlObject;

    Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCOUNTERHINT_CONTEXT)Object->Context;
	DtlObject = Context->DtlObject;

    hWndBar = GetDlgItem(hWnd, IDC_PROGRESS);

	Percent = Context->ScannedNumber * 100 / Context->NumberToScan;
	StringCchPrintf(Buffer, MAX_PATH, L"%%%u of records are scanned ...", Percent);

    SetDlgItemText(hWnd, IDC_STATIC, Buffer);
	SendMessage(hWndBar, PBM_SETPOS, Percent, 0);

	if (Context->ScannedNumber >= Context->NumberToScan) {
		KillTimer(hWnd, 1);
		EndDialog(hWnd, IDOK);
	}

	return 0;
}

ULONG CALLBACK
CounterScanWorkItem(
	IN PVOID Context
	)
{
	PCOUNTERHINT_CONTEXT Scan;
	PMSP_DTL_OBJECT DtlObject;
	PBTR_RECORD_HEADER Record;
	PVOID Buffer;
	ULONG Number;

	Scan = (PCOUNTERHINT_CONTEXT)Context;
	DtlObject = Scan->DtlObject;
	
	Buffer = VirtualAlloc(NULL, BspPageSize, MEM_COMMIT, PAGE_READWRITE);

	for(Number = 0; Number < Scan->NumberToScan; Number += 1) {
		MspCopyRecord(DtlObject, Number, Buffer, BspPageSize);
		Record = (PBTR_RECORD_HEADER)Buffer;
		MspInsertCounterEntryByRecord(DtlObject, Record);
		Scan->ScannedNumber += 1;
	}
	
	return 0;
}