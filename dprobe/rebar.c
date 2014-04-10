//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

//
// N.B. This is for REBARINFO structure, it has different size
// for XP and VISTA, band insertion will fail if we fill wrong
// size of REBARINFO.
//

#undef  _WIN32_WINNT
#define _WIN32_WINNT 0x0501

#include "rebar.h"
#include "list.h"
#include "sdk.h"

TBBUTTON FindBarButtons[] = {
	{ 0, IDM_FINDBACKWARD, TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
	{ 1, IDM_FINDFORWARD, TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
	{ 2, IDM_AUTOSCROLL, TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
};

PREBAR_OBJECT
RebarCreateObject(
	IN HWND hWndParent, 
	IN UINT Id 
	)
{
	HWND hWndRebar;
	LRESULT Result;
	PREBAR_OBJECT Object;

	hWndRebar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
							   WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | RBS_VARHEIGHT | 
							   WS_CLIPSIBLINGS | CCS_TOP | CCS_NODIVIDER | RBS_AUTOSIZE|
							    WS_GROUP,
							   0, 0, 0, 0, hWndParent, UlongToHandle(Id), SdkInstance, NULL);
	if (!hWndRebar) {
		return NULL;
	}

	Object = (PREBAR_OBJECT)SdkMalloc(sizeof(REBAR_OBJECT));
	ZeroMemory(Object, sizeof(REBAR_OBJECT));

	Object->hWndRebar = hWndRebar;
	Object->RebarInfo.cbSize  = sizeof(REBARINFO);

	Result = SendMessage(hWndRebar, RB_SETBARINFO, 0, (LPARAM)&Object->RebarInfo);
	if (!Result) {
		SdkFree(Object);
		return NULL;
	}
	
	SendMessage(hWndRebar, RB_SETUNICODEFORMAT, (WPARAM)TRUE, (LPARAM)0);

	InitializeListHead(&Object->BandList);
	return Object;
}

HWND
RebarCreateFindbar(
	IN PREBAR_OBJECT Object	
	)
{
	HWND hWnd;
	HWND hWndRebar;
	TBBUTTON Tb = {0};
	TBBUTTONINFO Tbi = {0};

	hWndRebar = Object->hWndRebar;
	ASSERT(hWndRebar != NULL);

	hWnd = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, 
		                  WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
	                      TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | 
						  CCS_NODIVIDER | CCS_TOP | CCS_NORESIZE,
						  0, 0, 0, 0, 
		                  hWndRebar, (HMENU)UlongToHandle(REBAR_BAND_FINDBAR), SdkInstance, NULL);

    ASSERT(hWnd != NULL);

    SendMessage(hWnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0L);
	SendMessage(hWnd, TB_ADDBUTTONS, sizeof(FindBarButtons) / sizeof(TBBUTTON), (LPARAM)FindBarButtons);
	SendMessage(hWnd, TB_AUTOSIZE, 0, 0);
	return hWnd;
}

BOOLEAN
RebarSetFindbarImage(
	IN HWND hWndFindbar,
	IN COLORREF Mask,
	OUT HIMAGELIST *Normal,
	OUT HIMAGELIST *Grayed
	)
{
	ULONG Number;
	HIMAGELIST himlNormal;
	HIMAGELIST himlGrayed;
	HICON hicon;

	Number = sizeof(FindBarButtons) / sizeof(TBBUTTON);

	himlNormal = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, Number, Number);

	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_FINDBACKWARD));
	ImageList_AddIcon(himlNormal, hicon);
	
	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_FINDFORWARD));
	ImageList_AddIcon(himlNormal, hicon);
	
	hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_AUTOSCROLL));
	ImageList_AddIcon(himlNormal, hicon);
	
    SendMessage(hWndFindbar, TB_SETIMAGELIST, 0L, (LPARAM)himlNormal);
    SendMessage(hWndFindbar, TB_SETHOTIMAGELIST, 0L, (LPARAM)himlNormal);

	himlGrayed = SdkCreateGrayImageList(himlNormal);
    SendMessage(hWndFindbar, TB_SETDISABLEDIMAGELIST, 0L, (LPARAM)himlGrayed);

	*Normal = himlNormal;
	*Grayed = himlGrayed;

	return TRUE;
}

HWND
RebarCreateEdit(
	IN PREBAR_OBJECT Object	
	)
{
	HWND hWnd;
	HWND hWndRebar;

	hWndRebar = Object->hWndRebar;
	ASSERT(hWndRebar != NULL);

	hWnd = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", NULL, 
                          WS_CHILD | WS_BORDER | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS | ES_LEFT, 
						  0, 0, 0, 0, hWndRebar, (HMENU)REBAR_BAND_EDIT, SdkInstance, NULL);
    ASSERT(hWnd != NULL);
	Edit_SetCueBannerText(hWnd, L"Search");
	return hWnd;
}

