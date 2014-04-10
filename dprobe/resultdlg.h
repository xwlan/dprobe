//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#ifndef _RESULTDLG_H_
#define _RESULTDLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dprobe.h"

INT_PTR
ResultDialog(
	IN HWND hWndParent,
	IN PBSP_PROCESS Process,
	IN struct _MSP_USER_COMMAND *Command
	);

INT_PTR CALLBACK
ResultProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ResultOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ResultOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ResultOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ResultOnCopy(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

VOID
ResultInsertFailure(
	IN PDIALOG_OBJECT Object,
	IN HWND hWndList,
	IN struct _MSP_USER_COMMAND *Command
	);

#ifdef __cplusplus
}
#endif

#endif