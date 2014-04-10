//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
// 

#ifndef _TREELISTVIEW_H_
#define _TREELISTVIEW_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "listview.h"

//
// Node state
//

#define NOBUTTON   0
#define COLLAPSED  1
#define EXPANDED   2

typedef struct _TREE_NODE {
	
	//
	// Value, pointer to customized structure
	//

	PVOID Value;

	//
	// State, NOBUTTON or COLLAPSED or EXPANDED
	//

	ULONG State; 

	struct _TREE_NODE *Flink;
	struct _TREE_NODE *Blink;
	
	//
	// Treelist only use the following relation pointers,
	// however, the above Flink, Blink may be useful to
	// client to keep track of the nodes in linear form
	//

	struct _TREE_NODE *Parent;
	struct _TREE_NODE *Sibling;
	struct _TREE_NODE *Child;

} TREE_NODE, *PTREE_NODE;

typedef LONG
(*TREELIST_INSERT_CALLBACK)(
	IN HWND hWnd, 
	IN LONG Item, 
	IN LONG Indent,
	IN TREE_NODE *Node 
	);

typedef struct _TREELIST_OBJECT {
	HWND hWnd;
	ULONG CtrlId;
	PTREE_NODE RootNode;
	PLISTVIEW_COLUMN Columns;
	ULONG ColumnCount;
	HIMAGELIST himlState;
	HIMAGELIST himlSmall;
	TREELIST_INSERT_CALLBACK InsertCallback;
} TREELIST_OBJECT, *PTREELIST_OBJECT;

//
// Dynamically create a treelist control
//

HWND 
TreeListCreate(
	IN HWND hWndParent, 
	IN UINT CtrlId,
	IN PLISTVIEW_COLUMN Column,
	IN ULONG ColumnCount,
	IN PTREE_NODE Root
	);

//
// This is used for static control in dialog
//

HWND 
TreeListInitialize(
	IN HWND hWndParent, 
	IN UINT ResourceId,
	IN PLISTVIEW_COLUMN Column,
	IN ULONG ColumnCount,
	IN PTREE_NODE Root
	);

LONG
TreeListBuildTree(
	IN HWND hWnd,
	IN TREE_NODE *Node,
	IN ULONG Item,
	IN ULONG Indent
	);

LRESULT CALLBACK 
TreeListProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wParam, 
	IN LPARAM lParam, 
	IN UINT_PTR uIdSubclass, 
	IN DWORD_PTR dwRefData
	);

LONG
TreeListInsertItem(
	IN HWND hWnd, 
	IN LONG Item, 
	IN LONG Indent, 
	IN TREE_NODE *Node
	);

LONG
TreeListInsertChildren(
	IN HWND hWnd,
	IN LONG Item,
	IN LONG Indent
	);

BOOL 
TreeListGetStateEx(
	IN HWND hWnd, 
	IN LONG Item, 
	OUT LONG *Indent, 
	OUT LONG *State, 
	OUT LPARAM *lParam
	);

BOOL 
TreeListGetState(
	IN HWND hWnd, 
	IN LONG Item, 
	OUT LONG *State 
	);

BOOL 
TreeListGetParam(
	IN HWND hWnd, 
	IN LONG Item, 
	OUT LPARAM *Param 
	);

BOOL
TreeListSetState(
	IN HWND hWnd,
	IN LONG Item,
	IN LONG State
	);

BOOL
TreeListSetStateEx(
	IN HWND hWnd,
	IN LONG Item,
	IN LONG State,
	IN LONG Indent,
	IN LPARAM Param
	);

BOOL 
TreeListDeleteChildren(
	IN HWND hWnd, 
	IN LONG Item, 
	IN LONG Indent
	);

LRESULT
TreeListOnLButtonDown(
	IN HWND hWnd, 
	IN WPARAM wParam, 
	IN LPARAM lParam
	);

LRESULT 
TreeListOnLButtonDbClick(
	IN HWND hWnd, 
	IN WPARAM wParam, 
	IN LPARAM lParam
	);

LRESULT 
TreeListOnKeyDown(
	IN HWND hWnd, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

#ifdef __cplusplus
}
#endif

#endif