//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#include "colorctrl.h"

#define SDK_COLOR_CLASS L"SdkColor"

BOOLEAN 
ColorInitialize(
	VOID
	)
{
    WNDCLASSEX wc = {0};

	wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = ColorProcedure;
    wc.hInstance = SdkInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = SDK_COLOR_CLASS;

	if (!RegisterClassEx(&wc)) {
        return FALSE;
	}

    return TRUE;
}

HWND 
ColorCreateControl(
    IN HWND hWndParent,
    IN INT_PTR Id
    )
{
	HWND hWnd;

    hWnd = CreateWindow(SDK_COLOR_CLASS, L"",
						WS_CHILD | WS_VISIBLE | WS_BORDER,
						0, 0, 3, 3, hWndParent, (HMENU)Id, SdkInstance,NULL);
	return hWnd;
}

LRESULT
ColorOnCreate(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    )
{
	PCOLOR_CONTROL Object;

	Object = (PCOLOR_CONTROL)SdkMalloc(sizeof(COLOR_CONTROL));
	ZeroMemory(Object, sizeof(COLOR_CONTROL));

	Object->hWnd = hWnd;
    Object->Color = RGB(0x00, 0x00, 0x00);
    Object->Hot = FALSE;

	SdkSetObject(hWnd, Object);
	ShowWindow(hWnd, SW_SHOW);
	return 0L;
}

LRESULT
ColorOnPaint(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    )
{
	PAINTSTRUCT Paint;
	RECT Rect;
	HDC hdc;
	PCOLOR_CONTROL Object;

	Object = (PCOLOR_CONTROL)SdkGetObject(hWnd);

	hdc = BeginPaint(hWnd, &Paint);
	GetClientRect(hWnd, &Rect);

	if (!Object->Hot) {
		SetDCBrushColor(hdc, Object->Color);
		FillRect(hdc, &Rect, GetStockObject(DC_BRUSH));
	} else {
		SetDCBrushColor(hdc, SdkBrightColor(Object->Color, 64));
		FillRect(hdc, &Rect, GetStockObject(DC_BRUSH));
	}

	EndPaint(hWnd, &Paint);
	return 0;
}

LRESULT
ColorOnMouseMove(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    )
{
    PCOLOR_CONTROL Object = NULL;
	TRACKMOUSEEVENT Mouse = { sizeof(Mouse) };

	Object = (PCOLOR_CONTROL)SdkGetObject(hWnd);
	if (Object && !Object->Hot) {

		Object->Hot = TRUE;
		InvalidateRect(hWnd, NULL, TRUE);

		Mouse.dwFlags = TME_LEAVE;
		Mouse.hwndTrack = hWnd;
		TrackMouseEvent(&Mouse);
	}

	return 0;
}

LRESULT
ColorOnMouseLeave(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    )
{
    PCOLOR_CONTROL Object = NULL;

	Object = (PCOLOR_CONTROL)SdkGetObject(hWnd);
	if (Object) {
	    Object->Hot = FALSE;
	    InvalidateRect(hWnd, NULL, TRUE);
	}
	return 0;
}

LRESULT
ColorOnLButtonDown(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    )
{
	PCOLOR_CONTROL Object;
    CHOOSECOLOR Choose = { sizeof(Choose) };
    COLORREF Colors[16] = { 0 };

	Object = (PCOLOR_CONTROL)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

    Choose.hwndOwner = hWnd;
    Choose.rgbResult = Object->Color;
    Choose.lpCustColors = Colors;
    Choose.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColor(&Choose)) {
        Object->Color = Choose.rgbResult;
        InvalidateRect(hWnd, NULL, TRUE);
    }

	return 0L;
}

LRESULT CALLBACK 
ColorProcedure(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp
    )
{
    PCOLOR_CONTROL Object = NULL;

    switch (uMsg) {

    case WM_CREATE:
		ColorOnCreate(hWnd, uMsg, wp, lp);
        break;

    case WM_DESTROY:
		Object = (PCOLOR_CONTROL)SdkGetObject(hWnd);
		SdkFree(Object);
        break;

    case WM_PAINT:
		ColorOnPaint(hWnd, uMsg, wp, lp);
        break;

    case WM_ERASEBKGND:
        return TRUE;

    case WM_MOUSEMOVE:
		ColorOnMouseMove(hWnd, uMsg, wp, lp);
        break;

    case WM_MOUSELEAVE:
        ColorOnMouseLeave(hWnd, uMsg, wp, lp);
        break;

    case WM_LBUTTONDOWN:
        ColorOnLButtonDown(hWnd, uMsg, wp, lp);
        break;

    case WM_COLOR_SETCOLOR:
		Object = (PCOLOR_CONTROL)SdkGetObject(hWnd);
        Object->Color = (COLORREF)wp;
        return TRUE;

    case WM_COLOR_GETCOLOR:
		Object = (PCOLOR_CONTROL)SdkGetObject(hWnd);
		return (LRESULT)Object->Color;
    }

    return DefWindowProc(hWnd, uMsg, wp, lp);
}