//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#include "detaildlg.h"
#include "recorddlg.h"
#include "stackdlg.h"
#include "mspdtl.h"
#include "msputility.h"
#include "bsp.h"

INT_PTR
DetailDialog(
	IN HWND hWndParent,
	IN HWND hWndTrace,
	struct _MSP_DTL_OBJECT *DtlObject,
	IN DETAIL_PAGE_TYPE InitialPage
	)
{
	DIALOG_OBJECT Object = {0};
	DETAIL_DIALOG_CONTEXT Context = {0};
	INT_PTR Return;
	
	Context.InitialType = InitialPage;
	Context.hWndTrace = hWndTrace;
	Context.DtlObject = DtlObject;

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_DETAIL;
	Object.Procedure = DetailProcedure;

	Return = DialogCreate(&Object);
	return Return;
}

INT_PTR CALLBACK
DetailProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	INT_PTR Status;
	Status = FALSE;

	switch (uMsg) {
		
		case WM_COMMAND:
			DetailOnCommand(hWnd, uMsg, wp, lp);
			Status = TRUE;
			break;
		
		case WM_INITDIALOG:
			DetailOnInitDialog(hWnd, uMsg, wp, lp);
			SdkCenterWindow(hWnd);
			Status = TRUE;
		
		case WM_NOTIFY:
			Status = DetailOnNotify(hWnd, uMsg, wp, lp);
			break;

	}

	return Status;
}

LRESULT
DetailOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndPage;
	PDIALOG_OBJECT Object;
	PDETAIL_DIALOG_CONTEXT Context;
	HWND hWndTab;
	TCITEM TabItem = {0};
	WCHAR Buffer[MAX_PATH];
	ULONG Style;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PDETAIL_DIALOG_CONTEXT)Object->Context;

	//
	// Allocate record buffer
	//

	Context->Buffer = SdkMalloc(4096);
	Context->BufferLength = 4096;

	hWndTab = GetDlgItem(hWnd, IDC_TAB_DETAIL);
	Style = GetWindowLong(hWndTab, GWL_STYLE) | WS_TABSTOP;
	SetWindowLong(hWndTab, GWL_STYLE, Style);

	//
	// Insert tab ctrl items
	//

	LoadString(SdkInstance, IDS_TAB_RECORD, Buffer, MAX_PATH);
	TabItem.mask = TCIF_TEXT;
	TabItem.pszText = Buffer;
	TabCtrl_InsertItem(hWndTab, 0, &TabItem);

	LoadString(SdkInstance, IDS_TAB_STACK, Buffer, MAX_PATH);
	TabItem.mask = TCIF_TEXT;
	TabItem.pszText = Buffer;
	TabCtrl_InsertItem(hWndTab, 1, &TabItem);

	//
	// Load icon for upward and downward button
	//

	Context->hIconUpward = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_UPWARD));
	Context->hIconDownward = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_DOWNWARD));

	//
	// Set detail window title and its icon
	//

	LoadString(SdkInstance, IDS_CALLDETAIL, Buffer, MAX_PATH);
	SetWindowText(hWnd, Buffer);
	SdkSetMainIcon(hWnd);

	hWndPage = RecordDialog(hWnd);
	Context->hWndPages[DetailPageRecord] = hWndPage;
	SetWindowLong(hWndPage, GWLP_ID, IDD_DIALOG_RECORD);
	ShowWindow(hWndPage, SW_HIDE);
	
	hWndPage = StackDialog(hWnd, 0);
	Context->hWndPages[DetailPageStack] = hWndPage;
	SetWindowLong(hWndPage, GWLP_ID, IDD_DIALOG_STACK);
	ShowWindow(hWndPage, SW_HIDE);

	//
	// Set initial page
	//

	TabCtrl_SetCurSel(hWndTab, Context->InitialType);
	DetailAdjustPosition(Context->hWndPages[Context->InitialType]);
	DetailUpdateData(Object);
	return 0L;
}

