//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

//
// N.B. This is for REBARINFO structure, it has different size
// for XP and VISTA, band insertion will fail if we fill wrong
// size of REBARINFO.
//

#undef  _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#include "dprobe.h"
#include "reportdlg.h"
#include "stackdlg.h"
#include "logview.h"
#include "counterdlg.h"
#include "find.h"
#include "conditiondlg.h"
#include "detaildlg.h"
#include "frame.h"
#include "filterhint.h"
#include "mspdtl.h"
#include "mspprocess.h"
#include "msputility.h"
#include "counterhint.h"
#include "savedlg.h"

//
// N.B. Ensure the band id do not conflict with
// IDOK, IDCANCEL, IDC_LIST_REPORT
//

typedef enum _REPORT_REBAR_BAND_ID {
	REPORT_REBAR_ID     = 100,
	REPORT_BAND_TOOLBAR = 101,
	REPORT_BAND_EDIT    = 102,
} REPORT_REBAR_BAND_ID, *PREPORT_REBAR_BAND_ID;

TBBUTTON ReportToolbarButtons[] = {
	{ 0, IDM_FINDBACKWARD, TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
	{ 1, IDM_FINDFORWARD,  TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
	{ 2, IDM_REPORT_FILTER,TBSTATE_ENABLED, BTNS_DROPDOWN, 0L, 0},
	{ 3, IDM_COUNTER,      TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
	{ 4, IDM_FILE_SAVE,    TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
};

DIALOG_SCALER_CHILD ReportChildren[4] = {
	{ REPORT_REBAR_ID, AlignRight, AlignNone },
    { IDC_LIST_REPORT, AlignRight, AlignBottom },
    { IDC_STATIC, AlignRight, AlignBoth },
	{ IDOK, AlignBoth, AlignBoth },
};

DIALOG_SCALER ReportScaler = {
	{0,0}, {0,0}, {0,0}, 4, ReportChildren
};

//
// WM_REPORT_COUNTER drives to generate counter summary
//

#define WM_USER_GENERATE_COUNTER   (WM_USER + 20)

HWND
ReportDialog(
	IN HWND hWndParent,
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	PDIALOG_OBJECT Object;
	PREPORT_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	ZeroMemory(Object, sizeof(DIALOG_OBJECT));

	Context = (PREPORT_CONTEXT)SdkMalloc(sizeof(REPORT_CONTEXT));
	ZeroMemory(Context, sizeof(REPORT_CONTEXT));

	Context->DtlObject = DtlObject;
	InitializeListHead(&Context->IconListHead);

	Object->hWndParent = hWndParent;
	Object->Context = Context;
	Object->Procedure = ReportProcedure;
	Object->ResourceId = IDD_DIALOG_REPORT;

	return DialogCreateModeless(Object);
}

ULONG
ReportInitImageList(
	IN PDIALOG_OBJECT Object,
	IN HWND	hWnd
	)
{
	HICON hicon;
	ULONG Index;
	PREPORT_ICON Icon;
	HIMAGELIST himlReport;
	PREPORT_CONTEXT Context;

	himlReport = ImageList_Create(16, 16, ILC_COLOR32, 0, 0); 
	ListView_SetImageList(hWnd, himlReport, LVSIL_SMALL);

	Context = SdkGetContext(Object, REPORT_CONTEXT);
	Context->himlReport = himlReport;
    
	//
	// Add first icon as shell32.dll's default icon, this is
	// for those failure case when process's icon can't be extracted.
	//

	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_GENERIC));
	Index = ImageList_AddIcon(himlReport, hicon);
	DestroyIcon(hicon);
	
	Icon = (PREPORT_ICON)SdkMalloc(sizeof(REPORT_ICON));
	Icon->ProcessId = -1;
	Icon->ImageIndex = 0;

	InsertHeadList(&Context->IconListHead, &Icon->ListEntry);
	return S_OK;
}

ULONG
ReportQueryImageList(
	IN PDIALOG_OBJECT Object,
	IN ULONG ProcessId
	)
{
	PREPORT_ICON Icon;
	PLIST_ENTRY ListEntry;
	PREPORT_CONTEXT Context;

	Context = Object->Context;
	ListEntry = Context->IconListHead.Flink;

	while (ListEntry != &Context->IconListHead) {
		
		Icon = CONTAINING_RECORD(ListEntry, REPORT_ICON, ListEntry);
		if (Icon->ProcessId == ProcessId) {
			return Icon->ImageIndex;
		}
		ListEntry = ListEntry->Flink;
	}

	return 0;
}

LRESULT
ReportOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndCtrl;
	HWND hWndRebar;
	PDIALOG_OBJECT Object;
	PREPORT_CONTEXT Context;
	PMSP_DTL_OBJECT DtlObject;
	LV_COLUMN lvc = {0}; 
	ULONG i;
	RECT RebarRect;
	RECT ListRect;
	HWND hWndDesktop;
	ULONG Style;
	ULONG NumberOfRecords;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, REPORT_CONTEXT);
	DtlObject = Context->DtlObject;

	ASSERT(DtlObject != NULL);

	hWndRebar = ReportLoadBars(hWnd, REPORT_REBAR_ID);
	ASSERT(hWndRebar != NULL);

	GetWindowRect(hWndRebar, &RebarRect);

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_REPORT);
    ListView_SetUnicodeFormat(hWndCtrl, TRUE);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT,  LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_HEADERDRAGDROP, LVS_EX_HEADERDRAGDROP); 

	Style = GetWindowLong(hWndCtrl, GWL_STYLE);
	SetWindowLong(hWndCtrl, GWL_STYLE, Style | WS_CLIPSIBLINGS);

	//
	// Add support to imagelist (for Process Name column).
	//

	ReportInitImageList(Object, hWndCtrl);

	//
	// Insert dummy column
	//

	lvc.mask = 0; 
	ListView_InsertColumn(hWndCtrl, 0, &lvc);

    for (i = 0; i < LogColumnCount; i++) { 

		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
		lvc.pszText = LogViewColumn[i].Title;	
		lvc.cx = LogViewColumn[i].Width;     
		lvc.fmt = LogViewColumn[i].Align;
		ListView_InsertColumn(hWndCtrl, i + 1, &lvc);
    } 

	//
	// Delete dummy column
	//

	ListView_DeleteColumn(hWndCtrl, 0);
	ListView_SetColumnOrderArray(hWndCtrl, LogColumnCount, LogViewColumnOrder);

	GetWindowRect(hWndCtrl, &ListRect);

	hWndDesktop = GetDesktopWindow();
	MapWindowPoints(hWndDesktop, hWnd, (LPPOINT)&ListRect, 2);

	//
	// Adjust rebar position
	//

	MoveWindow(hWndRebar, 0, 0, ListRect.right - ListRect.left,
			   RebarRect.bottom - RebarRect.top, TRUE);

	//
	// Adjust list control position
	//

	ListRect.top = ListRect.top + RebarRect.bottom - RebarRect.top;
	MoveWindow(hWndCtrl, 0, RebarRect.bottom - RebarRect.top, 
			   ListRect.right - ListRect.left,
			   ListRect.bottom - ListRect.top, TRUE);

	//
	// Register dialog scaler
	//

	Object->Scaler = &ReportScaler;
	DialogRegisterScaler(Object);

	NumberOfRecords = DtlObject->IndexObject.NumberOfRecords;
	ListView_SetItemCountEx(hWndCtrl, NumberOfRecords, 
		                    LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);

	//
	// Set status text
	//

	ReportSetStatusText(hWnd, DtlObject);

	//
	// Set find callback context
	//

	ReportSetFindContext(hWnd, DtlObject);

	SetWindowText(hWnd, DtlObject->DataObject.Path);
	SdkSetMainIcon(hWnd);
	SdkCenterWindow(hWnd);

	//
	// If there's no summary, genereate it on demand
	// must enter stall mode to prevent race condition
	// when access data mapping.
	//

	if (DtlObject->Header.SummaryLength == 0) {
		ReportEnterStallMode(Object);
		PostMessage(hWnd, WM_USER_GENERATE_COUNTER, 0, (LPARAM)DtlObject);
	}

	return 0;
}

