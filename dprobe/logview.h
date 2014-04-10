//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _LOGVIEW_H_
#define _LOGVIEW_H_

#ifdef __cplusplus
extern "C" {
#endif
	
#include "dprobe.h"

enum LogViewColumnType {
	LogColumnProcess,
	LogColumnSequence,
	LogColumnTimestamp,
	LogColumnPID,
	LogColumnTID,
	LogColumnProbe,
	LogColumnProvider,
	LogColumnReturn,
	LogColumnDuration,
	LogColumnDetail,
	LogColumnCount	
};

typedef enum _LOGVIEW_MODE {
	LOGVIEW_UPDATE_MODE,
	LOGVIEW_PENDING_MODE,
	LOGVIEW_STALL_MODE,
} LOGVIEW_MODE;

typedef struct _LOGVIEW_IMAGE {
	LIST_ENTRY ListEntry;
	ULONG ProcessId;
	ULONG ImageIndex;
} LOGVIEW_IMAGE, *PLOGVIEW_IMAGE;

#define WM_LOGVIEW_STALL (WM_USER + 10)

extern int LogViewColumnOrder[LogColumnCount];
extern LISTVIEW_COLUMN LogViewColumn[LogColumnCount];
extern LISTVIEW_OBJECT LogViewObject;
extern LOGVIEW_MODE LogViewMode;

PLISTVIEW_OBJECT
LogViewCreate(
	IN HWND hWndParent,
	IN ULONG CtrlId
	);

ULONG
LogViewInitImageList(
	IN HWND hWnd	
	);

ULONG
LogViewAddImageList(
	IN PWSTR FullPath,
	IN ULONG ProcessId
	);

ULONG
LogViewQueryImageList(
	IN ULONG ProcessId
	);

VOID
LogViewSetItemCount(
	IN ULONG Count
	);

LRESULT
LogViewOnCustomDraw(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
LogViewOnGetDispInfo(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT 
LogViewOnContextMenu(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
LogViewOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

LRESULT
LogViewOnTimer(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

VOID
LogViewEnterStallMode(
	VOID
	);

VOID
LogViewExitStallMode(
	IN ULONG NumberOfRecords
	);

BOOLEAN
LogViewIsInStallMode(
	VOID
	);

ULONG
LogViewGetItemCount(
	VOID
	);

struct _FIND_OBJECT;

ULONG
LogViewFindCallback(
    IN struct _FIND_CONTEXT *Object
    );

LRESULT 
LogViewOnWmUserFilter(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	);

int
LogViewGetFirstSelected(
	VOID
	); 

VOID
LogViewSetAutoScroll(
	IN BOOLEAN AutoScroll
	);

BOOLEAN
LogViewGetAutoScroll(
	VOID
	);

LRESULT
LogViewOnCopy(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	);

ULONG
LogViewCopyRecordByIndex(
	IN ULONG Index,
	IN HANDLE FileHandle,
	IN struct _MSP_DTL_OBJECT *DtlObject
	);

#ifdef __cplusplus
}
#endif

#endif