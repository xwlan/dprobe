//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _APPLYDLG_H_
#define _APPLYDLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dprobe.h"

struct _MSP_USER_COMMAND;

INT_PTR
ApplyDialog(
	IN HWND hWndParent,
	IN PBSP_PROCESS Process,
	IN struct _MSP_USER_COMMAND *Command
	);

LRESULT CALLBACK
ApplyProcedure(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ApplyOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ApplyOnClose(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ApplyOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

ULONG CALLBACK
ApplyCallback(
	IN PVOID Packet,
	IN PVOID Context
	);

#ifdef __cplusplus
}
#endif

#endif