LRESULT
ReportOnUserGenerateCounter(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PMSP_DTL_OBJECT DtlObject;
	PDIALOG_OBJECT Object;
	ULONG Count;

	DtlObject = (PMSP_DTL_OBJECT)lp;
	CounterHintDialog(hWnd, DtlObject);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Count = MspGetRecordCount(DtlObject);

	//
	// Exit stall mode
	//

	ReportExitStallMode(Object, Count);
	return 0L;
}

LRESULT
ReportOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	return 0;
}

VOID
ReportSetFindContext(
	IN HWND hWnd,
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	PDIALOG_OBJECT Object;
	PREPORT_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PREPORT_CONTEXT)Object->Context;

	Context->FindContext.DtlObject = DtlObject;
	Context->FindContext.FindCallback = ReportFindCallback;
	Context->FindContext.hWndList = GetDlgItem(hWnd, IDC_LIST_REPORT);
	Context->FindContext.FindForward = TRUE;
	
	return;
}

VOID
ReportSetStatusText(
	IN HWND hWnd,
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	HWND hWndText;
	WCHAR Buffer[MAX_PATH];
	ULONG TotalRecords;
	ULONG FilteredCount;
	double Percent;
	PMSP_FILTER_FILE_OBJECT FileObject;
	
	TotalRecords = MspGetRecordCount(DtlObject);
	FileObject = MspGetFilterFileObject(DtlObject);

	if (FileObject != NULL) {

		FilteredCount = MspGetFilteredRecordCount(FileObject);
		Percent = (FilteredCount * 100.0) / (TotalRecords * 1.0);
		StringCchPrintf(Buffer, MAX_PATH, L"Showing %d records of %d (%%%.2f)", 
						FilteredCount, TotalRecords, Percent);
		
	} else {
		StringCchPrintf(Buffer, MAX_PATH, L"Showing %d records of %d (%%%d)", 
						TotalRecords, TotalRecords, 100);
	}

	hWndText = GetDlgItem(hWnd, IDC_STATIC);
	SetWindowText(hWndText, Buffer);
}

LRESULT 
ReportOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LRESULT Status;
	NMHDR *pNmhdr = (NMHDR *)lp;

	Status = 0;

	switch (pNmhdr->code) {
		
		case NM_CUSTOMDRAW:
			Status = ReportOnCustomDraw(hWnd, uMsg, wp, lp);
			break;

		case LVN_GETDISPINFO:
			Status = ReportOnGetDispInfo(hWnd, uMsg, wp, lp);
			break;

		case TBN_DROPDOWN:
			ReportOnTbnDropDown(hWnd, uMsg, wp, lp);
			break;

		case TTN_GETDISPINFO:
			ReportOnTtnGetDispInfo(hWnd, uMsg, wp, lp);
			break;
	}

	return Status;
}

LRESULT
ReportOnTtnGetDispInfo(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LPNMTTDISPINFO lpnmtdi = (LPNMTTDISPINFO)lp;
	lpnmtdi->uFlags = TTF_DI_SETITEM;

	switch (lpnmtdi->hdr.idFrom) {
		case IDM_FINDBACKWARD:
			StringCchCopy(lpnmtdi->szText, 80, L"Find Backward");
			break;
		case IDM_FINDFORWARD:
			StringCchCopy(lpnmtdi->szText, 80, L"Find Forward");
			break;
		case IDM_REPORT_FILTER:
			StringCchCopy(lpnmtdi->szText, 80, L"Record Filter");
			break;
		case IDM_COUNTER:
			StringCchCopy(lpnmtdi->szText, 80, L"Record Summary");
			break;
		case IDM_FILE_SAVE:
			StringCchCopy(lpnmtdi->szText, 80, L"Save Report");
			break;
	}

	return 0L;
}

LRESULT
ReportOnTbnDropDown(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LPNMHDR lpNmhdr = (LPNMHDR)lp;
	LPNMTOOLBAR lpnmtb = (LPNMTOOLBAR)lp;

	RECT      rc;
	TPMPARAMS tpm;
	HMENU     hPopupMenu = NULL;
	HMENU     hMenuLoaded;
	BOOL      bRet = FALSE;

	SendMessage(lpnmtb->hdr.hwndFrom, TB_GETRECT,
				(WPARAM)lpnmtb->iItem, (LPARAM)&rc);

	MapWindowPoints(lpnmtb->hdr.hwndFrom, HWND_DESKTOP, (LPPOINT)&rc, 2);                         

	tpm.cbSize = sizeof(TPMPARAMS);
	tpm.rcExclude = rc;

	hMenuLoaded = LoadMenu(SdkInstance, MAKEINTRESOURCE(IDR_MENU_FILTER)); 
	hPopupMenu = GetSubMenu(LoadMenu(SdkInstance,
		                    MAKEINTRESOURCE(IDR_MENU_FILTER)),0);

	TrackPopupMenuEx(hPopupMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,               
					 rc.left, rc.bottom, hWnd, &tpm); 

	DestroyMenu(hMenuLoaded);			
	return (FALSE);
}

LRESULT
ReportOnCustomDraw(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LRESULT Status = 0L;
	LPNMHDR pNmhdr = (LPNMHDR)lp; 
    LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)pNmhdr;

    switch(lvcd->nmcd.dwDrawStage)  {
        
        case CDDS_PREPAINT: 
            Status = CDRF_NOTIFYITEMDRAW;
            break;
        
        case CDDS_ITEMPREPAINT:
            Status = CDRF_NOTIFYSUBITEMDRAW;
            break;
        
        case CDDS_SUBITEM | CDDS_ITEMPREPAINT: 

			if (lvcd->iSubItem == LogColumnSequence) { 
				lvcd->clrTextBk = RGB(220, 220, 220);
			} else {
				lvcd->clrTextBk = RGB(255, 255, 255);
			}

			Status = CDRF_NEWFONT;
			break;
        
        default:
            Status = CDRF_DODEFAULT;
    }
    
    return Status;
}

