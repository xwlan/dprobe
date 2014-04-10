//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#include "treelist.h"
#include "treelistdata.h"

typedef enum _TreeListStyle {
	TreeListTreeStyle =  (WS_CHILD|TVS_HASLINES|TVS_HASBUTTONS|TVS_LINESATROOT),
	TreeListHeaderStyle = (WS_VISIBLE|WS_CHILD|HDS_BUTTONS|HDS_HORZ|HDS_FULLDRAG),
} TreeListStyle;

typedef enum _TreeListId {
	TreeListHeaderId,
	TreeListTreeId,
} TreeListId;

BOOLEAN
TreeListInitialize(
	VOID
	)
{
	WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(wc);

	SdkMainIcon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_APSARA));
    
	wc.hbrBackground  = GetSysColorBrush(COLOR_BTNFACE);
    wc.hCursor        = LoadCursor(0, IDC_ARROW);
    wc.hIcon          = SdkMainIcon;
    wc.hIconSm        = SdkMainIcon;
    wc.hInstance      = SdkInstance;
    wc.lpfnWndProc    = TreeListProcedure;
    wc.lpszClassName  = SDK_TREELIST_CLASS;

    RegisterClassEx(&wc);
    return TRUE;
}

PTREELIST_OBJECT
TreeListCreate(
	IN HWND hWndParent,
	IN BOOLEAN IsInDialog,
	IN INT_PTR MenuOrId,
	IN RECT *Rect,
	IN HWND hWndSort,
	IN TREELIST_FORMAT_CALLBACK Format,
	IN ULONG ColumnCount
	)
{
	HWND hWnd;
	PTREELIST_OBJECT Object;
	LONG Style;
	LONG ExStyle;

	Object = (PTREELIST_OBJECT)SdkMalloc(sizeof(TREELIST_OBJECT));
	ZeroMemory(Object, sizeof(TREELIST_OBJECT));

	Object->HeaderId = TreeListHeaderId;
	Object->TreeId = TreeListTreeId;
	Object->ColumnCount = 0;
	Object->SplitbarLeft = 0;
	Object->SplitbarBorder = 1;
	Object->MenuOrId = MenuOrId;
	Object->hWndSort = hWndSort;
	Object->FormatCallback = Format;

	if (!IsInDialog) {
		Style = WS_VISIBLE | WS_CHILD;
		ExStyle = WS_EX_CLIENTEDGE;
	} else {
		Style = WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS;
		ExStyle = WS_EX_CLIENTEDGE | WS_EX_CONTROLPARENT;
	}

    hWnd = CreateWindowEx(ExStyle, SDK_TREELIST_CLASS, L"", Style,
						  Rect->left, Rect->top, Rect->right - Rect->left, 
						  Rect->bottom - Rect->top, hWndParent, (HMENU)MenuOrId, 
						  SdkInstance, (PVOID)Object);

	Object->hWnd = hWnd;
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);
	return Object;
}

LRESULT CALLBACK 
TreeListProcedure(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp
    )
{
	LRESULT Status;

	switch (uMsg) {

		case WM_NOTIFY:

		    Status = TreeListOnNotify(hWnd, uMsg, wp, lp);
			if (Status) {
				return Status;
			}
			break;

		case WM_CREATE:
			TreeListOnCreate(hWnd, uMsg, wp, lp);
			break;

		case WM_SIZE:
			return TreeListOnSize(hWnd, uMsg, wp, lp);

		case WM_DESTROY:
			TreeListOnDestroy(hWnd, uMsg, wp, lp);
			break;

	}

	return DefWindowProc(hWnd, uMsg, wp, lp);
}

