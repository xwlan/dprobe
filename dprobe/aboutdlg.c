//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#include "dprobe.h"
#include "aboutdlg.h"
#include "dialog.h"

typedef struct _ABOUT_CONTEXT {
	HFONT hFontBold;
} ABOUT_CONTEXT, *PABOUT_CONTEXT;

LISTVIEW_COLUMN AbountColumn[3] = {
	{ 120, L"Module",   LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 90,  L"Version",  LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 148, L"Description",  LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

VOID
AboutDialog(
	IN HWND hWndParent
	)
{
	DIALOG_OBJECT Object = {0};
	ABOUT_CONTEXT Context = {0};

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_ABOUTBOX;
	Object.Procedure = AboutProcedure;
	
	DialogCreate(&Object);
}

INT_PTR CALLBACK
AboutProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LRESULT Status = 0;

	switch (uMsg) {

		case WM_INITDIALOG:
			return AboutOnInitDialog(hWnd, uMsg, wp, lp);			

		case WM_COMMAND:
			if (LOWORD(wp) == IDOK || LOWORD(wp) == IDCANCEL) {
				return AboutOnOk(hWnd, uMsg, wp, lp);
			}
		
		case WM_CTLCOLORSTATIC:
			return AboutOnCtlColorStatic(hWnd, uMsg, wp, lp);

		case WM_NOTIFY:
			return AboutOnNotify(hWnd, uMsg, wp, lp);
	}
	
	return Status;
}

LRESULT
AboutOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	NMLINK *lpnmlink = (NMLINK *)lp;

	if ((int)wp == IDC_SYSLINK) {
		if (lpnmlink->hdr.code == NM_CLICK) {
			ShellExecuteW(NULL, L"open", L"mailto:dprobebugs@gmail.com?subject=bug report", NULL, NULL, SW_SHOWNORMAL);  
			return 0;
		}
	}

	return 0;
}

LRESULT
AboutOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndChild;
	PDIALOG_OBJECT Object;
	PABOUT_CONTEXT Context;
	WCHAR Buffer[MAX_PATH];
	LVCOLUMN Column = {0};
	LVITEM Item = {0};
	PBSP_MODULE Module;
	LIST_ENTRY ListHead;
	int i;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PABOUT_CONTEXT)Object->Context;

	hWndChild = GetDlgItem(hWnd, IDC_STATIC_PRODUCT);
	LoadString(SdkInstance, IDS_PRODUCT, Buffer, MAX_PATH);

	if (BspIs64Bits) {
		wcscat_s(Buffer, MAX_PATH, L" x64");
	}
	SetWindowText(hWndChild, Buffer);
	
	hWndChild = GetDlgItem(hWnd, IDC_STATIC_BUILD);
	LoadString(SdkInstance, IDS_BUILD, Buffer, MAX_PATH);
	SetWindowText(hWndChild, Buffer);
	
	hWndChild = GetDlgItem(hWnd, IDC_LIST_VERSION);
	ListView_SetUnicodeFormat(hWndChild, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndChild, 
		                                LVS_EX_GRIDLINES,  
		                                LVS_EX_GRIDLINES);

	ListView_SetExtendedListViewStyleEx(hWndChild, 
		                                LVS_EX_FULLROWSELECT,  
		                                LVS_EX_FULLROWSELECT);

	Column.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 

	for (i = 0; i < 3; i++) { 
        Column.iSubItem = i;
		Column.pszText = AbountColumn[i].Title;	
		Column.cx = AbountColumn[i].Width;     
		Column.fmt = AbountColumn[i].Align;
		ListView_InsertColumn(hWndChild, i, &Column);
    } 

	InitializeListHead(&ListHead);
	BspQueryModule(GetCurrentProcessId(), TRUE, &ListHead);

	//
	// dprobe version
	//

	Module = BspGetModuleByName(&ListHead, L"dprobe.exe");

	Item.iItem = 0;
	Item.mask = LVIF_TEXT;

	Item.iSubItem = 0;
	Item.pszText = Module->Name;
	ListView_InsertItem(hWndChild, &Item);

	Item.iSubItem = 1;
	Item.pszText = Module->Version;
	ListView_SetItem(hWndChild, &Item);

	Item.iSubItem = 2;
	Item.pszText = Module->Description;
	ListView_SetItem(hWndChild, &Item);

	//
	// runtime version
	//

	Module = BspGetModuleByName(&ListHead, L"dprobe.btr.dll");

	Item.iItem = 1;
	Item.mask = LVIF_TEXT;

	Item.iSubItem = 0;
	Item.pszText = Module->Name;
	ListView_InsertItem(hWndChild, &Item);

	Item.iSubItem = 1;
	Item.pszText = Module->Version;
	ListView_SetItem(hWndChild, &Item);

	Item.iSubItem = 2;
	Item.pszText = Module->Description;
	ListView_SetItem(hWndChild, &Item);

	BspFreeModuleList(&ListHead);

	SetWindowText(hWnd, L"About");
	SdkSetMainIcon(hWnd);
	SdkCenterWindow(hWnd);
	return TRUE;
}

LRESULT
AboutOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PABOUT_CONTEXT Context;
	UINT Id;
	HFONT hFont;
	LOGFONT LogFont;
	RECT Rect;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PABOUT_CONTEXT)Object->Context;

	Id = GetDlgCtrlID((HWND)lp);
	if (Id == IDC_STATIC_PRODUCT) {

		if (!Context->hFontBold) { 

			hFont = GetCurrentObject((HDC)wp, OBJ_FONT);
			GetObject(hFont, sizeof(LOGFONT), &LogFont);
			LogFont.lfWeight = FW_BOLD;
			LogFont.lfQuality = CLEARTYPE_QUALITY;
			LogFont.lfCharSet = ANSI_CHARSET;
			hFont = CreateFontIndirect(&LogFont);

			Context->hFontBold = hFont;
			SendMessage((HWND)lp, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE);

			GetClientRect((HWND)lp, &Rect);
			InvalidateRect((HWND)lp, &Rect, TRUE);
		}

	}

	return 0;
}

LRESULT
AboutOnOk(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PABOUT_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, ABOUT_CONTEXT);

	if (Context->hFontBold) {
		DeleteFont(Context->hFontBold);
	}

	EndDialog(hWnd, IDOK);
	return TRUE;
}
