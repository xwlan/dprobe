//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "dprobe.h"
#include "counterdlg.h"
#include "treelist.h"
#include "mspdtl.h"

typedef enum _CounterColumnType {
	CounterName,
	CounterCount,
	CounterPercent,
	CounterException,
	CounterMinimum,
	CounterMaximum,
	CounterColumnCount,
} CounterColumnType;

HEADER_COLUMN 
CounterColumn[CounterColumnCount] = {
	{ {0}, 160, HDF_LEFT, FALSE, L" Name" },
	{ {0}, 60,  HDF_RIGHT, FALSE, L" Count" },
	{ {0}, 60,  HDF_RIGHT, FALSE, L" Percent" },
	{ {0}, 80,  HDF_RIGHT, FALSE, L" Exception" },
	{ {0}, 80,  HDF_RIGHT, FALSE, L" Minimum(ms)" },
	{ {0}, 80,  HDF_RIGHT, FALSE, L" Maximum(ms)" }
};

#define COUNTER_TIMER_ID  10

typedef struct _COUNTER_CONTEXT {
	PMSP_DTL_OBJECT DtlObject;
	PTREELIST_OBJECT TreeList;
	PMSP_COUNTER_OBJECT Counter;
	INT_PTR TimerId;
} COUNTER_CONTEXT, *PCOUNTER_CONTEXT;

INT_PTR
CounterDialog(
	IN HWND hWndOwner,
	IN struct _MSP_DTL_OBJECT *DtlObject
	)
{
	DIALOG_OBJECT Object = {0};
	COUNTER_CONTEXT Context = {0};
	INT_PTR Return;
	
	DIALOG_SCALER_CHILD Children[3] = {
		{ IDC_TREE_RECT, AlignRight, AlignBottom },
		{ IDC_TREE_COUNTER, AlignRight, AlignBottom },
		{ IDOK, AlignBoth, AlignBoth }
	};

	DIALOG_SCALER Scaler = {
		{0,0}, {0,0}, {0,0}, 3, Children
	};

	Context.DtlObject = DtlObject;

	Object.Context = &Context;
	Object.hWndParent = hWndOwner;
	Object.ResourceId = IDD_DIALOG_COUNTER;
	Object.Procedure = CounterProcedure;
	Object.Scaler = &Scaler;

	Return = DialogCreate(&Object);
	return Return;
}

LRESULT
CounterOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	//
	// Source       HIWORD(wParam)  LOWORD(wParam)  lParam 
	// Menu         0               MenuId          0   
	// Accelerator  1               AcceleratorId	0
	// Control      NotifyCode      ControlId       hWndCtrl
	//

	switch (LOWORD(wp)) {

	case IDOK:
	case IDCANCEL:
		CounterOnOk(hWnd, uMsg, wp, lp);
	    break;
	}

	return 0;
}

INT_PTR CALLBACK
CounterProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	INT_PTR Status;
	Status = FALSE;

	switch (uMsg) {
		
		case WM_COMMAND:
			CounterOnCommand(hWnd, uMsg, wp, lp);
			Status = TRUE;
			break;

		case WM_NOTIFY:
			CounterOnNotify(hWnd, uMsg, wp, lp);
			break;
		
		case WM_INITDIALOG:
			CounterOnInitDialog(hWnd, uMsg, wp, lp);
			SdkCenterWindow(hWnd);
			Status = TRUE;
			break;

		case WM_TIMER:
			CounterOnTimer(hWnd, uMsg, wp, lp);
			break;

		case WM_TREELIST_SORT:
			CounterOnTreeListSort(hWnd, uMsg, wp, lp);
			break;
	}

	return Status;
}

