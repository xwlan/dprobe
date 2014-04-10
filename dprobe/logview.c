//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "dprobe.h"
#include "logview.h"
#include "stackdlg.h"
#include "find.h"
#include "frame.h"
#include "conditiondlg.h"
#include "filterhint.h"
#include "detaildlg.h"
#include "mspdtl.h"
#include "mspprobe.h"

int LogViewColumnOrder[LogColumnCount] = {
	LogColumnSequence,
	LogColumnProcess,
	LogColumnTimestamp,
	LogColumnPID,
	LogColumnTID,
	LogColumnProbe,
	LogColumnProvider,
	LogColumnReturn,
	LogColumnDuration,
	LogColumnDetail,
};

//
// N.B. Process and Sequence column order are swapped when displayed
//

LISTVIEW_COLUMN LogViewColumn[LogColumnCount] = {
	{ 80, L"Process",		 LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 60,  L"Sequence",      LVCFMT_RIGHT,0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 100, L"Time",          LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 60,  L"PID",           LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 60,  L"TID",           LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 100, L"Probe",         LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 100, L"Provider",      LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 60,  L"Return",        LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 80,  L"Duration (ms)", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 360, L"Detail",        LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText }
};

LISTVIEW_OBJECT LogViewObject = 
{ NULL, 0, LogViewColumn, LogColumnCount, NULL, NULL, NULL, 0, 0 };

typedef struct _LOGVIEW_OBJECT {
	LISTVIEW_OBJECT ListObject;
	HIMAGELIST himlSmall;
	LIST_ENTRY ImageListHead;
	LOGVIEW_MODE Mode;
	BOOLEAN AutoScroll;
} LOGVIEW_OBJECT, *PLOGVIEW_OBJECT;

HIMAGELIST LogViewSmallIcon;
LIST_ENTRY LogViewImageList;

LOGVIEW_MODE LogViewMode = LOGVIEW_UPDATE_MODE;
BOOLEAN LogViewAutoScroll = FALSE;
HFONT LogViewBoldFont;
HFONT LogViewThinFont;

//
// N.B. record buffer is constructed as a guarded region,
//      the first and the last pages are no access, the 
//      wrapped 256K is accessible.
//

PVOID LogViewBuffer;
ULONG LogViewBufferSize;


#define LOGVIEW_TIMER_ID  6

ULONG
LogViewInitImageList(
	IN HWND	hWnd
	)
{
	SHFILEINFO sfi = {0};
	PLOGVIEW_IMAGE Object;
	HICON hicon;
	ULONG Index;

	LogViewSmallIcon = ImageList_Create(GetSystemMetrics(SM_CXSMICON), 
		                                GetSystemMetrics(SM_CYSMICON), 
										ILC_COLOR32, 0, 0); 

	ListView_SetImageList(hWnd, LogViewSmallIcon, LVSIL_SMALL);
    
	//
	// Add first icon as shell32.dll's default icon, this is
	// for those failure case when process's icon can't be extracted.
	//

	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_GENERIC));
	Index = ImageList_AddIcon(LogViewSmallIcon, hicon);
	DestroyIcon(sfi.hIcon);
	
	Object = (PLOGVIEW_IMAGE)SdkMalloc(sizeof(LOGVIEW_IMAGE));
	Object->ProcessId = -1;
	Object->ImageIndex = 0;

	InitializeListHead(&LogViewImageList);
	InsertHeadList(&LogViewImageList, &Object->ListEntry);

	return 0;
}

ULONG
LogViewAddImageList(
	IN PWSTR FullPath,
	IN ULONG ProcessId
	)
{
	SHFILEINFO sfi = {0};
	PLOGVIEW_IMAGE Object;
	ULONG Index;

	Index = LogViewQueryImageList(ProcessId);
	if (Index != 0) {
		return Index;
	}

	SHGetFileInfo(FullPath, 0, &sfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON);
	Index = ImageList_AddIcon(LogViewSmallIcon, sfi.hIcon);
	if (Index != -1) {
		DestroyIcon(sfi.hIcon);
	}
	
	Object = (PLOGVIEW_IMAGE)SdkMalloc(sizeof(LOGVIEW_IMAGE));
	Object->ProcessId = ProcessId;

	if (Index != -1) {
		Object->ImageIndex = Index;
	} else {
		Object->ImageIndex = 0;
	}

	InsertHeadList(&LogViewImageList, &Object->ListEntry);
	return Index;
}

