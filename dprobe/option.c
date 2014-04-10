//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#include "option.h"
#include "dprobe.h"
#include "mdb.h"
#include "logview.h"
#include <shlobj.h>

VOID
OptionDialogCreate(
	IN HWND hWndParent	
	)
{
	DIALOG_OBJECT Object = {0};
	OPTION_DIALOG_CONTEXT Context = {0};
	
	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_OPTION;
	Object.Procedure = OptionDialogProcedure;

	DialogCreate(&Object);
}

INT_PTR CALLBACK
OptionDialogProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	switch (uMsg) {
		case WM_INITDIALOG: 
			return OptionOnInitDialog(hWnd, uMsg, wp, lp);
		case WM_COMMAND:
			return OptionOnCommand(hWnd, uMsg, wp, lp);
		case WM_DRAWITEM:
			return OptionOnDrawItem(hWnd, uMsg, wp, lp);
	}

	return 0;
}

LRESULT
OptionOnDrawItem(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	POPTION_DIALOG_CONTEXT Context;
	LPDRAWITEMSTRUCT lpdis;
	RECT Rect;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (POPTION_DIALOG_CONTEXT)Object->Context;

	lpdis = (LPDRAWITEMSTRUCT)lp; 
	Rect = lpdis->rcItem;

	
	if ((lpdis->itemAction | ODA_SELECT) && (lpdis->itemState & ODS_SELECTED)) {

		//
		// If the item is selected, paint its background as highlight color,
		// and set its text color as highlight text color
		//

		SetBkColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
		SetTextColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
        FillRect(lpdis->hDC, &Rect, (HBRUSH)(COLOR_ACTIVECAPTION + 1));

		if (!Context->hBrushFocus) {
			Context->hBrushFocus = CreateSolidBrush(RGB(255, 255, 255));
		}

		//
		// Draw the frame rect to make its boundary more clear
		// note that we actually draw 2 rects
		//

        Rect.top = lpdis->rcItem.top + 1;
		Rect.left = lpdis->rcItem.left + 1;
		Rect.right = lpdis->rcItem.right - 1;
		Rect.bottom = lpdis->rcItem.bottom - 1;
		FrameRect(lpdis->hDC, &Rect, Context->hBrushFocus);

	} else {

		//
		// Set text color as black as usual, background as white
		//

		SetTextColor(lpdis->hDC, RGB(0, 0, 0));
        FillRect(lpdis->hDC, &Rect, (HBRUSH)(COLOR_WINDOW + 1));      
	}
	
	//
	// Compute the rect to make text output centered in the item rect
	//

	Rect = lpdis->rcItem;
	Rect.top = Rect.top + 6;

	SetBkMode(lpdis->hDC, TRANSPARENT);
	DrawText(lpdis->hDC, (PWSTR)lpdis->itemData, -1, &Rect, DT_CENTER | DT_SINGLELINE);

	return 0L;
}

VOID 
OptionPageAdjustPosition(
	IN HWND hWndParent,
	IN HWND hWndPage
	)
{
	HWND hWndPos;
	HWND hWndDesktop;
	RECT Rect;

	hWndPos = GetDlgItem(hWndParent, IDC_STATIC);
	GetWindowRect(hWndPos, &Rect);

	hWndDesktop = GetDesktopWindow();
	MapWindowPoints(hWndDesktop, hWndParent, (LPPOINT)&Rect, 2);

	MoveWindow(hWndPage, Rect.left, Rect.top, 
		       Rect.right - Rect.left, 
			   Rect.bottom - Rect.top, TRUE);
}
	