LRESULT 
ReportOnCommand(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	//
	// Source       HIWORD(wParam)  LOWORD(wParam)  lParam 
	// Menu         0               MenuId          0   
	// Accelerator  1               AcceleratorId	0
	// Control      NotifyCode      ControlId       hWndCtrl
	//

	switch (LOWORD(wp)) {

	case IDOK:
		ReportOnOk(hWnd, uMsg, wp, lp);
	    break;

	case IDCANCEL:
		ReportOnCancel(hWnd, uMsg, wp, lp);
	    break;

	case IDM_CALLSTACK:
		ReportOnStack(hWnd, uMsg, wp, lp);
		break;

	case IDM_CALLRECORD:
		ReportOnRecord(hWnd, uMsg, wp, lp);
		break;

	case IDM_FINDBACKWARD:
		ReportOnFindBackward(hWnd, uMsg, wp, lp);
		break;

	case IDM_FINDFORWARD:
		ReportOnFindForward(hWnd, uMsg, wp, lp);
		break;

	case IDM_COPY:
		ReportOnCopy(hWnd, uMsg, wp, lp);
		break;

	case IDM_COUNTER:
		ReportOnSummary(hWnd, uMsg, wp, lp);
		break;

	case IDM_REPORT_FILTER:
	case ID_FILTER_EDIT:
		ReportOnFilter(hWnd, uMsg, wp, lp);
		break;

	case ID_FILTER_RESET:
		ReportOnResetFilter(hWnd, uMsg, wp, lp);
		break;

	case ID_FILTER_IMPORT:
		ReportOnImportFilter(hWnd, uMsg, wp, lp);
		break;

	case ID_FILTER_EXPORT:
		ReportOnExportFilter(hWnd, uMsg, wp, lp);
		break;

	case IDM_FILE_SAVE:
		ReportOnSave(hWnd, uMsg, wp, lp);
		break;

	case ID_INCLUDE_PROCESS:
	case ID_INCLUDE_PID:
	case ID_INCLUDE_TID:
	case ID_INCLUDE_PROBE:
	case ID_INCLUDE_PROVIDER:
	case ID_INCLUDE_RETURN:
	case ID_INCLUDE_DETAIL:
	case ID_EXCLUDE_PROCESS:
	case ID_EXCLUDE_PID:
	case ID_EXCLUDE_TID:
	case ID_EXCLUDE_PROBE:
	case ID_EXCLUDE_PROVIDER:
	case ID_EXCLUDE_RETURN:
	case ID_EXCLUDE_DETAIL:

		//
		// Filter commands via context menu, it's typically a kind of incremental,
		// fast filter operation.
		//

		ReportOnFilterCommand(hWnd, LOWORD(wp));
		break;

	}

	return 0;
}

LRESULT 
ReportOnWmUserFilter(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PMSP_DTL_OBJECT DtlObject;
	PMSP_FILTER_FILE_OBJECT FileObject;
	ULONG Count;
	PDIALOG_OBJECT Object;
	PREPORT_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, REPORT_CONTEXT);

	DtlObject = Context->DtlObject;
	FileObject = MspGetFilterFileObject(DtlObject);

	if (FileObject != NULL) {
	
		ReportEnterStallMode(Object);

		FilterHintDialog(hWnd, DtlObject);
		Count = MspGetFilteredRecordCount(FileObject);

		ReportExitStallMode(Object, Count);

	} else {
		
		ReportEnterStallMode(Object);
		Count = MspGetRecordCount(DtlObject);
		ReportExitStallMode(Object, Count);

	}

	ReportSetStatusText(hWnd, DtlObject);
	return 0;
}

LRESULT 
ReportOnOk(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	)
{
    ReportOnClose(hWnd);
	return 0;
}

LRESULT 
ReportOnCancel(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	)
{
    ReportOnClose(hWnd);
	return 0;
}

VOID
ReportOnClose(
    IN HWND hWnd
    )
{
	PDIALOG_OBJECT Object;
	PREPORT_CONTEXT Context;
	PLIST_ENTRY ListEntry;
	PREPORT_ICON Icon;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PREPORT_CONTEXT)Object->Context;

	if (Context->hFontBold) {
		DeleteFont(Context->hFontBold);
	}

	MspCloseDtlReport(Context->DtlObject);

	while (IsListEmpty(&Context->IconListHead) != TRUE) {
		ListEntry = RemoveHeadList(&Context->IconListHead);
		Icon = CONTAINING_RECORD(ListEntry, REPORT_ICON, ListEntry);
		SdkFree(Icon);
	}	

	DestroyWindow(hWnd);

	SdkFree(Context);
	SdkFree(Object);
	PostThreadMessage(GetCurrentThreadId(), WM_QUIT, 0, 0);
}

LRESULT 
ReportOnStack(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PREPORT_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, REPORT_CONTEXT);

	DetailDialog(hWnd, GetDlgItem(hWnd, IDC_LIST_REPORT), 
		         Context->DtlObject, DetailPageStack);
	return 0;
}

LRESULT 
ReportOnRecord(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PREPORT_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, REPORT_CONTEXT);

	DetailDialog(hWnd, GetDlgItem(hWnd, IDC_LIST_REPORT), 
		         Context->DtlObject, DetailPageRecord);
	return 0;
}

LRESULT 
ReportOnContextMenu(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	int x, y;
	HMENU hMenu;
	HMENU hSubMenu;

	x = GET_X_LPARAM(lp); 
	y = GET_Y_LPARAM(lp); 

	if (x != -1 && y != -1) {
		
		hMenu = LoadMenu(SdkInstance, MAKEINTRESOURCE(IDR_MENU_LOGVIEW));
		hSubMenu = GetSubMenu(hMenu, 0);
		ASSERT(hSubMenu != NULL);

		TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
					   x, y, 0, hWnd, NULL);

		DestroyMenu(hMenu);
	} 

	return 0L;
}

