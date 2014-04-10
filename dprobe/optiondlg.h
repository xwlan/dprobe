//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
// 

#ifndef _OPTIONDLG_H_
#define _OPTIONDLG_H_

#include "dprobe.h"

#ifdef __cplusplus
extern "C" {
#endif

ULONG
OptionSheetCreate(
	__in HWND hWndParent
	);

INT CALLBACK 
OptionSheetProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in LPARAM lp
    );

INT_PTR CALLBACK 
OptionGeneralProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in LPARAM lp,
	__in WPARAM wp
    );

INT_PTR CALLBACK 
OptionAdvancedProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    );

ULONG
OptionSymbolInit(
	__in HWND hWnd
	);

INT_PTR CALLBACK 
OptionSymbolProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    );

//
// Highlight
//

ULONG
OptionHighlightInit(
	__in HWND hWnd
	);

LRESULT
OptionHighlightOnCustomDraw(
	__in HWND hWnd,
	__in NMHDR *lpnmhdr
	);

LRESULT
OptionHighlightOnDbClick(
	__in HWND hWnd,
	__in NMHDR *lpnmhdr
	);

LRESULT
OptionHighlightOnCommand(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

LRESULT
OptionHighlightOnNotify(
	__in HWND hWnd,
	__in UINT uMsg,
	__in WPARAM wp,
	__in LPARAM lp
	);

INT_PTR CALLBACK 
OptionHighlightProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    );

INT_PTR CALLBACK 
OptionGraphProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    );

INT_PTR CALLBACK 
OptionStorageProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    );

INT_PTR CALLBACK 
OptionDebugConsoleProcedure(
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wp,
    __in LPARAM lp
    );

#ifdef __cplusplus
}
#endif

#endif