LRESULT CALLBACK 
TreeListTreeProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp,
	IN UINT_PTR uIdSubclass, 
	IN DWORD_PTR dwData
	)
{
	TVHITTESTINFO hti;
	WORD x, y;

	if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK || 
		uMsg == WM_RBUTTONDOWN) {

		x = GET_X_LPARAM(lp); 
		y = GET_Y_LPARAM(lp);
		hti.pt.x = x;
		hti.pt.y = y;

		TreeView_HitTest(hWnd, &hti);

		if(hti.flags == TVHT_ONITEMRIGHT) {

			TreeView_SelectItem(hWnd, hti.hItem);

			if (uMsg == WM_LBUTTONDBLCLK) {
				TreeView_Expand(hWnd, hti.hItem, TVE_TOGGLE);
			}
		}

	}

    return DefSubclassProc(hWnd, uMsg, wp, lp);
} 

LRESULT
TreeListOnCreate(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	CREATESTRUCT *lpcs;
	PTREELIST_OBJECT Object;
	HWND hWndTree;
	HWND hWndHeader;

	lpcs = (CREATESTRUCT *)lp;
	Object = (PTREELIST_OBJECT)lpcs->lpCreateParams;
	Object->hWnd = hWnd;
	SdkSetObject(hWnd, Object);
	
	hWndHeader = CreateWindowEx(0, WC_HEADER, NULL, TreeListHeaderStyle,
								0, 0, 0, 0, hWnd, (HMENU)UlongToPtr(TreeListHeaderId), 
								SdkInstance, NULL);
	Object->hWndHeader = hWndHeader;
	Header_SetUnicodeFormat(hWndHeader, TRUE);
	SdkSetDefaultFont(hWndHeader);

	hWndTree = CreateWindowEx(0, WC_TREEVIEW, NULL, TreeListTreeStyle, 
							  0, 0, 0, 0, 
                              hWnd, (HMENU)UlongToPtr(TreeListTreeId), 
							  SdkInstance, NULL);
	Object->hWndTree = hWndTree;
	TreeView_SetUnicodeFormat(hWndTree, TRUE);

	SetWindowSubclass(hWndTree, TreeListTreeProcedure, 0, (DWORD_PTR)Object);

	Object->ScrollOffset = 0;
	Object->clrBack = GetSysColor(COLOR_WINDOW);
	Object->clrText = GetSysColor(COLOR_WINDOWTEXT);
	Object->clrHighlight = GetSysColor(COLOR_HIGHLIGHTTEXT);
	Object->hBrushBack = GetSysColorBrush(COLOR_HIGHLIGHT);
	Object->hBrushNormal = GetSysColorBrush(COLOR_WINDOW);
	Object->hBrushBar = GetStockObject(BLACK_BRUSH);	

	ShowWindow(hWndTree, SW_SHOW);
	return 0;
}

LRESULT
TreeListOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    )
{
	LPNMHDR pNmhdr = (LPNMHDR)lp;
	PTREELIST_OBJECT Object;

	Object = (PTREELIST_OBJECT)SdkGetObject(hWnd);

	if(Object->hWndTree == pNmhdr->hwndFrom) {

		switch (pNmhdr->code) {

		case NM_CUSTOMDRAW:
			return TreeListOnCustomDraw(Object, pNmhdr);

		case TVN_DELETEITEM:
			return TreeListOnDeleteItem(Object, pNmhdr);

		case TVN_ITEMEXPANDING :
			return TreeListOnItemExpanding(Object, pNmhdr);
		}
	}

	else if(Object->hWndHeader == pNmhdr->hwndFrom) {

		switch (pNmhdr->code) {

		case HDN_BEGINTRACK:
			return TreeListOnBeginTrack(Object, pNmhdr);

		case HDN_TRACK:
			return TreeListOnTrack(Object, pNmhdr);

		case HDN_ENDTRACK:
			return TreeListOnEndTrack(Object, pNmhdr);

		case HDN_DIVIDERDBLCLICK:
			return TreeListOnDividerDbclk(Object, pNmhdr);

		case HDN_ITEMCLICK:
			return TreeListOnItemClick(Object, pNmhdr);
		}
	}

	return 0;
}

