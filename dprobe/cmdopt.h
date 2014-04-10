//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#ifndef _CMDOPT_H_
#define _CMDOPT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dprobe.h"

BOOLEAN
CmdlineDialogCreate(
	IN HWND hWndOwner
	);

INT_PTR CALLBACK
CmdlineProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CmdlineOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CmdlineOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT 
CmdlineOnCustomDraw(
	IN HWND hWnd,
	IN NMHDR *pNmhdr
	);

#ifdef __cplusplus
}
#endif
#endif