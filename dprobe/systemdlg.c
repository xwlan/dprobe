//
// xwlan@msn.com
// Apsaras Labs
// Copyright(C) 2009-2010
//

#include "systemdlg.h"

LISTVIEW_COLUMN SystemColumn[4] = {
	{ 200,  L"Routine",  LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 100,  L"Category", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 300,  L"Module",   LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

#define SYSTEM_COLUMN_ROUTINE  0
#define SYSTEM_COLUMN_CATEGORY 1 
#define SYSTEM_COLUMN_MODULE   2 

//
// Global build-in filter handle and registration object
//

HMODULE SystemFilterModule;
PBTR_FILTER SystemRegistration;

BOOLEAN
SystemDialogCreate(
	IN HWND hWndParent,
	IN PDIALOG_RESULT Result
	)
{
	DIALOG_OBJECT Object;
	
	DIALOG_SCALER_CHILD Children[3] = {
		{ IDC_LIST_SYSTEM_PROBE, AlignRight, AlignBottom },
		{ IDOK,     AlignBoth, AlignBoth },
		{ IDCANCEL, AlignBoth, AlignBoth }
	};

	DIALOG_SCALER Scaler = {
		{0,0}, {0,0}, {0,0}, 3, Children
	};

	//
	// Bind result object to dialog context, Context 
	// is a pointer to PSP_PROCESS structure
	//
	
	ASSERT(Result != NULL);
	Object.Context = Result;
	
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_SYSTEM_PROBE;
	Object.Procedure = SystemProcedure;
	Object.Scaler = &Scaler;

	DialogCreate(&Object);

	ASSERT((ULONG_PTR)Result == (ULONG_PTR)Object.Context);

	return TRUE;
}

LRESULT
SystemOnCommand(
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
		SystemOnOk(hWnd, uMsg, wp, lp);
	    break;

	case IDCANCEL:
		SystemOnCancel(hWnd, uMsg, wp, lp);
	    break;

	}

	return 0;
}

INT_PTR CALLBACK
SystemProcedure(
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
			SystemOnCommand(hWnd, uMsg, wp, lp);
			Status = TRUE;
			break;
		
		case WM_INITDIALOG:
			SystemOnInitDialog(hWnd, uMsg, wp, lp);
			SdkCenterWindow(hWnd);
			Status = TRUE;
	}

	return Status;
}

ULONG
SystemLoadProbes(
	IN HWND hWnd,
	IN PPSP_PROCESS Process
	)
{
	int i;											   
	LVITEM item = {0};
	BOOLEAN SetBit;
	PBTR_FILTER FilterObject;

	FilterObject = Process->System->FilterObject;

	for (i = 0; i < FilterObject->ProbesCount; i++) {

		//
		// Routine
		//

		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 0;
		item.pszText = FilterObject->Probes[i].ProbeProcedure;
		ListView_InsertItem(hWnd, &item);

		//
		// Category
		//

		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 1;
		item.pszText = FilterObject->Probes[i].Description;
		ListView_SetItem(hWnd, &item);

		//
		// Module
		//

		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 2;
		item.pszText = FilterObject->Probes[i].ProbeModule;
		ListView_SetItem(hWnd, &item);

		SetBit = RtlTestBit(Process->System->CommitBitMap, i);
		if (SetBit) {
			ListView_SetCheckState(hWnd, i, TRUE);
		}
		
	}

	return 0;
}

