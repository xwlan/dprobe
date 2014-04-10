//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _STOPDLG_H_
#define _STOPDLG_H_

#include "dprobe.h"

#ifdef __cplusplus
extern "C" {
#endif

INT_PTR
StopDialog(
	IN HWND hWndParent,
	OUT PLIST_ENTRY ListHead,
	OUT PULONG Count
	);

LRESULT
StopOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT CALLBACK
StopProcedure(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
StopOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
StopOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
StopOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
StopOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

ULONG
StopInsertProcess(
	IN PDIALOG_OBJECT Object
	);

VOID
StopCleanUp(
	IN HWND hWnd
	);

#ifdef __cplusplus
}
#endif

#endif
