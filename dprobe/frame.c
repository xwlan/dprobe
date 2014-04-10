//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "sdk.h"
#include "frame.h"
#include "menu.h"
#include "listview.h"
#include "dprobe.h"
#include "csp.h"
#include "importdlg.h"
#include "stopdlg.h"
#include "savedlg.h"
#include "progressdlg.h"
#include "conditiondlg.h"
#include "perfdlg.h"
#include "perfwnd.h"
#include "rebar.h"
#include "option.h"
#include "detaildlg.h"
#include "rundlg.h"
#include "procdump.h"
#include "main.h"
#include "probingdlg.h"
#include "resultdlg.h"
#include "counterdlg.h"
#include "reportdlg.h"
#include "mspdtl.h"
#include "mspprocess.h"
#include "msputility.h"
#include "fastdlg.h"
#include "filterdlg.h"
#include "euladlg.h"

PWSTR SdkFrameClass = L"SdkFrame";

//
// Custom WM to update status bar
//

PWSTR WmUserStatus = L"WM_USER_STATUS";
PWSTR WmUserFilter = L"WM_USER_FILTER";

UINT WM_USER_STATUS;
UINT WM_USER_FILTER;

FRAME_MENU_ITEM FrameProbingMap[64] = {
	{ 0, 0, {0} },
};

BOOLEAN
FrameRegisterClass(
	VOID
	)
{
    WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(wc);

	SdkMainIcon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_APSARA));
    
	wc.hbrBackground  = GetSysColorBrush(COLOR_BTNFACE);
    wc.hCursor        = LoadCursor(0, IDC_ARROW);
    wc.hIcon          = SdkMainIcon;
    wc.hIconSm        = SdkMainIcon;
    wc.hInstance      = SdkInstance;
    wc.lpfnWndProc    = FrameProcedure;
    wc.lpszClassName  = SdkFrameClass;
	wc.lpszMenuName   = MAKEINTRESOURCE(IDR_DPROBE);

    RegisterClassEx(&wc);
    return TRUE;
}

HWND 
FrameCreate(
	IN PFRAME_OBJECT Object 
	)
{
	BOOLEAN Status;
	HWND hWnd;

	Status = FrameRegisterClass();
	ASSERT(Status);

	if (Status != TRUE) {
		return NULL;
	}

	hWnd = CreateWindowEx(0, SdkFrameClass, L"", 
						  WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
						  0, 0,
						  800, 600,
						  0, 0, SdkInstance, (PVOID)Object);

	ASSERT(hWnd);

	if (hWnd != NULL) {
		SdkSetMainIcon(hWnd);
		ShowWindow(hWnd, SW_SHOW);
		UpdateWindow(hWnd);
		return hWnd;

	} else {
		return NULL;
	}
}

VOID
FrameGetViewRect(
	IN PFRAME_OBJECT Object,
	OUT RECT *rcView
	)
{
	RECT rcClient;
	RECT rcRebar = {0};
    RECT rcStatusBar = {0};

    GetClientRect(Object->hWnd, &rcClient);
    
    if (Object->hWndStatusBar) {
        GetWindowRect(Object->hWndStatusBar, &rcStatusBar);
    }
	
    if (Object->Rebar != NULL) {
		GetWindowRect(Object->Rebar->hWndRebar, &rcRebar);
    }
	
	rcView->top    = rcClient.top + (rcRebar.bottom - rcRebar.top);
	rcView->left   = rcClient.left;
	rcView->right  = rcClient.right;
    rcView->bottom = rcClient.bottom - rcClient.top - (rcStatusBar.bottom - rcStatusBar.top);

	if ((rcView->bottom <= rcView->top) || (rcView->right <= rcView->left)){
        rcView->top = 0;
		rcView->left = 0;
		rcView->right = 0;
		rcView->left = 0;    
    }
}

LRESULT 
FrameOnContextMenu(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	int x, y;
	POINT Point;
	RECT Rect;
	PFRAME_OBJECT Object;

	x = GET_X_LPARAM(lp); 
	y = GET_Y_LPARAM(lp); 

	Point.x = x;
	Point.y = y;

	ScreenToClient(hWnd, &Point);
	Object = (PFRAME_OBJECT)SdkGetObject(hWnd);
	FrameGetViewRect(Object, &Rect);

	if (Point.x >= Rect.left && Point.x <= Rect.right && 
		Point.y >= Rect.top  && Point.y <= Rect.bottom ) {

		LogViewOnContextMenu(hWnd, uMsg, wp, lp);
	}

	return 0L;
}

LRESULT
FrameOnFastProbe(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PBSP_PROCESS Process;
	PMSP_USER_COMMAND Command;
	INT_PTR Return;

	Return = ProcessDialog(hWnd, &Process);
	if (Return == IDCANCEL || !Process) {
		return 0;
	}

	Return = FastDialog(hWnd, Process, &Command);
	if (Return == IDCANCEL || !Command) {
		return 0; 
	} 

	if (Command != NULL) {
		ApplyDialog(hWnd, Process, Command);
	}

	if (Command->FailureCount != 0) {
		ResultDialog(hWnd, Process, Command);
		MspFreeFailureList(Command);
	}

	LogViewAddImageList(Process->FullPath, Process->ProcessId);

	FastFreeCommand(Command);
	BspFreeProcess(Process);
	return 0;
}

LRESULT
FrameOnFilterProbe(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PBSP_PROCESS Process;
	PMSP_USER_COMMAND Command;
	INT_PTR Return;

	Return = ProcessDialog(hWnd, &Process);
	if (Return == IDCANCEL || !Process) {
		return 0;
	}

	Return = FilterDialog(hWnd, Process, &Command);
	if (Return == IDCANCEL || !Command) {
		return 0; 
	} 

	if (Command != NULL) {
		ApplyDialog(hWnd, Process, Command);
	}

	if (Command->FailureCount != 0) {
		ResultDialog(hWnd, Process, Command);
		MspFreeFailureList(Command);
	}

	LogViewAddImageList(Process->FullPath, Process->ProcessId);

	FilterFreeCommand(Command);
	BspFreeProcess(Process);
	return 0;
}

LRESULT
FrameOnExit(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	FrameOnClose(hWnd, uMsg, wp, lp);	
	PostQuitMessage(0);
	return 0;
}

LRESULT
FrameOnOpen(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	ULONG Status;
	OPENFILENAME Ofn;
	WCHAR Path[MAX_PATH];	
	WCHAR Buffer[MAX_PATH];	
	HANDLE ThreadHandle;
	PMSP_DTL_OBJECT DtlObject;

	ZeroMemory(&Ofn, sizeof Ofn);
	ZeroMemory(Path, sizeof(Path));

	Ofn.lStructSize = sizeof(Ofn);
	Ofn.hwndOwner = hWnd;
	Ofn.hInstance = SdkInstance;
	Ofn.lpstrFilter = Ofn.lpstrFilter = L"D Probe Trace Log (*.dtl)\0*.dtl\0\0";
	Ofn.lpstrFile = Path;
	Ofn.nMaxFile = sizeof(Path); 
	Ofn.lpstrTitle = L"D Probe";
	Ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	Status = GetOpenFileName(&Ofn);
	if (!Status) {
		return 0;
	}

	DtlObject = NULL;
	Status = MspOpenDtlReport(Ofn.lpstrFile, &DtlObject);

	if (Status == MSP_STATUS_REQUIRE_32BITS) {
		MessageBox(hWnd, L"Please use 32 bits dprobe to open 32 bits report.", 
			       L"D Probe", MB_OK|MB_ICONWARNING);
		return 0;
	}

	else if (Status == MSP_STATUS_REQUIRE_64BITS) {
		MessageBox(hWnd, L"Please use 64 bits dprobe to open 64 bits report.", 
			       L"D Probe", MB_OK|MB_ICONWARNING);
		return 0;
	}

	else if (Status != MSP_STATUS_OK) {
		StringCchPrintf(Buffer, MAX_PATH, L"Error 0x%08x occurred when open %s",
			            Status, Path);
		MessageBox(hWnd, Buffer, L"D Probe", MB_OK|MB_ICONERROR);
		return 0;
	}

	ThreadHandle = CreateThread(NULL, 0, ReportDialogThread, DtlObject, 0, NULL);
	if (ThreadHandle) {
		CloseHandle(ThreadHandle);
	}

	return 0;
}