LRESULT
CounterOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCOUNTER_CONTEXT Context;
	PTREELIST_OBJECT TreeList;
	PMSP_DTL_OBJECT DtlObject;
	HWND hWndRect;
	HWND hWndDesktop;
	RECT Rect;
	TVINSERTSTRUCT tvi = {0};
	ULONG Number;
	ULONG NumberOfGroups;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, COUNTER_CONTEXT);
	DtlObject = Context->DtlObject;

	hWndDesktop = GetDesktopWindow();
	hWndRect = GetDlgItem(hWnd, IDC_TREE_RECT);
	GetWindowRect(hWndRect, &Rect);
	MapWindowRect(hWndDesktop, hWnd, &Rect);

	ShowWindow(hWndRect, SW_HIDE);

	TreeList = TreeListCreate(hWnd, TRUE, IDC_TREE_COUNTER, 
							  &Rect, hWnd, 
							  CounterFormatCallback, 
							  CounterColumnCount);

	Context->TreeList = TreeList;

	for(Number = 0; Number < CounterColumnCount; Number += 1) {
		TreeListInsertColumn(TreeList, Number, 
			                 CounterColumn[Number].Align, 
							 CounterColumn[Number].Width, 
							 CounterColumn[Number].Name );
	}

	SetWindowText(hWnd, L"Probe Summary");
	SdkSetMainIcon(hWnd);

	NumberOfGroups = CounterInsertGroup(Object);
	if (!NumberOfGroups) {
		return TRUE;
	}

	if (DtlObject->Type != DTL_REPORT) {
		Context->TimerId = SetTimer(hWnd, COUNTER_TIMER_ID, 1000, NULL);
	} else {
		CounterUpdate(TreeList);
	}

	return TRUE;
}

ULONG
CounterInsertGroup(
	IN PDIALOG_OBJECT Object
	)
{
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	PMSP_DTL_OBJECT DtlObject;
	PCOUNTER_CONTEXT Context;
	PTREELIST_OBJECT TreeList;
	PMSP_COUNTER_GROUP Group;
	TVINSERTSTRUCT tvi = {0};
	HTREEITEM hItemGroup;
	HTREEITEM hItemRoot;
	HWND hWndTree;
	PMSP_COUNTER_OBJECT Counter;
	ULONG Count;
	
	Context = SdkGetContext(Object, COUNTER_CONTEXT);
	DtlObject = Context->DtlObject;
	TreeList = Context->TreeList;
	hWndTree = TreeList->hWndTree;

	MspReferenceCounterObject(DtlObject, &Counter);
	Context->Counter = Counter;

	if (IsListEmpty(&Counter->GroupListHead)) {
		return 0;
	}

	Count = 0;

	//
	// Insert unique root node
	//

	tvi.hParent = NULL; 
	tvi.hInsertAfter = TVI_ROOT; 
	tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
	tvi.item.pszText = Counter->Name;
	tvi.item.lParam = (LPARAM)Counter; 
	hItemRoot = TreeView_InsertItem(hWndTree, &tvi);

	//
	// Insert process group
	//

	hItemGroup = hItemRoot;
	ListHead = &Counter->GroupListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {

		Group = CONTAINING_RECORD(ListEntry, MSP_COUNTER_GROUP, ListEntry);

		tvi.hParent = hItemRoot; 
		tvi.hInsertAfter = hItemGroup; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
		tvi.item.pszText = Group->Name;
		tvi.item.lParam = (LPARAM)Group; 
		hItemGroup = TreeView_InsertItem(hWndTree, &tvi);

		CounterInsertEntry(hWndTree, hItemGroup, Group);

		ListEntry = ListEntry->Flink;
		Count += 1;
	}

	return Count;
}