ULONG
LogViewQueryImageList(
	IN ULONG ProcessId
	)
{
	PLIST_ENTRY ListEntry;
	PLOGVIEW_IMAGE Object;

	ListEntry = LogViewImageList.Flink;

	while (ListEntry != &LogViewImageList) {
		
		Object = CONTAINING_RECORD(ListEntry, LOGVIEW_IMAGE, ListEntry);
		if (Object->ProcessId == ProcessId) {
			return Object->ImageIndex;
		}
		ListEntry = ListEntry->Flink;
	}

	return 0;
}

PVOID
LogViewCreateBuffer(
	VOID
	)
{
	PVOID Address;

	//
	// N.B. The logview's buffer is built as a paged buffer,
	// the first page and last page are not committed, the
	// middle page is committed, this can help to detect buffer
	// overflow just in time.
	//

	Address = (PVOID)VirtualAlloc(NULL, BspPageSize * 3, MEM_RESERVE, PAGE_NOACCESS);
	Address = VirtualAlloc((PCHAR)Address + BspPageSize, BspPageSize, MEM_COMMIT, PAGE_READWRITE);

	LogViewBuffer = Address;
	LogViewBufferSize = BspPageSize;
	return Address;
}

VOID CALLBACK 
LogViewTimerProcedure(
	IN HWND hWnd,
    UINT uMsg,
    UINT_PTR idEvent,
    DWORD dwTime
	)
{
	if (idEvent == LOGVIEW_TIMER_ID) {
		LogViewOnTimer(hWnd, uMsg, 0, 0);
	}
}

PLISTVIEW_OBJECT
LogViewCreate(
	IN HWND hWndParent,
	IN ULONG CtrlId
	)
{
	HWND hWnd;
	DWORD dwStyle;
	ULONG i;
	LVCOLUMN lvc = {0}; 
	PMDB_DATA_ITEM MdbItem;
	BOOLEAN AutoScroll;

	dwStyle = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_CLIPSIBLINGS | 
		      LVS_OWNERDATA | LVS_SINGLESEL;
	
	hWnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, 0, dwStyle, 
						  0, 0, 0, 0, hWndParent, (HMENU)UlongToPtr(CtrlId), SdkInstance, NULL);

	ASSERT(hWnd != NULL);
	
    ListView_SetUnicodeFormat(hWnd, TRUE);

    dwStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
	SetWindowLongPtr(hWnd, GWL_STYLE, (dwStyle & ~LVS_TYPEMASK) | LVS_REPORT | LVS_SHOWSELALWAYS);

	SdkSetObject(hWnd, &LogViewObject);

	ListView_SetExtendedListViewStyleEx(hWnd, LVS_EX_FULLROWSELECT,  LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(hWnd, LVS_EX_HEADERDRAGDROP, LVS_EX_HEADERDRAGDROP); 
	ListView_SetExtendedListViewStyleEx(hWnd, LVS_EX_SUBITEMIMAGES , LVS_EX_SUBITEMIMAGES); 
	ListView_SetExtendedListViewStyleEx(hWnd, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER); 
	ListView_SetExtendedListViewStyleEx(hWnd, LVS_EX_INFOTIP, LVS_EX_INFOTIP); 

	//
	// Add support to imagelist (for Process Name column).
	//

	LogViewInitImageList(hWnd);

	//
	// N.B. Because the first column is always left aligned, we have to 
	// first insert a dummy column and then delete it to make the first
	// column text right aligned.
	//

	lvc.mask = 0; 
	ListView_InsertColumn(hWnd, 0, &lvc);

    for (i = 0; i < LogColumnCount; i++) { 

		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
        lvc.iSubItem = i;
		lvc.pszText = LogViewColumn[i].Title;	
		lvc.cx = LogViewColumn[i].Width;     
		lvc.fmt = LogViewColumn[i].Align;
		ListView_InsertColumn(hWnd, i + 1, &lvc);
    } 

	//
	// Delete dummy column
	//

	ListView_DeleteColumn(hWnd, 0);

	ListView_SetColumnOrderArray(hWnd, LogColumnCount, LogViewColumnOrder);

	LogViewObject.CtrlId = CtrlId;
	LogViewObject.hWnd = hWnd;
	
	LogViewBuffer = LogViewCreateBuffer();
	ASSERT(LogViewBuffer != NULL);

	SetTimer(hWnd, LOGVIEW_TIMER_ID, 1000, LogViewTimerProcedure);

	//
	// Set autoscroll if configured
	//

	MdbItem = MdbGetData(MdbAutoScroll);
	AutoScroll = (BOOLEAN)_wtoi(MdbItem->Value);
	LogViewSetAutoScroll(AutoScroll);

	return &LogViewObject;
}