LRESULT
FrameOnSize(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	RECT ClientRect;
	PFRAME_OBJECT Object;
	LISTVIEW_OBJECT *ListView;

	Object = (PFRAME_OBJECT)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

	//
	// Adjust toolbar, statusbar automatically
	//

	if (Object->Rebar){
		RebarAdjustPosition(Object->Rebar);
	}
	if (Object->hWndStatusBar){
		SendMessage(Object->hWndStatusBar, WM_SIZE, 0, 0);	
	}
	
	FrameGetViewRect(Object, &ClientRect);

	//
	// Adjust view position after toolbar, statusbar
	//

	if (Object->View) {
		ListView = (PLISTVIEW_OBJECT)Object->View;
		MoveWindow(ListView->hWnd, 
				   ClientRect.left, 
				   ClientRect.top, 
				   ClientRect.right - ClientRect.left, 
				   ClientRect.bottom - ClientRect.top,
				   TRUE);
	}
	return 0;
}

LRESULT
FrameOnEditCondition(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PMSP_DTL_OBJECT DtlObject;
	PCONDITION_OBJECT Current;
	PCONDITION_OBJECT Old;
	PCONDITION_OBJECT New;
	BOOLEAN Updated;

	DtlObject = MspCurrentDtlObject();
	Current = MspGetFilterCondition(DtlObject);

	if (!Current) {
		
		Old = (PCONDITION_OBJECT)SdkMalloc(sizeof(CONDITION_OBJECT));
		RtlZeroMemory(Old, sizeof(*Old));
		InitializeListHead(&Old->ConditionListHead);

		ConditionDialog(hWnd, Old, &New, &Updated);

		if (Updated) {
			MspAttachFilterCondition(DtlObject, New);
		}
		else {
			ASSERT(!New);
		}
		
		ConditionFree(Old);
		return 0;

	}

	ConditionDialog(hWnd, Current, &New, &Updated);

	if (Updated) {
		Old = MspAttachFilterCondition(DtlObject, New);
		ASSERT(Old == Current);
		ConditionFree(Current);
	}

	if (Updated) {
		FrameOnWmUserFilter(hWnd, uMsg, wp, lp);
	}

    return 0;
}

LRESULT
FrameOnResetCondition(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PMSP_DTL_OBJECT DtlObject;
	PMSP_FILTER_FILE_OBJECT FileObject;
	PCONDITION_OBJECT Condition;

	DtlObject = MspCurrentDtlObject();
	FileObject = MspGetFilterFileObject(DtlObject);

	if (!FileObject) {
		return 0;
	}

	Condition = MspDetachFilterCondition(DtlObject);
	ConditionFree(Condition);

	FrameOnWmUserFilter(hWnd, uMsg, wp, lp);
    return 0;
}

LRESULT
FrameOnExportCondition(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	OPENFILENAME Ofn;
	ULONG Status;
	WCHAR Path[MAX_PATH];	
	PWCHAR Extension;
	PMSP_DTL_OBJECT DtlObject;
	PCONDITION_OBJECT Condition;

	DtlObject = MspCurrentDtlObject();
	Condition = MspGetFilterCondition(DtlObject);
	if (!Condition) {
		return 0;
	}

	ZeroMemory(&Ofn, sizeof Ofn);
	ZeroMemory(Path, sizeof(Path));

	Ofn.lStructSize = sizeof(Ofn);
	Ofn.hwndOwner = hWnd;
	Ofn.hInstance = SdkInstance;
	Ofn.lpstrFilter = L"D Probe Filter Condition (*.dfc)\0*.dfc\0\0";
	Ofn.lpstrFile = Path;
	Ofn.nMaxFile = sizeof(Path); 
	Ofn.lpstrTitle = L"D Probe";
	Ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT; 

	GetSaveFileName(&Ofn);

	StringCchCopy(Path, MAX_PATH, Ofn.lpstrFile);
	wcslwr(Path);

	Extension = wcsstr(Path, L".dfc");
	if (!Extension) {
		StringCchCat(Path, MAX_PATH, L".dfc");
	}

	DeleteFile(Path);

	Status = ConditionExportFile(Condition, Path);
	if (Status != S_OK) {
		MessageBox(hWnd, L"Failed to export filter condition file!", L"D Probe", MB_OK|MB_ICONERROR);
	}

	return 0;
}

LRESULT
FrameOnImportCondition(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	ULONG Status;
	WCHAR Path[MAX_PATH];
	OPENFILENAME Ofn;
	PMSP_DTL_OBJECT DtlObject;
	PCONDITION_OBJECT Object;
	PCONDITION_OBJECT New;
	PCONDITION_OBJECT Current;
	BOOLEAN Updated;
	BOOLEAN IsIdentical;
	INT_PTR Return;

	ZeroMemory(&Ofn, sizeof Ofn);
	ZeroMemory(Path, sizeof(Path));

	//
	// Get dfc file name from file dialog
	//

	Ofn.lStructSize = sizeof(Ofn);
	Ofn.hwndOwner = hWnd;
	Ofn.hInstance = SdkInstance;
	Ofn.lpstrFilter = L"D Probe Filter Condition (*.dfc)\0*.dfc\0\0";
	Ofn.lpstrFile = Path;
	Ofn.nMaxFile = sizeof(Path); 
	Ofn.lpstrTitle = L"D Probe";
	Ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	Status = GetOpenFileName(&Ofn);
	if (!Status) {
		return 0;
	}

	StringCchCopy(Path, MAX_PATH, Ofn.lpstrFile);
	Status = ConditionImportFile(Path, &Object);
	if (Status != S_OK) {
		MessageBox(hWnd, L"Failed to read filter condition file!", L"D Probe", MB_OK | MB_ICONERROR);
		return 0;
	}

	Return = ConditionDialog(hWnd, Object, &New, &Updated);
	if (Return != IDOK) {
		ConditionFree(Object);
		return 0;
	}

	DtlObject = MspCurrentDtlObject();
	Current = MspGetFilterCondition(DtlObject);
	IsIdentical = FALSE;

	if (!Current) {

		if (Updated) {
			MspAttachFilterCondition(DtlObject, New);
			ConditionFree(Object);
		}
		else {
			MspAttachFilterCondition(DtlObject, Object);
		}
	}
	else {

		if (Updated) {

			if (ConditionIsIdentical(New, Current) != TRUE) {
				Current = MspAttachFilterCondition(DtlObject, New);
				ConditionFree(Current);
			} else {
				IsIdentical = TRUE;
			}

			ConditionFree(Object);

		}
		else {

			if (ConditionIsIdentical(Object, Current) != TRUE) {
				Current = MspAttachFilterCondition(DtlObject, Object);
				ConditionFree(Current);
			} else {
				ConditionFree(Object);
				IsIdentical = TRUE;
			}
		}
	}

	if (!IsIdentical) {
		FrameOnWmUserFilter(hWnd, WM_USER_FILTER, 0, 0);
	}

	return 0;
}