VOID
CounterInsertEntry(
	IN HWND hWndTree,
	IN HTREEITEM hItemGroup,
	IN struct _MSP_COUNTER_GROUP *Group
	)
{
	PMSP_COUNTER_ENTRY Entry;
	TVINSERTSTRUCT tvi = {0};
	HTREEITEM hTreeItem;
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	WCHAR Name[MAX_PATH];
	PWCHAR Ptr;

	hTreeItem = hItemGroup;

	//
	// Insert active counter list
	//

	ListHead = &Group->ActiveListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Entry = CONTAINING_RECORD(ListEntry, MSP_COUNTER_ENTRY, ListEntry);
		tvi.hParent = hItemGroup; 
		tvi.hInsertAfter = hTreeItem; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 

		//
		// N.B. The name is formatted as dll!api, we need provide only api name
		// to BspUnDecorateSymbolName
		//

		if (Entry->Active) {
			Ptr = wcschr(Entry->Active->Name, L'!');
			BspUnDecorateSymbolName(Ptr + 1, Name, MAX_PATH, UNDNAME_NAME_ONLY);
			tvi.item.pszText = Name;
		} else {
			Ptr = wcschr(Entry->Retire.Name, L'!');
			BspUnDecorateSymbolName(Ptr + 1, Name, MAX_PATH, UNDNAME_NAME_ONLY);
			tvi.item.pszText = Name;
		}

		tvi.item.lParam = (LPARAM)Entry; 
		hTreeItem = TreeView_InsertItem(hWndTree, &tvi);
		ListEntry = ListEntry->Flink;
	}

	//
	// Insert retire counter list
	//

	ListHead = &Group->RetireListHead;
	ListEntry = ListHead->Flink;

	while (ListEntry != ListHead) {
		Entry = CONTAINING_RECORD(ListEntry, MSP_COUNTER_ENTRY, ListEntry);
		tvi.hParent = hItemGroup; 
		tvi.hInsertAfter = hTreeItem; 
		tvi.item.mask = TVIF_TEXT | TVIF_PARAM; 
		tvi.item.pszText = L"";
		tvi.item.lParam = (LPARAM)Entry; 
		hTreeItem = TreeView_InsertItem(hWndTree, &tvi);
		ListEntry = ListEntry->Flink;
	}
}

LRESULT
CounterOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCOUNTER_CONTEXT Context;
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, COUNTER_CONTEXT);
	
	if (Context->Counter != NULL) {
		MspReleaseCounterObject(Context->DtlObject, 
			                    Context->Counter);
	}

	if (Context->TimerId != 0) {
		KillTimer(hWnd, Context->TimerId);
	}

	EndDialog(hWnd, IDOK);
	return 0;
}

LRESULT
CounterOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	NMHDR *lpnmhdr = (NMHDR *)lp;

	switch (lpnmhdr->code) {

		case LVN_ITEMCHANGED:
			break;

		case LVN_COLUMNCLICK:
			break;
			
	}

	return 0L;
}

LRESULT
CounterOnTimer(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PCOUNTER_CONTEXT Context;
	PTREELIST_OBJECT TreeList;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, COUNTER_CONTEXT);
	TreeList = Context->TreeList;
	CounterUpdate(TreeList);

	return 0;
}

VOID
CounterUpdate(
	IN struct _TREELIST_OBJECT *TreeList
	)
{
	RECT Rect;

	GetClientRect(TreeList->hWndTree, &Rect);
	InvalidateRect(TreeList->hWndTree, &Rect, TRUE);
}

typedef struct _COUNTER_SORT_CONTEXT {
	PMSP_COUNTER_GROUP Group;
	LIST_SORT_ORDER Order;
	ULONG Column;
} COUNTER_SORT_CONTEXT, *PCOUNTER_SORT_CONTEXT;

LRESULT
CounterOnTreeListSort(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	ULONG Column;
	TVSORTCB Tsc = {0};
	PTREELIST_OBJECT TreeList;
	PDIALOG_OBJECT Object;
	PCOUNTER_CONTEXT Context;
	HTREEITEM hItemGroup;
	HTREEITEM hItemRoot;
	TVITEM tvi = {0};
	COUNTER_SORT_CONTEXT SortContext;

	Column = (ULONG)wp;
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, COUNTER_CONTEXT);
	TreeList = Context->TreeList;
	
	hItemRoot = TreeView_GetRoot(TreeList->hWndTree);
	hItemGroup = TreeView_GetChild(TreeList->hWndTree, hItemRoot);

	while (hItemGroup != NULL) {
		
		tvi.mask = TVIF_PARAM;
		tvi.hItem = hItemGroup;
		TreeView_GetItem(TreeList->hWndTree, &tvi);

		SortContext.Group = (PMSP_COUNTER_GROUP)tvi.lParam;
		SortContext.Column = Column;
		SortContext.Order = TreeList->SortOrder;

		Tsc.hParent = hItemGroup;
		Tsc.lParam = (LPARAM)&SortContext;
		Tsc.lpfnCompare = CounterCompareCallback;
		TreeView_SortChildrenCB(TreeList->hWndTree, &Tsc, 0);

		hItemGroup = TreeView_GetNextSibling(TreeList->hWndTree, hItemGroup);
	}

	return 0;
}

