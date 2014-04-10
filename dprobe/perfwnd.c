//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#include "perfwnd.h"

PWSTR SdkPerfMonClass = L"SdkPerfMonClass";

BOOLEAN
PerfMonRegisterClass(
	VOID
	)
{
	WNDCLASSEX wc = {0};

	wc.cbSize = sizeof wc;
	wc.hbrBackground  = GetSysColorBrush(COLOR_BTNFACE);
    wc.hCursor        = LoadCursor(0, IDC_ARROW);
    wc.hIcon          = SdkMainIcon;
    wc.hIconSm        = SdkMainIcon;
    wc.hInstance      = SdkInstance;
    wc.lpfnWndProc    = PerfMonProcedure;
    wc.lpszClassName  = SdkPerfMonClass;

    return RegisterClassEx(&wc);
}

HWND
PerfMonWindowCreate(
	IN HWND hWndParent
	)
{
	BOOLEAN Status;
	HWND hWnd;

	PerfMonRegisterClass();

	hWnd = CreateWindowEx(0, SdkPerfMonClass, L"", 
			WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
			0, 0, 600, 400,
			hWndParent, 0, SdkInstance, NULL);

	ASSERT(hWnd);

	if (hWnd != NULL) {

		ShowWindow(hWnd, SW_SHOW);
		UpdateWindow(hWnd);
		return hWnd;

	} else {
		return NULL;
	}
}

LRESULT CALLBACK
PerfMonProcedure(
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
		    PerfMonOnNotify(hWnd, uMsg, wp, lp);
			break;

		case WM_CREATE:
			PerfMonOnCreate(hWnd, uMsg, wp, lp);
			break;

		case WM_SIZE:
			PerfMonOnSize(hWnd, uMsg, wp, lp);
			break;

		case WM_CLOSE:
			PerfMonOnClose(hWnd, uMsg, wp, lp);
			break;

		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			break;

	}

	return DefWindowProc(hWnd, uMsg, wp, lp);
}

LRESULT
PerfMonOnCreate(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PPERFMON_OBJECT Object;
	int i;

	Object = (PPERFMON_OBJECT)SdkMalloc(sizeof(PERFMON_OBJECT));
	Object->hWnd = hWnd;

	for(i = 0; i < PERFMON_CTRL_NUMBER; i++) {
		Object->CtrlId[i] = i;
	}

	SdkSetObject(hWnd, Object);

	SdkCenterWindow(hWnd);
	PerfMonCreateChildren(Object);
	return 0;
}

LRESULT
PerfMonOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	return 0;
}

LRESULT
PerfMonOnPaint(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	return 0;
}

LRESULT
PerfMonOnSize(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	return 0;
}

LRESULT
PerfMonOnClose(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	return 0;
}

ULONG
PerfMonCreateChildren(
	IN PPERFMON_OBJECT Object 
	)
{
	HWND hWnd;
	HWND hWndParent;
	ULONG Style;
	RECT PerfRect;
	RECT ChildRect;

	ASSERT(Object->hWnd != NULL);	

	ASSERT(hWnd != NULL);
	Style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_BORDER;
	hWnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, 0, Style, 
						  0, 0, 0, 0, Object->hWnd, 
						  (HMENU)Object->CtrlId[PerfMonTaskListId], 
						  SdkInstance, NULL);

	ASSERT(hWnd != NULL);

    Style = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
	SetWindowLongPtr(hWnd, GWL_STYLE, (Style & ~LVS_TYPEMASK) | LVS_REPORT | LVS_NOCOLUMNHEADER);
    ListView_SetUnicodeFormat(hWnd, TRUE);
	ListView_SetExtendedListViewStyleEx(hWnd, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	Object->hWndList = hWnd;

	GetClientRect(Object->hWnd, &PerfRect);	
	
	ChildRect.left = PerfRect.left;
	ChildRect.top = PerfRect.top;
	ChildRect.right = PerfRect.right / 4;
	ChildRect.bottom = PerfRect.bottom;

	MoveWindow(hWnd, ChildRect.left, ChildRect.top, 
			   ChildRect.right - ChildRect.left,
			   ChildRect.bottom - ChildRect.top, TRUE);

	return S_OK;
}