LRESULT
ReportOnGetDispInfo(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	ULONG Status;
	LV_DISPINFO* lpdi;
    LV_ITEM *lpvi;
	PBTR_RECORD_HEADER Record;
	PMSP_FILTER_FILE_OBJECT FileObject;
	PMSP_DTL_OBJECT DtlObject;
	ULONG Count;
	WCHAR Buffer[1024];
	PVOID ReportBuffer;
	PDIALOG_OBJECT Object;
	PREPORT_CONTEXT Context;

    lpdi = (LV_DISPINFO *)lp;
    lpvi = &lpdi->item;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, REPORT_CONTEXT);

	if (ReportIsInStallMode(Object)) {
		return 0;
	}

	DtlObject = Context->DtlObject;
	FileObject = MspGetFilterFileObject(DtlObject);

	ReportBuffer = _alloca(MspPageSize);

	if (!FileObject) {

		//
		// Ensure the request record index is bounded
		//

		Count = MspGetRecordCount(DtlObject);
		if (!Count || ((ULONG)lpvi->iItem > Count - 1)) {
			return 0;
		}

		Status = MspCopyRecord(DtlObject, lpvi->iItem, ReportBuffer, MspPageSize);
		if (Status != MSP_STATUS_OK) {
			return 0;
		}

	}
	else {

		Count = MspGetFilteredRecordCount(FileObject);
		if (!Count || ((ULONG)lpvi->iItem > Count - 1)) {
			return 0;
		}

		Status = MspCopyRecordFiltered(FileObject, lpvi->iItem, ReportBuffer, MspPageSize);
		if (Status != MSP_STATUS_OK) {
			return 0;
		}

	}
      
	Record = (PBTR_RECORD_HEADER)ReportBuffer;

    if (lpvi->mask & LVIF_TEXT){
		
		switch (lpvi->iSubItem) {

			case LogColumnSequence:

				if (lpvi->mask & LVIF_IMAGE) {
					lpvi->mask &= ~LVIF_IMAGE;
					lpvi->iImage = 0;
				}

				if (lpvi->stateMask & (LVIS_OVERLAYMASK | LVIS_STATEIMAGEMASK)) {
					lpvi->state = 0;
				}

				MspDecodeRecord(DtlObject, Record, DECODE_SEQUENCE, Buffer, lpvi->cchTextMax);
				break;

			case LogColumnProcess:
				MspDecodeRecord(DtlObject, Record, DECODE_PROCESS, Buffer, lpvi->cchTextMax);
				break;

			case LogColumnTimestamp:
				MspDecodeRecord(DtlObject, Record, DECODE_TIMESTAMP, Buffer, lpvi->cchTextMax);
				break;

			case LogColumnPID:
				MspDecodeRecord(DtlObject, Record, DECODE_PID, Buffer, lpvi->cchTextMax);
				break;

			case LogColumnTID:
				MspDecodeRecord(DtlObject, Record, DECODE_TID, Buffer, lpvi->cchTextMax);
				break;

			case LogColumnProbe:
				MspDecodeRecord(DtlObject, Record, DECODE_PROBE, Buffer, lpvi->cchTextMax);
				break;

			case LogColumnProvider:
				MspDecodeRecord(DtlObject, Record, DECODE_PROVIDER, Buffer, lpvi->cchTextMax);
				break;

			case LogColumnDuration:
				MspDecodeRecord(DtlObject, Record, DECODE_DURATION, Buffer, lpvi->cchTextMax);
				break;

			case LogColumnReturn:	
				MspDecodeRecord(DtlObject, Record, DECODE_RETURN, Buffer, lpvi->cchTextMax);
				break;

			case LogColumnDetail:
				MspDecodeRecord(DtlObject, Record, DECODE_DETAIL, Buffer, lpvi->cchTextMax);
				break;
		}

		StringCchCopy(lpvi->pszText, lpvi->cchTextMax, Buffer);
    }
	
	if (lpvi->iSubItem == LogColumnProcess) {
		lpvi->iImage = ReportQueryImageList(Object, Record->ProcessId);
		lpvi->mask |= LVIF_IMAGE;
	}

    return 0L;
}

INT_PTR CALLBACK
ReportProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	switch (uMsg) {

		case WM_INITDIALOG:
			return ReportOnInitDialog(hWnd, uMsg, wp, lp);

		case WM_CTLCOLORSTATIC:
			return ReportOnCtlColorStatic(hWnd, uMsg, wp, lp);

		case WM_NOTIFY:
			return ReportOnNotify(hWnd, uMsg, wp, lp);

		case WM_COMMAND:
			return ReportOnCommand(hWnd, uMsg, wp, lp);

		case WM_CONTEXTMENU:
			return ReportOnContextMenu(hWnd, uMsg, wp, lp);

		case WM_USER_GENERATE_COUNTER:
			return ReportOnUserGenerateCounter(hWnd, uMsg, wp, lp);
	}

	if (uMsg == WM_USER_FILTER) {
		ReportOnWmUserFilter(hWnd, uMsg, wp, lp);
	}

	return 0;
}

ULONG CALLBACK
ReportDialogThread(
	IN PVOID Context
	)
{
	HACCEL hAccel;
	MSG msg;
	HWND hWndReport;
	PMSP_DTL_OBJECT DtlObject;

	hAccel = LoadAccelerators(SdkInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));
	ASSERT(hAccel != NULL);

	DtlObject = (PMSP_DTL_OBJECT)Context;
	hWndReport = ReportDialog(NULL, DtlObject);
	ASSERT(hWndReport != NULL);

	ShowWindow(hWndReport, SW_SHOW);
	while (GetMessage(&msg, NULL, 0, 0)) {

		if (msg.message == WM_QUIT) {
			DestroyWindow(hWndReport);
			break;
		}
		
		if (!TranslateAccelerator(hWndReport, hAccel, &msg)) {
	        if (hWndReport != NULL && !IsDialogMessage(hWndReport, &msg)) {
	            TranslateMessage(&msg);
	            DispatchMessage(&msg);
	        }
		}
	}

	return 0;
}

HWND
ReportLoadBars(
	IN HWND hWnd,
	IN UINT Id
	)
{
	HWND hWndRebar;
	HWND hWndEdit;
	HWND hWndToolbar;
	HIMAGELIST Normal;
	HIMAGELIST Grayed;
	REBARINFO RebarInfo = {0};
	REBARBANDINFO BandInfo = {0};
	LRESULT Result;
	RECT Rect;
	SIZE Size;

	//
	// Create rebar control
	//

	hWndRebar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
							   WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | RBS_VARHEIGHT | 
							   WS_CLIPSIBLINGS | CCS_TOP | CCS_NODIVIDER | RBS_AUTOSIZE|
							   WS_GROUP,
							   0, 0, 0, 0, hWnd, UlongToHandle(Id), SdkInstance, NULL);
	if (!hWndRebar) {
		return NULL;
	}

	RebarInfo.cbSize  = sizeof(REBARINFO);
	Result = SendMessage(hWndRebar, RB_SETBARINFO, 0, (LPARAM)&RebarInfo);
	if (!Result) {
		return 0;
	}
	
	SendMessage(hWndRebar, RB_SETUNICODEFORMAT, (WPARAM)TRUE, (LPARAM)0);

	hWndToolbar = ReportCreateToolbar(hWndRebar, &Normal, &Grayed);
	SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0);
	SendMessage(hWndToolbar, TB_GETMAXSIZE, 0, (LPARAM)&Size);

	hWndEdit = ReportCreateEdit(hWndRebar);

	//
	// Insert edit band
	//

	BandInfo.cbSize = sizeof(REBARBANDINFO);
	BandInfo.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_STYLE;

	BandInfo.cxMinChild = 0;
	BandInfo.cyMinChild = Size.cy; 
	BandInfo.cx = 0;
	BandInfo.fStyle = RBBS_NOGRIPPER | RBBS_TOPALIGN;
	BandInfo.wID = REPORT_BAND_EDIT;
	BandInfo.hwndChild = hWndEdit;

	Result = SendMessage(hWndRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&BandInfo);
	if (!Result) {
		return NULL;
	}
	
	SendMessage(hWndEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(TRUE, 0));

	//
	// Then insert toolbar band
	//

	ZeroMemory(&BandInfo, sizeof(BandInfo));

	BandInfo.cbSize = sizeof(REBARBANDINFO);
	BandInfo.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_STYLE;

	GetWindowRect(hWndToolbar, &Rect);

	BandInfo.cxMinChild = Size.cx;
	BandInfo.cyMinChild = Size.cy;
	BandInfo.cx = Size.cx;
	BandInfo.fStyle = RBBS_NOGRIPPER | RBBS_TOPALIGN | RBBS_FIXEDSIZE;
	BandInfo.wID = REPORT_BAND_TOOLBAR;
	BandInfo.hwndChild = hWndToolbar;

	Result = SendMessage(hWndRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&BandInfo);
	if (!Result) {
		return NULL;
	}

	return hWndRebar;
}

