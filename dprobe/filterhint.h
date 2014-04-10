//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _FILTERHINT_H_
#define _FILTERHINT_H_

#include "dprobe.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _MSP_DTL_OBJECT;

typedef struct _FILTERHINT_CONTEXT {
	HWND hWndOwner;
	struct _MSP_DTL_OBJECT *DtlObject;
	ULONG NumberToScan;
	ULONG ScannedNumber;
	ULONG Cancel;
	HANDLE ThreadHandle;
} FILTERHINT_CONTEXT, *PFILTERHINT_CONTEXT;

BOOLEAN
FilterHintDialog(
	IN HWND hWndOwner,
	struct _MSP_DTL_OBJECT *DtlObject
	);

INT_PTR CALLBACK
FilterHintProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FilterHintOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FilterHintOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FilterHintOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FilterHintOnTimer(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

ULONG CALLBACK
FilterScanWorkItem(
	IN PVOID Context
	);

#ifdef __cplusplus
}
#endif

#endif