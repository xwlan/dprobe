//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#ifndef _TREELIST_H_
#define _TREELIST_H_

#include "sdk.h"
#include "treelistdata.h"
#include "listview.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDK_TREELIST_CLASS  L"SdkTreeList"

//
// TreeList sort call notification
//

#define WM_TREELIST_SORT  (WM_USER + 1)

typedef int 
(CALLBACK *TREELIST_COMPARE_CALLBACK)(
	IN LPARAM First, 
	IN LPARAM Second, 
	IN LPARAM Current 
	);

typedef struct _HEADER_COLUMN {
	RECT Rect;
	LONG Width;
	LONG Align;
	BOOL Hide;
	WCHAR Name[16];
} HEADER_COLUMN, *PHEADER_COLUMN;

typedef struct _TREELIST_OBJECT {

	HWND hWnd;
	BOOLEAN IsInDialog;
	INT_PTR MenuOrId;

	HWND hWndHeader;
	LONG HeaderId;
	LONG HeaderWidth;
	LONG ColumnCount;
	HEADER_COLUMN Column[16];

	HWND hWndTree;
	LONG TreeId;
	COLORREF clrBack;
	COLORREF clrText;
	COLORREF clrHighlight;
	HBRUSH hBrushBack;
	HBRUSH hBrushNormal;
	HBRUSH hBrushBar;

	LONG ScrollOffset;

	LONG SplitbarLeft;
	LONG SplitbarBorder;

	LIST_SORT_ORDER SortOrder;
	int LastClickedColumn;
	HWND hWndSort;

	TREELIST_FORMAT_CALLBACK FormatCallback;

	RECT ItemRect;
	BOOLEAN ItemFocus;
	PVOID Context;

} TREELIST_OBJECT, *PTREELIST_OBJECT;

BOOLEAN
TreeListInitialize(
	VOID
	);

PTREELIST_OBJECT
TreeListCreate(
	IN HWND hWndParent,
	IN BOOLEAN IsInDialog,
	IN INT_PTR MenuOrId,
	IN RECT *Rect,
	IN HWND hWndSort,
	IN TREELIST_FORMAT_CALLBACK Format,
	IN ULONG ColumnCount
	);

LRESULT CALLBACK 
TreeListProcedure(
    IN HWND hwnd,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

LRESULT CALLBACK 
TreeListTreeProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp,
	UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData
	);

LRESULT
TreeListOnCreate(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
TreeListOnDestroy(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    );

LRESULT
TreeListOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    );

LRESULT
TreeListOnCustomDraw(
	IN PTREELIST_OBJECT Object,
	IN LPNMHDR lp
	);

LRESULT
TreeListOnBeginTrack(
	IN PTREELIST_OBJECT Object,
	IN LPNMHDR lp
	);

LRESULT
TreeListOnTrack(
	IN PTREELIST_OBJECT Object,
	IN LPNMHDR lp
	);

LRESULT
TreeListOnEndTrack(
	IN PTREELIST_OBJECT Object,
	IN LPNMHDR lp
	);

LRESULT
TreeListOnDividerDbclk(
	IN PTREELIST_OBJECT Object,
	IN LPNMHDR lp
	);

LRESULT
TreeListOnItemClick(
	IN PTREELIST_OBJECT Object,
	IN LPNMHDR lp
	);

VOID
TreeListGetTreeRectMostRight(
	IN PTREELIST_OBJECT Object,
	OUT PLONG MostRight
	);

VOID
TreeListGetColumnTextExtent(
	IN PTREELIST_OBJECT Object,
	IN ULONG Column,
	OUT PLONG Extent 
	);

VOID
TreeListDrawSplitBar(
	IN PTREELIST_OBJECT Object,
	IN LONG x,
	IN LONG y,
	IN BOOLEAN EndTrack
	);

VOID 
TreeListDrawItem(
	IN PTREELIST_OBJECT Object,
	IN HDC hdc, 
	IN HTREEITEM hTreeItem,
	IN LPARAM lp
	);

LRESULT
TreeListOnDeleteItem(
	IN PTREELIST_OBJECT Object,
	IN LPNMHDR lp
	);

LRESULT
TreeListOnItemExpanding(
	IN PTREELIST_OBJECT Object,
	IN LPNMHDR lp
	);

LRESULT
TreeListOnPaint(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    );

LRESULT
TreeListOnSize(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    );

LRESULT
TreeListOnTimer(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    );

VOID
TreeListOnHScroll(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

VOID
TreeListResetScrollBar(
	IN PTREELIST_OBJECT Object
	);

LONG
TreeListInsertColumn(
	IN PTREELIST_OBJECT Object,
	IN LONG Column, 
	IN LONG Align,
	IN LONG Width,
	IN PWSTR Title
	);

#ifdef __cplusplus
}
#endif

#endif