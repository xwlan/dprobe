#include "optiondlg.h"
#include "sdk.h"
#include "mdb.h"
#include <shlobj.h>
#include <wchar.h>
#include "colorctrl.h"

VOID
OptionInitAdjustment(
	__in HWND hWnd
	)
{
	SdkCenterWindow(GetParent(hWnd));
	SetWindowText(GetParent(hWnd), L"Option");
}
		
ULONG
OptionSheetCreate(
	__in HWND hWndParent
	)
{
	PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[5];

	propSheetHeader.dwFlags = PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP |
							  PSH_PROPTITLE | PSH_USECALLBACK | PSH_USEPSTARTPAGE;

    propSheetHeader.hwndParent = hWndParent;
    propSheetHeader.pszCaption = L"Option";
    propSheetHeader.nPages = 0;
    propSheetHeader.pStartPage = L"General";
    propSheetHeader.phpage = pages;
    propSheetHeader.pfnCallback = OptionSheetProcedure;
 
	memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_OPTGENERAL);
    propSheetPage.pfnDlgProc = OptionGeneralProcedure;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

	memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_OPTSYMBOL);
    propSheetPage.pfnDlgProc = OptionSymbolProcedure;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);
	
	memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_OPTSTORAGE);
    propSheetPage.pfnDlgProc = OptionStorageProcedure;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);
	
	memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_OPTHIGHLIGHT);
    propSheetPage.pfnDlgProc = OptionHighlightProcedure;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

	memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_OPTDEBUGCONSOLE);
    propSheetPage.pfnDlgProc = OptionDebugConsoleProcedure;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

	PropertySheet(&propSheetHeader);
	return 0;
}

INT CALLBACK 
OptionSheetProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in LPARAM lp
    )
{
	return 0;
}

ULONG
OptionGeneralInit(
	__in HWND hWnd
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

	Item = MdbGetData(MdbAutoSaveDpc);
	ASSERT(Item != NULL);
	Enable = _wtoi(Item->Value);
	Enable = Enable ? BST_CHECKED : BST_UNCHECKED;
	CheckDlgButton(hWnd, IDC_CHECK_AUTODPC, Enable);

	Item = MdbGetData(MdbEnableLogging);
	ASSERT(Item != NULL);
	Enable = _wtoi(Item->Value);
	Enable = Enable ? BST_CHECKED : BST_UNCHECKED;
	CheckDlgButton(hWnd, IDC_CHECK_INTERNALLOGGING, Enable);
	return S_OK;
}

INT_PTR CALLBACK 
OptionGeneralProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in LPARAM lp,
	__in WPARAM wp
    )
{
	LPNMHDR header = (LPNMHDR)lp;

	switch (uMsg) {

	case WM_INITDIALOG:
		OptionInitAdjustment(hWnd);
		OptionGeneralInit(hWnd);
		break;

	case WM_NOTIFY: 
		break;
	}

	return FALSE;
}

INT_PTR CALLBACK 
OptionAdvancedProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
	LPNMHDR header = (LPNMHDR)lp;

	switch (uMsg) {

	case WM_INITDIALOG:
		OptionInitAdjustment(hWnd);
		break;

	case WM_NOTIFY: 
		/*switch (header->code) {
		case PSN_APPLY:
			SetWindowLongPtr(hWnd, DWLP_MSGRESULT, PSNRET_NOERROR);
			return TRUE;
		}*/
		break;
	}

	return FALSE;
}

ULONG
OptionSymbolInit(
	__in HWND hWnd
	)
{
	HWND hWndCtrl;
	PMDB_DATA_ITEM Item;
	BOOLEAN CaptureStack;

	Item = MdbGetData(MdbDbghelpPath);
	ASSERT(Item != NULL);

	hWndCtrl = GetDlgItem(hWnd, IDC_EDIT_DBGHELP);
	SetWindowText(hWndCtrl, Item->Value);
	
	Item = MdbGetData(MdbSymbolPath);
	ASSERT(Item != NULL);

	hWndCtrl = GetDlgItem(hWnd, IDC_EDIT_SYMPATH);
	SetWindowText(hWndCtrl, Item->Value);

	Item = MdbGetData(MdbCaptureStack);
	ASSERT(Item != NULL);

	CaptureStack = _wtoi(Item->Value);
	CaptureStack = CaptureStack ? BST_CHECKED : BST_UNCHECKED;
	CheckDlgButton(hWnd, IDC_CHECK_CAPTURE, CaptureStack);
	return S_OK;
}

