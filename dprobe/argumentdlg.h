//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2010
//

#ifndef _ARGUMENTDLG_H_
#define _ARGUMENTDLG_H_

#include "bsp.h"
#include "psp.h"
#include "dialog.h"

#ifdef __cplusplus
extern "C" {
#endif

LRESULT
ArgumentOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ArgumentOnCommand(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ArgumentOnAdd(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ArgumentOnRemove(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

BOOLEAN
ArgumentOnOk(
	IN HWND hWnd
	);

INT_PTR CALLBACK
ArgumentProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

BOOLEAN
ArgumentDialogCreate(
	IN HWND hWnd,
	IN PDIALOG_RESULT Result
	);

VOID
ArgumentInsertCallType(
	IN HWND hWnd,
	IN PPSP_PROBE Probe
	);

VOID
ArgumentInsertArgumentType(
	IN HWND hWnd,
	IN PPSP_PROBE Probe
	);

VOID
ArgumentInsertReturnType(
	IN HWND hWnd,
	IN PPSP_PROBE Probe
	);

VOID
ArgumentInsertArgument(
	IN HWND hWnd,
	IN PPSP_PROBE Probe
	);

VOID
ArgumentFillProbe(
	IN HWND hWnd,
	IN PPSP_PROBE Probe
	);

LRESULT
ArgumentOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

VOID
ArgumentFreeResources(
	IN HWND hWnd
	);

#ifdef __cplusplus
}
#endif

#endif