LRESULT
OptionOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndList;
	RECT Rect;
	LONG Height;
	PDIALOG_OBJECT Object;
	POPTION_DIALOG_CONTEXT Context;
	HWND hWndPage;
	HWND hWndFirst;

	hWndList = GetDlgItem(hWnd, IDC_LIST_OPTION);
	GetClientRect(hWndList, &Rect);

	Height = (Rect.bottom - Rect.top) / 8;
	SendMessage(hWndList, LB_SETITEMHEIGHT, 0, MAKELONG(Height, 0));

	SendMessage(hWndList, LB_INSERTSTRING, -1, (LPARAM)L"General");
	SendMessage(hWndList, LB_INSERTSTRING, -1, (LPARAM)L"Advanced");
	SendMessage(hWndList, LB_INSERTSTRING, -1, (LPARAM)L"Symbol");
	SendMessage(hWndList, LB_INSERTSTRING, -1, (LPARAM)L"Others");

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (POPTION_DIALOG_CONTEXT)Object->Context;

	//
	// Create solid white brush to paint background of each pane its children's background
	//

	Context->hBrushPane = CreateSolidBrush(RGB(255, 255, 255));

	Context->Pages[OptionPageGeneral].Procedure = OptionGeneralProcedure;
	Context->Pages[OptionPageGeneral].hWndParent = hWnd;
	Context->Pages[OptionPageGeneral].ResourceId = IDD_DIALOG_OPTGENERAL;
	hWndPage = DialogCreateModeless(&Context->Pages[OptionPageGeneral]);
	ShowWindow(hWndPage, SW_HIDE);
	hWndFirst = hWndPage;

	Context->Pages[OptionPageAdvanced].Procedure = OptionAdvancedProcedure;
	Context->Pages[OptionPageAdvanced].hWndParent = hWnd;
	Context->Pages[OptionPageAdvanced].ResourceId = IDD_DIALOG_OPTADVANCED;
	hWndPage = DialogCreateModeless(&Context->Pages[OptionPageAdvanced]);
	ShowWindow(hWndPage, SW_HIDE);

	Context->Pages[OptionPageSymbol].Procedure = OptionSymbolProcedure;
	Context->Pages[OptionPageSymbol].hWndParent = hWnd;
	Context->Pages[OptionPageSymbol].ResourceId = IDD_DIALOG_OPTSYMBOL;
	DialogCreateModeless(&Context->Pages[OptionPageSymbol]);
	ShowWindow(hWndPage, SW_HIDE);

	Context->Pages[OptionPageOthers].Procedure = OptionOthersProcedure;
	Context->Pages[OptionPageOthers].hWndParent = hWnd;
	Context->Pages[OptionPageOthers].ResourceId = IDD_DIALOG_OPTMISC;
	DialogCreateModeless(&Context->Pages[OptionPageOthers]);
	ShowWindow(hWndPage, SW_HIDE);

	SetWindowText(hWnd, L"Options");
	SdkSetMainIcon(hWnd);
	SdkCenterWindow(hWnd);
	
	ShowWindow(hWndFirst, SW_SHOW);
	SetFocus(hWndList);
	SendMessage(hWndList, LB_SETCURSEL, 0, 0);
	return TRUE;
}

LRESULT
OptionOnEraseBkgnd(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	RECT Rect;
	PDIALOG_OBJECT Object;
	POPTION_DIALOG_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(GetParent(hWnd));
	Context = (POPTION_DIALOG_CONTEXT)Object->Context;

	GetClientRect(hWnd, &Rect);
	FillRect(GetDC(hWnd), &Rect, Context->hBrushPane);
	return TRUE;
}

LRESULT
OptionOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	POPTION_DIALOG_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(GetParent(hWnd));
	Context = (POPTION_DIALOG_CONTEXT)Object->Context;
	return (LRESULT)Context->hBrushPane;
}

LRESULT
OptionSwitchPage(
	IN HWND hWnd
	)
{
	HWND hWndList;
	PDIALOG_OBJECT Object;
	POPTION_DIALOG_CONTEXT Context;
	LONG Number;
	LONG Current;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (POPTION_DIALOG_CONTEXT)Object->Context;

	hWndList = GetDlgItem(hWnd, IDC_LIST_OPTION);
	Current = (LONG)SendMessage(hWndList, LB_GETCURSEL, 0, 0);
	if (Current == LB_ERR) {
		return 0L;
	}

	for(Number = 0; Number < OptionPageNumber; Number += 1) {
		if (Number != Current) {
			ShowWindow(Context->Pages[Number].hWnd, SW_HIDE);
		} else {
			ShowWindow(Context->Pages[Number].hWnd, SW_SHOW);
		}	
	}
	
	OptionPageAdjustPosition(hWnd, Context->Pages[Current].hWnd);
	return 0L;
}

