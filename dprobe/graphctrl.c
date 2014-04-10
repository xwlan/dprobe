//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#include "graphctrl.h"
#include <math.h>

#define GRAPH_TIMER_ID    6
#define SDK_GRAPH_CLASS   L"SdkGraph"

BOOLEAN 
GraphInitialize(
	VOID
	)
{
    WNDCLASSEX wc = {0};

	wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = GraphProcedure;
    wc.hInstance = SdkInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = SDK_GRAPH_CLASS;

	if (!RegisterClassEx(&wc)) {
        return FALSE;
	}

    return TRUE;
}

PGRAPH_CONTROL
GraphCreateControl(
    IN HWND hWndParent,
    IN INT_PTR CtrlId,
	IN LPRECT Rect,
	IN COLORREF crBack,
	IN COLORREF crGrid,
	IN COLORREF crFill,
	IN COLORREF crEdge
    )
{
	HWND hWnd;
	PGRAPH_CONTROL Object;

	Object = (PGRAPH_CONTROL)SdkMalloc(sizeof(GRAPH_CONTROL));
	ZeroMemory(Object, sizeof(GRAPH_CONTROL));

	Object->CtrlId = CtrlId;
	Object->BackColor = crBack;
	//Object->GridColor = RGB(0x08, 0x80, 0x10);
	Object->GridColor = crGrid;
	Object->FillColor = crFill;
	Object->EdgeColor = crEdge;
	Object->TimerPeriod = 1000;
	Object->TimerId = GRAPH_TIMER_ID;
	Object->FirstPos = 12;

 //   hWnd = CreateWindowEx(WS_EX_CLIENTEDGE, SDK_GRAPH_CLASS, L"", 
    hWnd = CreateWindowEx(0, SDK_GRAPH_CLASS, L"", 
						  WS_VISIBLE | WS_CHILD,
						  Rect->left, Rect->top, 
						  Rect->right - Rect->left, 
						  Rect->bottom - Rect->top, 
						  hWndParent, (HMENU)CtrlId, 
						  SdkInstance, Object);
	return Object;
}

LRESULT
GraphOnCreate(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PGRAPH_CONTROL Object;
	LPCREATESTRUCT lpCreate;
	
	lpCreate = (LPCREATESTRUCT)lp;
	Object = (PGRAPH_CONTROL)lpCreate->lpCreateParams;
	ASSERT(Object != NULL);

	Object->hWnd = hWnd;
	SdkSetObject(hWnd, Object);

	ShowWindow(hWnd, SW_SHOW);
	GraphInvalidate(hWnd);

	return 0;
}

LRESULT
GraphOnPaint(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    )
{
	PAINTSTRUCT Paint;
	PGRAPH_CONTROL Object;
	HDC hdc;
	RECT Rect;

	Object = (PGRAPH_CONTROL)SdkGetObject(hWnd);
	ASSERT(Object != NULL);
	ASSERT(Object->hdcGraph != NULL);

	GetClientRect(hWnd, &Rect);
	
	hdc = BeginPaint(hWnd, &Paint);
	BitBlt(hdc, 0, 0, Rect.right, Rect.bottom, Object->hdcGraph, 0, 0, SRCCOPY);
	EndPaint(hWnd, &Paint);

	return 0;
}

LRESULT
GraphOnSize(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    )
{
	PGRAPH_CONTROL Object;

	Object = (PGRAPH_CONTROL)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

	GraphInvalidate(hWnd);
	return 0;
}

LRESULT
GraphOnTimer(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    )
{
	PGRAPH_CONTROL Object;

	Object = (PGRAPH_CONTROL)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

	GraphDraw(Object);
	return 0L;
}

LRESULT
GraphOnDestroy(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    )
{
	PGRAPH_CONTROL Object;

	Object = (PGRAPH_CONTROL)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

	if (Object->hbmpGraph) {
		DeleteObject(Object->hbmpGraph);
	}

	if (Object->hdcGraph) {
		DeleteDC(Object->hdcGraph);
	}

	Object->HistoryCallback = NULL;
	SdkFree(Object);

	SdkSetObject(hWnd, NULL);
	return 0L;
}

