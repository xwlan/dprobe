//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#ifndef _DUMPDLG_H_
#define _DUMPDLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dprobe.h"

INT_PTR
DumpDialog(
	IN HWND hWndParent	
	);

INT_PTR CALLBACK
DumpProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
DumpOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT 
DumpOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
DumpOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
DumpOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
DumpOnClose(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
DumpOnContextMenu(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
DumpOnSmallDump(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
DumpOnFullDump(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

VOID
DumpOnRefresh(
	IN HWND hWnd
	);

LRESULT 
DumpOnColumnClick(
	IN HWND hWnd,
	IN NMLISTVIEW *lpNmlv
	);

VOID
DumpInsertProcessList(
	IN PLISTVIEW_OBJECT Object,
	IN PLIST_ENTRY ListHead
	);

int CALLBACK
DumpSortCallback(
	IN LPARAM First, 
	IN LPARAM Second,
	IN LPARAM Param
	);

#ifdef __cplusplus
}
#endif

#endif