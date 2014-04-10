//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#include "cmdopt.h"

typedef enum _CmdColumnType {
	CmdColumnOption,
	CmdColumnComment,
	CmdColumnNumber,
} CmdColumnType;

typedef struct _CMDLINE_DIALOG_CONTEXT {
	HFONT hFontOld;
	HFONT hFontBold;
} CMDLINE_DIALOG_CONTEXT, *PCMDLINE_DIALOG_CONTEXT;

LISTVIEW_COLUMN CmdColumn[CmdColumnNumber] = {
	{ 170, L"Option",  LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 300, L"Comment", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

PWSTR CmdOptions[][2] = {
	{ L"/eula", L"Accept EULA without pop up dialog box" },
	{ L"/log <full path name>", L"Open a previously captured dtl log file" },
	{ L"/dpc <full path name>", L"Import probe object from a *.dpc file to execute" },
	{ L"/pid <PID>", L"Apply *.dpc file to specified PID" },
	{ L"/save <full path name>", L"Save captured log as specified full path name" },
	{ L"/time <number of seconds>", L"Stop probing when specified number of seconds elapse" },
	{ L"/size <number of MB bytes>", L"Stop probing when log file reach the specified size" },
	{ L"/count <number>", L"Stop probing when record number reach the specified count" },
	{ NULL, NULL }
};

PWSTR CmdlineExample = 
	L"Example:\r\n"
	L"dprobe /log \"d:\\sample.dtl\"\r\n"
	L"dprobe /dpc \"d:\\explorer.dpc\" /pid 2048 /save \"d:\\explorer.dtl\" /time 60\r\n\r\n"
	L"Note:\r\n"
	L"/time, /size, /count are mutual exclusive.";

BOOLEAN
CmdlineDialogCreate(
	IN HWND hWndOwner
	)
{
	DIALOG_OBJECT Object = {0};
	CMDLINE_DIALOG_CONTEXT Context = {0};

	Object.Context = &Context;
	Object.hWndParent = hWndOwner;
	Object.ResourceId = IDD_DIALOG_COMMAND;
	Object.Procedure = CmdlineProcedure;

	DialogCreate(&Object);
	return TRUE;
}

LRESULT 
CmdlineOnClose(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCMDLINE_DIALOG_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCMDLINE_DIALOG_CONTEXT)Object->Context;
	DeleteObject(Context->hFontBold);
	return 0L;
}

INT_PTR CALLBACK
CmdlineProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {
		
		case WM_COMMAND:
			if (LOWORD(wp) == IDOK || LOWORD(wp) == IDCANCEL) {
				CmdlineOnClose(hWnd, uMsg, wp, lp);
				EndDialog(hWnd, IDOK);
			}
			break;

		case WM_INITDIALOG:
			CmdlineOnInitDialog(hWnd, uMsg, wp, lp);
			Status = TRUE;

		case WM_NOTIFY:
			
			//
			// N.B. Don't forget to return status back to window procedure,
			// otherwise it will not work for WM_NOTIFY
			//

			Status = CmdlineOnNotify(hWnd, uMsg, wp, lp);
			break;
	}

	return Status;
}

LRESULT
CmdlineOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndOptions;
	LVCOLUMN lvc = {0}; 
	LVITEM item = {0};
	int i;

	hWndOptions = GetDlgItem(hWnd, IDC_LIST_OPTIONS);
	ASSERT(hWndOptions);

    ListView_SetUnicodeFormat(hWndOptions, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndOptions, 
		                                LVS_EX_FULLROWSELECT,  
										LVS_EX_FULLROWSELECT);

	//
	// N.B. Refer to msdn, which claim we can achieve better result for font rendering
	// if send CCM_SETVERSION message before insert any item
	//

	SendMessage(hWndOptions, CCM_SETVERSION, (WPARAM)5, 0);

    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 
	for (i = 0; i < CmdColumnNumber; i++) { 
        lvc.iSubItem = i;
		lvc.pszText = CmdColumn[i].Title;	
		lvc.cx = CmdColumn[i].Width;     
		lvc.fmt = CmdColumn[i].Align;
		ListView_InsertColumn(hWndOptions, i, &lvc);
    } 

	i = 0;

	while (CmdOptions[i][0] != NULL) {
			
		item.mask = LVIF_TEXT;
		item.iItem = i;
		item.iSubItem = 0;
		item.pszText = CmdOptions[i][0];
		ListView_InsertItem(hWndOptions, &item);

		item.iSubItem = 1;
		item.pszText = CmdOptions[i][1];
		ListView_SetItem(hWndOptions, &item);

		i += 1;
	}

	SetWindowText(GetDlgItem(hWnd, IDC_STATIC), CmdlineExample);
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)SdkMainIcon);
	SdkCenterWindow(hWnd);
	return TRUE;
}

LRESULT
CmdlineOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LRESULT Status;
	NMHDR *pNmhdr = (NMHDR *)lp;

	Status = 0L;
	switch (pNmhdr->code) {
		case NM_CUSTOMDRAW:
			Status = CmdlineOnCustomDraw(hWnd, pNmhdr);
			break;
	}
	return Status;
}

LRESULT 
CmdlineOnCustomDraw(
	IN HWND hWnd,
	IN NMHDR *pNmhdr
	)
{
    LRESULT Status = 0L;
    LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)pNmhdr;
	PDIALOG_OBJECT Object;
	PCMDLINE_DIALOG_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PCMDLINE_DIALOG_CONTEXT)Object->Context;

    switch(lvcd->nmcd.dwDrawStage)  {
        
        case CDDS_PREPAINT: 
            Status = CDRF_NOTIFYITEMDRAW;
            break;
        
        case CDDS_ITEMPREPAINT:
            Status = CDRF_NOTIFYSUBITEMDRAW;
            break;
        
        case CDDS_SUBITEM | CDDS_ITEM | CDDS_PREPAINT: 
          
			if (!Context->hFontOld) {
				LOGFONT LogFont;
				Context->hFontOld = GetCurrentObject(lvcd->nmcd.hdc, OBJ_FONT);
				GetObject(Context->hFontOld, sizeof(LOGFONT), &LogFont);
				LogFont.lfWeight = FW_BOLD;
				Context->hFontBold = CreateFontIndirect(&LogFont);
			}

			//
			// Bold the command line column
			//

			if (lvcd->iSubItem == 0) {
				SelectObject(lvcd->nmcd.hdc, Context->hFontBold);
			} else {
				SelectObject(lvcd->nmcd.hdc, Context->hFontOld);
			}

            Status = CDRF_NEWFONT;
			break;
        
        default:
            Status = CDRF_DODEFAULT;
    }
    
    return Status;
}