HWND
ReportCreateToolbar(
	IN HWND hWndRebar,
	OUT HIMAGELIST *Normal,
	OUT HIMAGELIST *Grayed
	)
{
	HWND hWnd;
	TBBUTTON Tb = {0};
	TBBUTTONINFO Tbi = {0};
	ULONG Number;
	HIMAGELIST himlNormal;
	HIMAGELIST himlGrayed;
	HICON hicon;
	ULONG Extended;

	hWnd = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, 
		                  WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP |
	                      TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_TOP | CCS_NORESIZE,
						  0, 0, 0, 0, 
		                  hWndRebar, (HMENU)UlongToHandle(REPORT_BAND_TOOLBAR), SdkInstance, NULL);

    ASSERT(hWnd != NULL);

	//
	// Set extended style to support button arrow
	//

	Extended = (ULONG)SendMessage(hWnd, TB_GETEXTENDEDSTYLE, 0, 0);
	SendMessage(hWnd, TB_SETEXTENDEDSTYLE, 0, Extended | TBSTYLE_EX_DRAWDDARROWS);

	//
	// Insert toolbar buttons
	//

    SendMessage(hWnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0L);
	SendMessage(hWnd, TB_ADDBUTTONS, sizeof(ReportToolbarButtons) / sizeof(TBBUTTON), 
		        (LPARAM)ReportToolbarButtons);

	SendMessage(hWnd, TB_AUTOSIZE, 0, 0);

	Number = sizeof(ReportToolbarButtons) / sizeof(TBBUTTON);

	himlNormal = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, Number, Number);
	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_FINDBACKWARD));
	ImageList_AddIcon(himlNormal, hicon);
	
	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_FINDFORWARD));
	ImageList_AddIcon(himlNormal, hicon);
	
	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_FILTER));
	ImageList_AddIcon(himlNormal, hicon);
	
	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_SUMMARY));
	ImageList_AddIcon(himlNormal, hicon);
	
	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_SAVE));
	ImageList_AddIcon(himlNormal, hicon);

    SendMessage(hWnd, TB_SETIMAGELIST, 0L, (LPARAM)himlNormal);
    SendMessage(hWnd, TB_SETHOTIMAGELIST, 0L, (LPARAM)himlNormal);

	himlGrayed = SdkCreateGrayImageList(himlNormal);
    SendMessage(hWnd, TB_SETDISABLEDIMAGELIST, 0L, (LPARAM)himlGrayed);

	*Normal = himlNormal;
	*Grayed = himlGrayed;

	return hWnd;
}

HWND
ReportCreateEdit(
	IN HWND hWndRebar
	)
{
	HWND hWnd;

	hWnd = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", NULL, 
                          WS_CHILD | WS_BORDER | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | ES_LEFT, 
						  0, 0, 0, 0, hWndRebar, (HMENU)REPORT_BAND_EDIT, SdkInstance, NULL);
    ASSERT(hWnd != NULL);
	Edit_SetCueBannerText(hWnd, L"Search");
	return hWnd;
}

LRESULT
ReportOnFindForward(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PREPORT_CONTEXT Context;
    FIND_CALLBACK FindCallback;
	LONG Current;
	HWND hWndRebar;
	HWND hWndEdit;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PREPORT_CONTEXT)Object->Context;
	FindCallback = Context->FindContext.FindCallback;
	
	if (!FindCallback || !Context->FindContext.DtlObject) {
		return 0L;
	}

	Current = ListViewGetFirstSelected(Context->FindContext.hWndList);
	Context->FindContext.PreviousIndex = Current;

	hWndRebar = GetDlgItem(hWnd, REPORT_REBAR_ID);
	hWndEdit = GetDlgItem(hWndRebar, REPORT_BAND_EDIT);
	GetWindowText(hWndEdit, Context->FindContext.FindString, MAX_PATH);

	__try {
		Context->FindContext.FindForward = TRUE;
		(*FindCallback)(&Context->FindContext);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
	}

	return 0L;
}

LRESULT
ReportOnFindBackward(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PREPORT_CONTEXT Context;
    FIND_CALLBACK FindCallback;
	LONG Current;
	HWND hWndRebar;
	HWND hWndEdit;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PREPORT_CONTEXT)Object->Context;
	FindCallback = Context->FindContext.FindCallback;
	
	if (!FindCallback || !Context->FindContext.DtlObject) {
		return 0L;
	}

	Current = ListViewGetFirstSelected(Context->FindContext.hWndList);
	Context->FindContext.PreviousIndex = Current;

	hWndRebar = GetDlgItem(hWnd, REPORT_REBAR_ID);
	hWndEdit = GetDlgItem(hWndRebar, REPORT_BAND_EDIT);
	GetWindowText(hWndEdit, Context->FindContext.FindString, MAX_PATH);

	__try {
		Context->FindContext.FindForward = FALSE;
		(*FindCallback)(&Context->FindContext);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
	}

	return 0L;
}
	
