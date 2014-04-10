//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#ifndef _EULADLG_H_
#define _EULADLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"

INT_PTR
EulaDialog(
	IN HWND hWndParent	
	);

INT_PTR CALLBACK
EulaProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
EulaOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

DWORD CALLBACK
EulaStreamCallback(
	IN DWORD_PTR dwCookie,
    IN LPBYTE pbBuff,
    IN LONG cb,
    OUT LONG *pcb
	);

#ifdef __cplusplus
}
#endif

#endif