int CALLBACK
CounterCompareCallback(
	IN LPARAM First, 
	IN LPARAM Second,
	IN LPARAM Param
	)
{
	PCOUNTER_SORT_CONTEXT Context;
	PMSP_COUNTER_GROUP Group;
	PMSP_COUNTER_ENTRY Entry1;
	PMSP_COUNTER_ENTRY Entry2;
	PWCHAR Name1, Name2;
	ULONG Count1, Count2;
	int Result;

	Context = (PCOUNTER_SORT_CONTEXT)Param;
	Group = Context->Group;
	Entry1 = (PMSP_COUNTER_ENTRY)First;
	Entry2 = (PMSP_COUNTER_ENTRY)Second;
	Result = 0;

	switch (Context->Column) {

		case CounterName:
			if (Entry1->Active != NULL) {
				Name1 = (PWCHAR)Entry1->Active->Name;
			} else {
				Name1 = (PWCHAR)Entry1->Retire.Name;
			}
			if (Entry2->Active != NULL) {
				Name2 = (PWCHAR)Entry2->Active->Name;
			}
			else {
				Name2 = (PWCHAR)Entry2->Retire.Name;
			}
			Result = (int)_wcsicmp(Name1, Name2);
			break;

		case CounterCount:
		case CounterPercent:

			if (Entry1->Active != NULL) {
				Count1 = Entry1->Active->NumberOfCalls;
			} else {
				Count1 = Entry1->Retire.NumberOfCalls;
			}
			if (Entry2->Active != NULL) {
				Count2 = Entry2->Active->NumberOfCalls;
			}
			else {
				Count2 = Entry2->Retire.NumberOfCalls;
			}
			Result = (int)(Count1 - Count2);
			break;

		case CounterException:
			if (Entry1->Active != NULL) {
				Count1 = Entry1->Active->NumberOfExceptions;
			} else {
				Count1 = Entry1->Retire.NumberOfExceptions;
			}
			if (Entry2->Active != NULL) {
				Count2 = Entry2->Active->NumberOfExceptions;
			}
			else {
				Count2 = Entry2->Retire.NumberOfExceptions;
			}
			Result = (int)(Count1 - Count2);
			break;

		case CounterMinimum:
			if (Entry1->Active != NULL) {
				Count1 = Entry1->Active->MinimumDuration;
			} else {
				Count1 = Entry1->Retire.MinimumDuration;
			}
			if (Entry2->Active != NULL) {
				Count2 = Entry2->Active->MinimumDuration;
			}
			else {
				Count2 = Entry2->Retire.MinimumDuration;
			}
			Result = (int)(Count1 - Count2);
			break;

		case CounterMaximum:
			if (Entry1->Active != NULL) {
				Count1 = Entry1->Active->MaximumDuration;
			} else {
				Count1 = Entry1->Retire.MaximumDuration;
			}
			if (Entry2->Active != NULL) {
				Count2 = Entry2->Active->MaximumDuration;
			}
			else {
				Count2 = Entry2->Retire.MaximumDuration;
			}
			Result = (int)(Count1 - Count2);
			break;
	}

	if (Context->Order == SortOrderAscendent) {
		return Result;
	} else {
		return -Result;
	}
}

