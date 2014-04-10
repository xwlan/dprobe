//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#ifndef _RECORDDLG_H_
#define _RECORDDLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dprobe.h"

HWND
RecordDialog(
	IN HWND hWndParent	
	);

INT_PTR CALLBACK
RecordDialogProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
RecordOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
RecordOnEraseBkgnd(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
RecordOnCopy(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
RecordOnDestroy(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

VOID
RecordUpdateData(
	IN HWND hWnd,
	IN HWND hWndLogView,
	IN PBTR_RECORD_HEADER Record
	);

LRESULT
RecordOnCtlColorStatic(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

#ifdef __cplusplus
}
#endif

#endif