LRESULT
OptionOnClose(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	POPTION_DIALOG_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (POPTION_DIALOG_CONTEXT)Object->Context;

	if (Context->hBrushFocus) {
		DeleteObject(Context->hBrushFocus);
	}

	if (Context->hBrushPane) {
		DeleteObject(Context->hBrushPane);
	}

	if (Context->hFontBold) {
		DeleteObject(Context->hFontBold);
	}

	EndDialog(hWnd, (INT_PTR)lp);
	return 0L;
}
	
LRESULT
OptionOnOk(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	POPTION_DIALOG_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (POPTION_DIALOG_CONTEXT)Object->Context;

	OptionGeneralCommit(Context->Pages[OptionPageGeneral].hWnd);
	OptionAdvancedCommit(Context->Pages[OptionPageAdvanced].hWnd);
	OptionSymbolCommit(Context->Pages[OptionPageSymbol].hWnd);
	OptionOthersCommit(Context->Pages[OptionPageOthers].hWnd);

	OptionOnClose(hWnd, uMsg, wp, (LPARAM)IDOK);
	return 0L;
}

LRESULT
OptionOnCommand(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LRESULT Status = 0;

	switch (LOWORD(wp)) {

		case IDOK:
			OptionOnOk(hWnd, uMsg, wp, lp);
			break;

		case IDCANCEL:
			OptionOnClose(hWnd, uMsg, wp, (LPARAM)IDCANCEL);
			break;

		case IDC_LIST_OPTION:
			if (HIWORD(wp) == LBN_SELCHANGE) {
				Status = OptionSwitchPage(hWnd);
				break;
			}
	}

	return Status;
}

ULONG
OptionGeneralInit(
	IN HWND hWnd
	)
{
	HWND hWndCtrl;
	PMDB_DATA_ITEM Item;
	BOOLEAN Enable;

	Item = MdbGetData(MdbSearchEngine);
	ASSERT(Item != NULL);

	hWndCtrl = GetDlgItem(hWnd, IDC_EDIT_SEARCHENGINE);
	SetWindowText(hWndCtrl, Item->Value);
	
	Item = MdbGetData(MdbAutoScroll);
	ASSERT(Item != NULL);
	Enable = _wtoi(Item->Value);
	Enable = Enable ? BST_CHECKED : BST_UNCHECKED;
	CheckDlgButton(hWnd, IDC_CHECK_AUTOSCROLL, Enable);

	LogViewSetAutoScroll(Enable);
	return S_OK;
}

ULONG
OptionGeneralCommit(
	IN HWND hWnd
	)
{
	HWND hWndCtrl;
	PMDB_DATA_ITEM Item;
	BOOLEAN Enable;
	INT Checked;
	WCHAR Buffer[MAX_PATH];

	//
	// Default search engine
	//

	Item = MdbGetData(MdbSearchEngine);
	ASSERT(Item != NULL);

	hWndCtrl = GetDlgItem(hWnd, IDC_EDIT_SEARCHENGINE);
	GetWindowText(hWndCtrl, Buffer, MAX_PATH);

	if (wcsicmp(Buffer, Item->Value) != 0) {
		MdbSetData(MdbSearchEngine, Buffer);
	}
	
	//
	// Autoscroll of LogView 
	//

	Item = MdbGetData(MdbAutoScroll);
	ASSERT(Item != NULL);

	Checked = IsDlgButtonChecked(hWnd, IDC_CHECK_AUTOSCROLL);
	Enable = (Checked == BST_CHECKED) ? TRUE : FALSE;

	_itow(Enable, Buffer, 10);
	if (wcsicmp(Buffer, Item->Value) != 0) {
		MdbSetData(MdbAutoScroll, Buffer);
	}

	LogViewSetAutoScroll(Enable);
	return S_OK;
}

