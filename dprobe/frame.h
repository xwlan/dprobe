//
// Apsara Labs
// lan.john@gmail.com
// Copyrigth(C) 2009-2012
//

#ifndef _FRAME_H_
#define _FRAME_H_

#include "sdk.h"
#include "rebar.h"
#include "statusbar.h"
#include "splitbar.h"
#include "find.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _FRAME_MENU_ITEM {
	UINT CommandId;
	ULONG ProcessId;
	WCHAR ProcessName[MAX_PATH];
} FRAME_MENU_ITEM, *PFRAME_MENU_ITEM;

typedef struct _FRAME_OBJECT {
	HWND hWnd;
	REBAR_OBJECT *Rebar;
	HWND hWndStatusBar;
	PVOID View;
	HWND hWndTree;
	FIND_CONTEXT FindContext;
	FRAME_MENU_ITEM *ProbingMenuMap;
} FRAME_OBJECT, *PFRAME_OBJECT;

enum FRAME_CHILD_ID {
	FrameRebarId     = 10,
	FrameStatusBarId = 20,
	FrameChildViewId = 30,
	FrameTreeViewId  = 40,
} ;

BOOLEAN
FrameRegisterClass(
	VOID
	);

HWND 
FrameCreate(
	IN PFRAME_OBJECT Object 
	);

VOID
FrameGetViewRect(
	IN PFRAME_OBJECT Object,
	OUT RECT *rcView
	);

LRESULT 
FrameOnContextMenu(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FrameOnSize(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FrameOnOpen(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FrameOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FrameOnCounter(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnPerformance(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnTtnGetDispInfo(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FrameOnCreate(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FrameOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FrameOnClose(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
FrameOnExit(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT CALLBACK
FrameProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnImport(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnExport(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnOption(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

//
// Probing process command handler
//

LRESULT
FrameOnProbingCommand(
	IN HWND hWnd,
	UINT CommandId
	);

//
// Incremental filter command handler
//

LRESULT
FrameOnFilterCommand(
	IN HWND hWnd,
	UINT CommandId
	);

VOID
FrameRetrieveFindString(
	IN PFRAME_OBJECT Object,
	OUT PWCHAR Buffer,
	IN ULONG Length
	);

LRESULT
FrameOnFindForward(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnFindBackward(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnAutoScroll(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnSave(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnStopProbe(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnRunProbe(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnResetCondition(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnEditCondition(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnFind(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnLog(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnRegisterdWmFind(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnWmUserStatus(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

BOOLEAN
FrameLoadBars(
	IN PFRAME_OBJECT Object
	);

typedef struct _FRAME_DTL_DATA {
	HWND hWndFrame;
} FRAME_DTL_DATA, *PFRAME_DTL_DATA;

VOID
FrameSharedDataCallback(
	IN struct _DTL_SHARED_DATA *SharedData,
	IN PVOID CallbackContext
	);

LRESULT
FrameOnCallStack(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnCallTree(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnCallRecord(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnCommandOption(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnWmUserFilter(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnDump(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
FrameOnCopy(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

VOID
FrameInitializeProbingMap(
	VOID
	);

PFRAME_MENU_ITEM
FrameFindMenuItemById(
	IN ULONG ProcessId 
	);

PFRAME_MENU_ITEM
FrameFindMenuItemByCommand(
	IN UINT CommandId
	);

VOID
FrameInsertProbingMenu(
	IN PBSP_PROCESS Process
	);

VOID
FrameDeleteProbingMenu(
	IN ULONG ProcessId
	);

ULONG
FrameCreateTree(
	IN PFRAME_OBJECT Object,
	IN UINT TreeId
	);

#ifdef _DEBUG

//
// N.B. Kernel mode test routine
//

LRESULT
FrameOnTestKernelMode(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

#endif


extern UINT WM_USER_STATUS;
extern UINT WM_USER_FILTER;


#ifdef __cplusplus
}
#endif

#endif