//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#include "resultdlg.h"
#include "mspprocess.h"

typedef enum _RESULT_FIELD {
	ResultModule,
	ResultFunction,
	ResultAction,
	ResultStatus,
	ResultNumber,
} RESULT_FIELD, *PRESULT_FIELD;

PWSTR ResultFieldName[ResultNumber] = {
	L"Module",
	L"Function",
	L"Action",
	L"Status",
};

LISTVIEW_COLUMN 
ResultColumn[ResultNumber] = {
	{ 80,  L"Module",   LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 140, L"Function", LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 60,  L"Action",   LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 100,  L"Status",  LVCFMT_LEFT, 0, TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

typedef struct _RESULT_CONTEXT {
	PBSP_PROCESS Process;
	PMSP_USER_COMMAND Command;
	HFONT hFontBold;
} RESULT_CONTEXT, *PRESULT_CONTEXT;

INT_PTR
ResultDialog(
	IN HWND hWndParent,
	IN PBSP_PROCESS Process,
	IN struct _MSP_USER_COMMAND *Command
	)
{
	DIALOG_OBJECT Object = {0};
	RESULT_CONTEXT Context = {0};
	INT_PTR Return;

	DIALOG_SCALER_CHILD Children[3] = {
		{ IDC_LIST_RESULT,  AlignRight, AlignBottom },
		{ IDC_BUTTON_COPY, AlignBoth, AlignBoth },
		{ IDOK, AlignBoth, AlignBoth }
	};

	DIALOG_SCALER Scaler = {
		{0,0}, {0,0}, {0,0}, 3, Children
	};

	if (!Command || !Command->FailureCount) {
		return 0;
	}

	Context.Process = Process;
	Context.Command = Command;

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_RESULT;
	Object.Scaler = &Scaler;
	Object.Procedure = ResultProcedure;

	Return = DialogCreate(&Object);
	return Return;
}

INT_PTR CALLBACK
ResultProcedure(
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
			ResultOnInitDialog(hWnd, uMsg, wp, lp);
			Status = TRUE;
			break;

		case WM_COMMAND:
			return ResultOnCommand(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
ResultOnCommand(
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
			return ResultOnOk(hWnd, uMsg, wp, lp);
		case IDC_BUTTON_COPY:
			return ResultOnCopy(hWnd, uMsg, wp, lp);

	}

	return 0;
}

LRESULT
ResultOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	EndDialog(hWnd, IDOK);
	return 0;
}

LRESULT
ResultOnCopy(
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
	SIZE_T Length;
	SIZE_T TotalLength;
	BOOLEAN Copy;
	PWSTR Text;
	WCHAR Buffer[MAX_PATH];

	hWndList = GetDlgItem(hWnd, IDC_LIST_RESULT);
	Count = ListView_GetItemCount(hWndList); 

	Text = NULL;
	Copy = FALSE;
	TotalLength = 0;

repeat:

	for(Number = 0; Number < Count; Number += 1) {

		Item.mask = LVIF_TEXT;
		Item.iItem = Number;
		Item.pszText = Buffer;
		Item.cchTextMax = MAX_PATH;

		Item.iSubItem = 0;
		ListView_GetItem(hWndList, &Item);

		if (!Copy) {
			StringCchLength(Item.pszText, MAX_PATH, &Length);
			TotalLength += (Length + 4);
		} else {
			StringCchCat(Text, TotalLength, Item.pszText);
			StringCchCat(Text, TotalLength, L",");
		}

		Item.iSubItem = 1;
		ListView_GetItem(hWndList, &Item);

		if (!Copy) {
			StringCchLength(Item.pszText, MAX_PATH, &Length);
			TotalLength += (Length + 4);
		} else {
			StringCchCat(Text, TotalLength, Item.pszText);
			StringCchCat(Text, TotalLength, L",");
		}

		Item.iSubItem = 2;
		ListView_GetItem(hWndList, &Item);
		
		if (!Copy) {
			StringCchLength(Item.pszText, MAX_PATH, &Length);
			TotalLength += (Length + 4);
		} else {
			StringCchCat(Text, TotalLength, Item.pszText);
			StringCchCat(Text, TotalLength, L",");
		}

		Item.iSubItem = 3;
		ListView_GetItem(hWndList, &Item);
		
		if (!Copy) {

			StringCchLength(Item.pszText, MAX_PATH, &Length);
			TotalLength += (Length + 4);
			TotalLength += sizeof(L"\r\n");

		} else {
			StringCchCat(Text, TotalLength, Item.pszText);
			StringCchCat(Text, TotalLength, L"\r\n");
		}
	}

	if (!Copy) {

		Text = (PWSTR)SdkMalloc((ULONG)TotalLength * sizeof(WCHAR));
		Copy = TRUE;
		goto repeat;

	}

    Data = GlobalAlloc(GMEM_MOVEABLE, sizeof(WCHAR) * TotalLength);
	Memory = GlobalLock(Data);
	RtlCopyMemory(Memory, Text, sizeof(WCHAR) * TotalLength); 
	GlobalUnlock(Data);

	if (OpenClipboard(hWnd)) {
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, Data);
        CloseClipboard();
    }

	GlobalFree(Data);
	SdkFree(Text);
	return 0;
}

LRESULT
ResultOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndList;
	LVCOLUMN Column = {0};
	LVITEM Item = {0};
	PDIALOG_OBJECT Object;
	PRESULT_CONTEXT Context;
	PBSP_PROCESS Process;
	WCHAR Buffer[MAX_PATH];
	ULONG i;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, RESULT_CONTEXT);
	Process = Context->Process;

	hWndList = GetDlgItem(hWnd, IDC_LIST_RESULT);
	ListView_SetUnicodeFormat(hWndList, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	
    Column.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 
	  
    for (i = 0; i < ResultNumber; i++) { 

        Column.iSubItem = i;
		Column.pszText = ResultColumn[i].Title;	
		Column.cx = ResultColumn[i].Width;     
		Column.fmt = ResultColumn[i].Align;

		ListView_InsertColumn(hWndList, i, &Column);
    } 

	ResultInsertFailure(Object, hWndList, Context->Command);
	StringCchPrintf(Buffer, MAX_PATH, L"D Probe (Failure report for %s:%u)",
					Process->Name, Process->ProcessId);

	SetWindowText(hWnd, Buffer);
	SdkSetMainIcon(hWnd);
	return 0L;
}