INT_PTR CALLBACK 
OptionSymbolProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
	BOOL Status = FALSE;
	WCHAR Path[MAX_PATH];	

	switch (uMsg) {

	case WM_INITDIALOG:
		OptionInitAdjustment(hWnd);
		OptionSymbolInit(hWnd);
		break;

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

ULONG
OptionHighlightInit(
	__in HWND hWnd
	)
{
	HWND hWndList;
	PMDB_DATA_ITEM Data;
	LVCOLUMN Column = {0};
	LVITEM Item = {0};

	Data = MdbGetData(MdbHighlightDuration);
	SetWindowText(GetDlgItem(hWnd, IDC_EDIT_DURATION), Data->Value);

	hWndList = GetDlgItem(hWnd, IDC_LIST_COLOR);
	ListView_SetUnicodeFormat(hWndList, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndList,
		                                LVS_EX_FULLROWSELECT,
										LVS_EX_FULLROWSELECT);
	
    ListView_SetExtendedListViewStyleEx(hWndList,
		                                LVS_EX_CHECKBOXES,
										LVS_EX_CHECKBOXES);

	Column.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 
	Column.iSubItem = 0;
	Column.pszText = NULL;	
	Column.cx = 300;     
	Column.fmt = LVCFMT_LEFT;

	ListView_InsertColumn(hWndList, 0, &Column);

	Item.mask = LVIF_TEXT | LVIF_PARAM;
	Item.iItem = 0;
	Item.iSubItem = 0;
	Item.pszText = L"New Record";
	Item.lParam = (LPARAM)MdbNewColor;
	ListView_InsertItem(hWndList, &Item);

	Item.mask = LVIF_TEXT | LVIF_PARAM;
	Item.iItem = 1;
	Item.iSubItem = 0;
	Item.pszText = L"Manual Record";
	Item.lParam = (LPARAM)MdbManualColor;
	ListView_InsertItem(hWndList, &Item);

	Item.mask = LVIF_TEXT | LVIF_PARAM;
	Item.iItem = 2;
	Item.iSubItem = 0;
	Item.pszText = L"AddOn Record";
	Item.lParam = (LPARAM)MdbAddOnColor;
	ListView_InsertItem(hWndList, &Item);

	return S_OK;
}

LRESULT
OptionHighlightOnCustomDraw(
	__in HWND hWnd,
	__in NMHDR *lpnmhdr
	)
{ 
	ULONG Status;
	PMDB_DATA_ITEM Data;
	LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)lpnmhdr;
	COLORREF Color = (ULONG)lvcd->nmcd.lItemlParam;

	switch(lvcd->nmcd.dwDrawStage)  {

		case CDDS_PREPAINT: 
			Status = CDRF_NOTIFYITEMDRAW;
			break;

		case CDDS_ITEMPREPAINT:
			Status = CDRF_NOTIFYSUBITEMDRAW;
			break;

		case CDDS_SUBITEM | CDDS_ITEM | CDDS_PREPAINT:

			//
			// lvcd->nmcd.dwItemSpec contain index of item
			//

			Data = MdbGetData(Color);
			lvcd->clrText = RGB(0, 0, 0);
			lvcd->clrTextBk = _wtoi(Data->Value);
			Status = CDRF_NEWFONT;
			break;

		default:
			Status = CDRF_DODEFAULT;
	}

	return Status;
}

LRESULT
OptionHighlightOnDbClick(
	__in HWND hWnd,
	__in NMHDR *lpnmhdr
	)
{
	HWND hWndList;
	NMLISTVIEW *lpnmlv = (LPNMLISTVIEW)lpnmhdr;
	CHOOSECOLOR Dialog = { sizeof(Dialog) };
	COLORREF Colors[16] = { 0 };
	PMDB_DATA_ITEM Item;
	ULONG Offset;

	hWndList = GetDlgItem(hWnd, IDC_LIST_COLOR);
	ListViewGetParam(hWndList, lpnmlv->iItem, (LPARAM *)&Offset);
	Item = MdbGetData(Offset);

	Dialog.hwndOwner = hWnd;
	Dialog.rgbResult = 	_wtoi(Item->Value);
	Dialog.lpCustColors = Colors;
	Dialog.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT;

	if (ChooseColor(&Dialog)) {
		WCHAR Buffer[MAX_PATH];
		StringCchPrintf(Buffer, MAX_PATH, L"%d", Dialog.rgbResult);
		MdbSetData(Offset, Buffer);
	}

	return 0L;
}