VOID
DetailAdjustPosition(
	IN HWND hWnd
	)
{
	HWND hWndTab;
	HWND hWndDesktop;
	HWND hWndParent;
	RECT rcTab;

	DWORD dwDlgBase = GetDialogBaseUnits(); 
    int cxMargin = LOWORD(dwDlgBase) / 4; 
    int cyMargin = HIWORD(dwDlgBase) / 8; 
	
	hWndParent = GetParent(hWnd);
	hWndTab = GetDlgItem(hWndParent, IDC_TAB_DETAIL);
	GetWindowRect(hWndTab, &rcTab);

	TabCtrl_AdjustRect(hWndTab, FALSE, &rcTab);

	hWndDesktop = GetDesktopWindow();
	MapWindowPoints(hWndDesktop, hWndParent, (LPPOINT)&rcTab, 2);
	SetWindowPos(hWnd, hWndTab,
		         rcTab.left + 1, rcTab.top + 1,
			     rcTab.right - rcTab.left - 1,
			     rcTab.bottom - rcTab.top - 1, SWP_SHOWWINDOW);
	return;
}

LRESULT
DetailOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	switch (LOWORD(wp)) {

		case IDOK:
		case IDCANCEL:
		case IDCLOSE:
			DetailOnClose(hWnd, uMsg, wp, lp);
			EndDialog(hWnd, IDOK);
			break;

		case IDC_BUTTON_UPWARD:
			DetailOnUpward(hWnd, uMsg, wp, lp);
			break;
		
		case IDC_BUTTON_DOWNWARD:
			DetailOnDownward(hWnd, uMsg, wp, lp);
			break;

		case IDC_BUTTON_COPY:
			DetailOnCopy(hWnd, uMsg, wp, lp);
			break;

	}

	return 0L;
}

LRESULT
DetailOnClose(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PDETAIL_DIALOG_CONTEXT Context;
	PDIALOG_OBJECT Stack;
	PDIALOG_OBJECT Record;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PDETAIL_DIALOG_CONTEXT)Object->Context;
	SdkFree(Context->Buffer);

	Stack = (PDIALOG_OBJECT)SdkGetObject(Context->hWndPages[DetailPageStack]);
	DestroyWindow(Context->hWndPages[DetailPageStack]);
	SdkFree(Stack);

	Record = (PDIALOG_OBJECT)SdkGetObject(Context->hWndPages[DetailPageRecord]);
	DestroyWindow(Context->hWndPages[DetailPageRecord]);
	SdkFree(Record);

	DestroyIcon(Context->hIconDownward);
	DestroyIcon(Context->hIconUpward);
	return 0;
}

LRESULT
DetailOnCopy(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PDETAIL_DIALOG_CONTEXT Context;
	HWND hWndTab;
	LONG Current;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PDETAIL_DIALOG_CONTEXT)Object->Context;

	hWndTab = GetDlgItem(hWnd, IDC_TAB_DETAIL);
	Current = TabCtrl_GetCurSel(hWndTab);

	switch (Current) {

		case DetailPageRecord:
			RecordOnCopy(Context->hWndPages[DetailPageRecord], 
				         uMsg, wp, lp);
			break;

		case DetailPageStack:
			StackOnCopy(Context->hWndPages[DetailPageStack], 
				        uMsg, wp, lp);
			break;
	}

	return 0;
}

LRESULT
DetailOnUpward(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LONG Selected;
	PDIALOG_OBJECT Object;
	PDETAIL_DIALOG_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PDETAIL_DIALOG_CONTEXT)Object->Context;

	Selected = ListViewGetFirstSelected(Context->hWndTrace);
	if (Selected == -1) {
		return 0;
	}

	if (Selected != 0) {
		ListViewSelectSingle(Context->hWndTrace, Selected - 1);
		ListView_EnsureVisible(Context->hWndTrace, Selected - 1, FALSE);
		DetailUpdateData(Object);
	}

	return 0;
}

LRESULT
DetailOnDownward(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LONG Selected;
	PDIALOG_OBJECT Object;
	PDETAIL_DIALOG_CONTEXT Context;
	ULONG Count;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PDETAIL_DIALOG_CONTEXT)Object->Context;

	Selected = ListViewGetFirstSelected(Context->hWndTrace);
	if (Selected == -1) {
		return 0;
	}

	Selected += 1;
	Count = ListView_GetItemCount(Context->hWndTrace);

	//
	// Avoid select invalid item
	//

	if ((ULONG)Selected < Count) {
		ListViewSelectSingle(Context->hWndTrace, Selected);
		ListView_EnsureVisible(Context->hWndTrace, Selected, FALSE);
		DetailUpdateData(Object);
	}

	return 0;
}

