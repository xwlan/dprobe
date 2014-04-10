//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "importdlg.h"
#include "dprobe.h"
#include "csp.h"
#include "fastdlg.h"
#include "filterdlg.h"
#include "mspprocess.h"
#include "msputility.h"

enum {
	TaskColumnName,
	TaskColumnPID,
	TaskColumnSID,
	TaskColumnArgument,
	TaskColumnNumber,
};

enum {
	ProbeColumnName,
	ProbeColumnProvider,
	ProbeColumnNumber,
};

LISTVIEW_COLUMN ImportTaskColumn[TaskColumnNumber] = {
	{ 100, L"Name",  LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 60,  L"PID",  LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 60,  L"SID",  LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 360, L"Argument",  LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

LISTVIEW_COLUMN ImportProbeColumn[ProbeColumnNumber] = {
	{ 200,  L"Probe",    LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 200,  L"Provider", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

typedef struct _IMPORT_CONTEXT {
	BOOLEAN LastPage;
	WCHAR Path[MAX_PATH];
	LIST_ENTRY ProcessList;
	struct _MSP_USER_COMMAND *Command;
	HFONT hBoldFont;
} IMPORT_CONTEXT, *PIMPORT_CONTEXT;

INT_PTR
ImportDialog(
	IN HWND hWndParent,
	IN PWSTR Path,
	OUT PLIST_ENTRY ProcessList,
	OUT PULONG ProcessCount,
	OUT struct _MSP_USER_COMMAND **Command
	)
{
	DIALOG_OBJECT Object = {0};
	IMPORT_CONTEXT Context = {0};
	INT_PTR Return;
	PLIST_ENTRY ListEntry;
	
	DIALOG_SCALER_CHILD Children[4] = {
		{ IDC_LIST_IMPORT,  AlignRight, AlignBottom },
		{ IDC_BUTTON_NEXT,  AlignBoth, AlignBoth },
		{ IDC_STATIC, AlignNone, AlignBoth },
		{ IDCANCEL, AlignBoth, AlignBoth }
	};

	DIALOG_SCALER Scaler = {
		{0,0}, {0,0}, {0,0}, 4, Children
	};

	InitializeListHead(ProcessList);
	InitializeListHead(&Context.ProcessList);
	*Command = NULL;
	*ProcessCount = 0;

	Context.LastPage = FALSE;
	StringCchCopy(Context.Path, MAX_PATH, Path);

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_IMPORT;
	Object.Procedure = ImportProcedure;
	Object.Scaler = &Scaler;

	Return = DialogCreate(&Object);

	while (IsListEmpty(&Context.ProcessList) != TRUE) {
		ListEntry = RemoveHeadList(&Context.ProcessList);
		InsertTailList(ProcessList, ListEntry);
		(*ProcessCount) += 1;
	}

	*Command = Context.Command;
	return Return;
}

LRESULT
ImportOnCommand(
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

	case IDCANCEL:
		if (HIWORD(wp) == 0) {
			ImportOnCancel(hWnd, uMsg, wp, lp);
		}
	    break;

	case IDC_BUTTON_NEXT:
		if (HIWORD(wp) == 0) {
			ImportOnNext(hWnd, uMsg, wp, lp);
		}
	    break;

	}

	return 0;
}

INT_PTR CALLBACK
ImportProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	INT_PTR Status;
	Status = FALSE;

	switch (uMsg) {
		
		case WM_COMMAND:
			ImportOnCommand(hWnd, uMsg, wp, lp);
			Status = TRUE;
			break;
		
		case WM_INITDIALOG:
			return ImportOnInitDialog(hWnd, uMsg, wp, lp);

		case WM_CTLCOLORSTATIC:
			return ImportOnCtlColorStatic(hWnd, uMsg, wp, lp);

		case WM_NOTIFY:
			return ImportOnNotify(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT 
ImportOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LRESULT lres = 0;
	NMHDR *pNmhdr = (NMHDR *)lp;

	if (pNmhdr->idFrom != IDC_LIST_IMPORT) {
		return 0;
	}

	switch (pNmhdr->code) {
		case LVN_COLUMNCLICK:
			return ImportOnColumnClick(pNmhdr->hwndFrom, (NM_LISTVIEW *)lp);
			
	}

	return 0;
}

LRESULT
ImportOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LVCOLUMN lvc = {0}; 
	PDIALOG_OBJECT Object;
	HWND hWndList;
	HWND hWndText;
	LONG Style;
	ULONG Status;
	int i;

	hWndList = GetDlgItem(hWnd, IDC_LIST_IMPORT);
    ListView_SetUnicodeFormat(hWndList, TRUE);
    ListView_SetExtendedListViewStyle(hWndList, LVS_EX_FULLROWSELECT);

	//
	// Enable group feature of list view control
	//

	Style = GetWindowLong(hWndList, GWL_STYLE);
	SetWindowLong(hWndList, GWL_STYLE, Style | LVS_ALIGNTOP);
	ListView_EnableGroupView(hWndList, TRUE); 

    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 

    for (i = 0; i < ProbeColumnNumber; i++) { 
        lvc.iSubItem = i;
		lvc.pszText = ImportProbeColumn[i].Title;	
		lvc.cx = ImportProbeColumn[i].Width;     
		lvc.fmt = ImportProbeColumn[i].Align;
		ListView_InsertColumn(hWndList, i, &lvc);
	}

	hWndText = GetDlgItem(hWnd, IDC_STATIC);
	SetWindowText(hWndText, L"");

	SetWindowText(hWnd, L"Import Probe");
	SdkSetMainIcon(hWnd);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Status = ImportInsertProbe(Object);

	if (Status != ERROR_SUCCESS) {
		return FALSE;
	}

	SdkCenterWindow(hWnd);
	return TRUE;
}

ULONG
ImportReadFileHead(
	IN PWSTR Path,
	OUT PCSP_FILE_HEADER Head
	)
{
	HANDLE FileHandle;
	ULONG CompleteBytes;

	FileHandle = CreateFile(Path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 
		                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		return GetLastError();
	}

	ReadFile(FileHandle, Head, sizeof(CSP_FILE_HEADER), &CompleteBytes, NULL);
	CloseHandle(FileHandle);

	return ERROR_SUCCESS;
}

ULONG
ImportInsertProbe(
	IN PDIALOG_OBJECT Object
	)
{
	PIMPORT_CONTEXT Context;
	PMSP_USER_COMMAND Command;
	CSP_FILE_HEADER Head;
	ULONG Status;

	Context = SdkGetContext(Object, IMPORT_CONTEXT);

	Status = ImportReadFileHead(Context->Path, &Head);
	if (Status != ERROR_SUCCESS) {
		MessageBox(Object->hWnd, L"Failed to import probe!", 
			       L"D Probe", MB_OK|MB_ICONERROR);
		return ERROR_INVALID_PARAMETER;
	}

	Command = NULL;

	if (Head.Type == PROBE_FAST) {
		Status = FastImportProbe(Context->Path, &Command);
	}

	if (Head.Type == PROBE_FILTER) {
		Status = FilterImportProbe(Context->Path, &Command);
	}

	if (!Command) {
		MessageBox(Object->hWnd, L"Failed to import probe!", 
			       L"D Probe", MB_OK|MB_ICONERROR);
		return ERROR_INVALID_PARAMETER;
	}

	if (Head.Type == PROBE_FAST) {
		Status = ImportInsertFast(Object, Command);
	}

	if (Head.Type == PROBE_FILTER) {
		Status = ImportInsertFilter(Object, Command);
	}

	if (Status == ERROR_SUCCESS) {
		Context->Command = Command;
	} else {
		Context->Command = NULL;
	}

	return Status;
}

ULONG
ImportInsertFast(
	IN PDIALOG_OBJECT Object,
	IN struct _MSP_USER_COMMAND *Command
	)
{
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_USER_COMMAND BtrCommand;
	LVITEM lvi = {0};
	LVGROUP Group = {0};
	HWND hWndList;
	HWND hWndText;
	ULONG Number;
	PWCHAR Unicode;
	WCHAR Buffer[MAX_PATH];
	ULONG DllOrdinal;
	int index;

	DllOrdinal = 0;
	index = 0;
	hWndList = GetDlgItem(Object->hWnd, IDC_LIST_IMPORT);

	ListHead = &Command->CommandList;	
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {

		BtrCommand = CONTAINING_RECORD(ListEntry, BTR_USER_COMMAND, ListEntry);

		Group.cbSize = sizeof(LVGROUP);
		Group.mask = LVGF_HEADER | LVGF_GROUPID;
		Group.pszHeader = BtrCommand->DllName; 
		Group.iGroupId = DllOrdinal;

		if (BspIsVistaAbove()) {
			Group.mask |= LVGF_STATE;
			Group.state = LVGS_COLLAPSIBLE;
		}

		ListView_InsertGroup(hWndList, -1, &Group); 

		for (Number = 0; Number < BtrCommand->Count; Number += 1) {
		
			lvi.mask = LVIF_TEXT | LVIF_GROUPID | LVIF_COLUMNS;
			lvi.iItem = index;
			lvi.iSubItem = 0;
			lvi.iGroupId = DllOrdinal;

			MspConvertAnsiToUnicode(BtrCommand->Probe[Number].ApiName, &Unicode);
			BspUnDecorateSymbolName(Unicode, Buffer, MAX_PATH, UNDNAME_COMPLETE);
			MspFree(Unicode);

			lvi.pszText = Buffer;
			ListView_InsertItem(hWndList, &lvi);

			lvi.mask = LVIF_TEXT;
			lvi.iItem = index;
			lvi.iSubItem = 1;
			lvi.iGroupId = DllOrdinal;
			lvi.pszText = L"Fast Probe";
			ListView_SetItem(hWndList, &lvi);

			index += 1;
		
		}

		DllOrdinal += 1;
		ListEntry = ListEntry->Flink;
	}

	hWndText = GetDlgItem(Object->hWnd, IDC_STATIC);
	StringCchPrintf(Buffer, MAX_PATH, L"Total %u probes imported.", index);
	SetWindowText(hWndText, Buffer);
	return ERROR_SUCCESS;
}

ULONG
ImportInsertFilter(
	IN PDIALOG_OBJECT Object,
	IN struct _MSP_USER_COMMAND *Command
	)
{
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PBTR_USER_COMMAND BtrCommand;
	PBTR_FILTER Filter;
	LVITEM lvi = {0};
	LVGROUP Group = {0};
	HWND hWndList;
	HWND hWndText;
	ULONG Number;
	WCHAR Buffer[MAX_PATH];
	ULONG DllOrdinal;
	ULONG FilterOrdinal;
	int index;

	DllOrdinal = 0;
	index = 0;
	hWndList = GetDlgItem(Object->hWnd, IDC_LIST_IMPORT);

	ListHead = &Command->CommandList;	
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {

		BtrCommand = CONTAINING_RECORD(ListEntry, BTR_USER_COMMAND, ListEntry);
		
		//
		// N.B. Filter's DllName must be filled as filter's full path, dpc file
		// contain a valid GUID, when imported from dpc file, DllName should be
		// filled via map GUID to FilterPath
		//

		Filter = MspQueryDecodeFilterByPath(BtrCommand->DllName);
		if (!Filter) {

			StringCchPrintf(Buffer, MAX_PATH, L"Can not find filter %s!", 
				            wcsrchr(BtrCommand->DllName, L'\\') + 1);

			MessageBox(Object->hWnd, Buffer, L"D Probe", MB_OK|MB_ICONERROR);
			return ERROR_INVALID_PARAMETER;

		}

		Group.cbSize = sizeof(LVGROUP);
		Group.mask = LVGF_HEADER | LVGF_GROUPID;
		Group.pszHeader = Filter->FilterName; 
		Group.iGroupId = DllOrdinal;

		if (BspIsVistaAbove()) {
			Group.mask |= LVGF_STATE;
			Group.state = LVGS_COLLAPSIBLE;
		}

		ListView_InsertGroup(hWndList, -1, &Group); 


		for (Number = 0; Number < BtrCommand->Count; Number += 1) {
		
			lvi.mask = LVIF_TEXT | LVIF_GROUPID | LVIF_COLUMNS;
			lvi.iItem = index;
			lvi.iSubItem = 0;
			lvi.iGroupId = DllOrdinal;

			FilterOrdinal = BtrCommand->Probe[Number].Number;
			StringCchPrintf(Buffer, MAX_PATH, L"%s!%s", Filter->Probes[FilterOrdinal].DllName, 
							Filter->Probes[FilterOrdinal].ApiName);

			lvi.pszText = Buffer;
			ListView_InsertItem(hWndList, &lvi);

			lvi.mask = LVIF_TEXT;
			lvi.iItem = index;
			lvi.iSubItem = 1;
			lvi.iGroupId = DllOrdinal;
			lvi.pszText = Filter->FilterName;
			ListView_SetItem(hWndList, &lvi);

			index += 1;
		
		}

		DllOrdinal += 1;
		ListEntry = ListEntry->Flink;
	}

	hWndText = GetDlgItem(Object->hWnd, IDC_STATIC);
	StringCchPrintf(Buffer, MAX_PATH, L"Total %u probes imported.", index);
	SetWindowText(hWndText, Buffer);
	return ERROR_SUCCESS;
}

LRESULT
ImportOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	ImportCleanUp(Object, FALSE);
	EndDialog(hWnd, IDCANCEL);
	return 0;
}

LRESULT
ImportOnNext(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PIMPORT_CONTEXT Context;
	PBSP_PROCESS Process;
	HWND hWndText;
	HWND hWndNext;
	HWND hWndList;
	int index;
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, IMPORT_CONTEXT);
	
	if (!Context->LastPage) {

		Context->LastPage = TRUE;

		hWndText = GetDlgItem(hWnd, IDC_STATIC);
		ShowWindow(hWndText, SW_HIDE);

		hWndNext = GetDlgItem(hWnd, IDC_BUTTON_NEXT);
		SetWindowText(hWndNext, L"OK");

		ImportInsertTask(Object);
		return 0;
	}

	hWndList = GetDlgItem(hWnd, IDC_LIST_IMPORT);
	index = ListViewGetFirstSelected(hWndList);
	if (index == -1) {
		MessageBox(hWnd, L"Process not selected!", L"D Probe", MB_OK|MB_ICONWARNING);
		return 0;
	}

	while (index != -1) {

		ListViewGetParam(hWndList, index, (LPARAM *)&Process);
		ASSERT(Process != NULL);

		InsertTailList(&Context->ProcessList, &Process->ListEntry);
		ListViewSetParam(hWndList, index, (LPARAM)NULL);

		index = ListViewGetNextSelected(hWndList, index);
	}

	ImportCleanUp(Object, TRUE);
	EndDialog(hWnd, IDOK);
	return 0;
}

LRESULT
ImportOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PIMPORT_CONTEXT Context;
	UINT Id;
	HFONT hFont;
	LOGFONT LogFont;
	RECT Rect;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PIMPORT_CONTEXT)Object->Context;
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