LRESULT
OptionHighlightOnNotify(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	ULONG Status = 0L;
	NMHDR *lpnmhdr = (NMHDR *)lp;

	if (LOWORD(wp) != IDC_LIST_COLOR) {
		return 0L;
	}

	switch (lpnmhdr->code) {
		case NM_CUSTOMDRAW:
			Status = OptionHighlightOnCustomDraw(hWnd, lpnmhdr);
			break;

		case NM_DBLCLK:
			Status = OptionHighlightOnDbClick(hWnd, lpnmhdr);
			break;
			
	}

	return Status;
}

LRESULT
OptionHighlightOnCommand(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	)
{
	ULONG Status = 0L;
	ULONG i, Count;
	HWND hWndList;

	hWndList = GetDlgItem(hWnd, IDC_LIST_COLOR);
	Count = ListView_GetItemCount(hWndList);

	switch (LOWORD(wp)) {
		case IDC_BUTTON_ENABLEALL:
			for(i = 0; i < Count; i++) {
				ListViewSetCheck(hWndList, i, TRUE);
			}
			break;

		case IDC_BUTTON_DISABLEALL:
			for(i = 0; i < Count; i++) {
				ListViewSetCheck(hWndList, i, FALSE);
			}
			break;
	}

	return 0L;
}

INT_PTR CALLBACK 
OptionHighlightProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
	LPNMHDR header = (LPNMHDR)lp;
	ULONG Status;

	switch (uMsg) {

	case WM_INITDIALOG:
		OptionInitAdjustment(hWnd);
		OptionHighlightInit(hWnd);
		break;

	case WM_NOTIFY:
		Status = OptionHighlightOnNotify(hWnd, uMsg, wp, lp);
		SetWindowLong(hWnd, DWLP_MSGRESULT, Status);
		return TRUE;

	case WM_COMMAND:
		OptionHighlightOnCommand(hWnd, uMsg, wp, lp);
		break;
	}

	return FALSE;
}

INT_PTR CALLBACK 
OptionGraphProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
	LPNMHDR header = (LPNMHDR)lp;

	switch (uMsg) {

	case WM_INITDIALOG:
		OptionInitAdjustment(hWnd);
		break;

	case WM_NOTIFY: 
		break;
	}

	return FALSE;
}

INT_PTR CALLBACK 
OptionStorageProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
	LPNMHDR header = (LPNMHDR)lp;

	switch (uMsg) {

	case WM_INITDIALOG:
		OptionInitAdjustment(hWnd);
		break;

	case WM_NOTIFY: 
		break;
	}

	return FALSE;
}

ULONG
OptionDebugConsoleInit(
	__in HWND hWnd
	)
{
	HWND hWndCtrl;
	HWND hWndColor;
	PMDB_DATA_ITEM Item;
	BOOLEAN Enable;

	Item = MdbGetData(MdbConsoleBackColor);
	ASSERT(Item != NULL);
	hWndColor = ColorCreateControl(hWnd, 1);
	hWndCtrl = GetDlgItem(hWnd, IDC_STATIC_BACKCOLOR);
	SdkCopyControlRect(hWnd, hWndCtrl, hWndColor);
	ColorSetColor(hWndColor, _wtoi(Item->Value));
	
	Item = MdbGetData(MdbConsoleForeColor);
	ASSERT(Item != NULL);
	hWndColor = ColorCreateControl(hWnd, 2);
	hWndCtrl = GetDlgItem(hWnd, IDC_STATIC_FORECOLOR);
	SdkCopyControlRect(hWnd, hWndCtrl, hWndColor);
	ColorSetColor(hWndColor, _wtoi(Item->Value));

	Item = MdbGetData(MdbConsoleCommandColor);
	ASSERT(Item != NULL);
	hWndColor = ColorCreateControl(hWnd, 3);
	hWndCtrl = GetDlgItem(hWnd, IDC_STATIC_CMDCOLOR);
	SdkCopyControlRect(hWnd, hWndCtrl, hWndColor);
	ColorSetColor(hWndColor, _wtoi(Item->Value));

	Item = MdbGetData(MdbEnableDebugExtension);
	ASSERT(Item != NULL);
	Enable = _wtoi(Item->Value);
	Enable = Enable ? BST_CHECKED : BST_UNCHECKED;
	CheckDlgButton(hWnd, IDC_CHECK_INTERNALLOGGING, Enable);
	return S_OK;
}

INT_PTR CALLBACK 
OptionDebugConsoleProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    )
{
	LPNMHDR header = (LPNMHDR)lp;

	switch (uMsg) {

	case WM_INITDIALOG:
		OptionInitAdjustment(hWnd);
		OptionDebugConsoleInit(hWnd);
		break;

	case WM_NOTIFY: 
		break;
	}

	return FALSE;
}