LRESULT
DetailOnCustomDraw(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LRESULT Status = 0L;
	LPNMHDR lpNmhdr = (LPNMHDR)lp;
    LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)lpNmhdr;
	PDIALOG_OBJECT Object;
	PDETAIL_DIALOG_CONTEXT Context;
	LONG x, y;
	HICON hicon;
	int CtrlId;
	RECT Rect;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PDETAIL_DIALOG_CONTEXT)Object->Context;
	CtrlId = GetDlgCtrlID(lpNmhdr->hwndFrom);

	if (CtrlId == IDC_BUTTON_UPWARD) {
		hicon = Context->hIconUpward;
	} else {
		hicon = Context->hIconDownward;
	}

    switch(lvcd->nmcd.dwDrawStage)  {
        
        case CDDS_PREPAINT: 

			//
			// Calculate to blit the icon in center of button rect
			//

			GetClientRect(lpNmhdr->hwndFrom, &Rect);
			x = (Rect.right - Rect.left - 16) / 2;
			y = (Rect.bottom - Rect.top - 16) / 2;

			DrawIconEx(lvcd->nmcd.hdc, x, y, hicon, 
				       16, 16, 0, NULL, DI_NORMAL);

            Status = CDRF_SKIPDEFAULT;
            break;
        
        default:
            Status = CDRF_DODEFAULT;
    }

	return Status;
}

LRESULT
DetailOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LRESULT Status = 0;
	LPNMHDR lpNmhdr;

	lpNmhdr = (LPNMHDR)lp;

	switch (lpNmhdr->code) {

		case NM_CUSTOMDRAW:

			if (lpNmhdr->idFrom == IDC_BUTTON_UPWARD || 
				lpNmhdr->idFrom == IDC_BUTTON_DOWNWARD) {

				Status = DetailOnCustomDraw(hWnd, uMsg, wp, lp);	
			}

			break;

		case TCN_SELCHANGE:
			DetailOnSelChange(hWnd, uMsg, wp, lp);
			break;
	}

	return Status;
}

LRESULT
DetailOnSelChange(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LRESULT Status = 0;
	PDIALOG_OBJECT Object;
	PDETAIL_DIALOG_CONTEXT Context;
	HWND hWndTab;
	LONG Current;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PDETAIL_DIALOG_CONTEXT)Object->Context;

	hWndTab = GetDlgItem(hWnd, IDC_TAB_DETAIL);
	Current = TabCtrl_GetCurSel(hWndTab);

	if (Current == DetailPageRecord) {
		ShowWindow(Context->hWndPages[DetailPageStack], SW_HIDE);
		DetailAdjustPosition(Context->hWndPages[DetailPageRecord]);
	} 
	else if (Current == DetailPageStack) {
		ShowWindow(Context->hWndPages[DetailPageRecord], SW_HIDE);
		DetailAdjustPosition(Context->hWndPages[DetailPageStack]);
	}

	return 0L;
}

ULONG
DetailUpdateData(
	IN PDIALOG_OBJECT Object
	)
{
	ULONG Status;
	LONG Selected;
	PMSP_DTL_OBJECT DtlObject;
	PDETAIL_DIALOG_CONTEXT Context;
	PMSP_FILTER_FILE_OBJECT FileObject;
	PMSP_STACKTRACE_DECODED Decoded;
	PMSP_STACKTRACE_OBJECT StackTrace;
	HWND hWnd;

	Context = (PDETAIL_DIALOG_CONTEXT)Object->Context;
	hWnd = Context->hWndTrace;
	DtlObject = Context->DtlObject;

	Selected = ListViewGetFirstSelected(hWnd);
	if (Selected == -1) {
		return 0;
	}
	
	FileObject = DtlObject->FilterFileObject;
	if (!FileObject) {
		MspCopyRecord(DtlObject, Selected, Context->Buffer, Context->BufferLength);
	} else {
		MspCopyRecordFiltered(FileObject, Selected, Context->Buffer, Context->BufferLength);
	}

	//
	// Get stack trace
	//

	StackTrace = &DtlObject->StackTrace;
	Decoded = (PMSP_STACKTRACE_DECODED)SdkMalloc(sizeof(MSP_STACKTRACE_DECODED));
	Status = MspGetStackTrace((PBTR_RECORD_HEADER)Context->Buffer, StackTrace, Decoded);
	Context->StackTrace = Decoded;

	//
	// Update record and stack trace pages
	//

	RecordUpdateData(Context->hWndPages[DetailPageRecord], 
		             Context->hWndTrace, 
					 (PBTR_RECORD_HEADER)Context->Buffer);

	StackUpdateData(Context->hWndPages[DetailPageStack], Decoded);
	return 0;
}