LRESULT
ReportOnCopy(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndList;
	LONG Index;
	LONG Count;
	LONG Number;
	ULONG Length;
	WCHAR Path[MAX_PATH];
	WCHAR FileName[MAX_PATH];
	HANDLE FileHandle;
	HANDLE MappingHandle;
	PVOID MappedAddress;
	ULONG Complete;
	ULONG Status;
	HANDLE Data;
	PVOID Memory;
	PDIALOG_OBJECT Object;
	PREPORT_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, REPORT_CONTEXT);

	hWndList = GetDlgItem(hWnd, IDC_LIST_REPORT);
	Count = ListView_GetSelectedCount(hWndList);
	if (!Count) {
		return 0;
	}

	if (Count > 100) {
		MessageBox(hWnd, L"Only first 100 records are copied.", L"D Probe", MB_OK|MB_ICONINFORMATION);
		Count = 100;
	}

	//
	// Create temporary file to store decoded records
	//

	GetTempPath(MAX_PATH, Path);
	GetTempFileName(Path, L"btr", 0, FileName);

	FileHandle = CreateFile(FileName, GENERIC_READ|GENERIC_WRITE, 
		                    FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, 
						    CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
	if (FileHandle == INVALID_HANDLE_VALUE) {
		MessageBox(hWnd, L"Copy failed!", L"D Probe", MB_OK|MB_ICONERROR);
		return 0;
	}

	Number = 0;
	Index = ListViewGetFirstSelected(hWndList);

	while (Index != -1) {

		LogViewCopyRecordByIndex(Index, FileHandle, Context->DtlObject);
		Index = ListViewGetNextSelected(hWndList, Index);

		Number += 1;
		if (Number >= 100) {
			break;
		}
	}

	//
	// Terminate UNICODE string
	//

	Status = 0;
	WriteFile(FileHandle, &Status, sizeof(Status), &Complete, NULL);
	SetEndOfFile(FileHandle);

	MappingHandle = CreateFileMapping(FileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!MappingHandle) {
		Status = GetLastError();
		MessageBox(hWnd, L"Copy failed!", L"D Probe", MB_OK|MB_ICONERROR);
		goto Exit;
	}

	MappedAddress = MapViewOfFile(MappingHandle, FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, 0);
	if (!MappedAddress) {
		Status = GetLastError();
		MessageBox(hWnd, L"Copy failed!", L"D Probe", MB_OK|MB_ICONERROR);
		goto Exit;
	}

	//
	// The quirky clipboard API has to use GlobalAlloc etc.
	//

	Length = (ULONG)wcslen(MappedAddress);
    Data = GlobalAlloc(GMEM_MOVEABLE, sizeof(WCHAR) * (Length + 1));
	Memory = GlobalLock(Data);
	RtlCopyMemory(Memory, MappedAddress, sizeof(WCHAR) * (Length + 1)); 
	GlobalUnlock(Data);

	if (OpenClipboard(hWnd)) {
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, Data);
        CloseClipboard();
    }

	GlobalFree(Data);

Exit:

	if (MappedAddress) {
		UnmapViewOfFile(MappedAddress);
	}

	if (MappingHandle) {
		CloseHandle(MappingHandle);
	}

	if (FileHandle) {
		CloseHandle(FileHandle);
	}

	DeleteFile(FileName);
	return 0;
}

LRESULT
ReportOnSummary(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PREPORT_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, REPORT_CONTEXT);

	CounterDialog(hWnd, Context->DtlObject); 
	return 0;
}

LRESULT
ReportOnSave(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	OPENFILENAME Ofn;
	WCHAR Path[MAX_PATH];	
	MSP_SAVE_TYPE Type;
	PREPORT_CONTEXT Context;
	PDIALOG_OBJECT Object;

	ZeroMemory(&Ofn, sizeof Ofn);
	ZeroMemory(Path, sizeof(Path));

	Ofn.lStructSize = sizeof(Ofn);
	Ofn.hwndOwner = hWnd;
	Ofn.hInstance = SdkInstance;
	Ofn.lpstrFilter = L"D Probe Trace Log (*.dtl)\0*.dtl\0Comma Separated Value (*.csv)\0\0";
	Ofn.lpstrFile = Path;
	Ofn.nMaxFile = sizeof(Path); 
	Ofn.lpstrTitle = L"D Probe";
	Ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT; 

	if (!GetSaveFileName(&Ofn)) {
		return 0;
	}

	StringCchCopy(Path, MAX_PATH, Ofn.lpstrFile);
	wcslwr(Path);

	if (Ofn.nFilterIndex == 1) {
		if (!wcsstr(Path, L".dtl")) {
			StringCchCat(Path, MAX_PATH, L".dtl");
		}
		Type = MSP_SAVE_DTL;
	} 
	
	if (Ofn.nFilterIndex == 2) {
		if (!wcsstr(Path, L".csv")) {
			StringCchCat(Path, MAX_PATH, L".csv");
		}
		Type = MSP_SAVE_CSV;
	}
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, REPORT_CONTEXT);

	SaveDialog(hWnd, Context->DtlObject, Path, Type);
	return 0;
}

LRESULT
ReportOnImportFilter(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	ULONG Status;
	WCHAR Path[MAX_PATH];
	OPENFILENAME Ofn;
	PMSP_DTL_OBJECT DtlObject;
	PCONDITION_OBJECT Object;
	PCONDITION_OBJECT New;
	PCONDITION_OBJECT Current;
	BOOLEAN Updated;
	BOOLEAN IsIdentical;
	INT_PTR Return;
	PREPORT_CONTEXT Context;
	PDIALOG_OBJECT Dialog;

	ZeroMemory(&Ofn, sizeof Ofn);
	ZeroMemory(Path, sizeof(Path));

	//
	// Get dfc file name from file dialog
	//

	Ofn.lStructSize = sizeof(Ofn);
	Ofn.hwndOwner = hWnd;
	Ofn.hInstance = SdkInstance;
	Ofn.lpstrFilter = L"D Probe Filter Condition (*.dfc)\0*.dfc\0\0";
	Ofn.lpstrFile = Path;
	Ofn.nMaxFile = sizeof(Path); 
	Ofn.lpstrTitle = L"D Probe";
	Ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	Status = GetOpenFileName(&Ofn);
	if (!Status) {
		return 0;
	}

	StringCchCopy(Path, MAX_PATH, Ofn.lpstrFile);
	Status = ConditionImportFile(Path, &Object);
	if (Status != S_OK) {
		MessageBox(hWnd, L"Failed to read filter condition file!", L"D Probe", MB_OK | MB_ICONERROR);
		return 0;
	}

	Return = ConditionDialog(hWnd, Object, &New, &Updated);
	if (Return != IDOK) {
		ConditionFree(Object);
		return 0;
	}

	Dialog = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Dialog, REPORT_CONTEXT);

	DtlObject = Context->DtlObject;
	Current = MspGetFilterCondition(DtlObject);
	IsIdentical = FALSE;

	if (!Current) {

		if (Updated) {
			MspAttachFilterCondition(DtlObject, New);
			ConditionFree(Object);
		}
		else {
			MspAttachFilterCondition(DtlObject, Object);
		}
	}
	else {

		if (Updated) {

			if (ConditionIsIdentical(New, Current) != TRUE) {
				Current = MspAttachFilterCondition(DtlObject, New);
				ConditionFree(Current);
			} else {
				IsIdentical = TRUE;
			}

			ConditionFree(Object);

		}
		else {

			if (ConditionIsIdentical(Object, Current) != TRUE) {
				Current = MspAttachFilterCondition(DtlObject, Object);
				ConditionFree(Current);
			} else {
				ConditionFree(Object);
				IsIdentical = TRUE;
			}
		}
	}

	if (!IsIdentical) {
		ReportOnWmUserFilter(hWnd, WM_USER_FILTER, 0, 0);
	}

	return 0;
}

