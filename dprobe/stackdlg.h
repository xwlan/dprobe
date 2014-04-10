//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _STACKDLG_H_
#define _STACKDLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"
#include "mspdtl.h"

HWND
StackDialog(
	IN HWND hWndParent,
	IN struct _BTR_STACKTRACE_ENTRY *Entry
	);

LRESULT
StackOnInitDialog(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
StackOnDestroy(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
StackOnCopy(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
StackOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

INT_PTR CALLBACK
StackProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

ULONG
StackUpdateData(
	IN HWND hWnd,
	IN PMSP_STACKTRACE_DECODED Decoded 
	);

#ifdef __cplusplus
}
#endif

#endif