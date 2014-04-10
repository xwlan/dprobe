//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _REPORTDLG_H_
#define _REPORTDLG_H_

#ifdef __cplusplus
extern "C" {
#endif
	
#include "dprobe.h"
#include "find.h"

typedef struct _REPORT_ICON{
	LIST_ENTRY ListEntry;
	ULONG ProcessId;
	ULONG ImageIndex;
} REPORT_ICON, *PREPORT_ICON;

typedef struct _REPORT_CONTEXT {
	HIMAGELIST himlReport;
	LIST_ENTRY IconListHead;
	FIND_CONTEXT FindContext;
	LOGVIEW_MODE ReportMode;
	struct _MSP_DTL_OBJECT *DtlObject;
	HFONT hFontBold;
} REPORT_CONTEXT, *PREPORT_CONTEXT;

HWND
ReportDialog(
	IN HWND hWndParent,
	IN struct _MSP_DTL_OBJECT *DtlObject 
	);

ULONG
ReportInitImageList(
	IN PDIALOG_OBJECT Object,
	IN HWND	hWnd
	);

ULONG
ReportQueryImageList(
	IN PDIALOG_OBJECT Object,
	IN ULONG ProcessId
	);

LRESULT
ReportOnCustomDraw(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT 
ReportOnCommand(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT 
ReportOnOk(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT 
ReportOnCancel(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT 
ReportOnStack(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT 
ReportOnRecord(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ReportOnGetDispInfo(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT 
ReportOnContextMenu(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ReportOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ReportOnTtnGetDispInfo(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ReportOnTbnDropDown(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

INT_PTR CALLBACK
ReportProcedure(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

VOID
ReportOnClose(
    IN HWND hWnd
    );

HWND
ReportLoadBars(
	IN HWND hWnd,
	IN UINT Id
	);

HWND
ReportCreateToolbar(
	IN HWND hWndRebar,
	OUT HIMAGELIST *Normal,
	OUT HIMAGELIST *Grayed
	);

HWND
ReportCreateEdit(
	IN HWND hWndRebar
	);

ULONG CALLBACK
ReportDialogThread(
	IN PVOID Context
	);

LRESULT
ReportOnFindForward(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ReportOnFindBackward(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ReportOnCopy(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ReportOnSummary(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ReportOnSave(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ReportOnImportFilter(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ReportOnExportFilter(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ReportOnResetFilter(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ReportOnFilter(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

LRESULT
ReportOnUserGenerateCounter(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
ReportOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	);

VOID
ReportSetStatusText(
	IN HWND hWnd,
	IN struct _MSP_DTL_OBJECT *Object
	);

VOID
ReportSetFindContext(
	IN HWND hWnd,
	IN struct _MSP_DTL_OBJECT *DtlObject
	);

ULONG
ReportFindCallback(
    IN struct _FIND_CONTEXT *Object
    );

VOID
ReportEnterStallMode(
	IN PDIALOG_OBJECT Object 
	);

VOID
ReportExitStallMode(
	IN PDIALOG_OBJECT Object,
	IN ULONG Count
	);

BOOLEAN
ReportIsInStallMode(
	IN PDIALOG_OBJECT Object	
	);

LRESULT
ReportOnFilterCommand(
	IN HWND hWnd,
	IN UINT CommandId
	);

#ifdef __cplusplus
}
#endif

#endif