LRESULT
SystemOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndChild;
	LVCOLUMN lvc = {0}; 
	PDIALOG_OBJECT Object;
	PDIALOG_RESULT Result;
	PPSP_PROCESS Process;
	PPSP_FILTER Filter;
	WCHAR Buffer[MAX_PATH];
	int i;

	PRTL_BITMAP BitMap;
	BTR_GET_FILTER_REGISTRATION GetFilterRegistration;

	hWndChild = GetDlgItem(hWnd, IDC_LIST_SYSTEM_PROBE);
	ASSERT(hWnd != NULL);
	
    ListView_SetUnicodeFormat(hWnd, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndChild, 
		                                LVS_EX_FULLROWSELECT,  
										LVS_EX_FULLROWSELECT);

	ListView_SetExtendedListViewStyleEx(hWndChild, 
		                                LVS_EX_CHECKBOXES, 
										LVS_EX_CHECKBOXES); 

    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 

    for (i = 0; i < 3; i++) { 

        lvc.iSubItem = i;
		lvc.pszText = SystemColumn[i].Title;	
		lvc.cx = SystemColumn[i].Width;     
		lvc.fmt = SystemColumn[i].Align;

		ListView_InsertColumn(hWndChild, i, &lvc);
    } 

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

	Result = (PDIALOG_RESULT)Object->Context;
	Process = (PPSP_PROCESS)Result->Context;
	ASSERT(Process != NULL);

	wsprintf(Buffer, L"System Probe (%ws : %d)", Process->Name, Process->ProcessId);
	SetWindowText(hWnd, Buffer);

	//
	// Load the build-in filter module, and initialize process's 
	// registration field, it's only called once for all process,
	// they share the same SystemRegistration initially.
	//
	
	if (SystemFilterModule == NULL) {

		ASSERT(SystemRegistration == NULL);
		SystemFilterModule = LoadLibrary(L"dprobe.system.flt");

		ASSERT(SystemFilterModule != NULL);
		GetFilterRegistration = (BTR_GET_FILTER_REGISTRATION)GetProcAddress(SystemFilterModule, 
			                                                                "BtrGetFilterRegistration");
		ASSERT(GetFilterRegistration != NULL);	
		
		SystemRegistration = (*GetFilterRegistration)();
		ASSERT(SystemRegistration != NULL);	
		
	}

	//
	// Create bitmaps if this is initial filter loading for target process
	//

	if (Process->System == NULL) {
		Filter = PspCreateAddOnObject();
		Process->System = Filter;
	}

	if (Process->System->FilterObject == NULL) {

		Process->System->FilterObject = SystemRegistration;

		BitMap = RtlCreateBitMap(512);
		Process->System->PendingBitMap = BitMap;

		BitMap = RtlCreateBitMap(512);
		Process->System->CommitBitMap = BitMap;
		
	}

	SystemLoadProbes(hWndChild, Process);
	
	return TRUE;
}

LRESULT
SystemOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	return 0;
}

LRESULT
SystemOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	int i;											   
	int Number;
	BOOLEAN Check;
	HWND hWndChild;
	PPSP_PROCESS Process;
	PPSP_COMMAND Command;
	PDIALOG_OBJECT Object;
	PDIALOG_RESULT Result;
	PBTR_FILTER Registration;
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	ASSERT(Object != NULL);
	
	Result = (PDIALOG_RESULT)Object->Context;
	Process = (PPSP_PROCESS)Result->Context;
	ASSERT(Process != NULL);

	Registration = Process->System->FilterObject;

	hWndChild = GetDlgItem(hWnd, IDC_LIST_SYSTEM_PROBE);
	ASSERT(hWndChild != NULL);

	Number = ListView_GetItemCount(hWndChild);
	ASSERT(Registration->ProbesCount == Number);

	//
	// Temporarily store probe bits into PendingBitMap
	//

	for (i = 0; i < Number; i++) {

		Check = ListView_GetCheckState(hWndChild, i);	

		if (Check) {
			RtlSetBit(Process->System->PendingBitMap, i);
		} else {
			RtlClearBit(Process->System->PendingBitMap, i);
		}
	}
	
	Command = PspCreateCommandObject();
	Command->Type = AddOnProbe;
	Command->Process = Process;
	
	Result->Status = IDOK;
	Result->Context = Command;
	
	EndDialog(hWnd, IDOK);
	return FALSE;
}

LRESULT
SystemOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	EndDialog(hWnd, 0);
	return 0;
}
