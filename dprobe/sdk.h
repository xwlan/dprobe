//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#ifndef _SDK_H_
#define _SDK_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <assert.h>
#include "resource.h"

extern HINSTANCE SdkInstance;
extern HANDLE SdkHeapHandle;
extern HICON SdkMainIcon;

ULONG
SdkInitialize(
	IN HINSTANCE Instance	
	);

PVOID
SdkMalloc(
	IN ULONG Length
	);

VOID
SdkFree(
	IN PVOID BaseVa
	);

LONG_PTR
SdkSetObject(
	IN HWND hWnd,
	IN PVOID Object
	);

LONG_PTR
SdkGetObject(
	IN HWND hWnd
	);

VOID
SdkModifyStyle(
	IN HWND hWnd,
	IN ULONG Remove,
	IN ULONG Add,
	IN BOOLEAN ExStyle
	);

HIMAGELIST
SdkCreateGrayImageList(
	IN HIMAGELIST himlNormal
	);

VOID
SdkFillSolidRect(
	IN HDC hdc, 
	IN COLORREF cr, 
	IN RECT *rect
	);

VOID
SdkFillGradient(
	IN HDC hdc,
	IN COLORREF cr1,
	IN COLORREF cr2,
	IN RECT *Rect
	);

VOID
SdkCenterWindow(
	IN HWND hWnd
	);

HICON
SdkBitmapToIcon(
	IN HINSTANCE Instance,
	IN INT BitmapId
	);

BOOL
SdkSetMainIcon(
    IN HWND hWnd
    );

COLORREF 
SdkBrightColor(
    IN COLORREF Color,
    IN UCHAR Increment
    );

VOID 
SdkCopyControlRect(
    IN HWND hWndParent,
    IN HWND hWndFrom,
    IN HWND hWndTo
    );

HFONT
SdkCreateFont(
	VOID
	);

HFONT
SdkSetDefaultFont(
	IN HWND hWnd
	);

HFONT
SdkSetBoldFont(
	IN HWND hWnd
	);

BOOLEAN
SdkCopyFile(
	IN PWSTR Destine,
	IN PWSTR Source
	);

BOOLEAN
SdkCreateIconFile(
	IN HICON hIcon, 
	IN LONG Bits,
	IN PWSTR Path
	);

BOOLEAN
SdkSaveBitmap(
	IN HBITMAP hbitmap, 
	IN PWSTR filename, 
	IN int nColor
	);

LONG WINAPI 
SdkUnhandledExceptionFilter(
	__in struct _EXCEPTION_POINTERS* Pointers 
	);

VOID
SdkSetUnhandledExceptionFilter(
	VOID
	);

//
// Access Macros
//

#define SdkGetContext(_O, _T) \
	((_T *)_O->Context)

#define SdkSetContext(_O, _C) \
	(_O->Context = _C)

BOOL SaveIcon(TCHAR *szIconFile, HICON hIcon[], int nNumIcons);

//
// Colors for font or background, foreground rendering
//

#define WHITE  (RGB(255, 255, 255))
#define BLACK  (RGB(0, 0, 0))
#define GREEN  (RGB(0, 200, 0))
#define RED    (RGB(200, 0, 0))
#define BLUE   (RGB(0, 0, 200))

VOID
DebugTraceW(
	IN PWSTR Format,
	IN ...
	);

#ifndef ASSERT
#ifdef _DEBUG
#define ASSERT(_E) assert(_E)
#else
#define ASSERT(_E)
#endif
#endif

#if defined(_AMD64_)
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#if defined(_X86_)
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#pragma comment(lib, "comctl32.lib")

#ifdef __cplusplus
}
#endif

//
// The followings use C++ linkage
//

#endif