LRESULT CALLBACK 
GraphProcedure(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp
    )
{
    switch (uMsg) {

	case WM_CREATE:
		GraphOnCreate(hWnd, uMsg, wp, lp);
		break;

    case WM_DESTROY:
		GraphOnDestroy(hWnd, uMsg, wp, lp);
        break;

    case WM_SIZE:
		GraphOnSize(hWnd, uMsg, wp, lp);
        break;

    case WM_PAINT:
		GraphOnPaint(hWnd, uMsg, wp, lp); 
        break;

	case WM_TIMER:
		GraphOnTimer(hWnd, uMsg, wp, lp);
		break;

    case WM_ERASEBKGND:
        return TRUE;
    }

    return DefWindowProc(hWnd, uMsg, wp, lp);
}

BOOLEAN
GraphInvalidate(
	IN HWND hWnd
	)
{
	HDC hdc;
	HDC hdcGraph;
	HBITMAP hbmp;
	HBITMAP hbmpOld;	
	HBRUSH hBrush;
	HPEN hPen;
	PGRAPH_CONTROL Object;
	RECT Rect;
	POINT Point;
	int i;
	LONG Width, Height;
	LONG Grid;

	Object = (PGRAPH_CONTROL)SdkGetObject(hWnd);
	ASSERT(Object != NULL);

	if (Object->hbmpGraph) {
		ASSERT(Object->hdcGraph != NULL);
		DeleteObject(Object->hbmpGraph);
		SelectObject(Object->hdcGraph, Object->hbmpGraphOld);
	}
	
	if (Object->hdcGraph) {
		DeleteDC(Object->hdcGraph);
	}

	hdc = GetDC(hWnd);
	hdcGraph = CreateCompatibleDC(hdc);
	
	GetClientRect(hWnd, &Rect);
	Width = Rect.right - Rect.left;
	Height = Rect.bottom - Rect.top;

	hbmp = CreateCompatibleBitmap(hdc, Width, Height);
	hbmpOld = (HBITMAP)SelectObject(hdcGraph, hbmp);

	hBrush = CreateSolidBrush(Object->BackColor);
	FillRect(hdcGraph, &Rect, hBrush);
	DeleteObject(hBrush);

	hPen = CreatePen(PS_SOLID, 1, Object->GridColor);
	SelectObject(hdcGraph, hPen);

	/*
	Grid = Width / 12;
	for(i = Rect.left + Grid; i < Rect.right; i += Grid) {
		MoveToEx(hdcGraph, i, Rect.top, &Point);
		LineTo(hdcGraph, i, Rect.bottom);
	}*/

	//Grid = Height / 10;
	Grid = Height / 4;
	for(i = Rect.top; i < Rect.bottom; i += Grid) {
		MoveToEx(hdcGraph, Rect.left, i, &Point);
		LineTo(hdcGraph, Rect.right, i);
	}

	MoveToEx(hdcGraph, 0, 0, NULL);
	LineTo(hdcGraph, 0, Rect.bottom);

	MoveToEx(hdcGraph, Rect.right - 1, 0, NULL);
	LineTo(hdcGraph, Rect.right - 1, Rect.bottom - 1);

	DeleteObject(hPen);

	Object->hdcGraph = hdcGraph;
	Object->hbmpGraph = hbmp;
	Object->hbmpGraphOld = hbmpOld;

	InvalidateRect(hWnd, &Rect, TRUE);
	return TRUE;
}

ULONG
GraphStart(
	IN HWND hWnd
	)
{
	PGRAPH_CONTROL Object;	

	Object = (PGRAPH_CONTROL)SdkGetObject(hWnd);
	ASSERT(Object != NULL);
	
	Object->TimerId = SetTimer(hWnd, Object->TimerId, Object->TimerPeriod, NULL);
	return TRUE;
}

