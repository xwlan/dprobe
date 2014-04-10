//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _SAVEDLG_H_
#define _SAVEDLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dprobe.h"
#include "mspdtl.h"

typedef enum _SAVE_STATE {
	SaveStarted,
	SaveUserAborted,
	SaveMspAborted,
	SaveComplete,
} SAVE_STATE, *PSAVE_STATE;

typedef struct _SAVE_CONTEXT {
	struct _MSP_DTL_OBJECT *DtlObject;
	MSP_SAVE_TYPE Type;
	BOOLEAN AdjustFilterIndex;
	WCHAR Path[MAX_PATH];
	HWND hWnd;
	SAVE_STATE State;
	ULONG Count;
	WCHAR Text[MAX_PATH];
} SAVE_CONTEXT, *PSAVE_CONTEXT;

typedef struct _SAVE_NOTIFICATION {
	ULONG Percent;
	ULONG Error;
	BOOLEAN Aborted;
	BOOLEAN Cancel;
} SAVE_NOTIFICATION, *PSAVE_NOTIFICATION;

INT_PTR
SaveDialog(
	IN HWND hWndParent,
	IN struct _MSP_DTL_OBJECT *DtlObject,
	IN PWSTR Path,
	IN MSP_SAVE_TYPE Type
	);

INT_PTR CALLBACK
SaveProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
SaveOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
SaveOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
SaveOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
SaveOnUserSave(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

BOOLEAN CALLBACK
SaveCallback(
	IN PVOID Context,
	IN ULONG Percent,
	IN BOOLEAN Abort,
	IN ULONG Error
	);

#ifdef __cplusplus
}
#endif

#endif