LRESULT
FrameOnDebugConsole(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	return 0;
}

LRESULT
FrameOnCounter(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PMSP_DTL_OBJECT DtlObject;

	DtlObject = MspCurrentDtlObject();
	CounterDialog(hWnd, DtlObject);
	return 0;
}

LRESULT
FrameOnPerformance(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PMSP_DTL_OBJECT DtlObject;
	PMSP_PROCESS Process;
	PLIST_ENTRY ListEntry;

	DtlObject = MspCurrentDtlObject();

	ListEntry = DtlObject->ProcessListHead.Flink;
	while (ListEntry != &DtlObject->ProcessListHead) {
		Process = CONTAINING_RECORD(ListEntry, MSP_PROCESS, ListEntry);
		if (Process->State == MSP_STATE_STARTED) {
			PerfInsertTask(FALSE, Process->ProcessId, Process->BaseName);
		}
		ListEntry = ListEntry->Flink;
	}

	PerfInsertTask(TRUE, GetCurrentProcessId(), L"dprobe.exe");

	PerfDialog(hWnd);
	PerfDeleteAll();

	return 0;
}

LRESULT
FrameOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	UINT CommandId;

	//
	// Source       HIWORD(wParam)  LOWORD(wParam)  lParam 
	// Menu         0               MenuId          0   
	// Accelerator  1               AcceleratorId	0
	// Control      NotifyCode      ControlId       hWndCtrl
	//

	CommandId = LOWORD(wp);

	switch (CommandId) {

		case IDM_FILE_OPEN:
			FrameOnOpen(hWnd, uMsg, wp, lp);
			break;

		case IDM_EXIT:
			FrameOnExit(hWnd, uMsg, wp, lp);
			break;

		case IDM_FAST_PROBE:
			FrameOnFastProbe(hWnd, uMsg, wp, lp);
			break;

		case IDM_FILTER_PROBE:
			FrameOnFilterProbe(hWnd, uMsg, wp, lp);
			break;

		case IDM_ABOUT:
			AboutDialog(hWnd);
			break;

		case IDM_FILE_IMPORT:
			FrameOnImport(hWnd, uMsg, wp, lp);
			break;
		
		case IDM_FILE_EXPORT:
			FrameOnExport(hWnd, uMsg, wp, lp);
			break;

		case IDM_FILE_SAVE:
			FrameOnSave(hWnd, uMsg, wp, lp);
			break;

		case IDM_STOP_PROBE:
			FrameOnStopProbe(hWnd, uMsg, wp, lp);
			break;

		case IDM_RUN_PROBE:
			FrameOnRunProbe(hWnd, uMsg, wp, lp);
			break;

        case ID_FILTER_EDIT:
            FrameOnEditCondition(hWnd, uMsg, wp, lp);
            break;

		case ID_FILTER_RESET:
            FrameOnResetCondition(hWnd, uMsg, wp, lp);
            break;

		case ID_FILTER_IMPORT:
            FrameOnImportCondition(hWnd, uMsg, wp, lp);
            break;

		case ID_FILTER_EXPORT:
            FrameOnExportCondition(hWnd, uMsg, wp, lp);
            break;
			
		case IDM_COUNTER:
			FrameOnCounter(hWnd, uMsg, wp, lp);
			break;
		
		case IDM_OPTION:
			FrameOnOption(hWnd, uMsg, wp, lp);
			break;

		case IDM_PERFORMANCE:
			FrameOnPerformance(hWnd, uMsg, wp, lp);
			break;

		case IDM_DEBUGCONSOLE:
			FrameOnDebugConsole(hWnd, uMsg, wp, lp);
			break;

		case IDM_FINDFORWARD:
			FrameOnFindForward(hWnd, uMsg, wp, lp);
			break;

		case IDM_FINDBACKWARD:
			FrameOnFindBackward(hWnd, uMsg, wp, lp);
			break;

		case IDM_AUTOSCROLL:
			FrameOnAutoScroll(hWnd, uMsg, wp, lp);
			break;

		case IDM_CALLSTACK:
			FrameOnCallStack(hWnd, uMsg, wp, lp);
			break;

		case IDM_CALLRECORD:
			FrameOnCallRecord(hWnd, uMsg, wp, lp);
			break;
		
		case IDM_COMMANDLINEOPTION:
			FrameOnCommandOption(hWnd, uMsg, wp, lp);
			break;

		case IDM_LOG:
			FrameOnLog(hWnd, uMsg, wp, lp);
			break;
		
		case IDM_DUMP:
			FrameOnDump(hWnd, uMsg, wp, lp);
			break;
			
		case IDM_COPY:
			FrameOnCopy(hWnd, uMsg, wp, lp);
			break;

		case ID_INCLUDE_PROCESS:
		case ID_INCLUDE_PID:
		case ID_INCLUDE_TID:
		case ID_INCLUDE_PROBE:
		case ID_INCLUDE_PROVIDER:
		case ID_INCLUDE_RETURN:
		case ID_INCLUDE_DETAIL:
		case ID_EXCLUDE_PROCESS:
		case ID_EXCLUDE_PID:
		case ID_EXCLUDE_TID:
		case ID_EXCLUDE_PROBE:
		case ID_EXCLUDE_PROVIDER:
		case ID_EXCLUDE_RETURN:
		case ID_EXCLUDE_DETAIL:

			//
			// Filter commands via context menu, it's typically a kind of incremental,
			// fast filter operation.
			//

			FrameOnFilterCommand(hWnd, CommandId);
			break;

			//
			// Kernel mode test routine
			//

#ifdef _DEBUG 
		case ID_TEST_KERNELMODE:
			FrameOnTestKernelMode(hWnd, uMsg, wp, lp);
			break;
#endif

	}

	//
	// Command of probing processes
	//

	if (CommandId >= IDM_PROBING_FIRST && CommandId <= IDM_PROBING_LAST) {
		FrameOnProbingCommand(hWnd, CommandId);
	}

	return 0;
}

LRESULT
FrameOnProbingCommand(
	IN HWND hWnd,
	UINT CommandId
	)
{
	PFRAME_MENU_ITEM Item;

	Item = FrameFindMenuItemByCommand(CommandId);
	ASSERT(Item != NULL);
	
	ProbingDialogCreate(hWnd, Item->ProcessId);
	return 0L;
}