BOOLEAN
RebarInsertBands(
	IN PREBAR_OBJECT Object
	)
{
	HWND hWndEdit;
	HWND hWndFindbar;
	HWND hWndRebar;
	PREBAR_BAND Band;
	HIMAGELIST Normal;
	HIMAGELIST Grayed;
	LRESULT Result;
	RECT Rect;
	SIZE Size;

	hWndRebar = Object->hWndRebar;

	//
	// Create findbar which can be used to find forward or 
	// backward in the record cache, note thate we need first
	// create findbar to calculate the size of edit to fit,
	// but we want edit to be the first band, so we first
	// insert edit band, then findbar band.
	//

	hWndFindbar = RebarCreateFindbar(Object);
	if (!hWndFindbar) {
		return FALSE;
	}

	RebarSetFindbarImage(hWndFindbar, 0, &Normal, &Grayed);

	SendMessage(hWndFindbar, TB_AUTOSIZE, 0, 0);
	SendMessage(hWndFindbar, TB_GETMAXSIZE, 0, (LPARAM)&Size);

	//
	// Create edit which contain search text
	//

	hWndEdit = RebarCreateEdit(Object);
	if (!hWndEdit) {
		return FALSE;
	}

	Band = (PREBAR_BAND)SdkMalloc(sizeof(REBAR_BAND));
	ZeroMemory(Band, sizeof(REBAR_BAND));

	Band->hWndBand = hWndEdit;
	Band->BandId = REBAR_BAND_EDIT;

	Band->BandInfo.cbSize = sizeof(REBARBANDINFO);
	Band->BandInfo.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | 
						   RBBIM_STYLE;

	GetWindowRect(hWndEdit, &Rect);
	Band->BandInfo.cxMinChild = 0;
	Band->BandInfo.cyMinChild = Size.cy; 
	Band->BandInfo.cx = 0;
	Band->BandInfo.fStyle = RBBS_NOGRIPPER | RBBS_TOPALIGN;
	Band->BandInfo.wID = REBAR_BAND_EDIT;
	Band->BandInfo.hwndChild = hWndEdit;

	Result = SendMessage(hWndRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&Band->BandInfo);
	if (!Result) {
		SdkFree(Band);
		return FALSE;
	}
	
	SendMessage(hWndEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(TRUE, 0));
	InsertTailList(&Object->BandList, &Band->ListEntry);
	Object->NumberOfBands += 1;

	//
	// Then insert findbar band
	//

	Band = (PREBAR_BAND)SdkMalloc(sizeof(REBAR_BAND));
	ZeroMemory(Band, sizeof(REBAR_BAND));

	Band->hWndBand = hWndFindbar;
	Band->BandId = REBAR_BAND_FINDBAR;
	Band->himlNormal = Normal;
	Band->himlGrayed = Grayed;

	Band->BandInfo.cbSize = sizeof(REBARBANDINFO);
	Band->BandInfo.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID |
						   RBBIM_STYLE ;

	GetWindowRect(hWndFindbar, &Rect);

	Band->BandInfo.cxMinChild = Size.cx;
	Band->BandInfo.cyMinChild = Size.cy;
	Band->BandInfo.cx = Size.cx;
	Band->BandInfo.fStyle = RBBS_NOGRIPPER| RBBS_TOPALIGN | RBBS_FIXEDSIZE;
	Band->BandInfo.wID = REBAR_BAND_FINDBAR;
	Band->BandInfo.hwndChild = hWndFindbar;

	Result = SendMessage(hWndRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&Band->BandInfo);
	if (!Result) {
		SdkFree(Band);
		return FALSE;
	}

	InsertTailList(&Object->BandList, &Band->ListEntry);
	Object->NumberOfBands += 1;
	return TRUE;
}

VOID
RebarAdjustPosition(
	IN PREBAR_OBJECT Object
	)
{
	HWND hWndParent;
	RECT ClientRect;
	RECT Rect;

	hWndParent = GetParent(Object->hWndRebar);
	GetClientRect(hWndParent, &ClientRect);
	GetWindowRect(Object->hWndRebar, &Rect);

	MoveWindow(Object->hWndRebar, 0, 0, 
		       ClientRect.right - ClientRect.left, 
			   Rect.bottom - Rect.top, TRUE);
}

VOID
RebarCurrentEditString(
	IN PREBAR_OBJECT Object,
	OUT PWCHAR Buffer,
	IN ULONG Length
	)
{
	PREBAR_BAND Band;
	PLIST_ENTRY ListEntry;

	ListEntry = Object->BandList.Flink;

	//
	// N.B. Edit band is the second band object
	//

	Band = CONTAINING_RECORD(ListEntry, REBAR_BAND, ListEntry);
	SendMessage(Band->hWndBand,  WM_GETTEXT, (WPARAM)Length, (LPARAM)Buffer);
}