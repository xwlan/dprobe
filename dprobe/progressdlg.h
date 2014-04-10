//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _PROGRESSDLG_H_
#define _PROGRESSDLG_H_

#include "dprobe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef ULONG 
(CALLBACK *PROGRESS_DIALOG_CALLBACK)(
    IN HWND hWnd,
    IN PVOID Context
    );

//
// N.B. Caller which use progress dialog must appropriately
// fill and initialize the following members, progress dialog
// only use them, doesn't care their lifecycle.
//

typedef struct _PROGRESS_DIALOG_CONTEXT {
    HWND hWndOwner;
    HWND hWndDialog;
    BOOL CancelDisable;
    BOOL Cancel;
    BOOL Complete;
    PROGRESS_DIALOG_CALLBACK Callback;
    PVOID CallbackContext;
    BSP_LOCK ObjectLock;
    ULONG CurrentPercent;
    ULONG LastPercent;
    ULONG TimerId;
    WCHAR InitText[MAX_PATH];
    WCHAR CurrentText[MAX_PATH];
} PROGRESS_DIALOG_CONTEXT, *PPROGRESS_DIALOG_CONTEXT;

BOOLEAN
ProgressDialogCreate(
	IN PPROGRESS_DIALOG_CONTEXT Context
	);

INT_PTR CALLBACK
ProgressProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ProgressOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ProgressOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ProgressOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ProgressOnTimer(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);


BOOL CALLBACK
ProgressDialogCallback(
	IN HWND hWnd,
    IN ULONG Percent,
    IN PWSTR CurrentText,
    IN BOOL Cancel,
    IN BOOL Complete
	);

ULONG CALLBACK
ProgressDialogThread(
	IN PVOID Context
	);

#ifdef __cplusplus
}
#endif

#endif