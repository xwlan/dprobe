//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#ifndef _COLORCTRL_H
#define _COLORCTRL_H

#include "sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _COLOR_CONTROL {
	HWND hWnd;
    COLORREF Color;
    BOOLEAN Hot;
} COLOR_CONTROL, *PCOLOR_CONTROL;

BOOLEAN 
ColorInitialize(
	VOID
	);

HWND 
ColorCreateControl(
    IN HWND hWndParent,
    IN INT_PTR Id
    );

LRESULT
ColorOnCreate(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    );

LRESULT
ColorOnMouseMove(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    );

LRESULT
ColorOnMouseLeave(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    );

LRESULT
ColorOnLButtonDown(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    );

LRESULT
ColorOnPaint(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    );

LRESULT CALLBACK 
ColorProcedure(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp
    );

#define WM_COLOR_SETCOLOR (WM_APP + 1501)
#define WM_COLOR_GETCOLOR (WM_APP + 1502)

#define ColorSetColor(hWnd, Color) \
    SendMessage((hWnd), WM_COLOR_SETCOLOR, (WPARAM)(Color), 0)

#define ColorGetColor(hWnd) \
    ((COLORREF)SendMessage((hWnd), WM_COLOR_GETCOLOR, 0, 0))


#ifdef __cplusplus
}
#endif
#endif
