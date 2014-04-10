//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _COUNTERDLG_H_
#define _COUNTERDLG_H_

#include "dprobe.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _MSP_DTL_OBJECT;

INT_PTR
CounterDialog(
	IN HWND hWndOwner,
	IN struct _MSP_DTL_OBJECT *DtlObject
	);

INT_PTR CALLBACK
CounterProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CounterOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CounterOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CounterOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CounterOnTimer(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CounterOnTreeListSort(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CounterOnTreeListSort(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

int CALLBACK
CounterCompareCallback(
	IN LPARAM First, 
	IN LPARAM Second,
	IN LPARAM Param
	);

VOID CALLBACK
CounterFormatCallback(
	IN struct _TREELIST_OBJECT *TreeList,
	IN HTREEITEM hTreeItem,
	IN PVOID Value,
	IN ULONG Column,
	OUT PWCHAR Buffer,
	IN SIZE_T Length
	);

ULONG
CounterInsertGroup(
	IN PDIALOG_OBJECT Object
	);

VOID
CounterInsertEntry(
	IN HWND hWndTree,
	IN HTREEITEM hItemGroup,
	IN struct _MSP_COUNTER_GROUP *Group
	);

VOID
CounterUpdate(
	IN struct _TREELIST_OBJECT *TreeList
	);

VOID 
CounterFormatEntry(
	IN struct _MSP_COUNTER_ENTRY *Entry,
	IN ULONG Column,
	OUT PWCHAR Buffer,
	IN SIZE_T Length
	);

VOID 
CounterFormatGroup(
	IN struct _MSP_COUNTER_GROUP *Group,
	IN ULONG Column,
	OUT PWCHAR Buffer,
	IN SIZE_T Length
	);

VOID 
CounterFormatRoot(
	IN struct _MSP_COUNTER_OBJECT *Root,
	IN ULONG Column,
	OUT PWCHAR Buffer,
	IN SIZE_T Length
	);

VOID
CounterUpdateRoot(
	IN struct _MSP_COUNTER_OBJECT *Object
	);

#ifdef __cplusplus
}
#endif

#endif