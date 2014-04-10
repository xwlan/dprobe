//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2010
//

#include "dprobe.h"
#include "dialog.h"
#include "listview.h"
#include "messagetable.h"
#include "argumentdlg.h"
#include "bsp.h"
#include "psp.h"

LISTVIEW_COLUMN 
ArgumentColumn[2] = {
	{ 120, L"Type", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 240, L"Name", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText }
};

BOOLEAN
ArgumentDialogCreate(
	IN HWND hWnd,
	IN PDIALOG_RESULT Result
	)
{
	DIALOG_OBJECT Object = {0};

	Object.hWndParent = hWnd;
	Object.Procedure = ArgumentProcedure;
	Object.ResourceId = IDD_DIALOG_ARGUMENT;
	Object.Context = Result;
	Object.Scaler = NULL;
	Object.ObjectContext = NULL;

	return (BOOLEAN)DialogCreate(&Object);
}

LRESULT
ArgumentOnRemove(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	HWND hWndCtrl;
	LONG Selected;

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_ARGUMENT);
	Selected = ListViewGetFirstSelected(hWndCtrl);
	
	if (Selected != -1) {
		ListView_DeleteItem(hWndCtrl, Selected);
	}

	return 0;
}

LRESULT
ArgumentOnAdd(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	HWND hWndCtrl;
	ULONG Count, i;
	WCHAR Type[32];
	WCHAR Name[32];
	WCHAR Buffer[32];
	LVITEM Item = {0};
	BTR_ARGUMENT Argument;	

	hWndCtrl = GetDlgItem(hWnd, IDC_COMBO_ARGUMENT);
	ASSERT(hWndCtrl != NULL);

	Argument = (BTR_ARGUMENT)SendMessage(hWndCtrl, CB_GETCURSEL, 0, 0);
	if (Argument == CB_ERR) {
		MessageBox(hWnd, L"Please select argument type.", L"D Probe", MB_OK | MB_ICONWARNING);
		return 0;
	}

	StringCchCopy(Type, 30, ArgumentType[Argument]);

	hWndCtrl = GetDlgItem(hWnd, IDC_EDIT_ARGUMENT);
	GetWindowText(hWndCtrl, Name, 30); 

	if (Name[0] == L'\0') {
		MessageBox(hWnd, L"Please input argument name.", L"D Probe", MB_OK | MB_ICONWARNING);
		return 0;
	}

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_ARGUMENT);
	Count = ListView_GetItemCount(hWndCtrl);

	if (Count >= 15) {
		MessageBox(hWnd, L"Argument number overflows", L"D Probe", MB_OK | MB_ICONWARNING);
		return 0;
	}

	for(i = 0; i < Count; i++) {
		ListView_GetItemText(hWndCtrl, i, 1, Buffer, 30);
		if (_wcsicmp(Name, Buffer) == 0) {
			MessageBox(hWnd, L"Argument name already exist.", L"D Probe", MB_OK | MB_ICONWARNING);
			return 0;
		}
	}

	Item.mask = LVIF_TEXT | LVIF_PARAM;
	Item.iItem = Count;
	Item.iSubItem = 0;
	Item.pszText = Type;
	Item.lParam = Argument;
	ListView_InsertItem(hWndCtrl, &Item);

	Item.mask = LVIF_TEXT;
	Item.iSubItem = 1;
	Item.pszText = Name;
	ListView_SetItem(hWndCtrl, &Item);

	return 0;
}

LRESULT
ArgumentOnCommand(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	switch (LOWORD(wp)) {

	case IDOK:
		ArgumentOnOk(hWnd);
	    break;

	case IDCANCEL:
		ArgumentFreeResources(hWnd);
	    EndDialog(hWnd, IDCANCEL);
	    break;

	case IDC_BUTTON_ADD:
		ArgumentOnAdd(hWnd, uMsg, wp, lp);				
		break;

	case IDC_BUTTON_REMOVE:
		ArgumentOnRemove(hWnd, uMsg, wp, lp);
		break;

	}

	return 0;
}
			
INT_PTR CALLBACK
ArgumentProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	switch (uMsg) {

	case WM_COMMAND: 
		ArgumentOnCommand(hWnd, uMsg, wp, lp);
		break;

	case WM_INITDIALOG: 
		ArgumentOnInitDialog(hWnd, uMsg, wp, lp);
		break;

	case WM_CTLCOLORSTATIC:
		return ArgumentOnCtlColorStatic(hWnd, uMsg, wp, lp);
	}
	
	return 0;
}