LRESULT
TreeListOnCustomDraw(
	IN PTREELIST_OBJECT Object,
	IN LPNMHDR lp
	)
{
	LRESULT Status;
	LPNMTVCUSTOMDRAW lpcd = (LPNMTVCUSTOMDRAW)lp;

	Status = 0;

	switch (lpcd->nmcd.dwDrawStage) {

		case CDDS_PREPAINT:

			SetViewportOrgEx(lpcd->nmcd.hdc, Object->ScrollOffset, 0, NULL);
			Status = CDRF_NOTIFYITEMDRAW;
			break;

		case CDDS_ITEMPREPAINT:

			lpcd->clrText = Object->clrText;
			lpcd->clrTextBk = Object->clrBack;

			Object->ItemFocus = FALSE;
			if(lpcd->nmcd.uItemState & CDIS_FOCUS) {
				Object->ItemFocus = TRUE;
			}

			lpcd->nmcd.uItemState &= ~CDIS_FOCUS;
			TreeView_GetItemRect(Object->hWndTree, (HTREEITEM)lpcd->nmcd.dwItemSpec, 
				                 &Object->ItemRect, TRUE);

			Object->ItemRect.right = min(lpcd->nmcd.rc.right, Object->HeaderWidth);

			Status = CDRF_NOTIFYPOSTPAINT;
			break;

		case CDDS_ITEMPOSTPAINT:

			TreeListDrawItem(Object, lpcd->nmcd.hdc, 
							 (HTREEITEM)lpcd->nmcd.dwItemSpec, 
							 lpcd->nmcd.lItemlParam);
			break;
	}

	return Status;
}

VOID 
TreeListDrawItem(
	IN PTREELIST_OBJECT Object,
	IN HDC hdc, 
	IN HTREEITEM hTreeItem,
	IN LPARAM lp
	)
{
	RECT rc;
	RECT *Rect;
	LONG i;
	WCHAR Buffer[MAX_PATH];

	if (!lp) {
		return;
	}

	Rect = &Object->ItemRect;

	if(Object->ItemFocus) {
		FillRect(hdc, Rect, Object->hBrushBack);
		DrawFocusRect(hdc, Rect);
	}
	else {
		FillRect(hdc, Rect, Object->hBrushNormal);
	}

	//
	// Always write text as transparent 
	//

	SetBkMode(hdc, TRANSPARENT);

	if (Object->ItemFocus) {
		SetTextColor(hdc, Object->clrHighlight);
	} else {
		SetTextColor(hdc, Object->clrText);
	}

	rc = *Rect;

	for(i = 0; i < Object->ColumnCount; i++) {

		if(i != 0) {
			rc.left = Object->Column[i].Rect.left;
		}
		rc.right = Object->Column[i].Rect.right;

		TreeListFormatValue(Object, Object->FormatCallback, hTreeItem, (PVOID)lp, i, Buffer, MAX_PATH);
		DrawText(hdc, Buffer, -1, &rc, DT_BOTTOM|DT_SINGLELINE|DT_WORD_ELLIPSIS|Object->Column[i].Align);
	}
}

VOID
TreeListDrawSplitBar(
	IN PTREELIST_OBJECT Object,
	IN LONG x,
	IN LONG y,
	IN BOOLEAN EndTrack
	)
{
    HDC hdc;
	RECT Rect;
	HBRUSH hBrushOld;

	hdc = GetDC(Object->hWnd);
	if (!hdc) {
        return;
	}

	GetClientRect(Object->hWnd, &Rect);
	hBrushOld = SelectObject(hdc, Object->hBrushBar);

    //
    // Clean the splibar we drew last time 
    //

	if (Object->SplitbarLeft > 0) {
		PatBlt(hdc, Object->SplitbarLeft, y, 
			   Object->SplitbarBorder, 
			   Rect.bottom - Rect.top - y, 
			   PATINVERT );
    }

    if (!EndTrack) {

        Rect.left += x;
		PatBlt(hdc, Rect.left, y, Object->SplitbarBorder, 
			   Rect.bottom - Rect.top - y, PATINVERT);

		Object->SplitbarLeft = Rect.left;

    }
    else {

		Object->SplitbarLeft = 0xffff8001;
    }

	SelectObject(hdc, hBrushOld);
	ReleaseDC(Object->hWnd, hdc);
}

