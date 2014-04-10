//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _IMPORTDLG_H_
#define _IMPORTDLG_H_

#include "dprobe.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _MSP_USER_COMMAND;

INT_PTR
ImportDialog(
	IN HWND hWndParent,
	IN PWSTR Path,
	OUT PLIST_ENTRY ProcessList,
	OUT PULONG ProcessCount,
	OUT struct _MSP_USER_COMMAND **Command
	);

INT_PTR CALLBACK
ImportProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ImportOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ImportOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);
 
LRESULT 
ImportOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ImportOnNext(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ImportOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ImportOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT 
ImportOnColumnClick(
	IN HWND hWnd,
	IN NMLISTVIEW *lpNmlv
	);

int CALLBACK
ImportSortCallback(
	IN LPARAM First, 
	IN LPARAM Second,
	IN LPARAM Param
	);

ULONG
ImportReadFileHead(
	IN PWSTR Path,
	OUT PCSP_FILE_HEADER Head
	);

ULONG
ImportInsertProbe(
	IN PDIALOG_OBJECT Object
	);

ULONG
ImportInsertFast(
	IN PDIALOG_OBJECT Object,
	IN struct _MSP_USER_COMMAND *Command
	);

ULONG
ImportInsertFilter(
	IN PDIALOG_OBJECT Object,
	IN struct _MSP_USER_COMMAND *Command
	);

ULONG
ImportInsertTask(
	IN PDIALOG_OBJECT Object
	);

VOID
ImportCleanUp(
	IN PDIALOG_OBJECT Object,
	IN BOOLEAN Success
	);

VOID
ImportFreeCommand(
	IN struct _MSP_USER_COMMAND *Command
	);

#ifdef __cplusplus
}
#endif

#endif