LRESULT
ReportOnExportFilter(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	OPENFILENAME Ofn;
	ULONG Status;
	WCHAR Path[MAX_PATH];	
	PWCHAR Extension;
	PMSP_DTL_OBJECT DtlObject;
	PCONDITION_OBJECT Condition;
	PDIALOG_OBJECT Object;
	PREPORT_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, REPORT_CONTEXT);
	DtlObject = Context->DtlObject;

	Condition = MspGetFilterCondition(DtlObject);
	if (!Condition) {
		return 0;
	}

	ZeroMemory(&Ofn, sizeof Ofn);
	ZeroMemory(Path, sizeof(Path));

	Ofn.lStructSize = sizeof(Ofn);
	Ofn.hwndOwner = hWnd;
	Ofn.hInstance = SdkInstance;
	Ofn.lpstrFilter = L"D Probe Filter Condition (*.dfc)\0*.dfc\0\0";
	Ofn.lpstrFile = Path;
	Ofn.nMaxFile = sizeof(Path); 
	Ofn.lpstrTitle = L"D Probe";
	Ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT; 

	GetSaveFileName(&Ofn);

	StringCchCopy(Path, MAX_PATH, Ofn.lpstrFile);
	wcslwr(Path);

	Extension = wcsstr(Path, L".dfc");
	if (!Extension) {
		StringCchCat(Path, MAX_PATH, L".dfc");
	}

	DeleteFile(Path);

	Status = ConditionExportFile(Condition, Path);
	if (Status != S_OK) {
		MessageBox(hWnd, L"Failed to export filter condition file!", L"D Probe", MB_OK|MB_ICONERROR);
	}

	return 0;
}

LRESULT
ReportOnResetFilter(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
    PMSP_DTL_OBJECT DtlObject;
	PMSP_FILTER_FILE_OBJECT FileObject;
	PCONDITION_OBJECT Condition;
	PDIALOG_OBJECT Object;
	PREPORT_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, REPORT_CONTEXT);

	DtlObject = Context->DtlObject;
	FileObject = MspGetFilterFileObject(DtlObject);

	if (!FileObject) {
		return 0;
	}

	Condition = MspDetachFilterCondition(DtlObject);
	ConditionFree(Condition);

	ReportOnWmUserFilter(hWnd, uMsg, wp, lp);
    return 0;
}

LRESULT
ReportOnFilter(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
    PMSP_DTL_OBJECT DtlObject;
	PCONDITION_OBJECT Current;
	PCONDITION_OBJECT Old;
	PCONDITION_OBJECT New;
	BOOLEAN Updated;
	PDIALOG_OBJECT Object;
	PREPORT_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, REPORT_CONTEXT);

	DtlObject = Context->DtlObject;
	Current = MspGetFilterCondition(DtlObject);

	if (!Current) {
		
		Old = (PCONDITION_OBJECT)SdkMalloc(sizeof(CONDITION_OBJECT));
		RtlZeroMemory(Old, sizeof(*Old));
		InitializeListHead(&Old->ConditionListHead);

		ConditionDialog(hWnd, Old, &New, &Updated);

		if (Updated) {
			MspAttachFilterCondition(DtlObject, New);
		}
		else {
			ASSERT(!New);
		}
		
		ConditionFree(Old);
		return 0;

	}

	ConditionDialog(hWnd, Current, &New, &Updated);

	if (Updated) {
		Old = MspAttachFilterCondition(DtlObject, New);
		ASSERT(Old == Current);
		ConditionFree(Current);
	}

	if (Updated) {
		ReportOnWmUserFilter(hWnd, uMsg, wp, lp);
	}

    return 0;
}

ULONG
ReportFindCallback(
    IN struct _FIND_CONTEXT *Object
    )
{
	ULONG Current;
	PMSP_DTL_OBJECT DtlObject;
	PMSP_FILTER_FILE_OBJECT FileObject;
    PBTR_RECORD_HEADER Record;
	PWCHAR Buffer;
    PWCHAR Result;
	WCHAR FindString[MAX_PATH];

	Record = (PBTR_RECORD_HEADER)_alloca(BspPageSize);
	Buffer = (PWCHAR)_alloca(BspPageSize * sizeof(WCHAR));

    DtlObject = Object->DtlObject;
	FileObject = MspGetFilterFileObject(DtlObject);

	StringCchCopy(FindString, MAX_PATH, Object->FindString);
	wcslwr(FindString);

	//
	// If its previous index is zero and user is requesting backward find,
	// just return here since it ends
	//

	if (!Object->FindForward && Object->PreviousIndex == 0) {
		return 0;
	}

	if (Object->FindForward) {
	    Current = Object->PreviousIndex + 1;
	} else {
		Current = Object->PreviousIndex - 1;
	}

    if (MspGetRecordCount(DtlObject) < Current) {
        return 0;
    }

    Result = NULL;

	//
	// N.B. Find only support timestamp, probe, return and detail, total 4 fields.
	// furthmore, find is case sensitive.
	//

    while (FindIsComplete(Object, DtlObject, Current) != TRUE) {

		if (!FileObject) {
	        MspCopyRecord(DtlObject, Current, Record, BspPageSize);
		} else {
	        MspCopyRecordFiltered(FileObject, Current, Record, BspPageSize);
		}

		MspDecodeRecord(DtlObject, Record, DECODE_TIMESTAMP, Buffer, BspPageSize);
		wcslwr(Buffer);
        Result = wcsstr(Buffer, FindString);
        if (Result != NULL) {
            break;
        }

		MspDecodeRecord(DtlObject, Record, DECODE_PROBE, Buffer, BspPageSize);
		wcslwr(Buffer);
        Result = wcsstr(Buffer, FindString);
        if (Result != NULL) {
            break;
        }
		
		MspDecodeRecord(DtlObject, Record, DECODE_RETURN, Buffer, BspPageSize);
		wcslwr(Buffer);
        Result = wcsstr(Buffer, FindString);
        if (Result != NULL) {
            break;
        }

		MspDecodeRecord(DtlObject, Record, DECODE_DETAIL, Buffer, BspPageSize);
		wcslwr(Buffer);
        Result = wcsstr(Buffer, FindString);
        if (Result != NULL) {
            break;
        }

		if (Object->FindForward) {
	        Current += 1;
		} else {
			Current -= 1;
		}
    }

	//
	// Truncate to 32 bits, we don't support 64 bit record count currently
	//

    if (Current < (ULONG)MspGetRecordCount(DtlObject)) {

		ListViewSelectSingle(Object->hWndList, Current);
		ListView_EnsureVisible(Object->hWndList, Current, FALSE);
        SetFocus(Object->hWndList);

        Object->PreviousIndex = Current;
        return Current;

    } else {

        return FIND_NOT_FOUND;
    }
}

VOID
ReportEnterStallMode(
	IN PDIALOG_OBJECT Object 
	)
{
	RECT Rect;
	HWND hWndList;
	PREPORT_CONTEXT Context;

	Context = SdkGetContext(Object, REPORT_CONTEXT);
	Context->ReportMode = LOGVIEW_STALL_MODE;

	hWndList = GetDlgItem(Object->hWnd, IDC_LIST_REPORT);
	ListView_SetItemCount(hWndList, 0);

	GetClientRect(hWndList, &Rect);
	InvalidateRect(hWndList, &Rect, TRUE);
	UpdateWindow(hWndList);
}

