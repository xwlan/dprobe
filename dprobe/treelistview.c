//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
// 

#include "sdk.h"
#include "listview.h"
#include "treelistview.h"

LRESULT CALLBACK 
TreeListProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wParam, 
	IN LPARAM lParam, 
	IN UINT_PTR uIdSubclass, 
	IN DWORD_PTR dwRefData
	)
{
	switch (uMsg) {
		case WM_LBUTTONDOWN:
			TreeListOnLButtonDown(hWnd, wParam, lParam);
			break;
		case WM_LBUTTONDBLCLK:
			TreeListOnLButtonDbClick(hWnd, wParam, lParam);
			break;
		case WM_KEYDOWN:
			TreeListOnKeyDown(hWnd, wParam, lParam);
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

HWND 
TreeListCreate(
	IN HWND hWndParent, 
	IN UINT CtrlId,
	IN PLISTVIEW_COLUMN Column,
	IN ULONG ColumnCount,
	IN PTREE_NODE Root
	)
{
	HWND hWnd; 
    HIMAGELIST himlState; 
	HICON hicon;
	ULONG Style;
	PTREELIST_OBJECT Object;
	
	hWnd = CreateWindowEx(0, WC_LISTVIEW, 0, 
						  WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER, 
						  0, 0, 0, 0, hWndParent, (HMENU)UlongToHandle(CtrlId), 
						  SdkInstance, NULL);
	ASSERT(hWnd != NULL);

	//
	// Adjust its style to be report and unicode text
	//

    ListView_SetUnicodeFormat(hWnd, TRUE);
    ListView_SetExtendedListViewStyleEx(hWnd, LVS_EX_FULLROWSELECT,  LVS_EX_FULLROWSELECT);
	
	//
	// Create state image list, insert expand and collapse state icon
	//

	himlState = ImageList_Create(16, 16, ILC_MASK, 2, 0); 

    hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_COLLAPSED)); 
    ImageList_AddIcon(himlState, hicon); 
    hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_EXPANDED)); 
    ImageList_AddIcon(himlState, hicon); 

	ListView_SetImageList(hWnd, himlState, LVSIL_STATE);

    Style = (ULONG)GetWindowLongPtr(hWnd, GWL_STYLE);
	SetWindowLongPtr(hWnd, GWL_STYLE, (Style & ~LVS_TYPEMASK) | LVS_REPORT);

	//
	// Create tree list columns
	//

	ListViewCreateColumns(hWnd, Column, ColumnCount);
	
	//
	// Create treelist object and bind it to its hwnd
	//

	Object = (PTREELIST_OBJECT)SdkMalloc(sizeof(TREELIST_OBJECT));
	Object->hWnd = hWnd;
	Object->CtrlId = CtrlId;
	Object->Columns = Column;
	Object->ColumnCount = ColumnCount;
	Object->himlSmall = NULL;
	Object->himlState = himlState;
	Object->RootNode = Root;

	SdkSetObject(hWnd, Object);

	//
	// Insert tree list items
	//

	TreeListBuildTree(hWnd, Root, 0, 0);
	SetWindowSubclass(hWnd, TreeListProcedure, 0, 0);
	return hWnd;
}