VOID
LogViewSetItemCount(
	IN ULONG Count
	)
{
	if (LogViewMode != LOGVIEW_STALL_MODE) {

		if (!LogViewAutoScroll) {
			ListView_SetItemCountEx(LogViewObject.hWnd, Count, 
				                    LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
		} else {
			ListView_SetItemCount(LogViewObject.hWnd, Count);
			ListView_EnsureVisible(LogViewObject.hWnd, Count - 1, FALSE);
		}
	}
}

LRESULT 
LogViewOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LRESULT Status;
	LPNMHDR pNmhdr = (LPNMHDR)lp;

	Status = 0L;

	switch (pNmhdr->code) {
		
		case NM_CUSTOMDRAW:
			Status = LogViewOnCustomDraw(hWnd, uMsg, wp, lp);
			break;

		case LVN_GETDISPINFO:
			Status = LogViewOnGetDispInfo(hWnd, uMsg, wp, lp);
			break;
		
	}

	return Status;
}

LRESULT
LogViewOnOdCacheHint(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	return 0;
}

HFONT
LogViewCreateFont(
	IN HDC hdc
	)
{
	LOGFONT LogFont;
	HFONT hFont;

	//
	// Create bold font
	//

	hFont = GetCurrentObject(hdc, OBJ_FONT);
	LogViewThinFont = hFont;

	GetObject(hFont, sizeof(LOGFONT), &LogFont);
	LogFont.lfWeight = FW_BOLD;
	hFont = CreateFontIndirect(&LogFont);
	LogViewBoldFont = hFont;

	return LogViewBoldFont;
}

LRESULT
LogViewOnCustomDraw(
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
			} 
			else {
				lvcd->clrTextBk = RGB(255, 255, 255);
			}
			Status = CDRF_NEWFONT;
			break;
        
        default:
            Status = CDRF_DODEFAULT;
    }
    
    return Status;
}

ULONG
LogViewGetItemCount(
	VOID
	)
{
	return (ULONG)ListView_GetItemCount(LogViewObject.hWnd);
}

LRESULT 
LogViewOnContextMenu(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	int x, y;
	HMENU hMenu;
	HMENU hSubMenu;
	UINT Count;

	Count = ListView_GetSelectedCount(LogViewObject.hWnd);
	if (!Count) {
		return 0;
	}

	x = GET_X_LPARAM(lp); 
	y = GET_Y_LPARAM(lp); 

	if (x != -1 && y != -1) {
		
		if (Count == 1) { 
			hMenu = LoadMenu(SdkInstance, MAKEINTRESOURCE(IDR_MENU_LOGVIEW));
		}

		if (Count > 1) {
			hMenu = LoadMenu(SdkInstance, MAKEINTRESOURCE(IDR_MENU_LOGVIEW2));
		}

		hSubMenu = GetSubMenu(hMenu, 0);
		ASSERT(hSubMenu != NULL);

		TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
					   x, y, 0, hWnd, NULL);
		DestroyMenu(hMenu);

	} 

	return 0L;
}