LRESULT
ArgumentOnInitDialog(	
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LONG Selected;
	HWND hWndCtrl;
	PDIALOG_OBJECT Object;
	PDIALOG_RESULT Result;
	PPSP_PROBE Probe;
	WCHAR ModuleName[MAX_MODULE_NAME_LENGTH];
	WCHAR Buffer[MAX_FUNCTION_NAME_LENGTH];

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Result = (PDIALOG_RESULT)Object->Context;
	Probe = (PPSP_PROBE)Result->Context;
	ASSERT(Probe != NULL);

	hWndCtrl = GetDlgItem(GetParent(hWnd), IDC_LIST_MANUAL);
	Selected = ListViewGetFirstSelected(hWndCtrl);
	ListView_GetItemText(hWndCtrl, Selected, 0, ModuleName, MAX_PATH);

	hWndCtrl = GetDlgItem(hWnd, IDC_PROBE_NAME);
	StringCchPrintf(Buffer, MAX_FUNCTION_NAME_LENGTH, 
		            L"%ws!%ws", ModuleName, Probe->UndMethodName);

	SetWindowText(hWndCtrl, Buffer);

	ArgumentInsertCallType(hWnd, Probe);
	ArgumentInsertReturnType(hWnd, Probe);
	ArgumentInsertArgument(hWnd, Probe);
	ArgumentInsertArgumentType(hWnd, Probe);
	return 0;
}

BOOLEAN
ArgumentOnOk(
	IN HWND hWnd
	)
{
	HWND hWndCtrl;
	BTR_CALLTYPE CallType;
	BTR_RETURN_TYPE ReturnType;
	BTR_ARGUMENT Argument;
	PDIALOG_OBJECT Object;
	PDIALOG_RESULT Result;
	PPSP_PROBE Probe;
	LONG Current;
	LONG Count, i;
	BOOLEAN Changed;
	WCHAR Buffer[64];
	PPSP_ARGUMENT Cache;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Result = (PDIALOG_RESULT)Object->Context;
	Probe = (PPSP_PROBE)Result->Context;

	hWndCtrl = GetDlgItem(hWnd, IDC_COMBO_CONVENTION);
	CallType = (BTR_ARGUMENT)SendMessage(hWndCtrl, CB_GETCURSEL, 0, 0);
	
	hWndCtrl = GetDlgItem(hWnd, IDC_COMBO_RETURN);
	ReturnType = (BTR_ARGUMENT)SendMessage(hWndCtrl, CB_GETCURSEL, 0, 0);

	hWndCtrl = GetDlgItem(GetParent(hWnd), IDC_LIST_MANUAL);
	Current = ListViewGetFirstSelected(hWndCtrl);
	ListView_SetItemText(hWndCtrl, Current, 2, L"Y");

	Changed = FALSE;
	Cache = (PPSP_ARGUMENT)BspMalloc(sizeof(PSP_ARGUMENT));

	if (Probe->Current == ProbeCreating) {

		Probe->CallType = CallType;
		Probe->ReturnType = ReturnType;

	} else {

		Cache->CallType = CallType;
		if (Probe->CallType != CallType) {
			Changed = TRUE;
		}

		Cache->ReturnType = ReturnType;
		if (Probe->ReturnType != ReturnType) {
			Changed = TRUE;
		}
	}

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_ARGUMENT);
	Count = ListView_GetItemCount(hWndCtrl);

	if (Probe->Current != ProbeCreating) {
		Cache->ArgumentCount = Count;
		if (Count != Probe->ArgumentCount) {
			Changed = TRUE;
		}
	} else {
		Probe->ArgumentCount = Count;
	}

	if (Probe->Current == ProbeCreating) {

		for(i = 0; i < Count; i++) {

			ListViewGetParam(hWndCtrl, i, (LPARAM *)&Argument);
			ListView_GetItemText(hWndCtrl, i, 1, Buffer, 30);
			Probe->Parameters[i] = (BTR_ARGUMENT)Argument;
			StringCchCopy(&Probe->ParameterName[i][0], MAX_ARGUMENT_NAME_LENGTH, Buffer);

		}
	} 

	else {

		for(i = 0; i < Count; i++) {

			ListViewGetParam(hWndCtrl, i, (LPARAM *)&Argument);
			ListView_GetItemText(hWndCtrl, i, 1, Buffer, 30);

			Cache->Arguments[i] = (BTR_ARGUMENT)Argument;
			if (Probe->Parameters[i] != (BTR_ARGUMENT)Argument) {
				Changed = TRUE;
			}

			//
			// N.B. Event it has only naming change, we still consider it's a different probe object,
			// because we need decode record with new argument names.
			//

			StringCchCopy(&Cache->Names[i][0], MAX_ARGUMENT_NAME_LENGTH, Buffer);
			if (wcscmp(&Cache->Names[i][0], &Probe->ParameterName[i][0]) != 0) {
				Changed = TRUE;
			}
		}

	}

	if (Probe->Current != ProbeCreating && Changed) {
		Probe->Current = ProbeEditing;
		Probe->Cache = Cache;
	} else {
		BspFree(Cache);
	}
	
	Result->Context = Probe;
	Result->Status = IDOK;

	ArgumentFreeResources(hWnd);
	EndDialog(hWnd, IDOK);	
	return TRUE;
}

VOID
ArgumentFillProbe(
	IN HWND hWnd,
	IN PPSP_PROBE Probe
	)
{
	return;
}