HWND 
TreeListInitialize(
	IN HWND hWndParent, 
	IN UINT ResourceId,
	IN PLISTVIEW_COLUMN Column,
	IN ULONG ColumnCount,
	IN PTREE_NODE Root
	)
{
	HWND hWnd; 
    HIMAGELIST himlState; 
	HICON hicon;
	ULONG Style;
	PTREELIST_OBJECT Object;
	
	hWnd = GetDlgItem(hWndParent, ResourceId);

	ASSERT(hWnd != NULL);

	//
	// Adjust its style to be report and unicode text
	//

    ListView_SetUnicodeFormat(hWnd, TRUE);
    ListView_SetExtendedListViewStyleEx(hWnd, LVS_EX_FULLROWSELECT,  LVS_EX_FULLROWSELECT);
	
	//
	// Create state image list, insert expand and collapse state icon
	//

	himlState = ImageList_Create(16, 16, ILC_MASK, 2, 0); 

    hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_COLLAPSED)); 
    ImageList_AddIcon(himlState, hicon); 
    hicon = LoadIcon(SdkInstance, MAKEINTRESOURCE(IDI_ICON_EXPANDED)); 
    ImageList_AddIcon(himlState, hicon); 

	ListView_SetImageList(hWnd, himlState, LVSIL_STATE);

    Style = (ULONG)GetWindowLongPtr(hWnd, GWL_STYLE);
	SetWindowLongPtr(hWnd, GWL_STYLE, (Style & ~LVS_TYPEMASK) | LVS_REPORT);

	//
	// Create tree list columns
	//

	ListViewCreateColumns(hWnd, Column, ColumnCount);
	
	//
	// Create treelist object and bind it to its hwnd
	//

	Object = (PTREELIST_OBJECT)SdkMalloc(sizeof(TREELIST_OBJECT));
	Object->hWnd = hWnd;
	Object->CtrlId = ResourceId;
	Object->Columns = Column;
	Object->ColumnCount = ColumnCount;
	Object->himlSmall = NULL;
	Object->himlState = himlState;
	Object->RootNode = Root;

	SdkSetObject(hWnd, Object);

	//
	// Insert tree list items
	//

	TreeListBuildTree(hWnd, Root, 0, 0);
	SetWindowSubclass(hWnd, TreeListProcedure, 0, 0);
	return hWnd;
}

LONG
TreeListBuildTree(
	IN HWND hWnd,
	IN TREE_NODE *Node,
	IN ULONG Item,
	IN ULONG Indent
	)
{
	LONG Next;

	Next = Item;

	for(; Node != NULL; Node = Node->Sibling) {

		if (Node->Child != NULL) {
			Node->State = EXPANDED;
		}
		
		Next = TreeListInsertItem(hWnd, Next, Indent, Node); 
		
		if (Node->Child != NULL) {
			Next = TreeListBuildTree(hWnd, Node->Child, Next, Indent + 1);
		}

	}

	return Next;
}

BOOL 
TreeListSetState(
	IN HWND hWnd, 
	IN LONG Item, 
	IN LONG State 
	)
{
	LVITEM lvi = {0};

	lvi.mask = LVIF_STATE; 
	lvi.iItem = Item; 
	lvi.stateMask = LVIS_STATEIMAGEMASK; 
	lvi.state = INDEXTOSTATEIMAGEMASK(State); 

	return ListView_SetItem(hWnd, &lvi);
}

BOOL 
TreeListSetStateEx(
	IN HWND hWnd, 
	IN LONG Item, 
	IN LONG Indent, 
	IN LONG State, 
	IN LPARAM Param
	)
{
	LVITEM lvi = {0};

	lvi.mask = LVIF_STATE | LVIF_INDENT | LVIF_PARAM; 
	lvi.iItem = Item; 
	lvi.stateMask = LVIS_STATEIMAGEMASK; 
	lvi.state = INDEXTOSTATEIMAGEMASK(State); 
	lvi.iIndent = Indent;
	lvi.lParam = Param;

	return ListView_SetItem(hWnd, &lvi);
}

BOOL 
TreeListGetState(
	IN HWND hWnd, 
	IN LONG Item, 
	OUT LONG *State 
	)
{
	LVITEM lvi = {0};
  
	lvi.mask = LVIF_STATE;
	lvi.iItem = Item;
	lvi.stateMask = LVIS_STATEIMAGEMASK;
  
	if (ListView_GetItem(hWnd, &lvi)) {
		*State = STATEIMAGEMASKTOINDEX(lvi.state & LVIS_STATEIMAGEMASK);
		return TRUE;
	}

	return FALSE;
}

BOOL 
TreeListGetParam(
	IN HWND hWnd, 
	IN LONG Item, 
	OUT LPARAM *Param 
	)
{
	LVITEM lvi = {0};
  
	lvi.mask = LVIF_PARAM;
	lvi.iItem = Item;
	lvi.stateMask = LVIS_STATEIMAGEMASK;
  
	if (ListView_GetItem(hWnd, &lvi)) {
		*Param = lvi.lParam;
		return TRUE;
	}

	return FALSE;
}

