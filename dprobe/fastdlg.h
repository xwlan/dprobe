//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _FASTDLG_H_
#define _FASTDLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dprobe.h"

//
// Defs shared by both fast and filter dialog
//

struct _MSP_USER_COMMAND; 

//
// N.B. Custom TreeView state structure
//

typedef struct _SDK_TV_STATE {
	HTREEITEM hItem;
	ULONG NewState;
	ULONG OldState;
	LPARAM lParam;
} SDK_TV_STATE, *PSDK_TV_STATE;

//
// Define custom WM_MESSAGE to handle item changed of treeview item
// wp is not used, lp is a pointer to SDK_TV_STATE structure.
//

#define WM_TVN_ITEMCHANGED (WM_USER+100)
#define WM_TVN_UNCHECKITEM (WM_USER+101)
#define WM_TVN_CHECKITEM   (WM_USER+102)

//
// Fast dialog's defs 
//

typedef struct _FAST_CONTEXT {

	PBSP_PROCESS Process;
	LIST_ENTRY ModuleList;
	ULONG NumberOfSelected;
	HFONT hBoldFont;
	struct _MSP_USER_COMMAND *Command;

	PLISTVIEW_OBJECT ListObject;
	BOOLEAN DragMode;
	LONG SplitbarLeft;
	LONG SplitbarBorder;

} FAST_CONTEXT, *PFAST_CONTEXT;

typedef struct _FAST_ENTRY {
	LIST_ENTRY ListEntry;
	BOOLEAN Activate;
	BOOLEAN Spare[3];
	CHAR ApiName[MAX_PATH];
} FAST_ENTRY, *PFAST_ENTRY;

typedef struct _FAST_NODE {
	LIST_ENTRY ListEntry;
	ULONG NumberOfSelected;
	PBSP_MODULE Module;
	PBSP_EXPORT_DESCRIPTOR Api;
	ULONG ApiCount;
	ULONG Count;
	LIST_ENTRY FastList;
} FAST_NODE, *PFAST_NODE;

INT_PTR
FastDialog(
	IN HWND hWnd,
	IN PBSP_PROCESS Process,
	OUT struct _MSP_USER_COMMAND **Command
	);

INT_PTR CALLBACK
FastProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FastOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FastOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FastOnInfoTip(
	IN HWND hWnd,
	IN HWND hWndTree,
	IN LPNMTVGETINFOTIP lp
	);

LRESULT
FastOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FastOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FastOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

ULONG
FastInsertModule(
	IN PDIALOG_OBJECT Object
	);

ULONG
FastInsertApi(
	IN HWND hWndTree,
	IN HTREEITEM hParent,
	IN ULONG ProcessId,
	IN PFAST_CONTEXT Context,
	IN PBSP_MODULE Module,
	IN PBSP_EXPORT_DESCRIPTOR Api,
	IN ULONG ApiCount
	);

LRESULT
FastOnItemChanged(
	IN HWND hWnd,
	IN HWND hWndTree,
	IN HTREEITEM hItem,
	IN ULONG NewState,
	IN ULONG OldState,
	IN LPARAM Param
	);

LRESULT
FastOnWmItemChanged(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FastOnWmCheckItem(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FastOnWmUncheckItem(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FastOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT 
FastOnLButtonDown(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT 
FastOnLButtonUp(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT 
FastOnMouseMove(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

VOID
FastInsertListItem(
	IN HWND hWnd,
	IN HWND hWndTree,
	IN PFAST_NODE Node,
	IN HTREEITEM hTreeItem,
	IN PWCHAR Buffer,
	IN BOOLEAN Insert
	);

VOID
FastAdjustPosition(
	IN PDIALOG_OBJECT Object
	);

VOID
FastCheckChildren(
	IN HWND hWnd,
	IN PFAST_CONTEXT Context,
	IN PFAST_NODE Node,
	IN HWND hWndTree,
	IN HTREEITEM Parent,
	IN BOOLEAN Check
	);

VOID
FastCleanUp(
	IN HWND hWnd
	);

VOID
FastBuildCommand(
	IN PDIALOG_OBJECT Object,
	IN PFAST_CONTEXT Context
	);

VOID
FastFreeCommand(
	IN struct _MSP_USER_COMMAND *Command
	);

LRESULT CALLBACK
FastListProcedure(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp,
	IN UINT_PTR uIdSubclass,
	IN DWORD_PTR dwRefData
	);

int CALLBACK
FastSortCallback(
	IN LPARAM First, 
	IN LPARAM Second,
	IN LPARAM Param
	);

BOOLEAN
FastListOnDelete(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT 
FastOnColumnClick(
	IN HWND hWnd,
	IN NMLISTVIEW *lpNmlv
	);

ULONG
FastScanSelection(
	IN PDIALOG_OBJECT Object,
	OUT PLIST_ENTRY DllListHead
	);

ULONG 
FastExportProbe(
	IN PDIALOG_OBJECT Object,
	IN PWSTR Path
	);

ULONG 
FastImportProbe(
	IN PWSTR Path,
	OUT struct _MSP_USER_COMMAND **Command
	);

LRESULT
FastOnExport(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

#ifdef __cplusplus
}
#endif

#endif