LRESULT
FrameOnFilterCommand(
	IN HWND hWnd,
	UINT CommandId
	)
{
	ULONG Status;
	PMSP_DTL_OBJECT DtlObject;
	PMSP_FILTER_FILE_OBJECT FileObject;
	PCONDITION_OBJECT Condition;
	PBTR_RECORD_HEADER Record;
	WCHAR Value[MAX_PATH];
	PCONDITION_ENTRY Entry;
	int index;

	//
	// Get current user selected record
	//

	index = LogViewGetFirstSelected();
	if (index == -1) {
		return 0;
	}
	
	DtlObject = MspCurrentDtlObject();
	FileObject = MspGetFilterFileObject(DtlObject);

	Record = (PBTR_RECORD_HEADER)_alloca(BspPageSize);

	if (FileObject != NULL) {
		Status = MspCopyRecordFiltered(FileObject, index, Record, BspPageSize);
	} else {
		Status = MspCopyRecord(DtlObject, index, Record, BspPageSize);
	}

	if (Status != MSP_STATUS_OK) {
		return 0;
	}

	//
	// Build new condition by user selected command id
	// Include commands, map to EQUAL
	// Exclude commands, map to NOT EQUAL
	//
	//

	Entry = (PCONDITION_ENTRY)SdkMalloc(sizeof(CONDITION_ENTRY));
	
	switch (CommandId) {

		case ID_INCLUDE_PROCESS:
		case ID_EXCLUDE_PROCESS:

			Entry->Object = ConditionObjectProcess;
			Entry->Relation = ConditionRelationEqual;

			MspDecodeRecord(DtlObject, Record, DECODE_PROCESS, Value, MAX_PATH);
			StringCchCopy(Entry->StringValue, MAX_PATH, Value);

			if (CommandId == ID_INCLUDE_PROCESS) {
				Entry->Action = ConditionActionInclude;
			} else {
				Entry->Action = ConditionActionExclude;
			}
			break;

		case ID_INCLUDE_PID:
		case ID_EXCLUDE_PID:

			Entry->Object = ConditionObjectPID;
			Entry->Relation = ConditionRelationEqual;

			MspDecodeRecord(DtlObject, Record, DECODE_PID, Value, MAX_PATH);
			StringCchCopy(Entry->StringValue, MAX_PATH, Value);

			if (CommandId == ID_INCLUDE_PID) {
				Entry->Action = ConditionActionInclude;
			} else {
				Entry->Action = ConditionActionExclude;
			}
			break;

		case ID_INCLUDE_TID:
		case ID_EXCLUDE_TID:

			Entry->Object = ConditionObjectTID;
			Entry->Relation = ConditionRelationEqual;

			MspDecodeRecord(DtlObject, Record, DECODE_TID, Value, MAX_PATH);
			StringCchCopy(Entry->StringValue, MAX_PATH, Value);

			if (CommandId == ID_INCLUDE_TID) {
				Entry->Action = ConditionActionInclude;
			} else {
				Entry->Action = ConditionActionExclude;
			}
			break;
			
		case ID_INCLUDE_PROBE:
		case ID_EXCLUDE_PROBE:

			Entry->Object = ConditionObjectProbe;
			Entry->Relation = ConditionRelationEqual;

			MspDecodeProbe(DtlObject, Record, Value, MAX_PATH, FALSE);
			StringCchCopy(Entry->StringValue, MAX_PATH, Value);

			if (CommandId == ID_INCLUDE_PROBE) {
				Entry->Action = ConditionActionInclude;
			} else {
				Entry->Action = ConditionActionExclude;
			}
			break;
			
		case ID_INCLUDE_PROVIDER:
		case ID_EXCLUDE_PROVIDER:

			Entry->Object = ConditionObjectProvider;
			Entry->Relation = ConditionRelationEqual;

			MspDecodeRecord(DtlObject, Record, DECODE_PROVIDER, Value, MAX_PATH);
			StringCchCopy(Entry->StringValue, MAX_PATH, Value);

			if (CommandId == ID_INCLUDE_PROVIDER) {
				Entry->Action = ConditionActionInclude;
			} else {
				Entry->Action = ConditionActionExclude;
			}
			break;
			
		case ID_INCLUDE_RETURN:
		case ID_EXCLUDE_RETURN:
			
			Entry->Object = ConditionObjectReturn;
			Entry->Relation = ConditionRelationEqual;

			MspDecodeRecord(DtlObject, Record, DECODE_RETURN, Value, MAX_PATH);
			StringCchCopy(Entry->StringValue, MAX_PATH, Value);

			if (CommandId == ID_INCLUDE_RETURN) {
				Entry->Action = ConditionActionInclude;
			} else {
				Entry->Action = ConditionActionExclude;
			}
			break;

		case ID_INCLUDE_DETAIL:
		case ID_EXCLUDE_DETAIL:

			Entry->Object = ConditionObjectDetail;
			Entry->Relation = ConditionRelationContain;

			MspDecodeRecord(DtlObject, Record, DECODE_DETAIL, Value, MAX_PATH);
			StringCchCopy(Entry->StringValue, MAX_PATH, Value);
			
			if (CommandId == ID_INCLUDE_DETAIL) {
				Entry->Action = ConditionActionInclude;
			} else {
				Entry->Action = ConditionActionExclude;
			}
			break;

		default:
			ASSERT(0);
	}

	if (FileObject != NULL) {

		Condition = MspDetachFilterCondition(DtlObject);
		InsertHeadList(&Condition->ConditionListHead, &Entry->ListEntry);
		Condition->NumberOfConditions += 1;
		Condition->DtlObject = NULL;

		MspAttachFilterCondition(DtlObject, Condition);

	} else {

		Condition = (PCONDITION_OBJECT)SdkMalloc(sizeof(CONDITION_OBJECT));
		InitializeListHead(&Condition->ConditionListHead);
		Condition->NumberOfConditions = 1;
		Condition->DtlObject = NULL;
		InsertHeadList(&Condition->ConditionListHead, &Entry->ListEntry);

		MspAttachFilterCondition(DtlObject, Condition);
	}

	FrameOnWmUserFilter(hWnd, WM_USER_FILTER, 0, 0);
    return 0;
}

VOID
FrameRetrieveFindString(
	IN PFRAME_OBJECT Object,
	OUT PWCHAR Buffer,
	IN ULONG Length
	)
{
	PREBAR_OBJECT Rebar;

	Rebar = Object->Rebar;
	RebarCurrentEditString(Rebar, Buffer, Length);
	return;
}

LRESULT
FrameOnFindForward(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PFRAME_OBJECT Object;
    FIND_CALLBACK FindCallback;
	PLISTVIEW_OBJECT ListView;
	LONG Current;

	Object = (PFRAME_OBJECT)SdkGetObject(hWnd);
	FindCallback = Object->FindContext.FindCallback;
	
	if (!FindCallback || !Object->FindContext.DtlObject) {
		return 0L;
	}

	ListView = (PLISTVIEW_OBJECT)Object->View;
	Current = ListViewGetFirstSelected(ListView->hWnd);

	Object->FindContext.PreviousIndex = Current;
	FrameRetrieveFindString(Object, Object->FindContext.FindString, MAX_PATH);

	__try {
		Object->FindContext.FindForward = TRUE;
		(*FindCallback)(&Object->FindContext);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
	}

	return 0L;
}

LRESULT
FrameOnFindBackward(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PFRAME_OBJECT Object;
    FIND_CALLBACK FindCallback;
	PLISTVIEW_OBJECT ListView;
	LONG Current;

	Object = (PFRAME_OBJECT)SdkGetObject(hWnd);
	FindCallback = Object->FindContext.FindCallback;

	if (!FindCallback || !Object->FindContext.DtlObject) {
		return 0L;
	}

	ListView = (PLISTVIEW_OBJECT)Object->View;
	Current = ListViewGetFirstSelected(ListView->hWnd);

	Object->FindContext.PreviousIndex = Current;
	FrameRetrieveFindString(Object, Object->FindContext.FindString, MAX_PATH);

	__try {
		Object->FindContext.FindForward = FALSE;
		(*FindCallback)(&Object->FindContext);
	}__except(EXCEPTION_EXECUTE_HANDLER) {
		// do nothing
	}

	return 0L;
}

