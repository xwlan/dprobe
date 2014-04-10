//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#ifndef _PERFWND_H_
#define _PERFWND_H_

#include "dprobe.h"

#ifdef __cplusplus
extern "C" {
#endif 

#include "dprobe.h"

typedef enum _PERFMON_CTRL_ID {
	PerfMonStatusBarId,
	PerfMonTaskListId,
	PerfMonRightPaneId,
	PerfMonCpuGraphId,
	PerfMonMemoryGraphId,
	PerfMonIoGraphId,
	PerfMonCounterListId,
	PERFMON_CTRL_NUMBER,
} PERFMON_CTRL_ID;

typedef struct _PERFMON_OBJECT {
	HWND hWnd;
	HWND hWndList;
	HWND hWndPane;
	ULONG CtrlId[PERFMON_CTRL_NUMBER];
} PERFMON_OBJECT, *PPERFMON_OBJECT;

BOOLEAN
PerfMonRegisterClass(
	VOID
	);

HWND
PerfMonWindowCreate(
	IN HWND hWndParent
	);

LRESULT CALLBACK
PerfMonProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
PerfMonOnCreate(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
PerfMonOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
PerfMonOnPaint(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
PerfMonOnSize(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
PerfMonOnClose(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

ULONG
PerfMonCreateChildren(
	IN PPERFMON_OBJECT Object 
	);

#ifdef __cplusplus
}
#endif

#endif