typedef VOID 
(WINAPI *BTR_FORMAT_ERROR)(
	IN ULONG ErrorCode,
	IN BOOLEAN SystemError,
	OUT PWCHAR Buffer,
	IN ULONG BufferLength, 
	OUT PULONG ActualLength
	);

VOID
ResultInsertFailure(
	IN PDIALOG_OBJECT Object,
	IN HWND hWndList,
	IN struct _MSP_USER_COMMAND *Command
	)
{
	ULONG Number;
	LVITEM Item = {0};
	WCHAR Buffer[MAX_PATH];
	PMSP_FAILURE_ENTRY Failure;
	PLIST_ENTRY ListEntry;
	HMODULE BtrHandle;
	BTR_FORMAT_ERROR BtrErrorRoutine;
	ULONG Length;

	BtrHandle = GetModuleHandle(L"dprobe.btr.dll");
	ASSERT(BtrHandle != NULL);

	BtrErrorRoutine = (BTR_FORMAT_ERROR)GetProcAddress(BtrHandle, "BtrFormatError");
	ASSERT(BtrErrorRoutine != NULL);

	Number = 0;
	ListEntry = Command->FailureList.Flink;

	while (ListEntry != &Command->FailureList) {

		Failure = CONTAINING_RECORD(ListEntry, MSP_FAILURE_ENTRY, ListEntry);

		Item.mask = LVIF_TEXT;
		Item.iItem = Number;

		//
		// DllName 
		//

		Item.iSubItem = 0;
		Item.pszText = Failure->DllName;
		ListView_InsertItem(hWndList, &Item);

		//
		// ApiName
		//

		Item.iSubItem = 1;
		if (!Failure->DllLevel) {
			BspUnDecorateSymbolName(Failure->ApiName, Buffer, MAX_PATH, UNDNAME_NAME_ONLY);
			Item.pszText = Buffer;
		} else {
			Item.pszText = L"N/A";
		}
		ListView_SetItem(hWndList, &Item);

		//
		// Action
		//

		Item.iSubItem = 2;
		if (!Failure->DllLevel) {
			if (Failure->Activate) {
				Item.pszText = L"Activate";
			} else {
				Item.pszText = L"Deactivate";
			}
		} 
		else {
			Item.pszText = L"N/A";
		}
		ListView_SetItem(hWndList, &Item);

		//
		// Status
		//

		if ((Failure->Status & (ULONG)0xF0000000) == (ULONG)0xE0000000) {
			(*BtrErrorRoutine)(Failure->Status, FALSE, Buffer, MAX_PATH, &Length); 
		} else {
			(*BtrErrorRoutine)(Failure->Status, TRUE, Buffer, MAX_PATH, &Length); 
		}

		Item.iSubItem = 3;
		Item.pszText = Buffer;
		ListView_SetItem(hWndList, &Item);

		ListEntry = ListEntry->Flink;
	}
}