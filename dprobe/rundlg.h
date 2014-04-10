//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#ifndef _RUNDLG_H_
#define _RUNDLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dprobe.h"


INT_PTR
RunDialog(
	IN HWND hWndParent,
	IN PWCHAR ImagePath,
	IN PWCHAR Argument,
	IN PWCHAR WorkPath,
	IN PWCHAR DpcPath
	);

INT_PTR CALLBACK
RunProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
RunOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
RunOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
RunOnCommand(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
RunOnOk(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
RunOnCancel(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
RunOnImagePath(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
RunOnDpcPath(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

#ifdef __cplusplus
}
#endif

#endif