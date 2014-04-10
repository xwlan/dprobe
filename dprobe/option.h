//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#ifndef _OPTION_H_
#define _OPTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dprobe.h"

typedef enum _OptionPageType {
	OptionPageGeneral,
	OptionPageAdvanced,
	OptionPageSymbol,
	OptionPageOthers,
	OptionPageNumber,
} OptionPageType;

typedef struct _OPTION_DIALOG_CONTEXT {
	HFONT hFontOld;
	HFONT hFontBold;
	HBRUSH hBrushFocus;
	HBRUSH hBrushPane;
	DIALOG_OBJECT Pages[OptionPageNumber];
} OPTION_DIALOG_CONTEXT, *POPTION_DIALOG_CONTEXT;

VOID
OptionDialogCreate(
	IN HWND hWndParent	
	);

INT_PTR CALLBACK
OptionDialogProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
OptionOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
OptionOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
OptionOnDrawItem(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
OptionOnInitDialog(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
OptionOnCommand(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
OptionOnOk(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
OptionOnClose(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

INT_PTR CALLBACK 
OptionGeneralProcedure(
    IN HWND hWnd,
    IN UINT uMsg,
	IN WPARAM wp,
    IN LPARAM lp
    );

ULONG
OptionGeneralInit(
	IN HWND hWnd
	);

ULONG
OptionGeneralCommit(
	IN HWND hWnd
	);

INT_PTR CALLBACK 
OptionAdvancedProcedure(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp
    );

ULONG
OptionAdvancedInit(
	IN HWND hWnd
	);

ULONG
OptionAdvancedCommit(
	IN HWND hWnd
	);

INT_PTR CALLBACK 
OptionSymbolProcedure(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp
    );

ULONG
OptionSymbolInit(
	IN HWND hWnd
	);

ULONG
OptionSymbolCommit(
	IN HWND hWnd
	);

INT_PTR CALLBACK 
OptionOthersProcedure(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp
    );

ULONG
OptionOthersInit(
	IN HWND hWnd
	);

ULONG
OptionOthersCommit(
	IN HWND hWnd
	);

#ifdef __cplusplus
}
#endif

#endif