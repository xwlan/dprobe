//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#ifndef _PROBING_DLG_H_
#define _PROBING_DLG_H_

#include "dprobe.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOLEAN
ProbingDialogCreate(
	IN HWND hWndParent,
	IN ULONG ProcessId
	);

INT_PTR CALLBACK
ProbingProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

ULONG
ProbingLoadProbes(
	IN PDIALOG_OBJECT Object
	);

LRESULT
ProbingOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ProbingOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

#ifdef __cplusplus
}
#endif

#endif