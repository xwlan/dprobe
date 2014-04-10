//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "dprobe.h"
#include "processdlg.h"
#include "mspprocess.h"

typedef enum _TASK_COLUMN_TYPE {
	ProcessColumnName,
	ProcessColumnPID,
	ProcessColumnSID,
	ProcessColumnProbe,
	ProcessColumnPath,
	ProcessColumnNumber,
} TASK_COLUMN_TYPE;

LISTVIEW_COLUMN ProcessColumn[ProcessColumnNumber] = {
	{ 120, L"Name", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 60, L"PID",  LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeUInt },
	{ 60, L"SID",  LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeUInt },
	{ 60, L"Probe",  LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeUInt },
	{ 360, L"Path", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

INT_PTR
ProcessDialog(
	IN HWND hWndParent,
	OUT PBSP_PROCESS *Process
	)
{
	DIALOG_OBJECT Object = {0};
	INT_PTR Return;
	
	DIALOG_SCALER_CHILD Children[4] = {
		{ IDC_BUTTON_REFRESH, AlignNone, AlignBoth },
		{ IDOK, AlignBoth, AlignBoth },
		{ IDCANCEL, AlignBoth, AlignBoth },
		{ IDC_LIST_PROCESSVIEW, AlignRight, AlignBottom }
	};

	DIALOG_SCALER Scaler = {
		{0,0}, {0,0}, {0,0}, 4, Children
	};

	Object.Context = NULL;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_PROCESSVIEW;
	Object.Procedure = ProcessProcedure;
	Object.Scaler = &Scaler;

	Return = DialogCreate(&Object);
	*Process = (PBSP_PROCESS)Object.Context;
	return Return;
}

VOID
ProcessInitialize(
	IN PLISTVIEW_OBJECT Object
	)
{
	PLIST_ENTRY ListEntry;
	HWND hWnd;
	LVITEM lvi = {0};
	SHFILEINFO sfi = {0}; 
	WCHAR Buffer[MAX_PATH];
	int i;	
	DWORD_PTR Status;
	HANDLE ProcessHandle;
	ULONG CurrentId;
	BOOLEAN Probing;
	PBSP_PROCESS Process;
	LIST_ENTRY ListHead;

	ASSERT(Object != NULL);

	hWnd = Object->hWnd;
	i = 0;

	CurrentId = GetCurrentProcessId();

	InitializeListHead(&ListHead);
	BspQueryProcessList(&ListHead);

	while (IsListEmpty(&ListHead) != TRUE) {

		ListEntry = RemoveHeadList(&ListHead);
		Process = CONTAINING_RECORD(ListEntry, BSP_PROCESS, ListEntry);

		if (Process->ProcessId == 0 || Process->ProcessId == 4 ||
			Process->ProcessId == CurrentId ) {
			BspFreeProcess(Process);	
			continue;
		}
		
		if (BspIs64Bits) {

			//
			// N.B. D Probe x64 only list 64 bits process. 
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
			// N.B. D Probe x86 only list 32 bits process on 64 bits Windows. 
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
		
		RtlZeroMemory(&lvi, sizeof(lvi));
		RtlZeroMemory(&sfi, sizeof(sfi));
		
		Probing = MspIsProbing(Process->ProcessId);

		//
		// Attach process object to each inserted item
		//

		lvi.iItem = i;
		lvi.iSubItem = ProcessColumnName;
		lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
		lvi.pszText = Process->Name;
		lvi.lParam = (LPARAM)Process;

		Status = SHGetFileInfo(Process->FullPath, 0, &sfi, sizeof(SHFILEINFO), 
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

		lvi.iSubItem = ProcessColumnPID;
		lvi.mask = LVIF_TEXT;
		wsprintf(Buffer, L"%d", Process->ProcessId);
		lvi.pszText = Buffer;
		ListView_SetItem(hWnd, &lvi);
		
		lvi.iSubItem = ProcessColumnSID;
		lvi.mask = LVIF_TEXT;
		wsprintf(Buffer, L"%d", Process->SessionId);
		lvi.pszText = Buffer;
		ListView_SetItem(hWnd, &lvi);

		lvi.iSubItem = ProcessColumnProbe;
		lvi.mask = LVIF_TEXT;
		
		if (Probing) {
			lvi.pszText = L"Yes";
		} else {
			lvi.pszText = L"No";
		}	
		ListView_SetItem(hWnd, &lvi);

		lvi.iSubItem = ProcessColumnPath;
		lvi.pszText = Process->CommandLine;
		ListView_SetItem(hWnd, &lvi);

		i += 1;
	}
}

int CALLBACK
ProcessSortCallback(
	IN LPARAM First, 
	IN LPARAM Second,
	IN LPARAM Param
	)
{
	HWND hWnd;
    int Result;
    WCHAR FirstData[MAX_PATH + 1];
    WCHAR SecondData[MAX_PATH + 1];
	LIST_DATA_TYPE Type;
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
ProcessOnColumnClick(
	IN HWND hWnd,
	IN NMLISTVIEW *lpNmlv
	)
{
	HWND hWndHeader;
	int ColumnCount;
	LISTVIEW_OBJECT *Object;
	HDITEM hdi;
	int i;

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

    ColumnCount = Header_GetItemCount(hWndHeader);
    
    for (i = 0; i < ColumnCount; i++) {
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
    ListView_SortItemsEx(hWnd, ProcessSortCallback, (LPARAM)hWnd);

    return 0L;
}

LRESULT 
ProcessOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LRESULT lres = 0;
	NMHDR *pNmhdr = (NMHDR *)lp;

	if (pNmhdr->idFrom != IDC_LIST_PROCESSVIEW) {
		return 0;
	}

	switch (pNmhdr->code) {
		case LVN_COLUMNCLICK:
			return ProcessOnColumnClick(pNmhdr->hwndFrom, (NM_LISTVIEW *)lp);
			
	}

	return 0;
}

LRESULT
ProcessOnInitDialog(
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
	int i;

	hWndChild = GetDlgItem(hWnd, IDC_LIST_PROCESSVIEW);
	ASSERT(hWndChild != NULL);
	
    ListView_SetUnicodeFormat(hWndChild, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndChild, LVS_EX_FULLROWSELECT,  LVS_EX_FULLROWSELECT);
	ListView_SetExtendedListViewStyleEx(hWndChild, LVS_EX_HEADERDRAGDROP, LVS_EX_HEADERDRAGDROP); 

	himlSmall = ImageList_Create(16, 16, ILC_COLOR24, 0, 0); 
	ListView_SetImageList(hWndChild, himlSmall, LVSIL_SMALL);

    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 
	  
    for (i = 0; i < ProcessColumnNumber; i++) { 

        lvc.iSubItem = i;
		lvc.pszText = ProcessColumn[i].Title;	
		lvc.cx = ProcessColumn[i].Width;     
		lvc.fmt = ProcessColumn[i].Align;

		ListView_InsertColumn(hWndChild, i, &lvc);
    } 

	Object = (LISTVIEW_OBJECT *)SdkMalloc(sizeof(LISTVIEW_OBJECT));
	Object->Column = ProcessColumn;
	Object->Count = ProcessColumnNumber;
	Object->CtrlId = IDC_LIST_PROCESSVIEW;
	Object->hImlSmallIcon = himlSmall;
	Object->hWnd = hWndChild;
	
	SdkSetObject(hWndChild, Object);
	ProcessInitialize(Object);

	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)SdkMainIcon);
	return TRUE;
}

LRESULT
ProcessOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndList;
	PDIALOG_OBJECT Object;
	PBSP_PROCESS Process;
	LONG Selected;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

	hWndList = GetDlgItem(hWnd, IDC_LIST_PROCESSVIEW);			
	Selected = ListViewGetFirstSelected(hWndList);

	if (Selected == -1) {
		Object->Context = NULL;

	} else {

		//
		// Get selected process object and remove listview item
		// otherwise, ProcessCleanUp will free it
		//

		ListViewGetParam(hWndList, Selected, (LPARAM *)&Process);
		ListView_DeleteItem(hWndList, Selected);
		Object->Context = Process;
	}

	ProcessCleanUp(hWndList);
	EndDialog(hWnd, IDOK); 
	return 0;
}

LRESULT
ProcessOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndList;

	hWndList = GetDlgItem(hWnd, IDC_LIST_PROCESSVIEW);			
	ProcessCleanUp(hWndList);
	EndDialog(hWnd, IDCANCEL);
	return 0;
}

LRESULT
ProcessOnCommand(
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
			return ProcessOnOk(hWnd, uMsg, wp, lp);

		case IDCANCEL:
			return ProcessOnCancel(hWnd, uMsg, wp, lp);

		case IDC_BUTTON_REFRESH:
			ProcessOnRefresh(hWnd);
			break;
	}

	return 0;
}