LRESULT
FrameOnAutoScroll(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PFRAME_OBJECT Object;
	HMENU hMenu;
	UINT Flag;
	BOOLEAN AutoScroll;
	WCHAR Value[10];

	AutoScroll = LogViewGetAutoScroll();
	AutoScroll = !AutoScroll;

	LogViewSetAutoScroll(AutoScroll);
	_itow(AutoScroll, Value, 10);

	MdbSetData(MdbAutoScroll, Value);

	Object = (PFRAME_OBJECT)SdkGetObject(hWnd);
	hMenu = GetMenu(hWnd);
	Flag = MF_BYCOMMAND;

	if (AutoScroll) 
		Flag |= MF_CHECKED;
	else 
		Flag |= MF_UNCHECKED;

	CheckMenuItem(hMenu, IDM_AUTOSCROLL, Flag);

	return 0;
}

LRESULT
FrameOnOption(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	OptionDialogCreate(hWnd);
	return 0;
}

LRESULT
FrameOnImport(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	BOOL Status;
	PBSP_PROCESS Process;
	LIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	ULONG Count;
	PMSP_USER_COMMAND Command;
	INT_PTR Return;
	OPENFILENAME Ofn;
	WCHAR Path[MAX_PATH];	

	ZeroMemory(&Ofn, sizeof Ofn);
	ZeroMemory(Path, sizeof(Path));

	Ofn.lStructSize = sizeof(Ofn);
	Ofn.hwndOwner = hWnd;
	Ofn.hInstance = SdkInstance;
	Ofn.lpstrFilter = L"D Probe Configuration (*.dpc)\0*.dpc\0\0";
	Ofn.lpstrFile = Path;
	Ofn.nMaxFile = sizeof(Path); 
	Ofn.lpstrTitle = L"D Probe";
	Ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	Status = GetOpenFileName(&Ofn);
	if (Status == FALSE) {
		return 0;
	}

	StringCchCopy(Path, MAX_PATH, Ofn.lpstrFile);

	Count = 0;
	InitializeListHead(&ListHead);

	Return = ImportDialog(hWnd, Path, &ListHead, &Count, &Command);
	if (Return == IDCANCEL) {
		return 0;
	}

	if (!Command) { 
		return 0;
	}

	if (IsListEmpty(&ListHead)) {
		ImportFreeCommand(Command);
		return 0;
	}

	while (IsListEmpty(&ListHead) != TRUE) {

		ListEntry = RemoveHeadList(&ListHead);
		Process = CONTAINING_RECORD(ListEntry, BSP_PROCESS, ListEntry);

		//
		// N.B. We reuse the command object, ensure all necessary
		// fields are cleared. btr runtime guarantee to assign new
		// value to BTR_USER_COMMAND fields.
		//

		Command->ProcessId = Process->ProcessId;
		Command->Status = 0;

		ApplyDialog(hWnd, Process, Command);

		if (Command->FailureCount != 0) {
			ResultDialog(hWnd, Process, Command);
			MspFreeFailureList(Command);
		}

		InitializeListHead(&Command->FailureList);
		Command->FailureCount = 0;

		LogViewAddImageList(Process->FullPath, Process->ProcessId);
	}

	return 0;
}

LRESULT
FrameOnExport(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	return 0;
}

LRESULT
FrameOnNotify(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LPNMHDR lpNmhdr;

	lpNmhdr = (LPNMHDR)lp;

	if (lpNmhdr->idFrom == FrameChildViewId) {
		return LogViewOnNotify(hWnd, uMsg, wp, lp);
	}

	switch (lpNmhdr->code) {
		case TTN_GETDISPINFO:
			FrameOnTtnGetDispInfo(hWnd, uMsg, wp, lp);
	}

	return 0;
}

LRESULT
FrameOnTtnGetDispInfo(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LPNMTTDISPINFO lpnmtdi = (LPNMTTDISPINFO)lp;
	lpnmtdi->uFlags = TTF_DI_SETITEM;

	switch (lpnmtdi->hdr.idFrom) {
		case IDM_FINDBACKWARD:
			StringCchCopy(lpnmtdi->szText, 80, L"Find Backward");
			break;
		case IDM_FINDFORWARD:
			StringCchCopy(lpnmtdi->szText, 80, L"Find Forward");
			break;
		case IDM_AUTOSCROLL:
			StringCchCopy(lpnmtdi->szText, 80, L"Auto Scroll");
			break;
	}

	return 0L;
}

LRESULT
FrameOnCreate(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	CREATESTRUCT *lpcs;
	PFRAME_OBJECT Frame;
	PLISTVIEW_OBJECT LogView;
	WCHAR Buffer[MAX_PATH];
	PFRAME_DTL_DATA Context;
	UINT Flag;
	HMENU hMenu;



	lpcs = (CREATESTRUCT *)lp;
	Frame = (PFRAME_OBJECT)lpcs->lpCreateParams;

	LoadString(SdkInstance, IDS_APP_TITLE, Buffer, MAX_PATH);
	SetWindowText(hWnd, Buffer);

	SdkSetObject(hWnd, Frame);
	Frame->hWnd = hWnd;

	//
	// Register WM_USER_STATUS message
	//

	WM_USER_STATUS = RegisterWindowMessage(WmUserStatus);
	ASSERT(WM_USER_STATUS != 0);

	WM_USER_FILTER = RegisterWindowMessage(WmUserFilter);
	ASSERT(WM_USER_FILTER != 0);

	//
	// Load rebar and statusbar
	//

	FrameLoadBars(Frame);
	
	//
	// Create left tree view
	//

	//FrameCreateTree(Frame, FrameTreeViewId);

	LogView = LogViewCreate(hWnd, FrameChildViewId);
	ASSERT(LogView != NULL);

	Frame->View = LogView;

	//
	// Register shared data callback routine to update status bar
	//

	Context = (PFRAME_DTL_DATA)SdkMalloc(sizeof(FRAME_DTL_DATA));
	Context->hWndFrame = hWnd;

	//
	// Setup find context data and bind trace view callback
	//

	Frame->FindContext.hWndList = LogView->hWnd;
	Frame->FindContext.DtlObject = MspCurrentDtlObject();
	Frame->FindContext.FindForward = TRUE;
	Frame->FindContext.FindCallback = LogViewFindCallback;

	//
	// Initialize probing task menu map
	//

	Frame->ProbingMenuMap = FrameProbingMap;
	FrameInitializeProbingMap();
	
	//
	// Check autoscroll of logview
	//

	hMenu = GetMenu(hWnd);
	Flag = MF_BYCOMMAND;

	if (LogViewGetAutoScroll()) { 
		Flag |= MF_CHECKED;
	}
	else { 
		Flag |= MF_UNCHECKED;
	}

	CheckMenuItem(hMenu, IDM_AUTOSCROLL, Flag);

	SdkCenterWindow(hWnd);
	return 0;
}

LRESULT
FrameOnClose(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	MdbClose();
	MspUninitialize();
	return 0;
}