VOID CALLBACK
CounterFormatCallback(
	IN struct _TREELIST_OBJECT *TreeList,
	IN HTREEITEM hTreeItem,
	IN PVOID Value,
	IN ULONG Column,
	OUT PWCHAR Buffer,
	IN SIZE_T Length
	)
{
	PMSP_COUNTER_ENTRY Entry;

	Entry = (PMSP_COUNTER_ENTRY)Value;
	switch (Entry->Level) {
	
		case CounterEntryLevel:
			CounterFormatEntry(Entry, Column, Buffer, Length);
			break;

		case CounterGroupLevel:
			CounterFormatGroup((PMSP_COUNTER_GROUP)Entry, Column, Buffer, Length);
			break;

		case CounterRootLevel:
			CounterFormatRoot((PMSP_COUNTER_OBJECT)Entry, Column, Buffer, Length);
			break;

		default:
			__debugbreak();
	}

}

VOID 
CounterFormatEntry(
	IN PMSP_COUNTER_ENTRY Entry,
	IN ULONG Column,
	OUT PWCHAR Buffer,
	IN SIZE_T Length
	)
{
	PMSP_COUNTER_GROUP Group;
	PBTR_COUNTER_TABLE Table;
	PBTR_COUNTER_ENTRY Active;
	double Percent;
	double Duration;
	WCHAR Name[MAX_PATH];
	WCHAR Dll[MAX_PATH];
	PWCHAR Ptr;
	ULONG Distance;

	Active = Entry->Active;

	switch (Column) {

		case CounterName:
			if (Active != NULL) {

				Ptr = wcschr(Active->Name, L'!');
				BspUnDecorateSymbolName(Ptr + 1, Name, MAX_PATH, UNDNAME_NAME_ONLY);

				Distance = (ULONG)((ULONG_PTR)Ptr - (ULONG_PTR)Active->Name);
				memcpy(Dll, Active->Name, Distance);
				Dll[Distance / 2] = 0;

				StringCchPrintf(Buffer, Length, L"%s!%s", Dll, Name);

			} else {

				Ptr = wcschr(Entry->Retire.Name, L'!');
				BspUnDecorateSymbolName(Ptr + 1, Name, MAX_PATH, UNDNAME_NAME_ONLY);

				Distance = (ULONG)((ULONG_PTR)Ptr - (ULONG_PTR)Entry->Retire.Name);
				memcpy(Dll, Entry->Retire.Name, Distance);
				Dll[Distance / 2] = 0;

				StringCchPrintf(Buffer, Length, L"%s!%s", Dll, Name);
			}
			break;

		case CounterCount:
			if (Active != NULL) {
				StringCchPrintf(Buffer, Length, L"%u", Active->NumberOfCalls);
			} else {
				StringCchPrintf(Buffer, Length, L"%u", Entry->Retire.NumberOfCalls);
			}
			break;

		case CounterPercent:
			{
				ULONG Count;
				ULONG GroupCount;

				Group = Entry->Group;
				Table = Group->Table;

				if (Active != NULL) {
					Count = Active->NumberOfCalls;
				} else {
					Count = Entry->Retire.NumberOfCalls;
				}

				if (Table != NULL) {
					GroupCount = Table->NumberOfCalls;
				} else {
					GroupCount = Group->Retire.NumberOfCalls;
				}
				
				Percent = (Count * 100.0) / (GroupCount * 1.0);
				StringCchPrintf(Buffer, Length, L"%.3f", Percent);
			}
			break;

		case CounterException:
			if (Active != NULL) {
				StringCchPrintf(Buffer, Length, L"%u", Active->NumberOfExceptions);
			} else {
				StringCchPrintf(Buffer, Length, L"%u", Entry->Retire.NumberOfExceptions);
			}
			break;

		case CounterMinimum:
			if (Active != NULL) {
				Duration = BspComputeMilliseconds(Active->MinimumDuration);
			} else {
				Duration = BspComputeMilliseconds(Entry->Retire.MinimumDuration);
			}
			StringCchPrintf(Buffer, Length, L"%.3f", Duration);
			break;

		case CounterMaximum:
			if (Active != NULL) {
				Duration = BspComputeMilliseconds(Active->MaximumDuration);
			} else {
				Duration = BspComputeMilliseconds(Entry->Retire.MaximumDuration);
			}
			StringCchPrintf(Buffer, Length, L"%.3f", Duration);
			break;

		default:
			StringCchCopy(Buffer, Length, L"N/A");
	}

}

