//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#include "procdump.h"
#include "dialog.h"

#define DUMP_COLUMN_NUM 3

LISTVIEW_COLUMN DumpColumn[DUMP_COLUMN_NUM] = {
	{ 120, L"Name", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 60,  L"PID",  LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeUInt },
	{ 360, L"Path", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

typedef struct _DUMP_CONTEXT {
	HFONT hFontBold;
} DUMP_CONTEXT, *PDUMP_CONTEXT;

INT_PTR
DumpDialog(
	IN HWND hWndParent
	)
{
	DIALOG_OBJECT Object = {0};
	INT_PTR Return;
	DUMP_CONTEXT Context = {0};

	DIALOG_SCALER_CHILD Children[4] = {
		{ IDOK, AlignBoth, AlignBoth },
		{ IDC_BUTTON_REFRESH, AlignBoth, AlignBoth },
		{ IDC_STATIC, AlignNone, AlignBoth },
		{ IDC_LIST_PROCESS,AlignRight, AlignBottom }
	};

	DIALOG_SCALER Scaler = {
		{0,0}, {0,0}, {0,0}, 4, Children
	};

	Object.hWndParent = hWndParent;
	Object.Context = &Context;
	Object.ResourceId = IDD_DIALOG_DUMP;
	Object.Procedure = DumpProcedure;
	Object.Scaler = &Scaler;
	
	Return = DialogCreate(&Object);
	return Return;
}

LRESULT
DumpOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndChild;
	LVCOLUMN lvc = {0}; 
	HIMAGELIST himlSmall;
	PLISTVIEW_OBJECT Object;
	LIST_ENTRY ListHead;
	int i;

	hWndChild = GetDlgItem(hWnd, IDC_LIST_PROCESS);
	ASSERT(hWndChild != NULL);
	
    ListView_SetUnicodeFormat(hWndChild, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndChild, LVS_EX_FULLROWSELECT,  LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(hWndChild, LVS_EX_HEADERDRAGDROP, LVS_EX_HEADERDRAGDROP); 

	himlSmall = ImageList_Create(16, 16, ILC_COLOR24, 0, 0); 
	ListView_SetImageList(hWndChild, himlSmall, LVSIL_SMALL);

    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 
	  
    for (i = 0; i < DUMP_COLUMN_NUM; i++) { 

        lvc.iSubItem = i;
		lvc.pszText = DumpColumn[i].Title;	
		lvc.cx = DumpColumn[i].Width;     
		lvc.fmt = DumpColumn[i].Align;

		ListView_InsertColumn(hWndChild, i, &lvc);
    } 

	Object = (LISTVIEW_OBJECT *)SdkMalloc(sizeof(LISTVIEW_OBJECT));
	Object->Column = DumpColumn;
	Object->Count = DUMP_COLUMN_NUM;
	Object->CtrlId = IDC_LIST_PROCESS;
	Object->hImlSmallIcon = himlSmall;
	Object->hWnd = hWndChild;
	
	SdkSetObject(hWndChild, Object);

	BspQueryProcessList(&ListHead);
	
	DumpInsertProcessList(Object, &ListHead);

	SetWindowText(hWnd, L"Dump");
	SdkSetMainIcon(hWnd);
	SdkCenterWindow(hWnd);
	return TRUE;
}

VOID
DumpInsertProcessList(
	IN PLISTVIEW_OBJECT Object,
	IN PLIST_ENTRY ListHead
	)
{
	PBSP_PROCESS Process;
	PLIST_ENTRY ListEntry;
	HWND hWnd;
	LVITEM lvi = {0};
	SHFILEINFO sfi = {0}; 
	WCHAR Buffer[MAX_PATH];
	int i;	
	DWORD_PTR Status;
	HANDLE ProcessHandle;
	ULONG CurrentId;

	ASSERT(Object != NULL);

	hWnd = Object->hWnd;
	i = 0;
	CurrentId = GetCurrentProcessId();

	while (IsListEmpty(ListHead) != TRUE) {

		ListEntry = RemoveHeadList(ListHead);
		Process = CONTAINING_RECORD(ListEntry, BSP_PROCESS, ListEntry);

		if (Process->ProcessId == 0 || Process->ProcessId == 4 || 
			Process->ProcessId == CurrentId ) {
			BspFreeProcess(Process);	
			continue;
		}
		
		memset(&lvi, 0, sizeof(lvi));
		memset(&sfi, 0, sizeof(sfi));

		if (BspIs64Bits) {

			//
			// N.B. D Probe x64 version only list 64 bits process. 
			//

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

			//
			// N.B. D Probe x86 version only list 32 bits process on 64 bits Windows. 
			//

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
		
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT | LVIF_IMAGE;
		lvi.pszText = Process->Name;

		Status = SHGetFileInfo(Process->FullPath, 
			                   0, 
							   &sfi, 
							   sizeof(SHFILEINFO), 
							   SHGFI_ICON | SHGFI_SMALLICON | SHGFI_OPENICON);

		if (Status != 0) {
			lvi.iImage = ImageList_AddIcon(Object->hImlSmallIcon, sfi.hIcon);
			DestroyIcon(sfi.hIcon);
		} else {
			HICON hicon;
            ExtractIconEx(L"shell32.dll", 15, NULL, &hicon, 1);
			lvi.iImage = ImageList_AddIcon(Object->hImlSmallIcon, hicon);
			DestroyIcon(hicon);
		}

		ListView_InsertItem(hWnd, &lvi);

		lvi.iSubItem = 1;
		lvi.mask = LVIF_TEXT;
		wsprintf(Buffer, L"%d", Process->ProcessId);
		lvi.pszText = Buffer;
		ListView_SetItem(hWnd, &lvi);

		lvi.iSubItem = 2;
		lvi.pszText = Process->FullPath;
		ListView_SetItem(hWnd, &lvi);

		BspFreeProcess(Process);
		i += 1;
	}
}

int CALLBACK
DumpSortCallback(
	IN LPARAM First, 
	IN LPARAM Second,
	IN LPARAM Param
	)
{
    WCHAR FirstData[MAX_PATH + 1];
    WCHAR SecondData[MAX_PATH + 1];
	HWND hWnd;
	LIST_DATA_TYPE Type;
    int Result;
	LISTVIEW_OBJECT *Object;
	LISTVIEW_COLUMN *Column;

	hWnd = (HWND)Param;
	FirstData[0] = 0;
	SecondData[0] = 0;

	Object = (PLISTVIEW_OBJECT)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

	Column = Object->Column;
    ListView_GetItemText(hWnd, First,  Object->LastClickedColumn, FirstData,  MAX_PATH);
    ListView_GetItemText(hWnd, Second, Object->LastClickedColumn, SecondData, MAX_PATH);

	Type = Column[Object->LastClickedColumn].DataType;

    switch (Type) {

        case DataTypeInt:
        case DataTypeUInt:
            Result = _wtoi(FirstData) - _wtoi(SecondData);
            break;        
        
		case DataTypeInt64:
        case DataTypeUInt64:
        case DataTypeFloat:
            Result = (int)(_wtoi64(FirstData) - _wtoi64(SecondData));
            break;
        
		case DataTypeTime:
        case DataTypeText:
        case DataTypeBool:
            Result = _wcsicmp(FirstData, SecondData);
            break;

		default:
			ASSERT(0);
    }

	return Object->SortOrder ? Result : -Result;
}

LRESULT 
DumpOnColumnClick(
	IN HWND hWnd,
	IN NMLISTVIEW *lpNmlv
	)
{
	HWND hWndHeader;
	int nColumnCount;
	int i;
	HDITEM hdi;
	LISTVIEW_OBJECT *Object;

	Object = (PLISTVIEW_OBJECT)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

    if (Object->SortOrder == SortOrderNone){
        return 0;
    }

	if (Object->LastClickedColumn == lpNmlv->iSubItem) {
		Object->SortOrder = (LIST_SORT_ORDER)!Object->SortOrder;
    } else {
		Object->SortOrder = SortOrderAscendent;
    }
    
    hWndHeader = ListView_GetHeader(hWnd);
    ASSERT(hWndHeader);

    nColumnCount = Header_GetItemCount(hWndHeader);
    
    for (i = 0; i < nColumnCount; i++) {
        hdi.mask = HDI_FORMAT;
        Header_GetItem(hWndHeader, i, &hdi);
        
        if (i == lpNmlv->iSubItem) {
            hdi.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
            if (Object->SortOrder == SortOrderAscendent){
                hdi.fmt |= HDF_SORTUP;
            } else {
                hdi.fmt |= HDF_SORTDOWN;
            }
        } else {
            hdi.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
        } 
        
        Header_SetItem(hWndHeader, i, &hdi);
    }
    
	Object->LastClickedColumn = lpNmlv->iSubItem;
    ListView_SortItemsEx(hWnd, DumpSortCallback, (LPARAM)hWnd);

    return 0L;
}

LRESULT 
DumpOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LRESULT lres = 0;
	NMHDR *pNmhdr = (NMHDR *)lp;

	if (pNmhdr->idFrom != IDC_LIST_PROCESS) {
		return 0L;
	}

	switch (pNmhdr->code) {
		case LVN_COLUMNCLICK:
			lres = DumpOnColumnClick(pNmhdr->hwndFrom, (NM_LISTVIEW *)lp);
			break;
			
	}

	return lres;
}

LRESULT
DumpOnCommand(
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
		case IDCANCEL:
			DumpOnClose(hWnd, uMsg, wp, lp);
			EndDialog(hWnd, IDOK);
		    break;

		case IDC_BUTTON_REFRESH:
			DumpOnRefresh(hWnd);
			break;

		case ID_DUMP_SMALLDUMP:
			DumpOnSmallDump(hWnd, uMsg, wp, lp);
			break;
		
		case ID_DUMP_FULLDUMP:
			DumpOnFullDump(hWnd, uMsg, wp, lp);
			break;
	}

	return 0;
}

VOID
DumpOnRefresh(
	IN HWND hWnd
	)
{
	HWND hWndChild;
	LIST_ENTRY ListHead;
	int i, Count;
	PBSP_PROCESS Process;
	PLIST_ENTRY ListEntry;
	ULONG ProcessId;
	LVITEM lvi = {0};
	HIMAGELIST hImlSmall;
	SHFILEINFO sfi = {0};
	WCHAR Buffer[MAX_PATH];
	ULONG CurrentId;
	HANDLE ProcessHandle;

	ASSERT(hWnd != NULL);

	hWndChild = GetDlgItem(hWnd, IDC_LIST_PROCESS);
	ASSERT(hWndChild != NULL);

	BspQueryProcessList(&ListHead);

	Count = ListView_GetItemCount(hWndChild);
	for(i = 0; i < Count; ) {
		
		ListView_GetItemText(hWndChild, i, 1, Buffer, 31);

		ProcessId = _wtol(Buffer);
		Process = BspQueryProcessById(&ListHead, ProcessId);

		if (Process == NULL) {
			ListView_DeleteItem(hWndChild, i);
			Count = Count - 1;
			continue;

		} else {

			RemoveEntryList(&Process->ListEntry);
			BspFree(Process);
			i += 1;
		}
	}

	//
	// Insert new process entry
	//

	CurrentId = GetCurrentProcessId();
	hImlSmall = ListView_GetImageList(hWndChild, LVSIL_SMALL);

	while (IsListEmpty(&ListHead) != TRUE) {
		
		ListEntry = RemoveHeadList(&ListHead);
		Process = CONTAINING_RECORD(ListEntry, BSP_PROCESS, ListEntry);
		if (Process->ProcessId == 0 || Process->ProcessId == 4 ||
			Process->ProcessId == CurrentId ) {
			BspFree(Process);
			continue;
		}
		
		memset(&lvi, 0, sizeof(lvi));
		memset(&sfi, 0, sizeof(sfi));

		if (BspIs64Bits) {

			//
			// N.B. D Probe x64 version only list 64 bits process. 
			//

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

		} 

		else if (BspIsWow64) {

			//
			// N.B. D Probe x86 version only list 32 bits process on 64 bits Windows. 
			//

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
		}

		//
		// Name
		//

		lvi.iItem = Count;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_TEXT | LVIF_IMAGE;
		lvi.pszText = Process->Name;

		SHGetFileInfo(Process->FullPath, 0, &sfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON);
        lvi.iImage = ImageList_AddIcon(hImlSmall, sfi.hIcon);
		DestroyIcon(sfi.hIcon);

		ListView_InsertItem(hWndChild, &lvi);

		//
		// PID
		//

		lvi.iSubItem = 1;
		lvi.mask = LVIF_TEXT;
		wsprintf(Buffer, L"%d", Process->ProcessId);
		lvi.pszText = Buffer;
			
		ListView_SetItem(hWndChild, &lvi);
		
		//
		// Path
		//

		lvi.iSubItem = 2;
		lvi.pszText = Process->FullPath;
		ListView_SetItem(hWndChild, &lvi);

		ListView_EnsureVisible(hWndChild, Count, FALSE);
		BspFree(Process);
		Count += 1; 			
	}
}

LRESULT
DumpOnClose(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PLISTVIEW_OBJECT ListObject;
	HWND hWndChild;
	
	hWndChild = GetDlgItem(hWnd, IDC_LIST_PROCESS);
	ListObject = (PLISTVIEW_OBJECT)SdkGetObject(hWndChild);
	SdkFree(ListObject);
	return 0;
}

LRESULT
DumpOnSmallDump(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	HWND hWndChild;
	LONG Current;
	OPENFILENAME Ofn;
	WCHAR Path[MAX_PATH];	
	WCHAR BaseName[MAX_PATH];	
	WCHAR Buffer[MAX_PATH];	
	ULONG Status;
	ULONG ProcessId;

	hWndChild = GetDlgItem(hWnd, IDC_LIST_PROCESS);
	Current = ListViewGetFirstSelected(hWndChild);
	if (Current == -1) {
		return 0;
	}
	
	ListView_GetItemText(hWndChild, Current, 1, Buffer, MAX_PATH);
	ProcessId = (ULONG)_wtoi(Buffer);

	ListView_GetItemText(hWndChild, Current, 0, Path, MAX_PATH);
	_wsplitpath(Path, NULL, NULL, BaseName, NULL);
	StringCchPrintf(Buffer, MAX_PATH, L"%s.dmp", BaseName);

	ZeroMemory(&Ofn, sizeof(Ofn));

	Ofn.lStructSize = sizeof(Ofn);
	Ofn.hwndOwner = hWnd;
	Ofn.hInstance = SdkInstance;
	Ofn.lpstrFilter = L"Memory Dump File (*.dmp)\0\0";
	Ofn.lpstrFile = Buffer;
	Ofn.nMaxFile = sizeof(Buffer); 
	Ofn.lpstrTitle = L"D Probe";
	Ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT; 

	if (!GetSaveFileName(&Ofn)) {
		return 0L;
	}

	Status = BspCaptureMemoryDump(ProcessId, FALSE, Ofn.lpstrFile, NULL, FALSE);
	if (Status != ERROR_SUCCESS) {
		StringCchPrintf(Buffer, MAX_PATH, L"Error 0x%08x occurred when capturing dump.", Status);
		MessageBox(hWnd, Buffer, L"D Probe", MB_OK|MB_ICONERROR);	
	}

	return 0;
}

LRESULT
DumpOnFullDump(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	HWND hWndChild;
	LONG Current;
	OPENFILENAME Ofn;
	WCHAR Path[MAX_PATH];	
	WCHAR BaseName[MAX_PATH];
	WCHAR Buffer[MAX_PATH];	
	ULONG ProcessId;
	ULONG Status;

	hWndChild = GetDlgItem(hWnd, IDC_LIST_PROCESS);
	Current = ListViewGetFirstSelected(hWndChild);
	if (Current == -1) {
		return 0;
	}
	
	ListView_GetItemText(hWndChild, Current, 1, Buffer, MAX_PATH);
	ProcessId = (ULONG)_wtoi(Buffer);

	ListView_GetItemText(hWndChild, Current, 0, Path, MAX_PATH);
	_wsplitpath(Path, NULL, NULL, BaseName, NULL);
	StringCchPrintf(Buffer, MAX_PATH, L"%s.dmp", BaseName);

	ZeroMemory(&Ofn, sizeof(Ofn));

	Ofn.lStructSize = sizeof(Ofn);
	Ofn.hwndOwner = hWnd;
	Ofn.hInstance = SdkInstance;
	Ofn.lpstrFilter = L"Memory Dump File (*.dmp)\0\0";
	Ofn.lpstrFile = Buffer;
	Ofn.nMaxFile = sizeof(Buffer); 
	Ofn.lpstrTitle = L"D Probe";
	Ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT; 

	if (!GetSaveFileName(&Ofn)) {
		return 0L;
	}

	Status = BspCaptureMemoryDump(ProcessId, TRUE, Ofn.lpstrFile, NULL, FALSE);
	if (Status != ERROR_SUCCESS) {
		StringCchPrintf(Buffer, MAX_PATH, L"Error 0x%08x occurred when capturing dump.", Status);
		MessageBox(hWnd, Buffer, L"D Probe", MB_OK|MB_ICONERROR);	
	}

	return 0;
}

LRESULT
DumpOnContextMenu(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	int x, y;
	HMENU hMenu;
	HMENU hSubMenu;
	int id;

	id = GetDlgCtrlID((HWND)wp);

	if (id != IDC_LIST_PROCESS) {
		return 0L;
	}

	x = GET_X_LPARAM(lp); 
	y = GET_Y_LPARAM(lp); 

	if (x != -1 && y != -1) {
		
		hMenu = LoadMenu(SdkInstance, MAKEINTRESOURCE(IDR_MENU_DUMP));
		hSubMenu = GetSubMenu(hMenu, 0);
		ASSERT(hSubMenu != NULL);

		TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
					   x, y, 0, hWnd, NULL);

		DestroyMenu(hMenu);
	} 

	return 0L;
}

INT_PTR CALLBACK
DumpProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	INT_PTR Result = 0;

	switch (uMsg) {

		case WM_COMMAND:
			DumpOnCommand(hWnd, uMsg, wp, lp);
			break;

		case WM_INITDIALOG:
			DumpOnInitDialog(hWnd, uMsg, wp, lp);
			return TRUE;

		case WM_NOTIFY:
			return DumpOnNotify(hWnd, uMsg, wp, lp);

		case WM_CONTEXTMENU:
			Result = DumpOnContextMenu(hWnd, uMsg, wp, lp);
			break;
	}
	
	return Result;
}