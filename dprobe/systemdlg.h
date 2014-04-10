//
// xwlan@msn.com
// Apsaras Labs
// Copyright(C) 2009-2010
//

#ifndef _SYSTEM_DLG_H_
#define _SYSTEM_DLG_H_

#include "dprobe.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOLEAN
SystemDialogCreate(
	IN HWND hWndParent,
	IN PDIALOG_RESULT Result
	);

INT_PTR CALLBACK
SystemProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM	lp
	);

ULONG
SystemLoadProbes(
	IN HWND hWnd,
	IN PPSP_PROCESS Process
	);

LRESULT
SystemOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
SystemOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
SystemOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
SystemOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

#ifdef __cplusplus
}
#endif

#endif