VOID 
CounterFormatGroup(
	IN PMSP_COUNTER_GROUP Group,
	IN ULONG Column,
	OUT PWCHAR Buffer,
	IN SIZE_T Length
	)
{
	PMSP_COUNTER_OBJECT Root;
	PBTR_COUNTER_TABLE Table;
	double Percent;

	Table = Group->Table;
	switch (Column) {

		case CounterName:
			StringCchPrintf(Buffer, Length, L"%s", Group->Name);
			break;

		case CounterCount:
			if (Table != NULL) {
				StringCchPrintf(Buffer, Length, L"%u", Table->NumberOfCalls);
			} else {
				StringCchPrintf(Buffer, Length, L"%u", Group->Retire.NumberOfCalls);
			}
			break;

		case CounterPercent:

			Root = Group->Object;
			CounterUpdateRoot(Root);

			if (Table != NULL) {
				Percent = (Table->NumberOfCalls * 100.0) / (Root->NumberOfCalls * 1.0);
			} else {
				Percent = (Group->Retire.NumberOfCalls * 100.0) / (Root->NumberOfCalls * 1.0);
			}
			StringCchPrintf(Buffer, Length, L"%.3f", Percent);
			break;

		case CounterException:

			if (Table != NULL) {
				StringCchPrintf(Buffer, Length, L"%u", Table->NumberOfExceptions);
			} else {
				StringCchPrintf(Buffer, Length, L"%u", Group->Retire.NumberOfExceptions);
			}
			break;

		case CounterMinimum:
		case CounterMaximum:
			StringCchCopy(Buffer, Length, L"N/A");
			break;

		default:
			StringCchCopy(Buffer, Length, L"N/A");
	}

}

VOID 
CounterFormatRoot(
	IN PMSP_COUNTER_OBJECT Root,
	IN ULONG Column,
	OUT PWCHAR Buffer,
	IN SIZE_T Length
	)
{
	switch (Column) {

		case CounterName:
			StringCchPrintf(Buffer, Length, L"%s", Root->Name);
			break;

		case CounterCount:
			CounterUpdateRoot(Root);
			StringCchPrintf(Buffer, Length, L"%u", Root->NumberOfCalls);
			break;

		case CounterPercent:
			StringCchCopy(Buffer, Length, L"100.000");
			break;

		case CounterException:
			CounterUpdateRoot(Root);
			StringCchPrintf(Buffer, Length, L"%u", Root->NumberOfExceptions);
			break;

		case CounterMinimum:
		case CounterMaximum:
			StringCchCopy(Buffer, Length, L"N/A");
			break;

		default:
			StringCchCopy(Buffer, Length, L"N/A");
	}
}

VOID
CounterUpdateRoot(
	IN struct _MSP_COUNTER_OBJECT *Object
	)
{
	PLIST_ENTRY ListEntry;
	PLIST_ENTRY ListHead;
	PMSP_COUNTER_GROUP Group;

	ListHead = &Object->GroupListHead;
	ListEntry = ListHead->Flink;

	Object->NumberOfCalls = 0;
	Object->NumberOfExceptions = 0;
	Object->NumberOfStackTraces = 0;

	while (ListEntry != ListHead) {
		Group = CONTAINING_RECORD(ListEntry, MSP_COUNTER_GROUP, ListEntry);
		if (Group->Table) {
			Object->NumberOfCalls += Group->Table->NumberOfCalls;
			Object->NumberOfStackTraces += Group->Table->NumberOfStackTraces;
			Object->NumberOfExceptions += Group->Table->NumberOfExceptions;
		} else {
			Object->NumberOfCalls += Group->Retire.NumberOfCalls;
			Object->NumberOfStackTraces += Group->Retire.NumberOfStackTraces;
			Object->NumberOfExceptions += Group->Retire.NumberOfExceptions;
		}
		ListEntry = ListEntry->Flink;
	}

}