VOID
ProcessOnRefresh(
	IN HWND hWnd
	)
{
	HWND hWndList;
	LIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PBSP_PROCESS Process;
	PBSP_PROCESS Attached;
	LVITEM lvi = {0};
	SHFILEINFO sfi = {0};
	HIMAGELIST hImlSmall;
	HANDLE ProcessHandle;
	BOOLEAN Probing;
	int i, Count;
	WCHAR Buffer[MAX_PATH];

	ASSERT(hWnd != NULL);

	hWndList = GetDlgItem(hWnd, IDC_LIST_PROCESSVIEW);
	ASSERT(hWndList != NULL);

	InitializeListHead(&ListHead);
	BspQueryProcessList(&ListHead);

	Count = ListView_GetItemCount(hWndList);

	//
	// First check whether old process list has terminated ones
	//

	for(i = 0; i < Count; ) {
		
		lvi.iItem = i;
		lvi.mask = LVIF_PARAM;
		ListView_GetItem(hWndList, &lvi);

		Attached = (PBSP_PROCESS)lvi.lParam;
		ASSERT(Attached != NULL);

		Process = BspQueryProcessById(&ListHead, Attached->ProcessId);

		//
		// Remove terminated process, free its attach process object
		//

		if (!Process) {
			BspFreeProcess(Attached);
			ListView_DeleteItem(hWndList, i);
			Count = Count - 1;
			continue;

		} else {
			RemoveEntryList(&Process->ListEntry);
			BspFreeProcess(Process);
			i += 1;
		}
	}

	//
	// Insert new process entry
	//

	hImlSmall = ListView_GetImageList(hWndList, LVSIL_SMALL);

	while (IsListEmpty(&ListHead) != TRUE) {
		
		ListEntry = RemoveHeadList(&ListHead);

		Process = CONTAINING_RECORD(ListEntry, BSP_PROCESS, ListEntry);
		if (Process->ProcessId == 0 || Process->ProcessId == 4) {
			BspFreeProcess(Process);
			continue;
		}
		
		memset(&lvi, 0, sizeof(lvi));
		memset(&sfi, 0, sizeof(sfi));

		Probing = MspIsProbing(Process->ProcessId);
		
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

		if (!_wcsicmp(Process->Name, L"dprobe.exe")) {
			BspFreeProcess(Process);	
			continue;
		}

		lvi.iItem = Count;
		lvi.iSubItem = ProcessColumnName;
		lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
		lvi.pszText = Process->Name;
		lvi.lParam = (LPARAM)Process;		

		SHGetFileInfo(Process->FullPath, 0, &sfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON);
        lvi.iImage = ImageList_AddIcon(hImlSmall, sfi.hIcon);
		DestroyIcon(sfi.hIcon);

		ListView_InsertItem(hWndList, &lvi);

		lvi.iSubItem = ProcessColumnPID;
		lvi.mask = LVIF_TEXT;
		wsprintf(Buffer, L"%d", Process->ProcessId);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndList, &lvi);

		lvi.iSubItem = ProcessColumnSID;
		lvi.mask = LVIF_TEXT;
		wsprintf(Buffer, L"%d", Process->SessionId);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndList, &lvi);

		lvi.iSubItem = ProcessColumnProbe;
		lvi.mask = LVIF_TEXT;
		if (Probing) {
			lvi.pszText = L"Yes";
		} else {
			lvi.pszText = L"No";
		}
		ListView_SetItem(hWndList, &lvi);

		lvi.iSubItem = ProcessColumnPath;
		lvi.pszText = Process->CommandLine;
		ListView_SetItem(hWndList, &lvi);
	
		ListView_EnsureVisible(hWndList, Count, FALSE);
		Count += 1; 			
	}
}

INT_PTR CALLBACK
ProcessProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	switch (uMsg) {

		case WM_COMMAND:
			return ProcessOnCommand(hWnd, uMsg, wp, lp);

		case WM_INITDIALOG:
			ProcessOnInitDialog(hWnd, uMsg, wp, lp);
			SdkCenterWindow(hWnd);
			return TRUE;

		case WM_NOTIFY:
			return ProcessOnNotify(hWnd, uMsg, wp, lp);
	}
	
	return 0;
}

VOID
ProcessCleanUp(
	IN HWND hWndList
	)
{
	int i, Count;
	PBSP_PROCESS Process;
	PLISTVIEW_OBJECT ListObject;
	LPARAM Param;

	Count = ListView_GetItemCount(hWndList);

	for(i = 0; i < Count; i += 1) {

		Param = 0;
		ListViewGetParam(hWndList, i, &Param);

		Process = (PBSP_PROCESS)Param;
		if (Process != NULL) {
			BspFreeProcess(Process);
		}
	}
	
	ListObject = (PLISTVIEW_OBJECT)SdkGetObject(hWndList);
	SdkFree(ListObject);
}

