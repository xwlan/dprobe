//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _PROCESSDLG_H_
#define _PROCESSDLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "dialog.h"

INT_PTR
ProcessDialog(
	IN HWND hWndParent,
	OUT PBSP_PROCESS *Process
	);

INT_PTR CALLBACK
ProcessProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ProcessOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

VOID
ProcessInitialize(
	IN PLISTVIEW_OBJECT Object
	);

int CALLBACK
ProcessSortCallback(
	IN LPARAM First, 
	IN LPARAM Second,
	IN LPARAM Param
	);

LRESULT 
ProcessOnColumnClick(
	IN HWND hWnd,
	IN NMLISTVIEW *lpNmlv
	);

LRESULT 
ProcessOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ProcessOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

VOID
ProcessOnRefresh(
	IN HWND hWnd	
	);

VOID
ProcessCleanUp(
	IN HWND hWndList
	);

#ifdef __cplusplus
}
#endif

#endif