LRESULT 
ImportOnColumnClick(
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
    ListView_SortItemsEx(hWnd, ImportSortCallback, (LPARAM)Object);

    return 0L;
}

int CALLBACK
ImportSortCallback(
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

	FirstData[0] = 0;
	SecondData[0] = 0;

	Object = (PLISTVIEW_OBJECT)Param;
	hWnd = Object->hWnd;

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

ULONG
ImportInsertTask(
	IN PDIALOG_OBJECT Object
	)
{
	PIMPORT_CONTEXT Context;
	HWND hWndList;
	LIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	LVITEM lvi = {0};
	LVCOLUMN lvc = {0};
	SHFILEINFO sfi = {0}; 
	WCHAR Buffer[MAX_PATH];
	HANDLE ProcessHandle;
	ULONG CurrentId;
	PBSP_PROCESS Process;
	int i;	
	PLISTVIEW_OBJECT ListObject;

	Context = SdkGetContext(Object, IMPORT_CONTEXT);
	hWndList = GetDlgItem(Object->hWnd, IDC_LIST_IMPORT);

	//
	// Attach a listview object to list control for sort
	//

	ListObject = (LISTVIEW_OBJECT *)SdkMalloc(sizeof(LISTVIEW_OBJECT));
	ListObject->Column = ImportTaskColumn;
	ListObject->Count = TaskColumnNumber;
	ListObject->CtrlId = IDC_LIST_IMPORT;
	ListObject->hImlSmallIcon = 0;
	ListObject->hWnd = hWndList;
	SdkSetObject(hWndList, ListObject);

	//
	// N.B. group must be disabled to insert task items
	//

	ListView_EnableGroupView(hWndList, FALSE);
	ListView_DeleteAllItems(hWndList);

	//
	// reset probe columns and insert a new column
	//
	
	lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 

    lvc.iSubItem = 0;
	lvc.pszText = ImportTaskColumn[0].Title;	
	lvc.cx = ImportTaskColumn[0].Width;     
	lvc.fmt = ImportTaskColumn[0].Align;
	ListView_SetColumn(hWndList, 0, &lvc);

    lvc.iSubItem = 1;
	lvc.pszText = ImportTaskColumn[1].Title;	
	lvc.cx = ImportTaskColumn[1].Width;     
	lvc.fmt = ImportTaskColumn[1].Align;
	ListView_SetColumn(hWndList, 1, &lvc);

    lvc.iSubItem = 2;
	lvc.pszText = ImportTaskColumn[2].Title;	
	lvc.cx = ImportTaskColumn[2].Width;     
	lvc.fmt = ImportTaskColumn[2].Align;
	ListView_InsertColumn(hWndList, 2, &lvc);

    lvc.iSubItem = 3;
	lvc.pszText = ImportTaskColumn[3].Title;	
	lvc.cx = ImportTaskColumn[3].Width;     
	lvc.fmt = ImportTaskColumn[3].Align;
	ListView_InsertColumn(hWndList, 3, &lvc);

	InitializeListHead(&ListHead);
	BspQueryProcessList(&ListHead);

	//
	// Query and build process list
	//

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
		
		if (!_wcsicmp(Process->Name, L"dprobe.exe") || 
			!_wcsicmp(Process->Name, L"dprobe64.exe")) {
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
		}
		
		RtlZeroMemory(&lvi, sizeof(lvi));
		RtlZeroMemory(&sfi, sizeof(sfi));
		
		//
		// Name
		//

		lvi.iItem = i;
		lvi.iSubItem = TaskColumnName;
		lvi.mask = LVIF_TEXT | LVIF_PARAM;
		lvi.pszText = wcslwr(Process->Name);
		lvi.lParam = (LPARAM)Process;
		ListView_InsertItem(hWndList, &lvi);

		//
		// PID
		//

		lvi.iSubItem = TaskColumnPID;
		lvi.mask = LVIF_TEXT;
		wsprintf(Buffer, L"%d", Process->ProcessId);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndList, &lvi);
		
		//
		// SID
		//

		lvi.iSubItem = TaskColumnSID;
		lvi.mask = LVIF_TEXT;
		wsprintf(Buffer, L"%d", Process->SessionId);
		lvi.pszText = Buffer;
		ListView_SetItem(hWndList, &lvi);
		
		//
		// Argument
		//

		lvi.iSubItem = TaskColumnArgument;
		lvi.mask = LVIF_TEXT;
		lvi.pszText = Process->CommandLine;
		ListView_SetItem(hWndList, &lvi);

		i += 1;
	}

	return 0;
}

