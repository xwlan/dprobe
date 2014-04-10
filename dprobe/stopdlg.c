//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "sdk.h"
#include "stopdlg.h"
#include "bsp.h"
#include "dialog.h"
#include "applydlg.h"
#include "mspprocess.h"

typedef enum _StopColumnType {
	StopColumnName,
	StopColumnPID,
	StopColumnSID,
	StopColumnArgument,
	StopColumnNumber,
} StopColumnType;

LISTVIEW_COLUMN StopColumn[StopColumnNumber] = {
	{ 160,  L"Name",     LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 40,   L"PID",      LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 40,   L"SID",      LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 400,  L"Argument", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

typedef struct _STOP_CONTEXT {
	LIST_ENTRY ProcessList;
	ULONG ProcessCount;
	HFONT hBoldFont;
} STOP_CONTEXT, *PSTOP_CONTEXT;


INT_PTR
StopDialog(
	IN HWND hWndParent,
	OUT PLIST_ENTRY ListHead,
	OUT PULONG Count
	)
{
	DIALOG_OBJECT Object = {0};
	STOP_CONTEXT Context = {0};
	PLIST_ENTRY ListEntry;
	INT_PTR Return;

	InitializeListHead(ListHead);
	InitializeListHead(&Context.ProcessList);

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_STOP;
	Object.Procedure = StopProcedure;

	Return = DialogCreate(&Object);

	while (IsListEmpty(&Context.ProcessList) != TRUE) {
		ListEntry = RemoveHeadList(&Context.ProcessList);
		InsertTailList(ListHead, ListEntry);
	}

	*Count = Context.ProcessCount;
	return Return;
}

LRESULT
StopOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	HWND hWndList;
	HWND hWndText;
	LVCOLUMN Column = {0};
	int i;

	hWndList = GetDlgItem(hWnd, IDC_LIST_STOP);
	ListView_SetUnicodeFormat(hWndList, TRUE);
	ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	Column.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 

	for (i = 0; i < StopColumnNumber; i++) { 

		Column.iSubItem = i;
		Column.pszText = StopColumn[i].Title;	
		Column.cx = StopColumn[i].Width;     
		Column.fmt = StopColumn[i].Align;

		ListView_InsertColumn(hWndList, i, &Column);
	} 

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	StopInsertProcess(Object);

	SetFocus(hWndList);
	ListViewSelectSingle(hWndList, 0);

	hWndText = GetDlgItem(hWnd, IDC_STATIC);
	SetWindowText(hWndText, L"Press CTRL to select multiple processes");
	SdkSetMainIcon(hWnd);
	SdkCenterWindow(hWnd);
	return 0;
}

LRESULT CALLBACK
StopProcedure(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	switch (uMsg) {

		case WM_INITDIALOG: 
			return StopOnInitDialog(hWnd, uMsg, wp, lp);

		case WM_COMMAND:
			return StopOnCommand(hWnd, uMsg, wp, lp);

		case WM_CTLCOLORSTATIC:
			return StopOnCtlColorStatic(hWnd, uMsg, wp, lp);
		
	}

	return 0;
}

ULONG
StopInsertProcess(
	IN PDIALOG_OBJECT Object
	)
{
	PLIST_ENTRY ListEntry;
	LIST_ENTRY ListHead;
	PBSP_PROCESS Process;
	LVITEM lvi = {0};
	WCHAR Buffer[MAX_PATH];
	BOOLEAN IsProbing;
	ULONG CurrentId;
	HANDLE ProcessHandle;
	HWND hWndList;
	int i = 0;

	CurrentId = GetCurrentProcessId();
	InitializeListHead(&ListHead);
	hWndList = GetDlgItem(Object->hWnd, IDC_LIST_STOP);

	BspQueryProcessList(&ListHead);

	while (IsListEmpty(&ListHead) != TRUE) {

		ListEntry = RemoveHeadList(&ListHead);
		Process = CONTAINING_RECORD(ListEntry, BSP_PROCESS, ListEntry);

		if (Process->ProcessId == 0 || Process->ProcessId == 4 ||
			Process->ProcessId == CurrentId ) {
			BspFreeProcess(Process);	
			continue;
		}
		
		if (!_wcsicmp(Process->Name, L"dprobe.exe") || 
			!_wcsicmp(Process->Name, L"dprobe64.exe")) {
			BspFreeProcess(Process);	
			continue;
		}

		if (BspIs64Bits) {

			ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, Process->ProcessId);
			if (!ProcessHandle) {
				BspFreeProcess(Process);	
				continue;
			}

			if (BspIsWow64Process(ProcessHandle)) {
				CloseHandle(ProcessHandle);
				BspFreeProcess(Process);	
				continue;
			}

			CloseHandle(ProcessHandle);
		} 

		else if (BspIsWow64) {
			
			ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, Process->ProcessId);
			if (!ProcessHandle) {
				BspFreeProcess(Process);	
				continue;
			}

			if (!BspIsWow64Process(ProcessHandle)) {
				CloseHandle(ProcessHandle);
				BspFreeProcess(Process);	
				continue;
			}
			
			CloseHandle(ProcessHandle);
		}
		
		IsProbing = MspIsProbing(Process->ProcessId);
		if (!IsProbing) {
			BspFreeProcess(Process);	
			continue;
		}

		RtlZeroMemory(&lvi, sizeof(lvi));
		
		//
		// Name
		//

		lvi.iItem = i;
		lvi.iSubItem = StopColumnName;
		lvi.mask = LVIF_TEXT | LVIF_PARAM;
		lvi.pszText = wcslwr(Process->Name);
		lvi.lParam = (LPARAM)Process;
		ListView_InsertItem(hWndList, &lvi);

		//
		// PID
		//

		lvi.iSubItem = StopColumnPID;
		lvi.mask = LVIF_TEXT;
		wsprintf(Buffer, L"%d", Process->ProcessId);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndList, &lvi);
		
		//
		// SID
		//

		lvi.iSubItem = StopColumnSID;
		lvi.mask = LVIF_TEXT;
		wsprintf(Buffer, L"%d", Process->SessionId);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndList, &lvi);
		
		//
		// Argument
		//

		lvi.iSubItem = StopColumnArgument;
		lvi.mask = LVIF_TEXT;
		lvi.pszText = Process->CommandLine;
		ListView_SetItem(hWndList, &lvi);

		i += 1;
	}

	return i;
}