LRESULT
TreeListOnBeginTrack(
	IN PTREELIST_OBJECT Object,
	IN LPNMHDR lp
	)
{
	return 0;
}

LRESULT
TreeListOnTrack(
	IN PTREELIST_OBJECT Object,
	IN LPNMHDR lp
	)
{
	LPNMHEADER phdn;
	RECT Rect;
	LONG x;
	LONG y;

	phdn = (LPNMHEADER)lp;
	Header_GetItemRect(Object->hWndHeader, phdn->iItem, &Rect);

	x = Rect.left + phdn->pitem->cxy;
	y = Rect.bottom - Rect.top;

	TreeListDrawSplitBar(Object, x, y, FALSE); 
	return 0;
}

LRESULT
TreeListOnEndTrack(
	IN PTREELIST_OBJECT Object,
	IN LPNMHDR lp
	)
{
	LPNMHEADER phdn;
	RECT Rect;
	LONG x;
	LONG y;
	
	phdn = (LPNMHEADER)lp;
	Header_GetItemRect(Object->hWndHeader, phdn->iItem, &Rect);

	x = Rect.left + phdn->pitem->cxy;
	y = Rect.bottom - Rect.top;

	TreeListDrawSplitBar(Object, x, y, TRUE); 
	PostMessage(Object->hWnd, WM_SIZE, 0, 0);
	return 0;
}

LRESULT
TreeListOnDividerDbclk(
	IN PTREELIST_OBJECT Object,
	IN LPNMHDR lp
	)
{
	LPNMHEADER phdn;
	LONG x;
	HDITEM Item = {0};

	phdn = (LPNMHEADER)lp;

	if (phdn->iItem == 0) {
		TreeListGetTreeRectMostRight(Object, &x);
	}
	else {
		TreeListGetColumnTextExtent(Object, phdn->iItem, &x);
		x -= GetSystemMetrics(SM_CXSMICON);
	}

	Item.mask = HDI_ORDER | HDI_WIDTH;
	Item.iOrder = phdn->iItem;
	Item.cxy = x;

	Header_SetItem(Object->hWndHeader, phdn->iItem, &Item);
	PostMessage(Object->hWnd, WM_SIZE, 0, 0);
	return 0;
}

LRESULT
TreeListOnItemClick(
	IN PTREELIST_OBJECT Object,
	IN LPNMHDR lp
	)
{
	LPNMHEADER phdn;
	ULONG ColumnCount;
	ULONG Current, i;
	HDITEM hdi = {0};

	if (!Object->hWndSort) {
		return 0;
	}

	phdn = (LPNMHEADER)lp;
	Current = phdn->iItem;

	if (Object->LastClickedColumn == Current) {
		Object->SortOrder = (LIST_SORT_ORDER)!Object->SortOrder;
    } else {
		Object->SortOrder = SortOrderAscendent;
    }
    
    ColumnCount = Header_GetItemCount(Object->hWndHeader);
    
    for (i = 0; i < ColumnCount; i++) {

        hdi.mask = HDI_FORMAT;
        Header_GetItem(Object->hWndHeader, i, &hdi);
        
        hdi.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);

        if (i == Current) {

            if (Object->SortOrder == SortOrderAscendent){
                hdi.fmt |= HDF_SORTUP;
            } else {
                hdi.fmt |= HDF_SORTDOWN;
            }

        } 
        
        Header_SetItem(Object->hWndHeader, i, &hdi);
    }
    
	Object->LastClickedColumn = Current;
	PostMessage(Object->hWndSort, WM_TREELIST_SORT, (WPARAM)Current, 0);
	return 0;
}