LRESULT
LogViewOnGetDispInfo(
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

	#define BUFFER_LENGTH 2048 
	WCHAR Buffer[BUFFER_LENGTH];

    lpdi = (LV_DISPINFO *)lp;
    lpvi = &lpdi->item;
	Buffer[BUFFER_LENGTH - 1] = 0;

	if (LogViewIsInStallMode()) {
		return 0;
	}

	DtlObject = MspCurrentDtlObject();
	FileObject = MspGetFilterFileObject(DtlObject);

	if (!FileObject) {

		//
		// Ensure the request record index is bounded
		//

		Count = MspGetRecordCount(DtlObject);
		if (!Count || ((ULONG)lpvi->iItem > Count - 1)) {
			return 0;
		}

		Status = MspCopyRecord(DtlObject, lpvi->iItem, LogViewBuffer, LogViewBufferSize);
		if (Status != MSP_STATUS_OK) {
			return 0;
		}

	}
	else {

		Count = MspGetFilteredRecordCount(FileObject);
		if (!Count || ((ULONG)lpvi->iItem > Count - 1)) {
			return 0;
		}

		Status = MspCopyRecordFiltered(FileObject, lpvi->iItem, LogViewBuffer, LogViewBufferSize);
		if (Status != MSP_STATUS_OK) {
			return 0;
		}

	}
      
	Record = (PBTR_RECORD_HEADER)LogViewBuffer;

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

				MspDecodeRecord(DtlObject, Record, DECODE_SEQUENCE, Buffer, BUFFER_LENGTH);
				break;

			case LogColumnProcess:
				MspDecodeRecord(DtlObject, Record, DECODE_PROCESS, Buffer, BUFFER_LENGTH);
				break;

			case LogColumnTimestamp:
				MspDecodeRecord(DtlObject, Record, DECODE_TIMESTAMP, Buffer, BUFFER_LENGTH);
				break;

			case LogColumnPID:
				MspDecodeRecord(DtlObject, Record, DECODE_PID, Buffer, BUFFER_LENGTH);
				break;

			case LogColumnTID:
				MspDecodeRecord(DtlObject, Record, DECODE_TID, Buffer, BUFFER_LENGTH);
				break;

			case LogColumnProbe:
				MspDecodeRecord(DtlObject, Record, DECODE_PROBE, Buffer, BUFFER_LENGTH);
				break;

			case LogColumnProvider:
				MspDecodeRecord(DtlObject, Record, DECODE_PROVIDER, Buffer, BUFFER_LENGTH);
				break;

			case LogColumnDuration:
				MspDecodeRecord(DtlObject, Record, DECODE_DURATION, Buffer, BUFFER_LENGTH);
				break;

			case LogColumnReturn:	
				MspDecodeRecord(DtlObject, Record, DECODE_RETURN, Buffer,BUFFER_LENGTH);
				break;

			case LogColumnDetail:
				MspDecodeRecord(DtlObject, Record, DECODE_DETAIL, Buffer, BUFFER_LENGTH);
				break;
		}

		StringCchCopy(lpvi->pszText, lpvi->cchTextMax, Buffer);
    }
	
	if (lpvi->iSubItem == LogColumnProcess) {
		lpvi->iImage = LogViewQueryImageList(Record->ProcessId);
		lpvi->mask |= LVIF_IMAGE;
	}

    return 0L;
}


LRESULT
LogViewOnTimer(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PMSP_DTL_OBJECT DtlObject;
	PMSP_FILTER_FILE_OBJECT FileObject;
	ULONG Number;

	if (LogViewIsInStallMode()) {
		return 0;
	}

	DtlObject = MspCurrentDtlObject();
	FileObject = MspGetFilterFileObject(DtlObject);

	if (!FileObject) {
		Number = MspGetRecordCount(DtlObject);
		LogViewSetItemCount(Number);
	}
		
	else {

		//
		// N.B. We need improve the filter algorithm,
		// if Number is big, this can freeze UI, currently
		// scan step is 100000/s records
		//

		Number = MspComputeFilterScanStep(FileObject);
		
		if (Number > 1000 * 100) {
			Number = 1000 * 100;
		}

		Number = MspScanFilterRecord(FileObject, Number);
		LogViewSetItemCount(Number);

	} 

	PostMessage(GetParent(hWnd), WM_USER_STATUS, 0, 0);
	return 0;
}

LRESULT
LogViewProcedure(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LRESULT Result = 0;

	switch (uMsg) {

		case WM_NOTIFY:
			Result = LogViewOnNotify(hWnd, uMsg, wp, lp);
			break;

		case WM_CONTEXTMENU:
			Result = LogViewOnContextMenu(hWnd, uMsg, wp, lp);

		case WM_TIMER:
			LogViewOnTimer(hWnd, uMsg, wp, lp);
			break;
	}

	if (uMsg == WM_USER_FILTER) {
		LogViewOnWmUserFilter(hWnd, uMsg, wp, lp);
	}

	return Result;
}