LRESULT CALLBACK
FrameProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (uMsg) {

		case WM_NOTIFY:
		    return FrameOnNotify(hWnd, uMsg, wp, lp);

		case WM_CREATE:
			FrameOnCreate(hWnd, uMsg, wp, lp);
			break;

		case WM_SIZE:
			FrameOnSize(hWnd, uMsg, wp, lp);
			break;

		case WM_CLOSE:
			FrameOnClose(hWnd, uMsg, wp, lp);
			PostQuitMessage(0);
			break;

		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			break;

		case WM_COMMAND:
			return FrameOnCommand(hWnd, uMsg, wp, lp);

		case WM_CONTEXTMENU:
			FrameOnContextMenu(hWnd, uMsg, wp, lp);
			break;

        default:
			
			if (uMsg == WM_USER_STATUS) {
				return FrameOnWmUserStatus(hWnd, uMsg, wp, lp);
			}

			if (uMsg == WM_USER_FILTER) {
				return FrameOnWmUserFilter(hWnd, uMsg, wp, lp);
			}
	}

	return DefWindowProc(hWnd, uMsg, wp, lp);
}

LRESULT
FrameOnStopProbe(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PBSP_PROCESS Process;
	PMSP_USER_COMMAND Command;
	ULONG Count;
	INT_PTR Return;

	if (MspIsIdle()) {
		MessageBox(hWnd, L"No active probed process!", L"D Probe", MB_OK);
		return 0;
	}

	Return = StopDialog(hWnd, &ListHead, &Count);
	if (Return == IDCANCEL || !Count) {
		return 0;
	}

	Command = (PMSP_USER_COMMAND)SdkMalloc(sizeof(MSP_USER_COMMAND));
	RtlZeroMemory(Command, sizeof(MSP_USER_COMMAND));
	Command->CommandType = CommandStop;

	while (IsListEmpty(&ListHead) != TRUE) {
	
		ListEntry = RemoveHeadList(&ListHead);
		Process = CONTAINING_RECORD(ListEntry, BSP_PROCESS, ListEntry);

		Command->ProcessId = Process->ProcessId;
		ApplyDialog(hWnd, Process, Command);

		BspFreeProcess(Process);
	}

	SdkFree(Command);
	return 0;
}

LRESULT
FrameOnRunProbe(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	ULONG Status;
	INT_PTR Return;
	CSP_FILE_HEADER Head;
	WCHAR ImagePath[MAX_PATH];
	WCHAR Argument[MAX_PATH];
	WCHAR WorkPath[MAX_PATH];
	WCHAR DpcPath[MAX_PATH];
	WCHAR Buffer[MAX_PATH];
	PMSP_USER_COMMAND ProbeCommand;
	PMSP_USER_COMMAND StartCommand;
	PBSP_PROCESS Process;
	BOOLEAN Managed;
	ULONG Type;

	Return = RunDialog(hWnd, ImagePath, Argument, WorkPath, DpcPath);
	if (Return != IDOK || !wcslen(ImagePath) || !wcslen(DpcPath)) {
		return 0;
	}

	Status = MspIsManagedModule(ImagePath, &Managed);
	if (Managed) {
		MessageBox(hWnd, L"Run don't support managed process!", L"D Probe", MB_OK|MB_ICONWARNING);
		return 0;
	}

	Status = MspGetImageType(ImagePath, &Type);
	if (Status != MSP_STATUS_OK) {
		MessageBox(hWnd, L"Invalid executable file!", L"D Probe", MB_OK|MB_ICONERROR);
		return 0;
	}

	if (BspIs64Bits && Type != IMAGE_FILE_MACHINE_AMD64) {
		MessageBox(hWnd, L"64 bits dprobe does not support 32 bits executable file!", L"D Probe", MB_OK|MB_ICONWARNING);
		return 0;
	}

	if (!BspIs64Bits && Type != IMAGE_FILE_MACHINE_I386) {
		MessageBox(hWnd, L"32 bits dprobe does not support 64 bits executable file!", L"D Probe", MB_OK|MB_ICONWARNING);
		return 0;
	}

	Status = ImportReadFileHead(DpcPath, &Head);
	if (Status != ERROR_SUCCESS) {
		StringCchPrintf(Buffer, MAX_PATH, L"Failed to open %s!", DpcPath);
		MessageBox(hWnd, Buffer, L"D Probe", MB_OK|MB_ICONERROR);
		return 0;
	}

	if (Head.Type == PROBE_FAST) {
		Status = FastImportProbe(DpcPath, &ProbeCommand);
	}
	
	if (Head.Type == PROBE_FILTER) {
		Status = FilterImportProbe(DpcPath, &ProbeCommand);
	}

	if (Status != ERROR_SUCCESS) {
		return 0;
	}

	StartCommand = (PMSP_USER_COMMAND)SdkMalloc(sizeof(MSP_USER_COMMAND));
	RtlZeroMemory(StartCommand, sizeof(MSP_USER_COMMAND));

	StartCommand->CommandType = CommandStartDebug;

	if (wcslen(ImagePath)) {
		StringCchCopy(StartCommand->ImagePath, MAX_PATH, ImagePath);
	} else {
		StartCommand->ImagePath[0] = 0;
	}

	if (wcslen(Argument)) {
		StringCchCopy(StartCommand->Argument, MAX_PATH, Argument);
	} else {
		StartCommand->Argument[0] = 0;
	}

	if (wcslen(WorkPath)) {
		StringCchCopy(StartCommand->WorkPath, MAX_PATH, WorkPath);
	} else {
		StartCommand->WorkPath[0] = 0;
	}

	//
	// Make a dummy process object, since currently the process is not
	// yet created.
	//

	Process = (PBSP_PROCESS)SdkMalloc(sizeof(BSP_PROCESS));
	_wsplitpath(ImagePath, NULL, NULL, Buffer, NULL);
	Process->Name = (PWSTR)SdkMalloc(MAX_PATH * 2);
	StringCchCopy(Process->Name, MAX_PATH, Buffer);
	Process->FullPath = (PWSTR)ImagePath;

	//
	// First issue the start debug command to launch the process under
	// control of MSP.
	//

	ApplyDialog(hWnd, Process, StartCommand);

	if (StartCommand->Status != MSP_STATUS_OK) {
		StringCchPrintf(Buffer, MAX_PATH, L"Failed to launch %s, LastError=0x%08x", 
						Process->Name, StartCommand->Status);
		MessageBox(hWnd, Buffer, L"D Probe", MB_OK|MB_ICONERROR); 
		return 0;
	}

	//
	// Then issue the probe command
	//

	ProbeCommand->ProcessId = StartCommand->ProcessId;
	Process->ProcessId = StartCommand->ProcessId;

	ApplyDialog(hWnd, Process, ProbeCommand);

	if (ProbeCommand->FailureCount != 0) {
		ResultDialog(hWnd, Process, ProbeCommand);
		MspFreeFailureList(ProbeCommand);
	}

	if (Head.Type == PROBE_FAST) {
		FastFreeCommand(ProbeCommand);
	}

	if (Head.Type == PROBE_FILTER) {
		FilterFreeCommand(ProbeCommand);
	}

	if (StartCommand->CompleteEvent != NULL) {
		SetEvent(StartCommand->CompleteEvent);
		CloseHandle(StartCommand->CompleteEvent);
	}
	
	LogViewAddImageList(Process->FullPath, Process->ProcessId);

	Process->FullPath = NULL;
	SdkFree(StartCommand);
	SdkFree(Process->Name);
	SdkFree(Process);
	return 0;
}

