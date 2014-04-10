//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
// 

#ifndef _OPENPROMPT_H_
#define _OPENPROMPT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dprobe.h"

INT_PTR
OpenPromptDialogCreate(
	IN HWND hWndParent	
	);

INT_PTR CALLBACK
OpenPromptDialogProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
OpenPromptOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
OpenPromptOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
OpenPromptOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
OpenPromptOnClose(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

#ifdef __cplusplus
}
#endif

#endif