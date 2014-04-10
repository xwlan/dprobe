//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#ifndef _CRASHDLG_H_
#define _CRASHDLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"

typedef enum _CRASH_ACTION {
	CrashMiniDump,
	CrashFullDump,
	CrashNoDump,
} CRASH_ACTION;

ULONG
CrashDialog(
	IN HWND hWndParent	
	);

INT_PTR CALLBACK
CrashProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CrashOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CrashOnCommand(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

DWORD CALLBACK
CrashStreamCallback(
	IN DWORD_PTR dwCookie,
    IN LPBYTE pbBuff,
    IN LONG cb,
    OUT LONG *pcb
	);

#ifdef __cplusplus
}
#endif

#endif