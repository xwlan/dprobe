//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#ifndef _GRAPHCTRL_H_
#define _GRAPHCTRL_H_

#include "dprobe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GRAPH_HISTORY{
	FLOAT Value[61];
	ULONG ValueTag;
	BOOLEAN Full;
	LONG LastValue;
	FLOAT MinimumValue;
	FLOAT MaximumValue;
	FLOAT Limits;
	FLOAT PreviousLimits;
	FLOAT Increment;
	FLOAT Average;
	COLORREF Color;
} GRAPH_HISTORY, *PGRAPH_HISTORY;

typedef struct _GRAPH_CONTROL *PGRAPH_CONTROL;

typedef ULONG 
(CALLBACK *GRAPH_HISTORY_CALLBACK)(
	IN PGRAPH_CONTROL Object,
	OUT PGRAPH_HISTORY *Primary,
	OUT PGRAPH_HISTORY *Secondary,
	IN PVOID Context
	);

typedef struct _GRAPH_CONTROL {
	HWND hWnd;
	INT_PTR CtrlId;
	LONG_PTR TimerId;
	LONG TimerPeriod;
    COLORREF BackColor;
	COLORREF GridColor;
	COLORREF FillColor;
	COLORREF EdgeColor;
	LONG FirstPos;
	HDC hdcGraph;
    HBITMAP hbmpGraph;
    HBITMAP hbmpGraphOld;
	ULONG HistoryCount;
	GRAPH_HISTORY_CALLBACK HistoryCallback;
	PVOID HistoryContext;
} GRAPH_CONTROL, *PGRAPH_CONTROL;

BOOLEAN
GraphInitialize(
	VOID
	);

PGRAPH_CONTROL
GraphCreateControl(
    IN HWND hWndParent,
    IN INT_PTR CtrlId,
	IN LPRECT Rect,
	IN COLORREF crBack,
	IN COLORREF crGrid,
	IN COLORREF crFill,
	IN COLORREF crEdge
    );

LRESULT CALLBACK 
GraphProcedure(
    IN HWND hwnd,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

LRESULT
GraphOnCreate(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
GraphOnDestroy(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    );

LRESULT
GraphOnPaint(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    );

LRESULT
GraphOnSize(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    );

LRESULT
GraphOnTimer(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    );

BOOLEAN
GraphInvalidate(
	IN HWND hWnd
	);

ULONG
GraphStart(
	IN HWND hWnd
	);

VOID
GraphDraw(
	IN PGRAPH_CONTROL Object
	);

#ifdef __cplusplus
}
#endif

#endif