VOID
TreeListGetColumnTextExtent(
	IN PTREELIST_OBJECT Object,
	IN ULONG Column,
	OUT PLONG Extent 
	)
{
	HTREEITEM hTreeItem;
	HWND hWndTree;
	SIZE Size;
	SIZE_T Length;
	HDC hdc;
	TVITEM tvi = {0};
	WCHAR Buffer[MAX_PATH];

	*Extent = 0;
	hWndTree = Object->hWndTree;
	hdc = GetDC(Object->hWndTree);

	hTreeItem = TreeView_GetFirstVisible(hWndTree);

	while (hTreeItem != NULL) {

		tvi.mask = TVIF_PARAM;
		TreeView_GetItem(hWndTree, &tvi);

		TreeListFormatValue(Object, Object->FormatCallback, hTreeItem, 
			               (PVOID)tvi.lParam, Column, Buffer, MAX_PATH);

		StringCchLength(Buffer, MAX_PATH, &Length);
		GetTextExtentPoint32(hdc, Buffer, (int)Length, &Size);

		*Extent = max(*Extent, Size.cx);

		hTreeItem = TreeView_GetNextVisible(hWndTree, hTreeItem);
	}
	
}

VOID
TreeListGetTreeRectMostRight(
	IN PTREELIST_OBJECT Object,
	OUT PLONG MostRight
	)
{
	HTREEITEM hTreeItem;
	HWND hWndTree;
	RECT Rect;

	hWndTree = Object->hWndTree;
	hTreeItem = TreeView_GetFirstVisible(hWndTree);
	*MostRight = 0;

	while (hTreeItem != NULL) {
		TreeView_GetItemRect(hWndTree, hTreeItem, &Rect, TRUE);
		*MostRight = max(Rect.right, *MostRight);
		hTreeItem = TreeView_GetNextVisible(hWndTree, hTreeItem);
	}
}

LRESULT
TreeListOnDeleteItem(
	IN PTREELIST_OBJECT Object,
	IN LPNMHDR lp
	)
{
	NMTREEVIEW* lptv = (NMTREEVIEW*)lp;
	return TRUE;
}

LRESULT
TreeListOnItemExpanding(
	IN PTREELIST_OBJECT Object,
	IN LPNMHDR lp
	)
{

#if defined (_M_X64)

	//
	// N.B. 64 bits tree custom draw has problem, we work around
	// by forcing redraw client area, 32 bits has no such problem,
	// it's under low priority investigation.
	//

	RECT Rect;
	POINT Point;
	
	GetCursorPos(&Point);
	GetClientRect(Object->hWndTree, &Rect);
	Rect.bottom = Point.y;

	InvalidateRect(Object->hWndTree, &Rect, TRUE);

#endif

	return FALSE;	
}

LRESULT
TreeListOnDestroy(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    )
{
	PTREELIST_OBJECT Object;
	Object = (PTREELIST_OBJECT)SdkGetObject(hWnd);
	SdkFree(Object);
	return 0;
}

VOID
TreeListComputeClientArea(
	IN RECT *Base,
	IN RECT *Tree,
	IN RECT *Header
	)
{
	*Header = *Base;
	Header->bottom = Header->top + GetSystemMetrics(SM_CYMENU);

	*Tree = *Base;
	Tree->top = Header->bottom;
}

