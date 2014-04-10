//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _FILTERDLG_H_
#define _FILTERDLG_H_

#include "dprobe.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _MSP_USER_COMMAND; 

typedef struct _FILTER_CONTEXT {

	PBSP_PROCESS Process;
	LIST_ENTRY FilterList;
	ULONG NumberOfSelected;
	HFONT hBoldFont;
	struct _MSP_USER_COMMAND *Command;

	PLISTVIEW_OBJECT ListObject;
	BOOLEAN DragMode;
	LONG SplitbarLeft;
	LONG SplitbarBorder;

} FILTER_CONTEXT, *PFILTER_CONTEXT;

typedef struct _FILTER_NODE {
	LIST_ENTRY ListEntry;
	PBTR_FILTER Filter;
	ULONG NumberOfSelected;
	ULONG Count;
	BTR_BITMAP BitMap;
	ULONG Bits[16];
} FILTER_NODE, *PFILTER_NODE;

INT_PTR
FilterDialog(
	IN HWND hWnd,
	IN PBSP_PROCESS Process,
	OUT struct _MSP_USER_COMMAND **Command
	);

INT_PTR CALLBACK
FilterProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FilterOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FilterOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FilterOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FilterOnInfoTip(
	IN HWND hWnd,
	IN HWND hWndTree,
	IN LPNMTVGETINFOTIP lp
	);

LRESULT
FilterOnItemChanged(
	IN HWND hWnd,
	IN HWND hWndTree,
	IN HTREEITEM hItem,
	IN ULONG NewState,
	IN ULONG OldState,
	IN LPARAM Param
	);

LRESULT
FilterOnWmItemChanged(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FilterOnWmUncheckItem(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FilterOnWmCheckItem(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FilterOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FilterOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FilterOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT 
FilterOnLButtonDown(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT 
FilterOnLButtonUp(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT 
FilterOnMouseMove(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FilterOnKeyDown(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

VOID
FilterAdjustPosition(
	IN PDIALOG_OBJECT Object
	);

ULONG
FilterInsertFilter(
	IN PDIALOG_OBJECT Object
	);

ULONG
FilterInsertProbe(
	IN PDIALOG_OBJECT Object,
	IN HWND hWndTree,
	IN HTREEITEM hTreeItem,
	IN PFILTER_NODE Node
	);

VOID
FilterCheckChildren(
	IN HWND hWnd,
	IN PFILTER_CONTEXT Context,
	IN PFILTER_NODE Node,
	IN HWND hWndTree,
	IN HTREEITEM Parent,
	IN BOOLEAN Check
	);

VOID
FilterInsertListItem(
	IN HWND hWnd,
	IN HWND hWndTree,
	IN PFILTER_NODE Node,
	IN HTREEITEM hTreeItem,
	IN PWCHAR Buffer,
	IN BOOLEAN Insert
	);

VOID
FilterBuildCommand(
	IN PDIALOG_OBJECT Object
	);

VOID
FilterCleanUp(
	IN HWND hWnd
	);

VOID
FilterFreeCommand(
	IN struct _MSP_USER_COMMAND *Command
	);

LRESULT CALLBACK
FilterListProcedure(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp,
	IN UINT_PTR uIdSubclass,
	IN DWORD_PTR dwRefData
	);

BOOLEAN
FilterListOnDelete(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT 
FilterOnColumnClick(
	IN HWND hWnd,
	IN NMLISTVIEW *lpNmlv
	);

int CALLBACK
FilterSortCallback(
	IN LPARAM First, 
	IN LPARAM Second,
	IN LPARAM Param
	);

ULONG
FilterScanSelection(
	IN PDIALOG_OBJECT Object,
	OUT PLIST_ENTRY DllListHead
	);

ULONG 
FilterExportProbe(
	IN PDIALOG_OBJECT Object,
	IN PWSTR Path
	);

ULONG 
FilterImportProbe(
	IN PWSTR Path,
	OUT struct _MSP_USER_COMMAND **Command
	);

LRESULT
FilterOnExport(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

#ifdef __cplusplus
}
#endif

#endif