VOID
GraphDraw(
	IN PGRAPH_CONTROL Object
	)
{
	RECT Rect;
	HDC hdc;
	HBRUSH hBrush;
	HBRUSH hBrushOld;
	HPEN hPen, hPenOld;
	HRGN hPolygon;
	LONG Width, Height;
	LONG Grid;
	LONG Move;
	POINT Point[61];
	POINT Point2[63];
	PGRAPH_HISTORY Primary;
	PGRAPH_HISTORY Secondary;
	PGRAPH_HISTORY History;
	FLOAT Data;
	LONG Last;
	LONG Count;
	LONG i, j;

	ASSERT(Object != NULL);
	ASSERT(Object->hdcGraph != NULL);

	ASSERT(Object->HistoryCallback != NULL);
	(*Object->HistoryCallback)(Object, &Primary, &Secondary, Object->HistoryContext);
	
	History = Primary;
	if (!History) {
		return;
	}

	hdc = Object->hdcGraph;

	GetClientRect(Object->hWnd, &Rect);

	Width = Rect.right - Rect.left;
	Height = Rect.bottom - Rect.top;
	Move = Width / 60;
	Grid = Width / 12;

	hBrush = CreateSolidBrush(Object->BackColor);
	hBrushOld = SelectObject(hdc, hBrush);
	FillRect(hdc, &Rect, hBrush);

	DeleteObject(hBrush);
	SelectObject(hdc, hBrushOld);

	Object->FirstPos -= Move;
	if(Object->FirstPos < 0) {
		Object->FirstPos += Grid;
	}
	
	i = j = 0;

	if (History->Full != TRUE) {
		
		Last = History->LastValue;
		for (i = Last; i >= 0; i--) {
			Point[i].x = Rect.right - (Last - i) * Move;	
			Data = Height * (1 - History->Value[i] / History->Limits);
			if (ceil(Data) - Data < 0.5) {
				Point[i].y = (ULONG)ceil(Data);
			} else {
				Point[i].y = (ULONG)floor(Data);
			}
		}
		
		memcpy(Point2, Point, sizeof(POINT) * (Last + 1));

		Point2[Last + 1].x = Rect.right;
		Point2[Last + 1].y = Rect.bottom;
		Point2[Last + 2].x = Point[0].x;
		Point2[Last + 2].y = Rect.bottom;

		Count = History->LastValue + 1;
	}

	else {
		
		Last = History->LastValue;
		for (i = 0; i < 61 - (Last + 1); i++) {
			Point[i].x = Rect.left + i * Move;	
			Data = Height * (1 - History->Value[Last + i + 1] / History->Limits);
			if (ceil(Data) - Data < 0.5) {
				Point[i].y = (ULONG)ceil(Data);
			} else {
				Point[i].y = (ULONG)floor(Data);
			}
		}
		
		for (j = 0; j < Last + 1; j++) {
			Point[i + j].x = Rect.left + (i + j) * Move;	
			Data = Height * (1 - History->Value[j] / History->Limits);
			if (ceil(Data) - Data < 0.5) {
				Point[i + j].y = (ULONG)ceil(Data);
			} else {
				Point[i + j].y = (ULONG)floor(Data);
			}
		}

		memcpy(Point2, Point, 61 * sizeof(POINT));
		
		Point2[60].x = Rect.right;
		Point2[61].x = Rect.right;
		Point2[61].y = Rect.bottom;
		Point2[62].x = Point[0].x;
		Point2[62].y = Rect.bottom;

		Count = 61;
		
	}

	//
	// Fill the ploygon region combined by sample points
	//

	hBrush = CreateSolidBrush(Object->FillColor);
	hBrushOld = SelectObject(hdc, hBrush);

	hPolygon = CreatePolygonRgn(Point2, Count + 2, ALTERNATE);
	FillRgn(hdc, hPolygon, hBrush);
	
	SelectObject(hdc, hBrushOld);
	DeleteObject(hBrush);
	DeleteObject(hPolygon);

	//
	// Fill the grid lines
	//

	hPen = CreatePen(PS_SOLID, 1, Object->GridColor);
	hPenOld = SelectObject(hdc, hPen);
	
	Grid = Height / 4;
	for(i = Rect.top; i < Rect.bottom; i += Grid) {
		MoveToEx(hdc, Rect.left, i, NULL);
		LineTo(hdc, Rect.right, i);
	}

	MoveToEx(hdc, 0, 0, NULL);
	LineTo(hdc, 0, Rect.bottom);

	MoveToEx(hdc, Rect.right - 1, 0, NULL);
	LineTo(hdc, Rect.right - 1, Rect.bottom - 1);

	SelectObject(hdc, hPenOld);
	DeleteObject(hPen);

	//
	// Draw the edge use deep color pen
	//

	hPen = CreatePen(PS_SOLID, 1, Object->EdgeColor);
	hPenOld = SelectObject(hdc, hPen);

	if (!History->Full) {
		Polyline(hdc, Point, Count);
	} else {
		Point[60].x = Rect.right - 1;
		Polyline(hdc, Point, Count);
	}

	SelectObject(hdc, hPenOld);
	DeleteObject(hPen);

	InvalidateRect(Object->hWnd, &Rect, TRUE);
}