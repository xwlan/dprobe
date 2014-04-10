//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#ifndef _DETAILDLG_H_
#define _DETAILDLG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dprobe.h"

struct _MSP_DTL_OBJECT;

typedef enum _DETAIL_PAGE_TYPE {
	DetailPageRecord,
	DetailPageStack,
	DetailPageNumber,
} DETAIL_PAGE_TYPE, *PDETAIL_PAGE_TYPE;

typedef struct _DETAIL_DIALOG_CONTEXT {
	HWND hWndTrace;
	PWCHAR Buffer;
	ULONG BufferLength;
	PVOID StackTrace;
	struct _MSP_DTL_OBJECT *DtlObject;
	HICON hIconUpward;
	HICON hIconDownward;
	DETAIL_PAGE_TYPE InitialType;
	HWND hWndPages[DetailPageNumber];
} DETAIL_DIALOG_CONTEXT, *PDETAIL_DIALOG_CONTEXT;

INT_PTR
DetailDialog(
	IN HWND hWndParent,
	IN HWND hWndTrace,
	struct _MSP_DTL_OBJECT *DtlObject,
	IN DETAIL_PAGE_TYPE InitialPage
	);

INT_PTR CALLBACK
DetailProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
DetailOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
DetailOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
DetailOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
DetailOnCustomDraw(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
DetailOnSelChange(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
DetailOnUpward(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
DetailOnDownward(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
DetailOnCopy(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
DetailOnClose(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

VOID
DetailAdjustPosition(
	IN HWND hWnd
	);

ULONG
DetailUpdateData(
	IN PDIALOG_OBJECT Object
	);

#ifdef __cplusplus
}
#endif

#endif