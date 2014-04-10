//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#include "recorddlg.h"
#include "logview.h"

typedef enum _RECORD_FIELD {
	RecordSequence,
	RecordProcessName,
	RecordStartTime,
	RecordProcessId,
	RecordThreadId,
	RecordProbeName,
	RecordProvider,
	RecordReturnValue,
	RecordDuration,
	RecordFieldNumber,
} RECORD_FIELD, *PRECORD_FIELD;

PWSTR RecordFieldName[] = {
	L"Sequence",
	L"Process",
	L"Time",
	L"PID",
	L"TID",
	L"Probe",
	L"Provider",
	L"Return",
	L"Duration (ms)",
};

LISTVIEW_COLUMN 
RecordColumn[2] = {
	{ 80, L"Name",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 330, L"Value", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText }
};

HWND
RecordDialog(
	IN HWND hWndParent	
	)
{
	PDIALOG_OBJECT Object;

	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	Object->Context = NULL;
	Object->hWndParent = hWndParent;
	Object->ResourceId = IDD_DIALOG_RECORD;
	Object->Procedure = RecordDialogProcedure;

	return DialogCreateModeless(Object);
}

INT_PTR CALLBACK
RecordDialogProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	INT_PTR Status;
	Status = FALSE;

	switch (uMsg) {
		
		case WM_INITDIALOG:
			RecordOnInitDialog(hWnd, uMsg, wp, lp);
			Status = TRUE;
			break;

		case WM_CTLCOLORSTATIC:
			Status = RecordOnCtlColorStatic(hWnd, uMsg, wp, lp);
			break;

		case WM_DESTROY:
			RecordOnDestroy(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
RecordOnCtlColorStatic(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	HBRUSH hBrush;

	if (GetDlgCtrlID((HWND)lp) == IDC_EDIT_RECORD) {
		Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
		hBrush = (HBRUSH)Object->Context;
		return (LRESULT)hBrush;
	}

	return 0L;
}

LRESULT
RecordOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndCtrl;
	LVCOLUMN Column = {0};
	LVITEM Item = {0};
	ULONG i;
	PDIALOG_OBJECT Object;
	HBRUSH hBrush;

	//
	// Create solid white brush
	//

	hBrush = CreateSolidBrush(RGB(255, 255, 255));
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Object->Context = (PVOID)hBrush;

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_RECORD);
	ListView_SetUnicodeFormat(hWndCtrl, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
    ListView_SetExtendedListViewStyleEx(hWndCtrl, LVS_EX_GRIDLINES, LVS_EX_GRIDLINES);
	
    Column.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 
	  
    for (i = 0; i < 2; i++) { 

        Column.iSubItem = i;
		Column.pszText = RecordColumn[i].Title;	
		Column.cx = RecordColumn[i].Width;     
		Column.fmt = RecordColumn[i].Align;

		ListView_InsertColumn(hWndCtrl, i, &Column);
    } 

	for (i = 0; i < RecordFieldNumber; i++) {

		Item.mask = LVIF_TEXT;
		Item.iItem = i;
		Item.iSubItem = 0;

		Item.pszText = RecordFieldName[i];
		ListView_InsertItem(hWndCtrl, &Item);
		
		Item.iSubItem = 1;
		Item.pszText = L"N/A";
		ListView_SetItem(hWndCtrl, &Item);
	}

	return 0L;
}

VOID
RecordUpdateData(
	IN HWND hWnd,
	IN HWND hWndLogView,
	IN PBTR_RECORD_HEADER Record
	)
{
	HWND hWndList;
	LONG Current;
	LVITEM Item = {0};
	WCHAR Buffer[2048];

	if (!Record) {
		return;
	}

	hWndList = GetDlgItem(hWnd, IDC_LIST_RECORD);
	Current = ListViewGetFirstSelected(hWndLogView);
	if (Current == -1) {
		return;
	}

	ListView_GetItemText(hWndLogView, Current, LogColumnSequence, Buffer, MAX_PATH);
	ListView_SetItemText(hWndList, RecordSequence, 1, Buffer);

	ListView_GetItemText(hWndLogView, Current, LogColumnProcess, Buffer, MAX_PATH);
	ListView_SetItemText(hWndList, RecordProcessName, 1, Buffer);

	ListView_GetItemText(hWndLogView, Current, LogColumnTimestamp, Buffer, MAX_PATH);
	ListView_SetItemText(hWndList, RecordStartTime, 1, Buffer);

	ListView_GetItemText(hWndLogView, Current, LogColumnPID, Buffer, MAX_PATH);
	ListView_SetItemText(hWndList, RecordProcessId, 1, Buffer);

	ListView_GetItemText(hWndLogView, Current, LogColumnTID, Buffer, MAX_PATH);
	ListView_SetItemText(hWndList, RecordThreadId, 1, Buffer);

	ListView_GetItemText(hWndLogView, Current, LogColumnProbe, Buffer, MAX_PATH);
	ListView_SetItemText(hWndList, RecordProbeName, 1, Buffer);

	ListView_GetItemText(hWndLogView, Current, LogColumnProvider, Buffer, MAX_PATH);
	ListView_SetItemText(hWndList, RecordProvider, 1, Buffer);

	ListView_GetItemText(hWndLogView, Current, LogColumnReturn, Buffer, MAX_PATH);
	ListView_SetItemText(hWndList, RecordReturnValue, 1, Buffer);

	ListView_GetItemText(hWndLogView, Current, LogColumnDuration, Buffer, MAX_PATH);
	ListView_SetItemText(hWndList, RecordDuration, 1, Buffer);

	ListView_GetItemText(hWndLogView, Current, LogColumnDetail, Buffer, MAX_PATH);
	SetDlgItemText(hWnd, IDC_EDIT_RECORD, Buffer);
}

LRESULT
RecordOnDestroy(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	if (Object->Context) {
		DeleteObject((HGDIOBJ)Object->Context);
	}

	return 0;
}

LRESULT
RecordOnCopy(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndList;
	LONG Number;
	LONG Count; 
	LVITEM Item;
	HANDLE Data;
	PVOID Memory;
	PWCHAR Buffer;
	SIZE_T Length;
	SIZE_T BufferLength;

    WCHAR Text[MAX_PATH];
	WCHAR Align[32];

	hWndList = GetDlgItem(hWnd, IDC_LIST_RECORD);
	Count = ListView_GetItemCount(hWndList); 
	Buffer = (PWCHAR)SdkMalloc(4096 * 2);
	BufferLength = 4096;

	Text[0] = 0;
	Buffer[0] = 0;

	//
	// Prefix a record property header
	//

	StringCchCat(Buffer, BufferLength, L"#Property \r\n");

	for(Number = 0; Number < Count; Number += 1) {

		Item.mask = LVIF_TEXT;
		Item.iItem = Number;
		Item.iSubItem = 0;
		Item.cchTextMax = sizeof(Text) / sizeof(WCHAR);
		Item.pszText = Text;

		//
		// Copy field name
		//

		ListView_GetItem(hWndList, &Item);
		StringCchPrintf(Align, 32, L"%-16ws", Text);
		StringCchCat(Buffer, BufferLength, Align);
		StringCchCat(Buffer, BufferLength, L"  ");

		//
		// Copy field value 
		//

		Item.iSubItem = 1;
		ListView_GetItem(hWndList, &Item);
		StringCchCat(Buffer, BufferLength, Text);
		StringCchCat(Buffer, BufferLength, L"\r\n");

	}

	//
	// Copy decoded parameters
	//

	StringCchCat(Buffer, BufferLength, L"\r\n#Parameters\r\n");

	StringCchLength(Buffer, BufferLength, &Length);
	Length += GetDlgItemText(hWnd, IDC_EDIT_RECORD, (PWCHAR)(Buffer + Length), 
		                     (ULONG)(BufferLength - Length - 1));

    Data = GlobalAlloc(GMEM_MOVEABLE, sizeof(WCHAR) * (Length + 1));
	Memory = GlobalLock(Data);
	RtlCopyMemory(Memory, Buffer, sizeof(WCHAR) * (Length + 1)); 
	GlobalUnlock(Data);

	if (OpenClipboard(hWnd)) {
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, Data);
        CloseClipboard();
    }

	GlobalFree(Data);
	SdkFree(Buffer);
	return 0;
}