VOID
ImportCleanUp(
	IN PDIALOG_OBJECT Object,
	IN BOOLEAN Success
	)
{
	HWND hWndList;
	int i, Count;
	LVITEM lvi = {0};
	PBSP_PROCESS Process;
	PIMPORT_CONTEXT Context;
	PLIST_ENTRY ListEntry;
	PLISTVIEW_OBJECT ListObject;

	Context = SdkGetContext(Object, IMPORT_CONTEXT);
	hWndList = GetDlgItem(Object->hWnd, IDC_LIST_IMPORT);
	Count = ListView_GetItemCount(hWndList);

	if (Context->LastPage) {
		
		for(i = 0; i < Count; i += 1) {

			lvi.iItem = Count;
			lvi.mask = LVIF_PARAM;
			ListView_GetItem(hWndList, &lvi);

			Process = (PBSP_PROCESS)lvi.lParam;
			if (Process != NULL) {
				BspFreeProcess(Process);
			}
		}

		ListObject = (PLISTVIEW_OBJECT)SdkGetObject(hWndList);

		if (ListObject) {
			SdkFree(ListObject);
		}
	}
	
	if (Context->hBoldFont) {
		DeleteFont(Context->hBoldFont);
	}

	if (!Success) {

		ImportFreeCommand(Context->Command);	

		if (Context->LastPage) { 
			while (IsListEmpty(&Context->ProcessList) != TRUE) {
				ListEntry = RemoveHeadList(&Context->ProcessList);
				Process = CONTAINING_RECORD(ListEntry, BSP_PROCESS, ListEntry);
				BspFreeProcess(Process);
			}
		}
	}
}

VOID
ImportFreeCommand(
	IN struct _MSP_USER_COMMAND *Command
	)
{
	PBTR_USER_COMMAND BtrCommand;
	PLIST_ENTRY ListEntry;

	if (!Command) {
		return;
	}

	while (IsListEmpty(&Command->CommandList) != TRUE) {
		ListEntry = RemoveHeadList(&Command->CommandList);
		BtrCommand = CONTAINING_RECORD(ListEntry, BTR_USER_COMMAND, ListEntry);	
		SdkFree(BtrCommand);
	}

	SdkFree(Command);
}