BOOL 
TreeListGetStateEx(
	IN HWND hWnd, 
	IN LONG Item, 
	OUT LONG *Indent, 
	OUT LONG *State, 
	OUT LPARAM *lParam
	)
{
	LVITEM lvi = {0};
  
	*Indent = -1;
	*State = -1;
	*lParam = 0;

	lvi.mask = LVIF_STATE | LVIF_INDENT | LVIF_PARAM;
	lvi.iItem = Item;
	lvi.stateMask = LVIS_STATEIMAGEMASK;
  
	if (ListView_GetItem(hWnd, &lvi)) {
		*Indent = lvi.iIndent;
		*State = STATEIMAGEMASKTOINDEX(lvi.state & LVIS_STATEIMAGEMASK);
		*lParam = lvi.lParam;
		return TRUE;
	}

	return FALSE;
}

LRESULT
TreeListOnLButtonDown(
	IN HWND hWnd, 
	IN WPARAM wParam, 
	IN LPARAM lParam
	)
{
	LVHITTESTINFO lvhti;
	LONG State;
	LONG Indent;
	BOOL fStatus;
	TREE_NODE *Node;
	LVFINDINFO lvfi = {0};

	if (wParam == MK_LBUTTON) {
		
		lvhti.pt.x = LOWORD(lParam);
		lvhti.pt.y = HIWORD(lParam);

		if (ListView_HitTest(hWnd, &lvhti) != -1) {
		
			if (lvhti.flags == LVHT_ONITEMSTATEICON) {
				
				fStatus = TreeListGetStateEx(hWnd, lvhti.iItem, &Indent, &State, (LPARAM *)&Node);
				if (fStatus != TRUE) {
					ASSERT(FALSE);
					return 0;
				}
				
				if (State == COLLAPSED) {
					
					TreeListInsertChildren(hWnd, lvhti.iItem, Indent);
					TreeListSetState(hWnd, lvhti.iItem, EXPANDED);
					Node->State = EXPANDED;


				} else if (State == EXPANDED) {
					
					TreeListDeleteChildren(hWnd, lvhti.iItem, Indent);
					TreeListSetState(hWnd, lvhti.iItem, COLLAPSED);
					Node->State = COLLAPSED;
					
				} else {

				}
			} 
		} 
	}

	return 0;
}

LONG
TreeListInsertChildren(
	IN HWND hWnd,
	IN LONG Item,
	IN LONG Indent
	)
{
	TREE_NODE *Node;
	LONG Next;

	Node = NULL;

	TreeListGetParam(hWnd, Item, (LPARAM *)&Node);
	ASSERT(Node != NULL);

	if (Node->Child == NULL) {
		return Item + 1;
	}

	Node = Node->Child;
	Next = Item + 1;

	do {
		Next = TreeListInsertItem(hWnd, Next, Indent + 1, Node);
		if (Node->Child != NULL && Node->State == EXPANDED) {
			Next = TreeListInsertChildren(hWnd, Next - 1, Indent + 1);
		}
		Node = Node->Sibling;

	} while (Node != NULL);

	return Next;
}

//
// N.B. This routine should be customized to do
//		proper text insertion. TreeListInsertCallback
//      is a sample routine to demo how to write a 
//      insert callback, Node must be set as lparam
//      of corresponding listview item
//

LONG
TreeListSampleInsertCallback(
	IN HWND hWnd, 
	IN LONG Item, 
	IN LONG Indent,
	IN TREE_NODE *Node 
	)
{
	/*PROCESS_ENTRY *Entry;
	WCHAR Buffer[MAX_PATH];

	Entry = (PROCESS_ENTRY *)Node->Value;
	wsprintf(Buffer, L"%ws", Entry->ProcessName);

	LVITEM lvi = {0};
	lvi.iItem = Item;
	lvi.pszText = Buffer;
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.iImage = 0;
	lvi.lParam = (LPARAM)Node;

	ListView_InsertItem(hWnd, &lvi);
	
	lvi.mask = LVIF_TEXT;
	lvi.iImage = -1;
	lvi.iItem = Item;

	lvi.iSubItem = 1;
	wsprintf(Buffer, L"%u", Entry->ProcessId);
	lvi.pszText = Buffer;
	ListView_SetItem(hWnd, &lvi);

	lvi.iSubItem = 2;
	ListView_SetItem(hWnd, &lvi);

	lvi.iSubItem = 3;
	ListView_SetItem(hWnd, &lvi);
	*/
	return 0;
}