ULONG
LogViewFindCallback(
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
LogViewEnterStallMode(
	VOID
	)
{
	RECT Rect;
	HWND hWnd;
	LONG Style;

	hWnd = LogViewObject.hWnd;

	KillTimer(hWnd, LOGVIEW_TIMER_ID);
	LogViewMode = LOGVIEW_STALL_MODE;

	//
	// Temporarily remove LVS_SHOWSELALWAYS
	//

	Style = GetWindowLong(hWnd, GWL_STYLE);
	Style &= ~LVS_SHOWSELALWAYS;
	SetWindowLong(hWnd, GWL_STYLE, Style);
	
	LogViewSetItemCount(0);

	GetClientRect(hWnd, &Rect);
	InvalidateRect(hWnd, &Rect, TRUE);
	UpdateWindow(hWnd);
}

VOID
LogViewExitStallMode(
	IN ULONG NumberOfRecords
	)
{
	HWND hWnd;
	LONG Style;

	hWnd = LogViewObject.hWnd;
	LogViewMode = LOGVIEW_UPDATE_MODE;

	//
	// Re-enabling LVS_SHOWSELALWAYS
	//

	Style = GetWindowLong(hWnd, GWL_STYLE);
	Style |= LVS_SHOWSELALWAYS;
	SetWindowLong(hWnd, GWL_STYLE, Style);

	//
	// N.B. according to MSDN, to force redraw immediately, we should call
	// UpdateWindow() after ListView_RedrawItems
	//

	LogViewSetItemCount(NumberOfRecords);
	ListView_RedrawItems(hWnd, 0, NumberOfRecords);
	UpdateWindow(LogViewObject.hWnd);
	
	SetTimer(hWnd, LOGVIEW_TIMER_ID, 1000, LogViewTimerProcedure);
}

BOOLEAN
LogViewIsInStallMode(
	VOID
	)
{
	return (LogViewMode == LOGVIEW_STALL_MODE);
}

LRESULT 
LogViewOnWmUserFilter(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PMSP_DTL_OBJECT DtlObject;
	PMSP_FILTER_FILE_OBJECT FileObject;
	ULONG Count;

	DtlObject = MspCurrentDtlObject();
	FileObject = MspGetFilterFileObject(DtlObject);

	if (FileObject != NULL) {
	
		LogViewEnterStallMode();

		FilterHintDialog(hWnd, DtlObject);
		Count = MspGetFilteredRecordCount(FileObject);

		LogViewExitStallMode(Count);
	}

	else {
		
		LogViewEnterStallMode();
		Count = MspGetRecordCount(DtlObject);
		LogViewExitStallMode(Count);

	}

	return 0;
}

VOID
LogViewSetAutoScroll(
	IN BOOLEAN AutoScroll
	)
{
	LogViewAutoScroll = AutoScroll;
}

BOOLEAN
LogViewGetAutoScroll(
	VOID
	)
{
	return LogViewAutoScroll;
}

int
LogViewGetFirstSelected(
	VOID
	)
{
	return ListViewGetFirstSelected(LogViewObject.hWnd);
}

LRESULT
LogViewOnCopy(
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
	PMSP_DTL_OBJECT DtlObject;

	hWndList = LogViewObject.hWnd;

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
	DtlObject = MspCurrentDtlObject();

	while (Index != -1) {

		LogViewCopyRecordByIndex(Index, FileHandle, DtlObject);
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

ULONG
LogViewCopyRecordByIndex(
	IN ULONG Index,
	IN HANDLE FileHandle,
	IN PMSP_DTL_OBJECT DtlObject
	)
{
	ULONG Status;
	PBTR_RECORD_HEADER Record;
	PMSP_FILTER_FILE_OBJECT FileObject;
	ULONG Count;
	ULONG Length;
	ULONG Complete;

	#define BUFFER_LENGTH 2048 
	WCHAR Buffer[BUFFER_LENGTH];

	Buffer[BUFFER_LENGTH - 1] = 0;

	if (LogViewIsInStallMode()) {
		ASSERT(0);
	}

	ASSERT(DtlObject != NULL);
	FileObject = MspGetFilterFileObject(DtlObject);

	if (!FileObject) {

		//
		// Ensure the request record index is bounded
		//

		Count = MspGetRecordCount(DtlObject);
		if (!Count || Index > Count - 1) {
			return 0;
		}

		Status = MspCopyRecord(DtlObject, Index, LogViewBuffer, LogViewBufferSize);
		if (Status != MSP_STATUS_OK) {
			return 0;
		}

	}
	else {

		Count = MspGetFilteredRecordCount(FileObject);
		if (!Count || Index > Count - 1) {
			return 0;
		}

		Status = MspCopyRecordFiltered(FileObject, Index, LogViewBuffer, LogViewBufferSize);
		if (Status != MSP_STATUS_OK) {
			return 0;
		}

	}
      
	Record = (PBTR_RECORD_HEADER)LogViewBuffer;

	MspDecodeRecord(DtlObject, Record, DECODE_SEQUENCE, Buffer, BUFFER_LENGTH);
	Length = (ULONG)wcslen(Buffer);
	Buffer[Length] = ' ';
	Buffer[Length + 1] = ' ';
	Length += 2;

	WriteFile(FileHandle, Buffer, Length * 2, &Complete, NULL);

	MspDecodeRecord(DtlObject, Record, DECODE_PROCESS, Buffer, BUFFER_LENGTH);
	Length = (ULONG)wcslen(Buffer);
	Buffer[Length] = ' ';
	Buffer[Length + 1] = ' ';
	Length += 2;

	WriteFile(FileHandle, Buffer, Length * 2, &Complete, NULL);

	MspDecodeRecord(DtlObject, Record, DECODE_TIMESTAMP, Buffer, BUFFER_LENGTH);
	Length = (ULONG)wcslen(Buffer);
	Buffer[Length] = ' ';
	Buffer[Length + 1] = ' ';
	Length += 2;

	WriteFile(FileHandle, Buffer, Length * 2, &Complete, NULL);

	MspDecodeRecord(DtlObject, Record, DECODE_PID, Buffer, BUFFER_LENGTH);
	Length = (ULONG)wcslen(Buffer);
	Buffer[Length] = ' ';
	Buffer[Length + 1] = ' ';
	Length += 2;

	WriteFile(FileHandle, Buffer, Length * 2, &Complete, NULL);

	MspDecodeRecord(DtlObject, Record, DECODE_TID, Buffer, BUFFER_LENGTH);
	Length = (ULONG)wcslen(Buffer);
	Buffer[Length] = ' ';
	Buffer[Length + 1] = ' ';
	Length += 2;

	WriteFile(FileHandle, Buffer, Length * 2, &Complete, NULL);

	MspDecodeRecord(DtlObject, Record, DECODE_PROBE, Buffer, BUFFER_LENGTH);
	Length = (ULONG)wcslen(Buffer);
	Buffer[Length] = ' ';
	Buffer[Length + 1] = ' ';
	Length += 2;

	WriteFile(FileHandle, Buffer, Length * 2, &Complete, NULL);

	MspDecodeRecord(DtlObject, Record, DECODE_PROVIDER, Buffer, BUFFER_LENGTH);
	Length = (ULONG)wcslen(Buffer);
	Buffer[Length] = ' ';
	Buffer[Length + 1] = ' ';
	Length += 2;

	WriteFile(FileHandle, Buffer, Length * 2, &Complete, NULL);

	MspDecodeRecord(DtlObject, Record, DECODE_DURATION, Buffer, BUFFER_LENGTH);
	Length = (ULONG)wcslen(Buffer);
	Buffer[Length] = ' ';
	Buffer[Length + 1] = ' ';
	Length += 2;

	WriteFile(FileHandle, Buffer, Length * 2, &Complete, NULL);

	MspDecodeRecord(DtlObject, Record, DECODE_RETURN, Buffer,BUFFER_LENGTH);
	Length = (ULONG)wcslen(Buffer);
	Buffer[Length] = ' ';
	Buffer[Length + 1] = ' ';
	Length += 2;

	WriteFile(FileHandle, Buffer, Length * 2, &Complete, NULL);

	MspDecodeRecord(DtlObject, Record, DECODE_DETAIL, Buffer, BUFFER_LENGTH);
	Length = (ULONG)wcslen(Buffer);
	Buffer[Length] = '\r';
	Buffer[Length + 1] = '\n';
	Length += 2;

	WriteFile(FileHandle, Buffer, Length * 2, &Complete, NULL);

	return 0L;
}