//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
// 

#include "openprompt.h"

typedef struct _OPENPROMPT_DIALOG_CONTEXT {
	HFONT hFontOld;
	HFONT hFontBold;
} OPENPROMPT_DIALOG_CONTEXT, *POPENPROMPT_DIALOG_CONTEXT;

INT_PTR
OpenPromptDialogCreate(
	IN HWND hWndParent	
	)
{
	DIALOG_OBJECT Object = {0};
	OPENPROMPT_DIALOG_CONTEXT Context = {0};

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_OPENPROMPT;
	Object.Procedure = OpenPromptDialogProcedure;
	
	return DialogCreate(&Object);
}

INT_PTR CALLBACK
OpenPromptDialogProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LRESULT Status = 0;

	switch (uMsg) {

		case WM_INITDIALOG:
			OpenPromptOnInitDialog(hWnd, uMsg, wp, lp);			
			return 1;

		case WM_COMMAND:
			OpenPromptOnCommand(hWnd, uMsg, wp, lp);
			break;
		
		case WM_CTLCOLORSTATIC:
			return OpenPromptOnCtlColorStatic(hWnd, uMsg, wp, lp);
	}
	
	return Status;
}

LRESULT
OpenPromptOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	switch (LOWORD(wp)) {
		case IDOK:
		case IDCANCEL:
		case IDCLOSE:
			OpenPromptOnClose(hWnd, uMsg, wp, lp);
			EndDialog(hWnd, LOWORD(wp));
			break;
	}

	return 0L;
}

LRESULT
OpenPromptOnClose(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	POPENPROMPT_DIALOG_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (POPENPROMPT_DIALOG_CONTEXT)Object->Context;

	DeleteObject(Context->hFontBold);
	return 0L;
}

LRESULT
OpenPromptOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	SdkSetMainIcon(hWnd);
	SetWindowText(hWnd, L"Open");
	SdkCenterWindow(hWnd);
	return TRUE;
}

LRESULT
OpenPromptOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	POPENPROMPT_DIALOG_CONTEXT Context;
	UINT Id;
	HFONT hFont;
	LOGFONT LogFont;
	RECT Rect;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (POPENPROMPT_DIALOG_CONTEXT)Object->Context;

	Id = GetDlgCtrlID((HWND)lp);
	if (Id == IDC_STATIC_PROMPT) {
		if (!Context->hFontBold) {

			/*hFont = GetCurrentObject((HDC)wp, OBJ_FONT);
			GetObject(hFont, sizeof(LOGFONT), &LogFont);
			LogFont.lfWeight = FW_BOLD;
			hFont = CreateFontIndirect(&LogFont);

			Context->hFontBold = hFont;
			SendMessage((HWND)lp, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE);

			GetClientRect((HWND)lp, &Rect);
			InvalidateRect((HWND)lp, &Rect, TRUE);*/
		}
	}

	return (LRESULT)NULL;
}