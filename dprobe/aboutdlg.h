//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#ifndef _ABOUTDLG_H_
#define _ABOUTDLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"

VOID
AboutDialog(
	IN HWND hWndParent	
	);

INT_PTR CALLBACK
AboutProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
AboutOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
AboutOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
AboutOnOk(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
AboutOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

#ifdef __cplusplus
}
#endif

#endif