LRESULT
FrameOnSave(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	OPENFILENAME Ofn;
	WCHAR Path[MAX_PATH];	
	MSP_SAVE_TYPE Type;
	PMSP_DTL_OBJECT DtlObject;
	PMSP_FILTER_FILE_OBJECT FileObject;
	ULONG Count;

	DtlObject = MspCurrentDtlObject();
	FileObject = MspGetFilterFileObject(DtlObject);

	if (FileObject != NULL) {
		Count = MspGetFilteredRecordCount(FileObject);
	} else {
		Count = MspGetRecordCount(DtlObject);
	}

	if (!Count) {
		MessageBox(hWnd, L"There's no records to save.", L"D Probe", MB_OK|MB_ICONWARNING);
		return 0;
	}

	ZeroMemory(&Ofn, sizeof Ofn);
	ZeroMemory(Path, sizeof(Path));

	Ofn.lStructSize = sizeof(Ofn);
	Ofn.hwndOwner = hWnd;
	Ofn.hInstance = SdkInstance;
	Ofn.lpstrFilter = L"D Probe Trace Log (*.dtl)\0*.dtl\0Comma Separated Value (*.csv)\0\0";
	Ofn.lpstrFile = Path;
	Ofn.nMaxFile = sizeof(Path); 
	Ofn.lpstrTitle = L"D Probe";
	Ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT; 

	if (!GetSaveFileName(&Ofn)) {
		return 0;
	}

	StringCchCopy(Path, MAX_PATH, Ofn.lpstrFile);
	wcslwr(Path);

	if (Ofn.nFilterIndex == 1) {
		if (!wcsstr(Path, L".dtl")) {
			StringCchCat(Path, MAX_PATH, L".dtl");
		}
		Type = MSP_SAVE_DTL;
	} 
	
	if (Ofn.nFilterIndex == 2) {
		if (!wcsstr(Path, L".csv")) {
			StringCchCat(Path, MAX_PATH, L".csv");
		}
		Type = MSP_SAVE_CSV;
	}
	
	SaveDialog(hWnd, DtlObject, Path, Type);
	return 0;
}

BOOLEAN
FrameLoadBars(
	IN PFRAME_OBJECT Object
	)
{
	HWND hWndFrame;
	PREBAR_OBJECT Rebar;
	HWND hWndStatusBar;
	INT StatusParts[2];

	hWndFrame = Object->hWnd;
	ASSERT(hWndFrame != NULL);

	//
	// Create rebar object and its children
	//

	Rebar = RebarCreateObject(hWndFrame, FrameRebarId);
	if (!Rebar) {
		return FALSE;
	}

	RebarInsertBands(Rebar);
	Object->Rebar = Rebar;

	//
	// Create statusbar object
	//

	hWndStatusBar = CreateWindowEx(0, STATUSCLASSNAME, NULL, 
								   WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
								   SBARS_SIZEGRIP | CCS_BOTTOM, 0, 0, 0, 0, hWndFrame, 
								   UlongToHandle(FrameStatusBarId), SdkInstance, NULL);

	ASSERT(hWndStatusBar != NULL);

	if (!hWndStatusBar) {
		return FALSE;
	}

	Object->hWndStatusBar = hWndStatusBar;

	//
	// Create 2 status parts
	//

	StatusParts[0] = 360;
	StatusParts[1] = -1;

	SendMessage(hWndStatusBar, SB_SETPARTS, 2, (LPARAM)&StatusParts);
	SendMessage(hWndStatusBar, SB_SETTEXT,  SBT_POPOUT | 0, (LPARAM)L"Ready");
	SendMessage(hWndStatusBar, SB_SETTEXT,  SBT_POPOUT | 1, (LPARAM)L"");
	ShowWindow(hWndStatusBar, SW_SHOW);
	return TRUE;
}

LRESULT
FrameOnWmUserStatus(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	ULARGE_INTEGER FreeBytes;
	WCHAR Drive[16];
	WCHAR Buffer[MAX_PATH];
	WCHAR Path[MAX_PATH];
	PFRAME_OBJECT Object;
	PMSP_DTL_OBJECT DtlObject;
	PMSP_FILTER_FILE_OBJECT FileObject;
	ULONG FilteredCount;
	ULONG Count;
	double Percent;

	DtlObject = MspCurrentDtlObject();
	FileObject = MspGetFilterFileObject(DtlObject);

	Count = MspGetRecordCount(DtlObject);

	if (FileObject) {
	
		FilteredCount = MspGetFilteredRecordCount(FileObject);
		Percent = (FilteredCount * 100.0) / (Count * 1.0);
	
		StringCchPrintf(Buffer, MAX_PATH, L"Showing filtered %u of total %u records (%%%.2f)",
			            FilteredCount, Count, Percent);
	} else {
		StringCchPrintf(Buffer, MAX_PATH, L"Showing %u of total %u records (%%%u)",
			            Count, Count, 100);
	}

	Object = (PFRAME_OBJECT)SdkGetObject(hWnd);
	SendMessage(Object->hWndStatusBar, SB_SETTEXT, SBT_POPOUT | 0, (LPARAM)Buffer);

	//
	// Set cache information
	//

	MspGetCachePath(Path, MAX_PATH);

	_wsplitpath((const WCHAR *)Path, Drive, NULL, NULL, NULL);
	_wcsupr(Drive);
	Path[0] = Drive[0];

	GetDiskFreeSpaceEx(Drive, &FreeBytes, NULL, NULL);
	StringCchPrintf(Buffer, MAX_PATH, L"Caching in %ws, %ws has %d MB free space",
					Path, Drive, FreeBytes.QuadPart / (1024 * 1024));

	SendMessage(Object->hWndStatusBar, SB_SETTEXT, SBT_POPOUT | 1, (LPARAM)Buffer);
	return 0;
}

LRESULT
FrameOnCallStack(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PFRAME_OBJECT Object;
	PLISTVIEW_OBJECT ListView;
	PMSP_DTL_OBJECT DtlObject;

	Object = (PFRAME_OBJECT)SdkGetObject(hWnd);
	ListView = (PLISTVIEW_OBJECT)Object->View;

	DtlObject = MspCurrentDtlObject(), 
	MspSetStackTraceBarrier();

	DetailDialog(hWnd, ListView->hWnd, 
				 DtlObject,
		         DetailPageStack);

	MspClearStackTraceBarrier();
	MspClearDbghelp();
	return 0;
}

LRESULT
FrameOnLog(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	CHAR Buffer[MAX_PATH];

	MspGetLogFilePath(Buffer, MAX_PATH);
	ShellExecuteA(hWnd, "open", Buffer, NULL, NULL, SW_SHOWNORMAL);
	return 0;
}

LRESULT
FrameOnCopy(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LogViewOnCopy(hWnd, uMsg, wp, lp);
	return 0;
}

LRESULT
FrameOnDump(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	DumpDialog(hWnd);
	return 0;
}

LRESULT
FrameOnCallRecord(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PFRAME_OBJECT Object;
	PLISTVIEW_OBJECT ListView;
	PMSP_DTL_OBJECT DtlObject;

	Object = (PFRAME_OBJECT)SdkGetObject(hWnd);
	ListView = (PLISTVIEW_OBJECT)Object->View;
	
	DtlObject = MspCurrentDtlObject();
	MspSetStackTraceBarrier();

	DetailDialog(hWnd, ListView->hWnd, 
		         DtlObject, 
				 DetailPageRecord);

	MspClearStackTraceBarrier();
	MspClearDbghelp();
	return 0;
}

LRESULT
FrameOnCommandOption(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	return 0;
}

