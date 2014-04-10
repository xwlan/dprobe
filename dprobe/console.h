//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#ifndef _DEBUGCONSOLE_H_
#define _DEBUGCONSOLE_H_

#ifdef __cplusplus 
extern "C" {
#endif

#include "dprobe.h"
#include <richedit.h>

typedef struct _CONSOLE_THREAD_CONTEXT {
	RECT Rect;
	WCHAR Caption[MAX_PATH];
} CONSOLE_THREAD_CONTEXT, *PCONSOLE_THREAD_CONTEXT;

typedef struct _CONSOLE_OBJECT {
	HWND hWnd;
	HWND hWndEdit;
	ULONG CmdStartPos;
	CHARFORMAT CharFormat;
	BSP_LOCK LockObject;
	COLORREF BackColor;
	COLORREF TextColor;
	WCHAR FilePath[MAX_PATH];
	HANDLE FileHandle;
	ULONG FileSizeQuota;
} CONSOLE_OBJECT, *PCONSOLE_OBJECT;

BOOLEAN
ConsoleCreate(
	IN RECT *Rect,
	IN PWSTR Title 
	);

BOOLEAN
ConsoleRegisterClass(
	VOID
	);

HWND
ConsoleCreateWindow(
	IN PCONSOLE_THREAD_CONTEXT Context
	);

LRESULT
ConsoleOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ConsoleOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT 
ConsoleOnClose(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT 
ConsoleOnSize(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT CALLBACK 
ConsoleEditProcedure(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp,
	IN UINT_PTR uIdSubclass,
    IN DWORD_PTR dwRefData
    );

LRESULT CALLBACK
ConsoleProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ConsoleOnFilter(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ConsoleOnProtected(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ConsoleOnKeyReturn(
	IN PCONSOLE_OBJECT Object
	);

ULONG
ConsoleSetFont(
	IN PCONSOLE_OBJECT Object
	);

ULONG
ConsoleSetText(
	IN PCONSOLE_OBJECT,
	IN PWSTR Text,
	IN ULONG Start,
	IN ULONG End
	);

ULONG
ConsoleGetText(
	IN PCONSOLE_OBJECT Object,
	IN PWCHAR Buffer,
	IN ULONG Length,
	IN ULONG First,
	IN ULONG Last 
	);

ULONG
ConsoleSetOption(
	IN PCONSOLE_OBJECT,
	IN WPARAM Op,
	IN ULONG Flag 
	);

ULONG
ConsoleGetTextLength(
	IN PCONSOLE_OBJECT Object
	);

ULONG
ConsoleGetSelection(
	IN PCONSOLE_OBJECT Object,
	OUT CHARRANGE *Range
	);

ULONG
ConsoleSelectText(
	IN PCONSOLE_OBJECT Object,
	IN ULONG Start,
	IN ULONG End
	);

ULONG
ConsoleHideSelection(
	IN PCONSOLE_OBJECT,
	IN BOOL Hide
	);

ULONG
ConsoleSetBkColor(
	IN PCONSOLE_OBJECT Object,
	IN COLORREF Color
	);

ULONG
ConsoleSetEventMask(
	IN PCONSOLE_OBJECT,
	IN ULONG Mask
	);

ULONG
ConsoleRunCommand(
	IN PCONSOLE_OBJECT Object,
	IN PWSTR Command,
	OUT PWCHAR *Buffer
	);

VOID
ConsoleSetCharFormat(
	IN PCONSOLE_OBJECT Object
	);

VOID
ConsoleInsertText(
	IN PCONSOLE_OBJECT Object,
	IN PWSTR Buffer
	);

VOID
ConsoleSetCaret(
	IN PCONSOLE_OBJECT Object
	);

VOID
ConsoleProtectText(
	IN PCONSOLE_OBJECT Object
	);

ULONG CALLBACK
ConsoleWorkThread(
	IN PVOID Context
	);

VOID
ConsoleCheckLength(
	IN PCONSOLE_OBJECT Object
	);

ULONG
ConsoleWriteFile(
	IN PCONSOLE_OBJECT Object
	);

ULONG CALLBACK
ConsoleStreamCallback(
	IN DWORD_PTR Cookie,
    IN PVOID Buffer,
    IN LONG Length,
    OUT PLONG ActualLength
	);

#ifdef __cplusplus 
}
#endif

#endif