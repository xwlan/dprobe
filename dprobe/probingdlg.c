//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "probingdlg.h"
#include "dprobe.h"

LISTVIEW_COLUMN ProbingColumn[3] = {
	{ 200, L"Probe",    LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 100, L"Module",   LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 200, L"Provider", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

BOOLEAN
ProbingDialogCreate(
	IN HWND hWndParent,
	IN ULONG ProcessId 
	)
{
	DIALOG_OBJECT Object = {0};
	
	DIALOG_SCALER_CHILD Children[2] = {
		{ IDC_LIST_PROBES,  AlignRight, AlignBottom },
		{ IDOK, AlignBoth, AlignBoth },
	};

	DIALOG_SCALER Scaler = {
		{0,0}, {0,0}, {0,0}, 2, Children
	};

	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_PROBING;
	Object.Procedure = ProbingProcedure;
	Object.Scaler = &Scaler;

	DialogCreate(&Object);
	return TRUE;
}

LRESULT
ProbingOnCommand(
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

		ProbingOnOk(hWnd, uMsg, wp, lp);
	    break;

	}

	return 0;
}

INT_PTR CALLBACK
ProbingProcedure(
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
			ProbingOnCommand(hWnd, uMsg, wp, lp);
			Status = TRUE;
			break;
		
		case WM_INITDIALOG:
			ProbingOnInitDialog(hWnd, uMsg, wp, lp);
			SdkCenterWindow(hWnd);
			Status = TRUE;
	}

	return Status;
}

LRESULT
ProbingOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndProbes;
	LVCOLUMN lvc = {0}; 
	PDIALOG_OBJECT Object;
	int i;

	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)SdkMainIcon);
	hWndProbes = GetDlgItem(hWnd, IDC_LIST_PROBES);

	ASSERT(hWnd != NULL);
	ASSERT(hWndProbes != NULL);
	
    ListView_SetUnicodeFormat(hWndProbes, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndProbes, 
		                                LVS_EX_FULLROWSELECT,  
										LVS_EX_FULLROWSELECT);

    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 

    for (i = 0; i < 3; i++) { 

        lvc.iSubItem = i;
		lvc.pszText = ProbingColumn[i].Title;	
		lvc.cx = ProbingColumn[i].Width;     
		lvc.fmt = ProbingColumn[i].Align;

		ListView_InsertColumn(hWndProbes, i, &lvc);
	}

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

	ProbingLoadProbes(Object);
	return TRUE;
}

ULONG
ProbingLoadProbes(
	IN PDIALOG_OBJECT Object
	)
{
	return 0;
}

LRESULT
ProbingOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	EndDialog(hWnd, IDOK);
	return FALSE;
}