VOID
TreeListUpdateColumn(
	IN PTREELIST_OBJECT Object	
	)
{
	HDITEM hdi;
	LONG i;

	Object->HeaderWidth = 0;

	for(i = 0; i < Object->ColumnCount; i++) {

		hdi.mask = HDI_WIDTH | HDI_LPARAM | HDI_FORMAT;
		Header_GetItem(Object->hWndHeader, i, &hdi);
		Object->Column[i].Width = hdi.cxy;

		if(i == 0) {
			Object->Column[i].Rect.left = 2;
		}
		else {
			Object->Column[i].Rect.left = Object->Column[i - 1].Rect.right + 2;
		}

		Object->Column[i].Rect.right = Object->Column[i].Rect.left  + 
			                           Object->Column[i].Width - 2;

		switch(hdi.fmt & HDF_JUSTIFYMASK) {

		case HDF_RIGHT:
			Object->Column[i].Align = DT_RIGHT;
			break;

		case HDF_CENTER:
			Object->Column[i].Align = DT_CENTER;
			break;
		case HDF_LEFT:
			Object->Column[i].Align = DT_LEFT;
			break;

		default:
			Object->Column[i].Align = DT_LEFT;
		}

		Object->HeaderWidth += Object->Column[i].Width;
	}
}

LRESULT
TreeListOnSize(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    )
{
	PTREELIST_OBJECT Object;
	RECT Base;
	RECT Tree;
	RECT Header;
	WPARAM Code;
	LONG Offset;
	
	Object = (PTREELIST_OBJECT)SdkGetObject(hWnd);
	Offset = Object->ScrollOffset;

	GetClientRect(Object->hWndTree, &Tree); 
	InvalidateRect(Object->hWndTree, &Tree, TRUE);

	TreeListResetScrollBar(Object);
	TreeListUpdateColumn(Object);

	GetClientRect(Object->hWnd, &Base);
	TreeListComputeClientArea(&Base, &Tree, &Header);

	SetWindowPos(Object->hWndHeader, HWND_TOP, 
		         Header.left, Header.top,
				 Header.right - Header.left,
				 Header.bottom - Header.top, 
				 SWP_NOZORDER);

	SetWindowPos(Object->hWndTree, HWND_TOP, 
		         Tree.left, Tree.top, 
				 Tree.right - Tree.left,
				 Tree.bottom - Tree.top,
				 SWP_NOZORDER);

	Code = (Offset << 16)  | (SB_THUMBPOSITION);
	SendMessage(Object->hWnd, WM_HSCROLL, Code, 0);

	InvalidateRect(Object->hWnd, &Base, FALSE);
	return 0;
}

LRESULT
TreeListOnTimer(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
    )
{
	return 0;
}

VOID
TreeListResetScrollBar(
	IN PTREELIST_OBJECT Object
	)
{
	RECT Rect;
	LONG ColumnCount;
	HDITEM HeaderItem;
	LONG TreeWidth;
	int i;

	HeaderItem.mask = HDI_WIDTH;

	GetClientRect(Object->hWndTree, &Rect);
	Object->HeaderWidth = 0;

	ColumnCount = Header_GetItemCount(Object->hWndHeader);

	for(i = 0; i < ColumnCount; i++) {
		Header_GetItem(Object->hWndHeader, i, &HeaderItem);
		Object->HeaderWidth += HeaderItem.cxy;
	}

	TreeWidth = Rect.right - Rect.left;

	//
	// if the width of header is beyond the limit of tree's client area
	// scrollbar is enabled
	//

	if(Object->HeaderWidth > TreeWidth) {
		SetScrollRange(Object->hWnd, SB_HORZ, 0, Object->HeaderWidth - TreeWidth, TRUE);
		SetScrollPos(Object->hWnd, SB_HORZ, 0, TRUE);
		SdkModifyStyle(Object->hWnd, 0, WS_HSCROLL, FALSE);
		Object->ScrollOffset = 0;
	}
	else {
		SetScrollRange(Object->hWnd, SB_HORZ, 0, 0, TRUE);
		SetScrollPos(Object->hWnd, SB_HORZ, 0, TRUE);
		SdkModifyStyle(Object->hWnd, WS_HSCROLL, 0, FALSE);
		Object->ScrollOffset = 0;
	}

}