INT_PTR CALLBACK 
OptionGeneralProcedure(
    IN HWND hWnd,
    IN UINT uMsg,
	IN WPARAM wp,
    IN LPARAM lp
    )
{
	LPNMHDR header = (LPNMHDR)lp;

	switch (uMsg) {

	case WM_INITDIALOG:
		OptionGeneralInit(hWnd);
		OptionPageAdjustPosition(GetParent(hWnd), hWnd);
		break;

	case WM_ERASEBKGND:
		return OptionOnEraseBkgnd(hWnd, uMsg, wp, lp);

	case WM_CTLCOLORSTATIC:
		return OptionOnCtlColorStatic(hWnd, uMsg, wp, lp);

	case WM_NOTIFY: 
		break;
	}

	return FALSE;
}

ULONG
OptionSymbolCommit(
	IN HWND hWnd
	)
{
	HWND hWndCtrl;
	PMDB_DATA_ITEM Item;
	BOOLEAN Enable;
	WCHAR Buffer[MAX_PATH];
	int Checked;

	//
	// Symbol path
	//

	Item = MdbGetData(MdbSymbolPath);
	ASSERT(Item != NULL);

	hWndCtrl = GetDlgItem(hWnd, IDC_EDIT_SYMPATH);
	GetWindowText(hWndCtrl, Buffer, MAX_PATH);

	if (wcsicmp(Buffer, Item->Value) != 0) {
		MdbSetData(MdbSymbolPath, Buffer);
	}

	//
	// Offline Decode 
	//

	Item = MdbGetData(MdbOfflineDecode);
	ASSERT(Item != NULL);

	Checked = IsDlgButtonChecked(hWnd, IDC_CHECK_OFFLINEDECODE);
	Enable = (Checked == BST_CHECKED) ? TRUE : FALSE;
	_itow(Enable, Buffer, MAX_PATH);

	if (wcsicmp(Buffer, Item->Value) != 0) {
		MdbSetData(MdbOfflineDecode, Buffer);
	}

	return S_OK;
}

ULONG
OptionSymbolInit(
	IN HWND hWnd
	)
{
	HWND hWndCtrl;
	PMDB_DATA_ITEM Item;
	int Enable;

	Item = MdbGetData(MdbSymbolPath);
	ASSERT(Item != NULL);

	hWndCtrl = GetDlgItem(hWnd, IDC_EDIT_SYMPATH);
	SetWindowText(hWndCtrl, Item->Value);

	//
	// Disable for v1, we use sympath dynamically
	//

	EnableWindow(hWndCtrl, FALSE);

	Item = MdbGetData(MdbOfflineDecode);
	ASSERT(Item != NULL);

	Enable = _wtoi(Item->Value);
	Enable = Enable ? BST_CHECKED : BST_UNCHECKED;
	CheckDlgButton(hWnd, IDC_CHECK_OFFLINEDECODE, Enable);

	//
	// Disable for v1
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_CHECK_OFFLINEDECODE);
	EnableWindow(hWndCtrl, FALSE);

	//
	// Disable for v1
	//

	hWndCtrl = GetDlgItem(hWnd, IDC_BUTTON_SYMPATH);
	EnableWindow(hWndCtrl, FALSE);

	return S_OK;
}