VOID
ReportExitStallMode(
	IN PDIALOG_OBJECT Object,
	IN ULONG Count
	)
{
	PREPORT_CONTEXT Context;
	HWND hWndList;

	Context = SdkGetContext(Object, REPORT_CONTEXT);
	Context->ReportMode = LOGVIEW_UPDATE_MODE;

	hWndList = GetDlgItem(Object->hWnd, IDC_LIST_REPORT);
	ListView_SetItemCount(hWndList, Count);

	ListView_RedrawItems(hWndList, 0, Count);
	UpdateWindow(hWndList);
}

BOOLEAN
ReportIsInStallMode(
	IN PDIALOG_OBJECT Object	
	)
{
	PREPORT_CONTEXT Context;
	Context = SdkGetContext(Object, REPORT_CONTEXT);
	return (Context->ReportMode == LOGVIEW_STALL_MODE) ? TRUE : FALSE;
}
	
LRESULT
ReportOnFilterCommand(
	IN HWND hWnd,
	IN UINT CommandId
	)
{
    ULONG Status;
	PDIALOG_OBJECT Object;
	PREPORT_CONTEXT Context;
	PMSP_DTL_OBJECT DtlObject;
	PMSP_FILTER_FILE_OBJECT FileObject;
	PCONDITION_OBJECT Condition;
	PBTR_RECORD_HEADER Record;
	WCHAR Value[MAX_PATH];
	PCONDITION_ENTRY Entry;
	HWND hWndList;
	int index;

	//
	// Get current user selected record
	//

	hWndList = GetDlgItem(hWnd, IDC_LIST_REPORT);
	index = ListViewGetFirstSelected(hWndList);
	if (index == -1) {
		return 0;
	}
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, REPORT_CONTEXT);
	DtlObject = Context->DtlObject;

	FileObject = MspGetFilterFileObject(DtlObject);

	Record = (PBTR_RECORD_HEADER)_alloca(BspPageSize);

	if (FileObject != NULL) {
		Status = MspCopyRecordFiltered(FileObject, index, Record, BspPageSize);
	} else {
		Status = MspCopyRecord(DtlObject, index, Record, BspPageSize);
	}

	if (Status != MSP_STATUS_OK) {
		return 0;
	}

	//
	// Build new condition by user selected command id
	// Include commands, map to EQUAL
	// Exclude commands, map to NOT EQUAL
	//
	//

	Entry = (PCONDITION_ENTRY)SdkMalloc(sizeof(CONDITION_ENTRY));
	
	switch (CommandId) {

		case ID_INCLUDE_PROCESS:
		case ID_EXCLUDE_PROCESS:

			Entry->Object = ConditionObjectProcess;
			Entry->Relation = ConditionRelationEqual;

			MspDecodeRecord(DtlObject, Record, DECODE_PROCESS, Value, MAX_PATH);
			StringCchCopy(Entry->StringValue, MAX_PATH, Value);

			if (CommandId == ID_INCLUDE_PROCESS) {
				Entry->Action = ConditionActionInclude;
			} else {
				Entry->Action = ConditionActionExclude;
			}
			break;

		case ID_INCLUDE_PID:
		case ID_EXCLUDE_PID:

			Entry->Object = ConditionObjectPID;
			Entry->Relation = ConditionRelationEqual;

			MspDecodeRecord(DtlObject, Record, DECODE_PID, Value, MAX_PATH);
			StringCchCopy(Entry->StringValue, MAX_PATH, Value);

			if (CommandId == ID_INCLUDE_PID) {
				Entry->Action = ConditionActionInclude;
			} else {
				Entry->Action = ConditionActionExclude;
			}
			break;

		case ID_INCLUDE_TID:
		case ID_EXCLUDE_TID:

			Entry->Object = ConditionObjectTID;
			Entry->Relation = ConditionRelationEqual;

			MspDecodeRecord(DtlObject, Record, DECODE_TID, Value, MAX_PATH);
			StringCchCopy(Entry->StringValue, MAX_PATH, Value);

			if (CommandId == ID_INCLUDE_TID) {
				Entry->Action = ConditionActionInclude;
			} else {
				Entry->Action = ConditionActionExclude;
			}
			break;
			
		case ID_INCLUDE_PROBE:
		case ID_EXCLUDE_PROBE:

			Entry->Object = ConditionObjectProbe;
			Entry->Relation = ConditionRelationEqual;

			MspDecodeProbe(DtlObject, Record, Value, MAX_PATH, FALSE);
			StringCchCopy(Entry->StringValue, MAX_PATH, Value);

			if (CommandId == ID_INCLUDE_PROBE) {
				Entry->Action = ConditionActionInclude;
			} else {
				Entry->Action = ConditionActionExclude;
			}
			break;
			
		case ID_INCLUDE_PROVIDER:
		case ID_EXCLUDE_PROVIDER:

			Entry->Object = ConditionObjectProvider;
			Entry->Relation = ConditionRelationEqual;

			MspDecodeRecord(DtlObject, Record, DECODE_PROVIDER, Value, MAX_PATH);
			StringCchCopy(Entry->StringValue, MAX_PATH, Value);

			if (CommandId == ID_INCLUDE_PROVIDER) {
				Entry->Action = ConditionActionInclude;
			} else {
				Entry->Action = ConditionActionExclude;
			}
			break;
			
		case ID_INCLUDE_RETURN:
		case ID_EXCLUDE_RETURN:
			
			Entry->Object = ConditionObjectReturn;
			Entry->Relation = ConditionRelationEqual;

			MspDecodeRecord(DtlObject, Record, DECODE_RETURN, Value, MAX_PATH);
			StringCchCopy(Entry->StringValue, MAX_PATH, Value);

			if (CommandId == ID_INCLUDE_RETURN) {
				Entry->Action = ConditionActionInclude;
			} else {
				Entry->Action = ConditionActionExclude;
			}
			break;

		case ID_INCLUDE_DETAIL:
		case ID_EXCLUDE_DETAIL:

			Entry->Object = ConditionObjectDetail;
			Entry->Relation = ConditionRelationContain;

			MspDecodeRecord(DtlObject, Record, DECODE_DETAIL, Value, MAX_PATH);
			StringCchCopy(Entry->StringValue, MAX_PATH, Value);
			
			if (CommandId == ID_INCLUDE_DETAIL) {
				Entry->Action = ConditionActionInclude;
			} else {
				Entry->Action = ConditionActionExclude;
			}
			break;

		default:
			ASSERT(0);
	}

	if (FileObject != NULL) {

		Condition = MspDetachFilterCondition(DtlObject);
		InsertHeadList(&Condition->ConditionListHead, &Entry->ListEntry);
		Condition->NumberOfConditions += 1;
		Condition->DtlObject = NULL;

		MspAttachFilterCondition(DtlObject, Condition);

	} else {

		Condition = (PCONDITION_OBJECT)SdkMalloc(sizeof(CONDITION_OBJECT));
		InitializeListHead(&Condition->ConditionListHead);
		Condition->NumberOfConditions = 1;
		Condition->DtlObject = NULL;
		InsertHeadList(&Condition->ConditionListHead, &Entry->ListEntry);

		MspAttachFilterCondition(DtlObject, Condition);
	}

	ReportOnWmUserFilter(hWnd, WM_USER_FILTER, 0, 0);
    return 0;
}