LONG
TreeListInsertItem(
	IN HWND hWnd, 
	IN LONG Item, 
	IN LONG Indent,
	IN TREE_NODE *Node 
	)
{
	PTREELIST_OBJECT Object;
	TREELIST_INSERT_CALLBACK InsertCallback;

	Object = (PTREELIST_OBJECT)SdkGetObject(hWnd);
	InsertCallback = Object->InsertCallback;

	(*InsertCallback)(hWnd, Item, Indent, Node);

	TreeListSetStateEx(hWnd, Item, Indent, Node->State, (LPARAM)Node);
	PostMessage(hWnd, LVM_ENSUREVISIBLE, Item, FALSE);

	//
	// Return next item index
	//

	return (Item + 1);
}

BOOL 
TreeListDeleteChildren(
	IN HWND hWnd, 
	IN LONG Item, 
	IN LONG Indent
	)
{
	LONG ChildIndent;
	LONG Count;
	LONG State;
	TREE_NODE *Node;
	BOOL fOk;

	Count = ListView_GetItemCount(hWnd);

	do {

		Node = NULL;
		fOk = TreeListGetStateEx(hWnd, Item + 1, &ChildIndent, &State, (LPARAM *)&Node);
		if (fOk != TRUE) {
			break;
		}

		ASSERT(Node != NULL);

		if (ChildIndent > Indent) {

			ListView_DeleteItem(hWnd, Item + 1);
			Node->State = State;
			Count = Count - 1;

		}

	}
	while ((ChildIndent > Indent) && (Item + 1 < Count));

	return TRUE;
}

LRESULT 
TreeListOnLButtonDbClick(
	IN HWND hWnd, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LVHITTESTINFO Info;
	LONG State;
	LONG Indent;
	POINTS Point;
	LPARAM Param;
	BOOL fOk;

	//
	// If the high-order bit is 1, the key is down; otherwise, it is up
	// 

	if (GetKeyState(VK_LBUTTON) & 0x8000) {

		Point = MAKEPOINTS(lp);
		Info.pt.x = Point.x;
		Info.pt.y = Point.y;

		ListView_HitTest(hWnd, &Info);
		ListView_HitTest(hWnd, &Info);

		//
		// If the item's icon or label is double clicked
		// 

		if (Info.flags & (LVHT_ONITEMICON | LVHT_ONITEMLABEL)) {
			
			fOk = TreeListGetStateEx(hWnd, Info.iItem, &Indent, &State, &Param);
			ASSERT(fOk);

			//if (State == COLLAPSED) {
			//	TreeListInsertItem(hWnd, Info.iItem, Indent, (TREE_NODE *)Param);
			//} else {
			//	TreeListDeleteChildren(hWnd, Info.iItem, Indent);
			//}
		}

	}

	return 0L;
}

LRESULT 
TreeListOnKeyDown(
	IN HWND hWnd,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	LONG Item;
	LONG State;
	LONG Indent;
	BOOL fOk;
	TREE_NODE *Node;
	LVFINDINFO lvfi;

	Item = ListView_GetNextItem(hWnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
	if (Item == -1) {
		return 0L;
	}
    
    fOk = TreeListGetStateEx(hWnd, Item, &Indent, &State, (LPARAM *)&Node);
	ASSERT(fOk);

	switch (wp) {
      
	 case VK_RIGHT:
		 
		 if (State == COLLAPSED) {
			 	
			TreeListInsertChildren(hWnd, Item, Indent);
			TreeListSetState(hWnd, Item, EXPANDED);
			Node->State = EXPANDED;

		 } 
		 
		 else if (State == EXPANDED) {

			ListViewSelectSingle(hWnd, Item + 1);

		 } 
		 
		 else {
			 //
			 // Leaf node, do nothing
			 //
		 }
		 
		 break; 
      
	 case VK_LEFT:
        
		 if (State == EXPANDED) {

			 TreeListDeleteChildren(hWnd, Item, Indent);
			 TreeListSetState(hWnd, Item, COLLAPSED);
			 Node->State = COLLAPSED;

		 } 

		 else {
			
			 if (Node->Parent != NULL) {
				 memset(&lvfi, 0, sizeof lvfi);
				 lvfi.flags = LVFI_PARAM;
				 lvfi.lParam = (LPARAM)(Node->Parent);
				 Item = ListView_FindItem(hWnd, -1, &lvfi);
				 ASSERT(Item != -1); 
				 ListViewSelectSingle(hWnd, Item);
			 }

		 }

		 break;
	}

	return 0L;
}