LONG
TreeListInsertColumn(
	IN PTREELIST_OBJECT Object,
	IN LONG Column, 
	IN LONG Align,
	IN LONG Width,
	IN PWSTR Title
	)
{
	HDITEM hdi = {0};

	hdi.mask = HDI_WIDTH | HDI_TEXT | HDI_FORMAT | HDI_LPARAM;
	hdi.cxy = Width;
	hdi.fmt = Align;
	hdi.pszText = Title;
	hdi.cchTextMax = MAX_PATH;
	hdi.lParam = (LPARAM)&Object->Column[Column];

	Header_InsertItem(Object->hWndHeader, Column, &hdi);
	Object->ColumnCount += 1;

	TreeListUpdateColumn(Object);
	return Column;
}

LONG
TreeListRoundUpPosition(
	IN LONG Position,
	IN LONG Unit
	)
{
	return (Position / Unit + 1) * Unit;
}

VOID
TreeListOnHScroll(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	RECT Rect;
	RECT TreeRect;
	RECT HeaderRect; 
	LONG WidthPage;
	LONG WidthLine;
	LONG CurrentPos;
	LONG PreviousPos;
	LONG ScrollMin;
	LONG ScrollMax;
	PTREELIST_OBJECT Object;
	SCROLLINFO ScrollInfo = {0};

	Object = (PTREELIST_OBJECT)SdkGetObject(hWnd);
	GetClientRect(Object->hWndTree, &Rect);

	WidthPage = abs(Rect.right - Rect.left);	
	WidthLine = 6;	
	CurrentPos = GetScrollPos(Object->hWnd, SB_HORZ);
	PreviousPos	= CurrentPos;	

	GetScrollRange(Object->hWnd, SB_HORZ, &ScrollMin, &ScrollMax);

	switch(LOWORD(wp))
	{
	case SB_LEFT:
		{
			CurrentPos = 0;
			break;
		}
	case SB_RIGHT:	
		{
			CurrentPos = ScrollMax;
			break;
		}
	case SB_LINELEFT:							
		{
			CurrentPos = max(CurrentPos - WidthLine, 0);
			break;
		}
	case SB_LINERIGHT:	
		{
			CurrentPos = min(CurrentPos + WidthLine, ScrollMax);
			break;
		}
	case SB_PAGELEFT:
		{
			CurrentPos = max(CurrentPos - WidthPage, 0);
			break;
		}
	case SB_PAGERIGHT:
		{
			CurrentPos = min(CurrentPos + WidthPage, ScrollMax);
			break;
		}
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		{
            ScrollInfo.cbSize = sizeof(ScrollInfo);
            ScrollInfo.fMask = SIF_TRACKPOS;
 
			GetScrollInfo(Object->hWnd, SB_HORZ, &ScrollInfo);

			if(ScrollInfo.nTrackPos == 0)
			{
				CurrentPos = 0;
			}
			else
			{
				CurrentPos = min(TreeListRoundUpPosition(ScrollInfo.nTrackPos, WidthLine), ScrollMax);
			}
			break;
		}
	default:
		{
			break;
		}
	};

	SetScrollPos(Object->hWnd, SB_HORZ, CurrentPos, TRUE);
	Object->ScrollOffset = -CurrentPos;

	GetClientRect(Object->hWndTree, &TreeRect);
	ScrollWindowEx(Object->hWndTree, PreviousPos - CurrentPos, 
		           0, NULL, NULL, NULL, NULL, SW_INVALIDATE);

	GetClientRect(Object->hWnd, &Rect);
	GetClientRect(Object->hWndHeader, &HeaderRect);

	if(TreeRect.right - TreeRect.left != 0) {

		LONG TreeWidth;

		TreeWidth = TreeRect.right - TreeRect.left;
		TreeWidth = TreeListRoundUpPosition(Object->HeaderWidth, TreeWidth);

		SetWindowPos(Object->hWndHeader, 
			         HWND_TOP, 
					 Object->ScrollOffset, 0, 
					 max(TreeWidth, WidthPage), 
					 HeaderRect.bottom - HeaderRect.top, 
					 SWP_SHOWWINDOW);
	}

	InvalidateRect(Object->hWndTree, &TreeRect, TRUE);
}