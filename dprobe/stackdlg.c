//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "dprobe.h"
#include "stackdlg.h"
#include "mspdtl.h"

HWND
StackDialog(
	IN HWND hWndParent,
	IN struct _BTR_STACKTRACE_ENTRY *Entry
	)
{
	PDIALOG_OBJECT Object;

	Object = (PDIALOG_OBJECT)SdkMalloc(sizeof(DIALOG_OBJECT));
	Object->Context = Entry;
	Object->hWndParent = hWndParent;
	Object->ResourceId = IDD_DIALOG_STACK;
	Object->Procedure = StackProcedure;

	return DialogCreateModeless(Object);
}

LRESULT
StackOnInitDialog(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	HWND hWndChild;
	PDIALOG_OBJECT Object;
	LVITEM lvi = {0};
	LVCOLUMN lvc = {0};
	PMSP_STACKTRACE_DECODED Entry;

	SetWindowText(hWnd, L"Call Stack");
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)SdkMainIcon);

	hWndChild = GetDlgItem(hWnd, IDC_LIST_STACKTRACE);
	ASSERT(hWnd != NULL);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Entry = (PMSP_STACKTRACE_DECODED)Object->Context;

    ListView_SetUnicodeFormat(hWndChild, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndChild, 
		                                LVS_EX_FULLROWSELECT,  
										LVS_EX_FULLROWSELECT);

	lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 
    lvc.iSubItem = 0;
	lvc.pszText = L"#";	
	lvc.cx = 25;     
	lvc.fmt = LVCFMT_LEFT;

	ListView_InsertColumn(hWndChild, 0, &lvc);

    lvc.iSubItem = 1;
	lvc.pszText = L"Frame";	
	lvc.cx = 360;     
	lvc.fmt = LVCFMT_LEFT;

	ListView_InsertColumn(hWndChild, 1, &lvc);

	//
	// Insert stack trace frames
	//

	if (Entry != NULL) {
		StackUpdateData(hWnd, Entry);
	}
	return 0;
}

LRESULT
StackOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	switch (LOWORD(wp)) {

		case IDC_BUTTON_COPY:
			StackOnCopy(hWnd, uMsg, wp, lp);
			break;
	}

	return 0;
}


LRESULT
StackOnDestroy(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PMSP_STACKTRACE_DECODED Entry;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Entry = (PMSP_STACKTRACE_DECODED)Object->Context;

	if (Entry != NULL) {
		SdkFree(Entry);
	}

	return 0;
}

LRESULT
StackOnCopy(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndList;
	LONG Number;
	LONG Count; 
	SIZE_T Length;
	LVITEM Item;
	HANDLE Data;
	PVOID Memory;

    WCHAR Text[MAX_PATH];
	WCHAR Buffer[MAX_PATH * 60];

	hWndList = GetDlgItem(hWnd, IDC_LIST_STACKTRACE);

	//
	// N.B. Don't set focus, otherwise it will cause
	// some clipped part does not be erased correctly,
	// a really weird issue.
	//
	//SetFocus(hWndList);

	Count = ListView_GetItemCount(hWndList); 
	if (!Count) {
		return 0;
	}

	Text[0] = 0;
	Buffer[0] = 0;

	StringCchCat(Buffer, MAX_PATH * 60, L"#Stack\r\n");

	for(Number = 0; Number < Count; Number += 1) {

		Item.mask = LVIF_TEXT;
		Item.iItem = Number;
		Item.iSubItem = 0;
		Item.cchTextMax = sizeof(Text) / sizeof(WCHAR);
		Item.pszText = Text;

		//
		// Copy frame #
		//

		ListView_GetItem(hWndList, &Item);
		StringCchCat(Buffer, MAX_PATH * 60, Text);
		StringCchCat(Buffer, MAX_PATH * 60, L"  ");

		//
		// Copy frame text
		//

		Item.iSubItem = 1;
		ListView_GetItem(hWndList, &Item);
		StringCchCat(Buffer, MAX_PATH * 60, Text);
		StringCchCat(Buffer, MAX_PATH * 60, L"\r\n");

	}

	StringCchLength(Buffer, MAX_PATH * 60, &Length);
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
	return 0;
}

INT_PTR CALLBACK
StackProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	switch (uMsg) {

		case WM_COMMAND:
			StackOnCommand(hWnd, uMsg, wp, lp);
			break;

		case WM_INITDIALOG:
			StackOnInitDialog(hWnd, uMsg, wp, lp);
			SdkCenterWindow(hWnd);
			return FALSE;

		case WM_DESTROY:
			StackOnDestroy(hWnd, uMsg, wp, lp);
			break;
	}
	
	return 0;
}

ULONG
StackUpdateData(
	IN HWND hWnd,
	IN PMSP_STACKTRACE_DECODED Decoded 
	)
{
	ULONG Depth;
	LVITEM lvi = {0};
	HWND hWndList;
	WCHAR Buffer[MAX_PATH];
	PWCHAR Unicode;
	PDIALOG_OBJECT Object;
	PMSP_STACKTRACE_DECODED Old;

	if (!Decoded) {
		return 0;
	}

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Old = (PMSP_STACKTRACE_DECODED)Object->Context;
	if (Old != NULL) {
		SdkFree(Old);
	}

	//
	// Attach update stack trace record to object context
	//

	Object->Context = (PVOID)Decoded;

	hWndList = GetDlgItem(hWnd, IDC_LIST_STACKTRACE);
	ListView_DeleteAllItems(hWndList);

	for(Depth = 0; Depth < Decoded->Depth; Depth += 1) {
		
		memset(&lvi, 0, sizeof(lvi));

		lvi.iItem = Depth;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT;

		wsprintf(Buffer, L"%02d", Depth);
		lvi.pszText = Buffer;
		ListView_InsertItem(hWndList, &lvi);

		lvi.iItem = Depth;
		lvi.iSubItem = 1;
		lvi.mask = LVIF_TEXT;

		BspConvertUTF8ToUnicode(Decoded->Text[Depth], &Unicode);
		lvi.pszText = Unicode; 

		//
		// N.B. For subitem, ListView_SetItem must be used instead of 
		// ListView_InsertItem
		//

		ListView_SetItem(hWndList, &lvi);
		BspFree(Unicode);
	}

	return 0;
}