LRESULT
StopOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PSTOP_CONTEXT Context;
	PBSP_PROCESS Process;
	HWND hWndList;
	int index;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, STOP_CONTEXT);

	hWndList = GetDlgItem(hWnd, IDC_LIST_STOP);
	index = ListViewGetFirstSelected(hWndList);

	while (index != -1) {

		ListViewGetParam(hWndList, index, (LPARAM *)&Process);	

		InsertTailList(&Context->ProcessList, &Process->ListEntry);
		Context->ProcessCount += 1;

		ListViewSetParam(hWndList, index, (LPARAM)NULL);	
		index = ListViewGetNextSelected(hWndList, index);
	}
	
	EndDialog(hWnd, IDOK);
	return TRUE;
}

LRESULT
StopOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	StopCleanUp(hWnd);
	EndDialog(hWnd, IDCANCEL);
	return 0;
}

LRESULT
StopOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	switch (LOWORD(wp)) {

	case IDOK:
		StopOnOk(hWnd, uMsg, wp, lp);
	    break;

	case IDCANCEL:
		StopOnCancel(hWnd, uMsg, wp, lp);
	    break;
	}

	return 0;
}

VOID
StopCleanUp(
	IN HWND hWnd
	)
{
	PDIALOG_OBJECT Object;
	PSTOP_CONTEXT Context;
	PBSP_PROCESS Process;
	HWND hWndList;
	int Count, i;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, STOP_CONTEXT);
	hWndList = GetDlgItem(hWnd, IDC_LIST_STOP);

	Count = ListView_GetItemCount(hWndList);
	for(i = 0; i < Count; i++) {
		ListViewGetParam(hWndList, i, (LPARAM *)&Process);
		if (Process != NULL) {
			BspFreeProcess(Process);
		}
	}

	if (Context->hBoldFont) {
		DeleteFont(Context->hBoldFont);
	}
}

LRESULT
StopOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PSTOP_CONTEXT Context;
	UINT Id;
	HFONT hFont;
	LOGFONT LogFont;
	RECT Rect;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PSTOP_CONTEXT)Object->Context;
	hFont = Context->hBoldFont;

	Id = GetDlgCtrlID((HWND)lp);
	if (Id == IDC_STATIC) {
		if (!hFont) {

			hFont = GetCurrentObject((HDC)wp, OBJ_FONT);
			GetObject(hFont, sizeof(LOGFONT), &LogFont);
			LogFont.lfWeight = FW_BOLD;
			hFont = CreateFontIndirect(&LogFont);

			Context->hBoldFont = hFont;
			SendMessage((HWND)lp, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE);

			GetClientRect((HWND)lp, &Rect);
			InvalidateRect((HWND)lp, &Rect, TRUE);
		}
	}

	return (LRESULT)0;
}