INT_PTR CALLBACK 
OptionSymbolProcedure(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp
    )
{
	BOOL Status = FALSE;
	WCHAR Path[MAX_PATH];	

	switch (uMsg) {

	case WM_INITDIALOG:
		OptionSymbolInit(hWnd);
		break;

	case WM_ERASEBKGND:
		return OptionOnEraseBkgnd(hWnd, uMsg, wp, lp);

	case WM_CTLCOLORSTATIC:
		return OptionOnCtlColorStatic(hWnd, uMsg, wp, lp);

	case WM_COMMAND:
            
		if (LOWORD(wp) == IDC_BUTTON_DBGHELP) {

			OPENFILENAME Ofn;
			ZeroMemory(&Ofn, sizeof Ofn);

			Ofn.lStructSize = sizeof(Ofn);
			Ofn.hwndOwner = hWnd;
			Ofn.hInstance = SdkInstance;
			Ofn.lpstrFilter = Ofn.lpstrFilter = L"dbghelp (dbghelp.dll)\0*dbghelp.dll\0\0";
			
			wcscpy_s(Path, MAX_PATH - 1, L"dbghelp.dll");
			Ofn.lpstrFile = Path;
			Ofn.nMaxFile = sizeof(Path); 
			Ofn.lpstrTitle = L"D Probe";
			Ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;

			Status = GetOpenFileName(&Ofn);
			if (Status) {
				SetWindowText(GetDlgItem(hWnd, IDC_EDIT_DBGHELP), Ofn.lpstrFile);
			}

			break;
		}

		if (LOWORD(wp) == IDC_BUTTON_SYMPATH) {

			PIDLIST_ABSOLUTE IdlList;
			BROWSEINFO BrowseInfo = {0};
			BrowseInfo.hwndOwner = hWnd;
			BrowseInfo.lpszTitle = L"Browse for symbol path:";
            BrowseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_DONTGOBELOWDOMAIN;

			IdlList = SHBrowseForFolder(&BrowseInfo);
			if (IdlList) {
				ZeroMemory(Path, sizeof(Path));
				Status = SHGetPathFromIDList(IdlList, Path);
				if (Status) {
					SetWindowText(GetDlgItem(hWnd, IDC_EDIT_SYMPATH), Path);
				}
			}
			break;
		}
	}

	return FALSE;
}

INT_PTR CALLBACK 
OptionAdvancedProcedure(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp
    )
{
	LPNMHDR header = (LPNMHDR)lp;

	switch (uMsg) {

	case WM_INITDIALOG:
		OptionAdvancedInit(hWnd);
		break;

	case WM_ERASEBKGND:
		return OptionOnEraseBkgnd(hWnd, uMsg, wp, lp);

	case WM_CTLCOLORSTATIC:
		return OptionOnCtlColorStatic(hWnd, uMsg, wp, lp);

	case WM_NOTIFY: 
		break;
	}

	return FALSE;
}

ULONG
OptionAdvancedInit(
	IN HWND hWnd
	)
{
	PMDB_DATA_ITEM Item;
	int Enable;

	if (!BspIs64Bits && !BspIsWow64) {

		Item = MdbGetData(MdbEnableKernel);
		ASSERT(Item != NULL);

		Enable = _wtoi(Item->Value);
		Enable = Enable ? BST_CHECKED : BST_UNCHECKED;
		CheckDlgButton(hWnd, IDC_CHECK_ENABLE_KERNEL, Enable);

	} else {

		//
		// Disable for either 64 bits, or wow64 process
		//

		CheckDlgButton(hWnd, IDC_CHECK_ENABLE_KERNEL, BST_UNCHECKED);
		EnableWindow(GetDlgItem(hWnd, IDC_CHECK_ENABLE_KERNEL), FALSE);
	}

	return S_OK;
}

ULONG
OptionAdvancedCommit(
	IN HWND hWnd
	)
{
	PMDB_DATA_ITEM Item;
	int Checked;
	BOOLEAN Enable;
	WCHAR Buffer[MAX_PATH];

	//
	// Enable kernel tracing, valid only for native 32 bits system
	// 

	if (!BspIs64Bits && !BspIsWow64) {

		Item = MdbGetData(MdbEnableKernel);
		ASSERT(Item != NULL);

		Checked = IsDlgButtonChecked(hWnd, IDC_CHECK_ENABLE_KERNEL);
		Enable = (Checked == BST_CHECKED) ? TRUE : FALSE;

		_itow(Enable, Buffer, 10);
		if (wcsicmp(Buffer, Item->Value) != 0) {
			MdbSetData(MdbEnableKernel, Buffer);
		}

	}

	return S_OK;
}

INT_PTR CALLBACK 
OptionOthersProcedure(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp
    )
{
	LPNMHDR header = (LPNMHDR)lp;

	switch (uMsg) {

	case WM_INITDIALOG:
		break;

	case WM_ERASEBKGND:
		return OptionOnEraseBkgnd(hWnd, uMsg, wp, lp);

	case WM_NOTIFY: 
		break;
	}

	return FALSE;
}

ULONG
OptionOthersInit(
	IN HWND hWnd
	)
{
	return 0;
}

ULONG
OptionOthersCommit(
	IN HWND hWnd
	)
{
	return 0;
}
