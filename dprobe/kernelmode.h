//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _KERNELMODE_H_
#define _KERNELMODE_H_

#include "dprobe.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOLEAN
KernelDialogCreate(
	IN HWND hWndParent
	);

INT_PTR CALLBACK
KernelProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
KernelOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
KernelOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
KernelOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
KernelOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

//
// Wake up system thread waiting on DISPATCH_LEVEL
//

LRESULT
KernelOnIrqlWakeUp(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

#ifdef __cplusplus
}
#endif

#endif