LRESULT
FrameOnWmUserFilter(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PFRAME_OBJECT Object;
	PLISTVIEW_OBJECT ListView;

	Object = (PFRAME_OBJECT)SdkGetObject(hWnd);
	ListView = (PLISTVIEW_OBJECT)Object->View;

	LogViewOnWmUserFilter(ListView->hWnd, WM_USER_FILTER, 0, 0);
	return 0;
}

VOID
FrameInitializeProbingMap(
	VOID
	)
{
	ULONG Number;

	for(Number = 0; Number < 64; Number += 1) {
		FrameProbingMap[Number].CommandId = IDM_PROBING_FIRST + Number;
		FrameProbingMap[Number].ProcessId = 0;
		FrameProbingMap[Number].ProcessName[0] = 0; 
	}

	return;
}

PFRAME_MENU_ITEM
FrameFindMenuItemById(
	IN ULONG ProcessId
	)
{
	ULONG Number;
	PFRAME_MENU_ITEM FirstFreeSlot = NULL;

	for(Number = 0; Number < 64; Number += 1) {
		if (FrameProbingMap[Number].ProcessId == ProcessId) {
			return &FrameProbingMap[Number];
		}

		if (!FirstFreeSlot) {
			if (FrameProbingMap[Number].ProcessId == 0) {
				FirstFreeSlot = &FrameProbingMap[Number];
			}
		}
	}

	return FirstFreeSlot; 
} 

PFRAME_MENU_ITEM
FrameFindMenuItemByCommand(
	IN UINT CommandId
	)
{
	ASSERT(CommandId >= IDM_PROBING_FIRST && CommandId <= IDM_PROBING_LAST);
	return &FrameProbingMap[CommandId - IDM_PROBING_FIRST];
}

VOID
FrameInsertProbingMenu(
	IN PBSP_PROCESS Process
	)
{
	HMENU hMainMenu;
	HMENU hSubMenu;
	PFRAME_MENU_ITEM Item;
	WCHAR Buffer[MAX_PATH];
	int Count;

	hMainMenu = GetMenu(hWndMain);
	hSubMenu = GetSubMenu(hMainMenu, 2);

	Count = GetMenuItemCount(hSubMenu);
	if (Count == 5) {

		//
		// This is first time to insert probing task, dynamically insert a separator
		//

		AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);
		DrawMenuBar(hWndMain);
	}

	Item = FrameFindMenuItemById(Process->ProcessId);
	if (!Item) {

		//
		// N.B. The ID map is full, just do nothing here, this is a rare case,
		// most user do not probe up to 64 processes simultaneously
		//

		return;
	}

	if (Item->ProcessId == Process->ProcessId) {

		//
		// Already exist, do nothing here
		//

		return;
	}

	//
	// This is a new probing process, FrameFindMenuItemById return an empty slot
	//

	ASSERT(Item->ProcessId == 0);

	Item->ProcessId = Process->ProcessId;
	StringCchCopy(Item->ProcessName, MAX_PATH, Process->Name);
	StringCchPrintf(Buffer, MAX_PATH, L"%s (%u)", Process->Name, Process->ProcessId);	

	AppendMenu(hSubMenu, MF_ENABLED | MF_STRING, Item->CommandId, Buffer);
	DrawMenuBar(hWndMain);
}

VOID
FrameDeleteProbingMenu(
	IN ULONG ProcessId
	)
{
	HMENU hMainMenu;
	HMENU hSubMenu;
	UINT CommandId;
	int Count;
	int i;

	hMainMenu = GetMenu(hWndMain);
	hSubMenu = GetSubMenu(hMainMenu, 2);

	Count = GetMenuItemCount(hSubMenu);
	for (i = 6; i < Count; i++) {

		//
		// Delete the died process menu by its command id
		//

		CommandId = GetMenuItemID(hSubMenu, i);

		if (FrameProbingMap[CommandId - IDM_PROBING_FIRST].ProcessId == ProcessId) {
			FrameProbingMap[CommandId - IDM_PROBING_FIRST].ProcessId = 0;
			FrameProbingMap[CommandId - IDM_PROBING_FIRST].ProcessName[0] = 0;
			DeleteMenu(hSubMenu, CommandId, MF_BYCOMMAND);
			break;
		}
	}	

	//
	// When there's no probing task, delete the dynamically inserted separator
	//

	Count = GetMenuItemCount(hSubMenu);
	if (Count == 6) {
		DeleteMenu(hSubMenu, 5, MF_BYPOSITION);
	}

	return;
}

ULONG
FrameCreateTree(
	IN PFRAME_OBJECT Object,
	IN UINT TreeId
	)
{
	RECT Rect; 
    HWND hWndTree;
	HIMAGELIST himl; 
	TVITEM tvi; 
    TVINSERTSTRUCT tvins; 
    static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST; 
    static HTREEITEM hPrevRootItem = NULL; 
    static HTREEITEM hPrevLev2Item = NULL; 
    HTREEITEM hti; 
	int nLevel = 1;

    GetClientRect(Object->hWnd, &Rect); 
    hWndTree = CreateWindowEx(WS_EX_CLIENTEDGE,
                              WC_TREEVIEW,
                              L"Tree View",
                              WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT, 
                              0, 0, Rect.right, Rect.bottom,
                              Object->hWnd, (HMENU)UlongToPtr(TreeId), 
							  SdkInstance, NULL);
	ASSERT(hWndTree != NULL);
	Object->hWndTree =  hWndTree;

    himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON), 
		                    GetSystemMetrics(SM_CYSMICON), 
							ILC_COLOR32, 0, 0); 
	ASSERT(himl != NULL);
    TreeView_SetImageList(hWndTree, himl, TVSIL_NORMAL); 


    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM; 

    // Set the text of the item. 
    tvi.pszText = L"Test"; 
    tvi.cchTextMax = sizeof(tvi.pszText)/sizeof(tvi.pszText[0]); 

    // Assume the item is not a parent item, so give it a 
    // document image. 
    tvi.iImage = 0; 
    tvi.iSelectedImage = 0; 

    // Save the heading level in the item's application-defined 
    // data area. 
    tvi.lParam = (LPARAM)0; 
    tvins.item = tvi; 
    tvins.hInsertAfter = hPrev; 

    // Set the parent item based on the specified level. 
    if (nLevel == 1) 
        tvins.hParent = TVI_ROOT; 
    else if (nLevel == 2) 
        tvins.hParent = hPrevRootItem; 
    else 
        tvins.hParent = hPrevLev2Item; 

    // Add the item to the tree-view control. 
    hPrev = (HTREEITEM)SendMessage(hWndTree, 
                                   TVM_INSERTITEM, 
                                   0,
                                   (LPARAM)(LPTVINSERTSTRUCT)&tvins); 

    // Save the handle to the item. 
    if (nLevel == 1) 
        hPrevRootItem = hPrev; 
    else if (nLevel == 2) 
        hPrevLev2Item = hPrev; 

    // The new item is a child item. Give the parent item a 
    // closed folder bitmap to indicate it now has child items. 
    if (nLevel > 1)
    { 
        hti = TreeView_GetParent(hWndTree, hPrev); 
        tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE; 
        tvi.hItem = hti; 
        tvi.iImage = 0; 
        tvi.iSelectedImage = 0; 
        TreeView_SetItem(hWndTree, &tvi); 
    } 

	return 0;
}

#ifdef _DEBUG

//
// N.B. Kernel mode test routine
//

#include "kernelmode.h"

LRESULT
FrameOnTestKernelMode(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	KernelDialogCreate(hWnd);
	return 0L;
}

#endif