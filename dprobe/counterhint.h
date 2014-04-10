//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _COUNTERHINT_H_
#define _COUNTERHINT_H_

#include "dprobe.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _MSP_DTL_OBJECT;

typedef struct _COUNTERHINT_CONTEXT {
	HWND hWndOwner;
	struct _MSP_DTL_OBJECT *DtlObject;
	ULONG NumberToScan;
	ULONG ScannedNumber;
} COUNTERHINT_CONTEXT, *PCOUNTERHINT_CONTEXT;

BOOLEAN
CounterHintDialog(
	IN HWND hWndOwner,
	struct _MSP_DTL_OBJECT *DtlObject
	);

INT_PTR CALLBACK
CounterHintProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
CounterHintOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CounterHintOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CounterHintOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
CounterHintOnTimer(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

ULONG CALLBACK
CounterScanWorkItem(
	IN PVOID Context
	);

#ifdef __cplusplus
}
#endif

#endif