VOID
ArgumentInsertCallType(
	IN HWND hWnd,
	IN PPSP_PROBE Probe
	)
{
	HWND hWndCtrl;
	ULONG Number;

	hWndCtrl = GetDlgItem(hWnd, IDC_COMBO_CONVENTION);
	SendMessage(hWndCtrl, CB_RESETCONTENT, 0, 0);

	for(Number = 0; Number < CallNumber; Number += 1) {
		SendMessage(hWndCtrl, CB_ADDSTRING, 0, (LPARAM)ArgumentCall[Number]);
	}

	if (Probe != NULL && Probe->Current != ProbeCreating) {
		SendMessage(hWndCtrl, CB_SETCURSEL, (WPARAM)Probe->CallType, 0);
	} else {
		SendMessage(hWndCtrl, CB_SETCURSEL, (WPARAM)CallStd, 0);
	}
}

VOID
ArgumentInsertReturnType(
	IN HWND hWnd,
	IN PPSP_PROBE Probe
	)
{
	HWND hWndCtrl;
	int Number;

	hWndCtrl = GetDlgItem(hWnd, IDC_COMBO_RETURN);
	SendMessage(hWndCtrl, CB_RESETCONTENT, 0, 0);

	for(Number = 0; Number < ReturnNumber; Number += 1) {
		SendMessage(hWndCtrl, CB_ADDSTRING, 0, (LPARAM)ArgumentReturn[Number]);
	}

	if (Probe != NULL && Probe->Current != ProbeCreating) {
		SendMessage(hWndCtrl, CB_SETCURSEL, (WPARAM)Probe->ReturnType, 0);
	} else {
		SendMessage(hWndCtrl, CB_SETCURSEL, (WPARAM)ReturnVoid, 0);
	}
}

VOID
ArgumentInsertArgumentType(
	IN HWND hWnd,
	IN PPSP_PROBE Probe
	)
{
	HWND hWndCtrl;
	ULONG Number;

	hWndCtrl = GetDlgItem(hWnd, IDC_COMBO_ARGUMENT);
	SendMessage(hWndCtrl, CB_RESETCONTENT, 0, 0);

	for(Number = 0; Number < ArgumentNumber; Number += 1) {
		SendMessage(hWndCtrl, CB_ADDSTRING, 0, (LPARAM)ArgumentType[Number]);
	}

	SendMessage(hWndCtrl, CB_SETCURSEL, (WPARAM)ArgumentInt32, 0);
}

VOID
ArgumentInsertArgument(
	IN HWND hWnd,
	IN PPSP_PROBE Probe
	)
{
	HWND hWndCtrl;
	LVCOLUMN Column = {0};
	BTR_ARGUMENT Argument;
	ULONG i;

	hWndCtrl = GetDlgItem(hWnd, IDC_LIST_ARGUMENT);
	ListView_SetUnicodeFormat(hWndCtrl, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndCtrl, 
		                                LVS_EX_FULLROWSELECT,
										LVS_EX_FULLROWSELECT);
	
    Column.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 
	  
    for (i = 0; i < 2; i++) { 

        Column.iSubItem = i;
		Column.pszText = ArgumentColumn[i].Title;	
		Column.cx = ArgumentColumn[i].Width;     
		Column.fmt = ArgumentColumn[i].Align;

		ListView_InsertColumn(hWndCtrl, i, &Column);
    } 

	if (Probe != NULL && Probe->Current != ProbeCreating) {
		
		for(i = 0; i < Probe->ArgumentCount; i++) {
			
			LVITEM Item = {0};
			Item.mask = LVIF_TEXT;
			Item.iItem = i;
			Item.iSubItem = 0;
			
			Argument = Probe->Parameters[i];
			Item.pszText = ArgumentType[Argument];
			ListView_InsertItem(hWndCtrl, &Item);

			Item.iSubItem = 1;
			Item.pszText = &Probe->ParameterName[i][0];
			ListView_SetItem(hWndCtrl, &Item);
		}
	}
	
}

LRESULT
ArgumentOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	UINT Id;
	HFONT hFont;
	LOGFONT LogFont;
	RECT Rect;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	hFont = (HFONT)Object->ObjectContext;

	Id = GetDlgCtrlID((HWND)lp);
	if (Id == IDC_PROBE_NAME) {
		if (!hFont) {

			hFont = GetCurrentObject((HDC)wp, OBJ_FONT);
			GetObject(hFont, sizeof(LOGFONT), &LogFont);
			LogFont.lfWeight = FW_BOLD;
			hFont = CreateFontIndirect(&LogFont);

			Object->ObjectContext = (PVOID)hFont;
			SendMessage((HWND)lp, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE);

			GetClientRect((HWND)lp, &Rect);
			InvalidateRect((HWND)lp, &Rect, TRUE);
		}
	}

	return (LRESULT)0;
}

VOID
ArgumentFreeResources(
	IN HWND hWnd
	)
{
	PDIALOG_OBJECT Object;
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	if (Object->ObjectContext) {
		DeleteFont((HFONT)Object->ObjectContext);
	}
}