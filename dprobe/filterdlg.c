//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "fastdlg.h"
#include "filterdlg.h"
#include "dprobe.h"
#include "mspprocess.h"
#include "splitbar.h"
#include "csp.h"

#define FILTER_TREE_STYLE  (TVS_INFOTIP | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_CHECKBOXES)

#define CHECKED   2 
#define UNCHECKED 1

#define IS_CHECKED(_V) \
	(((_V) >> 12) == 2 ? TRUE : FALSE)

#define IS_UNCHECKED(_V) \
	(((_V) >> 12) == 1 ? TRUE : FALSE)

PWSTR FILTER_INFOTIP_FORMAT = 
		L"                       \n"
		L"  Filter:       %s     \n"
		L"  Description:  %s     \n"
		L"  GUID:         {%s}   \n"
		L"  Version:      %d.%d  \n"
		L"  Author:       %s     \n"
		L"                       \n"
		L"\n";

LISTVIEW_COLUMN FilterColumn[2] = {
	{ 160, L"Filter", LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 200, L"Probe",  LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

#define FILTER_SELECT_STRING L"Total select %u probes"

INT_PTR
FilterDialog(
	IN HWND hWnd,
	IN PBSP_PROCESS Process,
	OUT struct _MSP_USER_COMMAND **Command
	)
{
	DIALOG_OBJECT Object = {0};
	FILTER_CONTEXT Context = {0};
	INT_PTR Return;

	DIALOG_SCALER_CHILD Children[6] = {
		{ IDC_TREE_FILTER, AlignRight, AlignBottom },
		{ IDC_LIST_FILTER, AlignBoth, AlignBottom },
		{ IDOK,     AlignBoth, AlignBoth },
		{ IDCANCEL, AlignBoth, AlignBoth },
		{ IDC_BUTTON_EXPORT, AlignBoth, AlignBoth },
		{ IDC_STATIC, AlignNone, AlignBoth }
	};

	DIALOG_SCALER Scaler = {
		{0,0}, {0,0}, {0,0}, 6, Children
	};

	*Command = NULL;

	InitializeListHead(&Context.FilterList);

	Context.Process = Process;
	Object.Context = &Context;
	Object.hWndParent = hWnd;
	Object.ResourceId = IDD_DIALOG_FILTER;
	Object.Procedure = FilterProcedure;
	Object.Scaler = &Scaler;

	Return = DialogCreate(&Object);
	*Command = Context.Command;
	return Return;
}

INT_PTR CALLBACK
FilterProcedure(
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
			return FilterOnCommand(hWnd, uMsg, wp, lp);
		
		case WM_INITDIALOG:
			FilterOnInitDialog(hWnd, uMsg, wp, lp);
			SdkCenterWindow(hWnd);
			Status = TRUE;
		
		case WM_NOTIFY:
			return FilterOnNotify(hWnd, uMsg, wp, lp);

		case WM_CTLCOLORSTATIC:
			return FilterOnCtlColorStatic(hWnd, uMsg, wp, lp);

		case WM_MOUSEMOVE:
			return FilterOnMouseMove(hWnd, uMsg, wp, lp);

		case WM_LBUTTONDOWN:
			return FilterOnLButtonDown(hWnd, uMsg, wp, lp);

		case WM_LBUTTONUP:
			return FilterOnLButtonUp(hWnd, uMsg, wp, lp);
		
		case WM_TVN_ITEMCHANGED:
			return FilterOnWmItemChanged(hWnd, uMsg, wp, lp);

		case WM_TVN_UNCHECKITEM:
			return FilterOnWmUncheckItem(hWnd, uMsg, wp, lp);
		
		case WM_TVN_CHECKITEM:
			return FilterOnWmCheckItem(hWnd, uMsg, wp, lp);
	}

	return Status;
}

LRESULT
FilterOnWmItemChanged(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PSDK_TV_STATE State;
	HWND hWndTree;
	TVITEMEX Item = {0};

	State = (PSDK_TV_STATE)lp;
	hWndTree = GetDlgItem(hWnd, IDC_TREE_FILTER);

	//
	// Get the current state of treeview item
	//

	Item.mask = TVIF_STATE | TVIF_PARAM;
	Item.stateMask = TVIS_STATEIMAGEMASK ;
	Item.hItem = State->hItem;
	TreeView_GetItem(hWndTree, &Item);

	State->NewState = Item.state;
	State->lParam = Item.lParam;

	//
	// Delegate to FastOnItemChanged()
	//

	FilterOnItemChanged(hWnd, hWndTree, State->hItem, State->NewState, State->OldState, State->lParam);
	SdkFree(State);

	return 0L;
}

//
// N.B. Only XP/2003 requires this routine
//

LRESULT
FilterOnWmUncheckItem(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	HWND hWndTree;
	TVITEMEX Item = {0};
	HTREEITEM hItem;
	ULONG NewState;
	ULONG OldState;

	hWndTree = GetDlgItem(hWnd, IDC_TREE_FILTER);

	//
	// Get the current state of treeview item
	//

	hItem = (HTREEITEM)wp;

	Item.mask = TVIF_STATE;
	Item.stateMask = TVIS_STATEIMAGEMASK ;
	Item.hItem = hItem;
	TreeView_GetItem(hWndTree, &Item);

	OldState = Item.state;

	//
	// Clear check state and get new state
	//

	TreeView_SetCheckState(hWndTree, hItem, FALSE);

	Item.mask = TVIF_STATE;
	Item.stateMask = TVIS_STATEIMAGEMASK ;
	Item.hItem = hItem;
	TreeView_GetItem(hWndTree, &Item);

	NewState = Item.state;

	//
	// N.B. This message can only be triggered to child item, NULL lParam
	//

	FilterOnItemChanged(hWnd, hWndTree, hItem, NewState, OldState, 0);
	
	//
	// Always clear check state of its parent
	//

	TreeView_SetCheckState(hWndTree, TreeView_GetParent(hWndTree, hItem), FALSE);
	return 0L;
}

//
// N.B. Only XP/2003 requires this routine
//

LRESULT
FilterOnWmCheckItem(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	HWND hWndTree;
	TVITEMEX Item = {0};
	HTREEITEM hItem;
	ULONG NewState;
	ULONG OldState;

	hWndTree = GetDlgItem(hWnd, IDC_TREE_FILTER);

	//
	// Get the current state of treeview item
	//

	hItem = (HTREEITEM)wp;

	Item.mask = TVIF_STATE;
	Item.stateMask = TVIS_STATEIMAGEMASK ;
	Item.hItem = hItem;
	TreeView_GetItem(hWndTree, &Item);

	OldState = Item.state;

	//
	// Check state and get new state
	//

	TreeView_SetCheckState(hWndTree, hItem, TRUE);

	Item.mask = TVIF_STATE;
	Item.stateMask = TVIS_STATEIMAGEMASK ;
	Item.hItem = hItem;
	TreeView_GetItem(hWndTree, &Item);

	NewState = Item.state;

	//
	// N.B. This message can only be triggered to child item, NULL lParam
	//

	FilterOnItemChanged(hWnd, hWndTree, hItem, NewState, OldState, 0);
	return 0L;
}

LRESULT
FilterOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	WCHAR Buffer[MAX_PATH];
	WCHAR Format[MAX_PATH];
	HWND hWndStatic;
	PBSP_PROCESS Process;
	PDIALOG_OBJECT Object;
	PFILTER_CONTEXT Context;
	HWND hWndTree;
	HWND hWndList;
	RECT ListRect, TreeRect;
	LVCOLUMN Column = {0};
	PLISTVIEW_OBJECT ListObject;
	int i;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, FILTER_CONTEXT);
	
	hWndList = GetDlgItem(hWnd, IDC_LIST_FILTER);
	hWndTree = GetDlgItem(hWnd, IDC_TREE_FILTER);

	GetWindowRect(hWndList, &ListRect);
	GetWindowRect(hWndTree, &TreeRect);

	//
	// Initialize splitbar information
	//

	Context->SplitbarBorder = ListRect.left - TreeRect.right;
	Context->SplitbarLeft = 0;
	Context->DragMode = FALSE;

	//
	// Insert list control columns
	//

	ListView_SetUnicodeFormat(hWndList, TRUE);
    ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
    ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_INFOTIP, LVS_EX_INFOTIP);

	Column.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 
	  
	for (i = 0; i < 2; i++) { 
        Column.iSubItem = i;
		Column.pszText = FilterColumn[i].Title;	
		Column.cx = FilterColumn[i].Width;     
		Column.fmt = FilterColumn[i].Align;
		ListView_InsertColumn(hWndList, i, &Column);
    } 
	
	ListObject = (PLISTVIEW_OBJECT)SdkMalloc(sizeof(LISTVIEW_OBJECT));
	ListObject->hWnd = hWndList;
	ListObject->Column = FilterColumn;
	ListObject->Count = 2;
	ListObject->SortOrder = SortOrderDescendent;
	ListObject->LastClickedColumn = 0;

	Context->ListObject = ListObject;

	//
	// Subclass list control to get chance to handle WM_CHAR, WM_KEYDOWN 
	// keyboard events.
	//

	SetWindowSubclass(hWndList, FilterListProcedure, 0, 0);

	//
	// Insert filter and its functions
	//

	FilterInsertFilter(Object);

	hWndStatic = GetDlgItem(hWnd, IDC_STATIC);
	LoadString(SdkInstance, IDS_FILTER_SELECT, Format, MAX_PATH);
	StringCchPrintf(Buffer, MAX_PATH, Format, 
		            Context->NumberOfSelected);

	SetWindowText(hWndStatic, Buffer);

	Process = Context->Process;
	LoadString(SdkInstance, IDS_FILTER_TITLE, Format, MAX_PATH);
	StringCchPrintf(Buffer, MAX_PATH, Format, 
		            Process->Name, Process->ProcessId);
	
	SetWindowText(hWnd, Buffer);
	SdkSetMainIcon(hWnd);

	if (Context->NumberOfSelected != 0) {
		ListViewSelectSingle(hWndList, 0);
		SetFocus(hWndList);
	}
	return TRUE;
}

ULONG
FilterInsertFilter(
	IN PDIALOG_OBJECT Object
	)
{
	ULONG Status;
	HWND hWnd;
	HWND hWndTree;
	PFILTER_CONTEXT Context;
	PBSP_PROCESS Process;
	PLIST_ENTRY ListEntry;
	PBTR_FILTER Filter;
	TVINSERTSTRUCT Item = {0};
	HTREEITEM hTreeItem;
	LONG Style;
	PFILTER_NODE Node;

	hWnd = Object->hWnd;
	hWndTree = GetDlgItem(hWnd, IDC_TREE_FILTER);
	TreeView_SetUnicodeFormat(hWndTree, TRUE);

	Style = GetWindowLong(hWndTree, GWL_STYLE);
	SetWindowLong(hWndTree, GWL_STYLE, Style | FILTER_TREE_STYLE);

	//
	// Query filter list for target process
	//

	Context = Object->Context;
	Process = Context->Process;

	Status = MspGetFilterList(Process->ProcessId, &Context->FilterList);
	if (Status != MSP_STATUS_OK) {
		return Status;
	}

	ListEntry = Context->FilterList.Flink;

	while (ListEntry != &Context->FilterList) { 

		Filter = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);
		
		Item.hParent = 0; 
		Item.hInsertAfter = TVI_ROOT; 
		Item.item.mask = TVIF_TEXT | TVIF_PARAM; 
		Item.item.pszText = Filter->FilterName;

		//
		// Attach filter node to each filter item
		//

		Node = (PFILTER_NODE)SdkMalloc(sizeof(FILTER_NODE));
		RtlZeroMemory(Node, sizeof(*Node));
		Node->Filter = Filter;
		BtrInitializeBitMap(&Node->BitMap, Node->Bits, Filter->ProbesCount);

		Item.item.lParam = (LPARAM)Node; 

		hTreeItem = TreeView_InsertItem(hWndTree, &Item);
		Status = FilterInsertProbe(Object, hWndTree, hTreeItem, Node);

		if (Status != S_OK) {
			TreeView_DeleteItem(hWndTree, hTreeItem);
		}

		ListEntry = ListEntry->Flink;
	}
    
	return 0;
}

ULONG
FilterInsertProbe(
	IN PDIALOG_OBJECT Object,
	IN HWND hWndTree,
	IN HTREEITEM hTreeItem,
	IN PFILTER_NODE Node
	)
{
	ULONG i;
	HWND hWndList;
	WCHAR ApiName[MAX_PATH];
	WCHAR Buffer[MAX_PATH];
	TVINSERTSTRUCT Item = {0};
	LVITEM ListItem = {0};
	HTREEITEM hInserted;
	PBTR_FILTER Filter;
	PFILTER_CONTEXT Context;
	BOOLEAN SetBit;
	
	Context = SdkGetContext(Object, FILTER_CONTEXT);
	hWndList = GetDlgItem(Object->hWnd, IDC_LIST_FILTER);
	Filter = Node->Filter;

	hInserted = hTreeItem;

	for (i = 0; i < Filter->ProbesCount; i++) {

		Item.hParent = hTreeItem; 
		Item.hInsertAfter = hInserted; 
		Item.item.mask = TVIF_TEXT | TVIF_PARAM; 
		
		//
		// Build item name as "DllName!ApiName" format
		//

		BspUnDecorateSymbolName(Filter->Probes[i].ApiName, 
			                    ApiName, MAX_PATH, UNDNAME_COMPLETE);
		//StringCchCopy(ApiName, MAX_PATH, Filter->Probes[i].ApiName);
		
		if (Filter->Probes[i].DllName) {
			StringCchPrintf(Buffer, MAX_PATH, L"%s!%s", 
				            _wcslwr(Filter->Probes[i].DllName), ApiName);
		} else {
			StringCchPrintf(Buffer, MAX_PATH, L"*!%s", ApiName);
		}

		SetBit = BtrTestBit(&Filter->BitMap, i);
		Item.item.lParam = (LPARAM)SetBit;
		Item.item.pszText = Buffer;

		//
		// Test whether it's an activated probe
		//

		hInserted = TreeView_InsertItem(hWndTree, &Item);
		if (SetBit) {
			if (BspIsVistaAbove()) {
				TreeView_SetCheckState(hWndTree, hInserted, TRUE);
			} else {
				PostMessage(GetParent(hWndTree), WM_TVN_CHECKITEM, (WPARAM)hInserted, 0);
			}
		}
		
	}

	return 0;
}

LRESULT
FilterOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	NMHDR *lpnmhdr = (NMHDR *)lp;
	HWND hWndTree;
	HTREEITEM hItem;
	TVITEMEX Item = {0};
	PSDK_TV_STATE State;

	hWndTree = GetDlgItem(hWnd, IDC_TREE_FILTER);

	if (IDC_TREE_FILTER == LOWORD(wp)) {

		switch (lpnmhdr->code)  {

			case TVN_GETINFOTIP:
				FilterOnInfoTip(hWnd, hWndTree, (LPNMTVGETINFOTIP)lp);
				break;

			case TVN_ITEMCHANGED:
				{
					NMTVITEMCHANGE *lpnmic;

					lpnmic = (NMTVITEMCHANGE *)lp;
					FilterOnItemChanged(hWnd, hWndTree, lpnmic->hItem,
									  lpnmic->uStateNew, lpnmic->uStateOld,
									  lpnmic->lParam);
				}
				break;
			
			case TVN_KEYDOWN:
				{
					LPNMTVKEYDOWN lpnmkd = (LPNMTVKEYDOWN)lp;

					if (BspIsVistaAbove()) {
						return 0;
					}
					
					if (lpnmkd->wVKey != VK_SPACE) {
						return 0;
					}

					//
					// User press SPACE key to check/uncheck item
					//

					hItem = TreeView_GetSelection(hWndTree);
					if (hItem != NULL) {

						Item.mask = TVIF_STATE;
						Item.stateMask = TVIS_STATEIMAGEMASK ;
						Item.hItem = hItem;
						TreeView_GetItem(hWndTree, &Item);

						State = (PSDK_TV_STATE)SdkMalloc(sizeof(SDK_TV_STATE));
						State->hItem = hItem;
						State->OldState = Item.state;
						
						//
						// N.B. SDK_TVN_ITEMCHANGED must retrieve the new state
						// of treeitem explicitly, we only fill oldstate here
						//

						State->NewState = 0;
						State->lParam = 0;

						PostMessage(hWnd, WM_TVN_ITEMCHANGED, 0, (LPARAM)State);

					}
				}

			case NM_CLICK:
				{
					DWORD dwpos;
					TVHITTESTINFO ht = {0};

					//
					// If it's VISTA above, we wait to handle TVN_ITEMCHANGED,
					// just ignore NM_CLICK
					//

					if (BspIsVistaAbove()) {
						return 0;
					}
					
					dwpos = GetMessagePos();
					ht.pt.x = GET_X_LPARAM(dwpos);
					ht.pt.y = GET_Y_LPARAM(dwpos);

					MapWindowPoints(HWND_DESKTOP, lpnmhdr->hwndFrom, &ht.pt, 1);

					TreeView_HitTest(lpnmhdr->hwndFrom, &ht);
					if(TVHT_ONITEMSTATEICON & ht.flags) {

						Item.mask = TVIF_STATE;
						Item.stateMask = TVIS_STATEIMAGEMASK ;
						Item.hItem = ht.hItem;
						TreeView_GetItem(hWndTree, &Item);

						State = (PSDK_TV_STATE)SdkMalloc(sizeof(SDK_TV_STATE));
						State->hItem = ht.hItem;
						State->OldState = Item.state;
						
						//
						// N.B. SDK_TVN_ITEMCHANGED must retrieve the new state
						// of treeitem explicitly, we only fill oldstate here
						//

						State->NewState = 0;
						State->lParam = 0;

						PostMessage(hWnd, WM_TVN_ITEMCHANGED, 0, (LPARAM)State);
					}

					//
					// Return 0 to continue default processing.
					//

					return 0;
				}

				break;
		}
	}

	if (IDC_LIST_FILTER == LOWORD(wp)) {
		
		switch (lpnmhdr->code)  {

			case LVN_COLUMNCLICK:
				return FilterOnColumnClick(hWnd, (LPNMLISTVIEW)lp);

		}
	}

	return 0;
}

VOID
FilterCheckChildren(
	IN HWND hWnd,
	IN PFILTER_CONTEXT Context,
	IN PFILTER_NODE Node,
	IN HWND hWndTree,
	IN HTREEITEM Parent,
	IN BOOLEAN Check
	)
{
	HTREEITEM Handle;
	TVITEMEX Item = {0};
	TVITEMEX ParentItem = {0};
	ULONG Unchecked, Checked;
	WCHAR Buffer[MAX_PATH];

	Handle = TreeView_GetChild(hWndTree, Parent);

	Checked = INDEXTOSTATEIMAGEMASK(CHECKED);
	Unchecked = INDEXTOSTATEIMAGEMASK(UNCHECKED);

	while (Handle != NULL) {
		
		Item.mask = TVIF_STATE | TVIF_TEXT;
		Item.stateMask = TVIS_STATEIMAGEMASK ;
		Item.hItem = Handle;
		Item.pszText = Buffer;
		Item.cchTextMax = MAX_PATH;

		TreeView_GetItem(hWndTree, &Item);

		if (Check) {

			if (Item.state & Unchecked) {

				Item.state = Checked;
				TreeView_SetItemState(hWndTree, Handle, Item.state, TVIS_STATEIMAGEMASK);

				//
				// N.B. TreeView_SetItemState will incur a TVN_ITEMCHANGED notification
				// for VISTA and above systems, however, we need explicitly insert item
				// here for Windows XP/ SERVER 2003.
				//

				if (!BspIsVistaAbove()) {

					//
					// Insert item
					//

					FilterInsertListItem(hWnd, hWndTree, Node,
						               Handle, Buffer, TRUE);

					Node->NumberOfSelected += 1;
					Context->NumberOfSelected += 1;

					StringCchPrintf(Buffer, MAX_PATH, FILTER_SELECT_STRING, 
									Context->NumberOfSelected);
					SetWindowText(GetDlgItem(hWnd, IDC_STATIC), Buffer);
				}
			}
		} 
		else {
			if (Item.state & Checked) {
				
				Item.state = Unchecked;
				TreeView_SetItemState(hWndTree, Handle, Item.state, TVIS_STATEIMAGEMASK);

				if (!BspIsVistaAbove()) {

					//
					// Delete item
					//

					FilterInsertListItem(hWnd, hWndTree, Node,
						               Handle, Buffer, FALSE);

					Node->NumberOfSelected -= 1;
					Context->NumberOfSelected -= 1;

					StringCchPrintf(Buffer, MAX_PATH, FILTER_SELECT_STRING, 
									Context->NumberOfSelected);
					SetWindowText(GetDlgItem(hWnd, IDC_STATIC), Buffer);
				}
			}
		}
	
		Handle = TreeView_GetNextSibling(hWndTree, Handle);
	}

	return;
}

LRESULT
FilterOnItemChanged(
	IN HWND hWnd,
	IN HWND hWndTree,
	IN HTREEITEM hItem,
	IN ULONG NewState,
	IN ULONG OldState,
	IN LPARAM Param
	)
{
	HTREEITEM Parent;
	ULONG Checked, Unchecked;
	PDIALOG_OBJECT Object;
	PFILTER_CONTEXT Context;
	WCHAR Buffer[MAX_PATH];
	WCHAR Format[MAX_PATH];
	PFILTER_NODE Node;
	PBTR_FILTER Filter;
	TVITEM Item = {0};

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, FILTER_CONTEXT);

	Checked = INDEXTOSTATEIMAGEMASK(CHECKED);
	Unchecked = INDEXTOSTATEIMAGEMASK(UNCHECKED);

	Parent = TreeView_GetParent(hWndTree, hItem);

	if (IS_CHECKED(NewState) && IS_UNCHECKED(OldState)) {

		if (Parent != NULL) {

			Item.mask = TVIF_PARAM;
			Item.hItem = Parent;
			TreeView_GetItem(hWndTree, &Item);

			Node = (PFILTER_NODE)Item.lParam;
			Filter = Node->Filter;

			Node->NumberOfSelected += 1;
			Context->NumberOfSelected += 1;

			if (Node->NumberOfSelected == Filter->ProbesCount) {
				TreeView_SetCheckState(hWndTree, Parent, TRUE);
			}

			//
			// Get tree item text and insert into list control
			//

			Item.mask = TVIF_TEXT;
			Item.hItem = hItem;
			Item.pszText = Buffer;
			Item.cchTextMax = MAX_PATH;
			TreeView_GetItem(hWndTree, &Item);

			FilterInsertListItem(hWnd, hWndTree, Node, hItem, Buffer, TRUE);

			LoadString(SdkInstance, IDS_FILTER_SELECT, Format, MAX_PATH);
			StringCchPrintf(Buffer, MAX_PATH, Format, Context->NumberOfSelected);
			SetWindowText(GetDlgItem(hWnd, IDC_STATIC), Buffer);

		} 
		
		else {
			
			Node = (PFILTER_NODE)Param;
			Filter = Node->Filter;

			if (Node->NumberOfSelected != Filter->ProbesCount) {
				FilterCheckChildren(hWnd, Context, Node, hWndTree, hItem, TRUE);
			}
		}
	}

	if (IS_UNCHECKED(NewState) && IS_CHECKED(OldState)) {

		if (Parent != NULL) {

			Item.mask = TVIF_PARAM;
			Item.hItem = Parent;
			TreeView_GetItem(hWndTree, &Item);

			Node = (PFILTER_NODE)Item.lParam;
			Node->NumberOfSelected -= 1;
			Context->NumberOfSelected -= 1;

			//
			// N.B. Parent node is checked only if all children are checked
			// so whenever a child is unchecked, we need uncheck its parent node.
			//

			TreeView_SetCheckState(hWndTree, Parent, FALSE);

			//
			// Delete item in list control
			//

			FilterInsertListItem(hWnd, hWndTree, Node, hItem, NULL, FALSE);

			LoadString(SdkInstance, IDS_FILTER_SELECT, Format, MAX_PATH);
			StringCchPrintf(Buffer, MAX_PATH, Format, Context->NumberOfSelected);
			SetWindowText(GetDlgItem(hWnd, IDC_STATIC), Buffer);

		} 
		
		else {

			Node = (PFILTER_NODE)Param;
			Filter = Node->Filter;

			//
			// N.B. This can happen only if initially all children are checked,
			// user then uncheck the parent node, if the user unchecked a child
			// node, the NumberOfSelected would be (Filter->ProbesCount - 1)
			//

			if (Node->NumberOfSelected == Filter->ProbesCount) {
				FilterCheckChildren(hWnd, Context, Node, hWndTree, hItem, FALSE);
			}
		}
	}

	TreeView_SelectItem(hWndTree, hItem);
	return 0;
}

VOID
FilterInsertListItem(
	IN HWND hWnd,
	IN HWND hWndTree,
	IN PFILTER_NODE Node,
	IN HTREEITEM hTreeItem,
	IN PWCHAR Buffer,
	IN BOOLEAN Insert
	)
{
	PBTR_FILTER Filter;
	HWND hWndList;
	LVITEM lvi = {0};
	LVFINDINFO lvfi = {0};
	int index;

	hWndList = GetDlgItem(hWnd, IDC_LIST_FILTER);

	if (Insert) {

		ASSERT(Node != NULL);
		ASSERT(Buffer != NULL);

		Filter = Node->Filter;
		index = ListView_GetItemCount(hWndList);

		lvi.mask = LVIF_PARAM | LVIF_TEXT;
		lvi.lParam = (LPARAM)hTreeItem;
		lvi.iItem = index;

		lvi.iSubItem = 0;
		lvi.pszText = Filter->FilterName; 
		ListView_InsertItem(hWndList, &lvi);

		lvi.mask = LVIF_TEXT;
		lvi.iSubItem = 1;
		lvi.pszText = Buffer; 
		ListView_SetItem(hWndList, &lvi);

		return;
	}

	//
	// Delete list item, search by hTreeItem,
	// note that start position is -1, not 0
	//

	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = (LPARAM)hTreeItem;
	index = ListView_FindItem(hWndList, -1, &lvfi);

	if (index != -1) {
		ListView_DeleteItem(hWndList, index);
	}
}

LRESULT
FilterOnInfoTip(
	IN HWND hWnd,
	IN HWND hWndTree,
	IN LPNMTVGETINFOTIP lp
	)
{
	HTREEITEM Parent;
	PBTR_FILTER Filter;
	PFILTER_NODE Node;
	RPC_WSTR Uuid = NULL;

	Parent = TreeView_GetParent(hWndTree, lp->hItem);

	if (!Parent) {

		Node = (PFILTER_NODE)lp->lParam;
		Filter = Node->Filter;

		UuidToString(&Filter->FilterGuid, &Uuid);
		StringCchPrintf(lp->pszText, lp->cchTextMax, FILTER_INFOTIP_FORMAT, 
						Filter->FilterName, Filter->Description, Uuid,
						Filter->MajorVersion, Filter->MinorVersion,
						Filter->Author );

		RpcStringFree(&Uuid);
	}

	return 0;	
}

LRESULT
FilterOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	switch (LOWORD(wp)) {

	case IDOK:
		return FilterOnOk(hWnd, uMsg, wp, lp);

	case IDCANCEL:
		return FilterOnCancel(hWnd, uMsg, wp, lp);

	case IDC_BUTTON_EXPORT:
		return FilterOnExport(hWnd, uMsg, wp, lp);
	}

	return 0;
}

LRESULT
FilterOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	FilterBuildCommand(Object);

	FilterCleanUp(hWnd);
	EndDialog(hWnd, IDOK);
	return 0;
}

VOID
FilterCleanUp(
	IN HWND hWnd
	)
{
	PDIALOG_OBJECT Object;
	PFILTER_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, FILTER_CONTEXT);

	MspFreeFilterList(&Context->FilterList);

	if (Context->hBoldFont) {
		DeleteFont(Context->hBoldFont);
	}

	if (Context->ListObject) {
		SdkFree(Context->ListObject);
	}
}

LRESULT
FilterOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	
	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	EndDialog(hWnd, IDCANCEL);
	return FALSE;
}

LRESULT
FilterOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PFILTER_CONTEXT Context;
	UINT Id;
	HFONT hFont;
	LOGFONT LogFont;
	RECT Rect;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PFILTER_CONTEXT)Object->Context;
	hFont = Context->hBoldFont;

	Id = GetDlgCtrlID((HWND)lp);
	if (Id == IDC_STATIC) {
		if (!hFont) {

			hFont = GetCurrentObject((HDC)wp, OBJ_FONT);
			GetObject(hFont, sizeof(LOGFONT), &LogFont);
			LogFont.lfWeight = FW_BOLD;
			hFont = CreateFontIndirect(&LogFont);

			Context->hBoldFont = hFont;
			SendMessage((HWND)lp, WM_SETFONT, (WPARAM)hFont, (LPARAM)TRUE);

			GetClientRect((HWND)lp, &Rect);
			InvalidateRect((HWND)lp, &Rect, TRUE);
		}
	}

	return (LRESULT)0;
}

VOID
FilterBuildCommand(
	IN PDIALOG_OBJECT Object
	)
{
	HWND hWndTree;
	HTREEITEM DllItem;
	HTREEITEM ApiItem;
	TVITEMEX Item = {0};
	ULONG Unchecked, Checked;
	ULONG ApiNumber;
	struct _MSP_USER_COMMAND* Command;
	struct _BTR_USER_COMMAND* BtrCommand;
	ULONG Length;
	PLIST_ENTRY ListEntry;
	ULONG Number;
	PFILTER_CONTEXT Context;
	PFILTER_NODE Node;
	PBTR_FILTER Filter;
	LIST_ENTRY FilterList;
	ULONG BitNumber;

	Context = SdkGetContext(Object, FILTER_CONTEXT);
	hWndTree = GetDlgItem(Object->hWnd, IDC_TREE_FILTER);
	DllItem = TreeView_GetChild(hWndTree, TVI_ROOT);

	Checked = INDEXTOSTATEIMAGEMASK(CHECKED);
	Unchecked = INDEXTOSTATEIMAGEMASK(UNCHECKED);
	
	InitializeListHead(&FilterList);

	while (DllItem != NULL) {
		
		RtlZeroMemory(&Item, sizeof(Item));
		Item.mask = TVIF_PARAM;
		Item.hItem = DllItem;
		TreeView_GetItem(hWndTree, &Item);

		Node = (PFILTER_NODE)Item.lParam;
		Filter = Node->Filter;

		//
		// Get first child of DllItem
		//

		ApiItem = TreeView_GetChild(hWndTree, DllItem);
		ApiNumber = 0;

		while (ApiItem != NULL) {

			ULONG IsChecked;

			RtlZeroMemory(&Item, sizeof(Item));
			Item.mask = TVIF_PARAM;
			Item.hItem = ApiItem;
			TreeView_GetItem(hWndTree, &Item);

			IsChecked = TreeView_GetCheckState(hWndTree, ApiItem);

			if ((IsChecked == 0 && Item.lParam != 0) || 
				(IsChecked == 1 && Item.lParam == 0)) {

				BtrSetBit(&Node->BitMap, ApiNumber);
				Node->Count += 1;
			} 
			
			ApiNumber += 1;
			ApiItem = TreeView_GetNextSibling(hWndTree, ApiItem);
		}
	
		if (Node->Count != 0) {
			InsertTailList(&FilterList, &Node->ListEntry);
		} else {
			SdkFree(Node);
		}

		DllItem = TreeView_GetNextSibling(hWndTree, DllItem);
	}

	//
	// Copy scanned result into MSP_USER_COMMAND's command list
	//

	Command = NULL;

	if (IsListEmpty(&FilterList) != TRUE) {

		Command = (PMSP_USER_COMMAND)SdkMalloc(sizeof(MSP_USER_COMMAND));
		RtlZeroMemory(Command, sizeof(MSP_USER_COMMAND));
		Command->CommandType = CommandProbe;
		Command->ProcessId = Context->Process->ProcessId;
		Command->Type = PROBE_FILTER;
		Command->Status = 0;
		Command->CommandCount = 0;
		Command->CommandLength = 0;
		Command->FailureCount = 0;

		InitializeListHead(&Command->CommandList);
		InitializeListHead(&Command->FailureList);

		Command->Callback = NULL;
		Command->CallbackContext = NULL;
	}

	while (IsListEmpty(&FilterList) != TRUE) {
	
		ListEntry = RemoveHeadList(&FilterList);
		Node = CONTAINING_RECORD(ListEntry, FILTER_NODE, ListEntry);	

		Length = FIELD_OFFSET(BTR_USER_COMMAND, Probe[Node->Count]);
		BtrCommand = (PBTR_USER_COMMAND)SdkMalloc(Length);
		BtrCommand->Length = Length;
		BtrCommand->Type = PROBE_FILTER;
		BtrCommand->Count = Node->Count;

		Filter = Node->Filter;
		StringCchCopyW(BtrCommand->DllName, MAX_PATH, Filter->FilterFullPath);
	
		BitNumber = 0;

		for(Number = 0; Number < Node->Count; Number += 1) {

			BOOLEAN Activate;

			BitNumber = BtrFindFirstSetBit(&Node->BitMap, BitNumber);
			Activate = BtrTestBit(&Filter->BitMap, BitNumber);

			BtrCommand->Probe[Number].Activate = !Activate;
			BtrCommand->Probe[Number].Number = BitNumber;

			BitNumber += 1;
		}

		InsertTailList(&Command->CommandList, &BtrCommand->ListEntry);
		Command->CommandCount += 1;
		Command->CommandLength += BtrCommand->Length;

		SdkFree(Node);
	}

	Context->Command = Command;
}

VOID
FilterFreeCommand(
	IN struct _MSP_USER_COMMAND *Command
	)
{
	PBTR_USER_COMMAND BtrCommand;
	PLIST_ENTRY ListEntry;

	ASSERT(Command != NULL);

	while (IsListEmpty(&Command->CommandList) != TRUE) {
		ListEntry = RemoveHeadList(&Command->CommandList);
		BtrCommand = CONTAINING_RECORD(ListEntry, BTR_USER_COMMAND, ListEntry);	
		SdkFree(BtrCommand);
	}

	SdkFree(Command);
}

LRESULT 
FilterOnLButtonDown(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	POINT pt;
	HDC hdc;
	RECT Rect, TreeRect, ListRect;
	HWND hWndTree, hWndList;
	LONG x, y;
	LONG Height;
	HCURSOR hCursor;
	PDIALOG_OBJECT Object;
	PFILTER_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, FILTER_CONTEXT);

	hWndTree = GetDlgItem(hWnd, IDC_TREE_FILTER);
	hWndList = GetDlgItem(hWnd, IDC_LIST_FILTER);

	GetWindowRect(hWnd, &Rect);
	GetWindowRect(hWndTree, &TreeRect);
	GetWindowRect(hWndList, &ListRect);

	//
	// Get mouse point in screen 
	//

	pt.x = GET_X_LPARAM(lp);
	pt.y = GET_Y_LPARAM(lp);
	ClientToScreen(hWnd, &pt);

	if (pt.x < TreeRect.left || pt.x > ListRect.right ||
		pt.y < TreeRect.top  || pt.y > TreeRect.bottom ) {
		return 0;
	}

	x = TreeRect.right - Rect.left;
	y = TreeRect.top - Rect.top;
	Height = TreeRect.bottom - TreeRect.top;

	Context->DragMode = TRUE;

	//
	// Capture the mouse and load its associate cursor
	//

	SetCapture(hWnd);
	hCursor = LoadCursor(NULL, IDC_SIZEWE);
	SetCursor(hCursor);

	hdc = GetWindowDC(hWnd);
	SplitBarDrawBar(hdc, x, y, Context->SplitbarBorder * 2, Height);
	ReleaseDC(hWnd, hdc);

	//
	// Track the splitbar position
	//

	Context->SplitbarLeft = x;
	return 0;
}

LRESULT 
FilterOnLButtonUp(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	POINT pt;
	HDC hdc;
	RECT Rect, TreeRect, ListRect;
	HWND hWndTree, hWndList;
	LONG x, y;
	LONG Height;
	LONG LeftLimit, RightLimit;
	PDIALOG_OBJECT Object;
	PFILTER_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, FILTER_CONTEXT);

	if (!Context->DragMode) {
		return 0;
	}

	hWndTree = GetDlgItem(hWnd, IDC_TREE_FILTER);
	hWndList = GetDlgItem(hWnd, IDC_LIST_FILTER);

	GetWindowRect(hWnd, &Rect);
	GetWindowRect(hWndTree, &TreeRect);
	GetWindowRect(hWndList, &ListRect);

	//
	// Get mouse point in screen 
	//

	pt.x = GET_X_LPARAM(lp);
	pt.y = GET_Y_LPARAM(lp);
	ClientToScreen(hWnd, &pt);

	LeftLimit = TreeRect.left + Context->SplitbarBorder * 8;
	RightLimit = ListRect.right - Context->SplitbarBorder * 8;

	pt.x = max(LeftLimit, pt.x);
	pt.x = min(RightLimit, pt.x);

	x = pt.x - Rect.left;
	y = TreeRect.top - Rect.top;
	Height = TreeRect.bottom - TreeRect.top;

	hdc = GetWindowDC(hWnd);
	SplitBarDrawBar(hdc, x, y, Context->SplitbarBorder * 2, Height);
	ReleaseDC(hWnd, hdc);

	Context->SplitbarLeft = x;
	Context->DragMode = FALSE;

	FilterAdjustPosition(Object);
	ReleaseCapture();
	return 0;
}

LRESULT 
FilterOnMouseMove(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	POINT pt;
	HDC hdc;
	RECT Rect, TreeRect, ListRect;
	HWND hWndTree, hWndList;
	LONG x, y;
	LONG Height;
	LONG LeftLimit, RightLimit;
	HCURSOR hCursor;
	PDIALOG_OBJECT Object;
	PFILTER_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, FILTER_CONTEXT);

	pt.x = GET_X_LPARAM(lp);
	pt.y = GET_Y_LPARAM(lp);
	ClientToScreen(hWnd, &pt);
	
	hWndTree = GetDlgItem(hWnd, IDC_TREE_FILTER);
	hWndList = GetDlgItem(hWnd, IDC_LIST_FILTER);

	GetWindowRect(hWnd, &Rect);
	GetWindowRect(hWndTree, &TreeRect);
	GetWindowRect(hWndList, &ListRect);

	if (!Context->DragMode) {

		if (pt.x < TreeRect.left || pt.x > ListRect.right || 
			pt.y < TreeRect.top  || pt.y > TreeRect.bottom ) {
			return 0;
		}
		
		hCursor = LoadCursor(NULL, IDC_SIZEWE);
		SetCursor(hCursor);
		return 0;
	}

	//
	// Get mouse point in screen 
	//
	
	LeftLimit = TreeRect.left + Context->SplitbarBorder * 8;
	RightLimit = ListRect.right - Context->SplitbarBorder * 8;

	pt.x = max(LeftLimit, pt.x);
	pt.x = min(RightLimit, pt.x);

	x = pt.x - Rect.left;
	y = TreeRect.top - Rect.top;
	Height = TreeRect.bottom - TreeRect.top;

	if (x != Context->SplitbarLeft) {

		hdc = GetWindowDC(hWnd);
		SplitBarDrawBar(hdc, Context->SplitbarLeft, y, Context->SplitbarBorder * 2, Height);
		SplitBarDrawBar(hdc, x, y, Context->SplitbarBorder * 2, Height);
		ReleaseDC(hWnd, hdc);

		Context->SplitbarLeft = x;
	}

	return 0;
}

VOID
FilterAdjustPosition(
	IN PDIALOG_OBJECT Object
	)
{
	HWND hWnd;
	HWND hWndTree;
	HWND hWndList;
	RECT ClientRect, ScreenRect;
	RECT TreeRect, ListRect;
	LONG TreeWidth, BoundWidth, ListWidth;
	POINT pt;
	PFILTER_CONTEXT Context;
	LONG SplitbarLeft;

	Context = SdkGetContext(Object, FILTER_CONTEXT);

	hWnd = Object->hWnd;
	hWndTree = GetDlgItem(hWnd, IDC_TREE_FILTER);
	hWndList = GetDlgItem(hWnd, IDC_LIST_FILTER);
	
	GetClientRect(hWnd, &ClientRect);

	pt.x = ClientRect.left;
	pt.y = ClientRect.top;
	ClientToScreen(hWnd, &pt);

	ClientRect.left = pt.x;
	ClientRect.top = pt.y;

	//
	// Convert splitbar x to be client coordinates,
	// note SplitBarLeft is offset to screen rect's left.
	//

	GetWindowRect(hWnd, &ScreenRect);
	SplitbarLeft = Context->SplitbarLeft + ScreenRect.left - ClientRect.left;

	GetWindowRect(hWndTree, &TreeRect);
	GetWindowRect(hWndList, &ListRect);
	BoundWidth = ListRect.right - TreeRect.left;

	//
	// Compute tree client coordinate and its width
	//

	TreeRect.left = TreeRect.left - ClientRect.left;
	TreeWidth = SplitbarLeft - TreeRect.left; 

	ListWidth = BoundWidth - TreeWidth - Context->SplitbarBorder;

	MoveWindow(hWndTree, 
		       TreeRect.left, 
			   TreeRect.top - ClientRect.top, 
			   TreeWidth,
			   TreeRect.bottom - TreeRect.top,
			   TRUE);

	MoveWindow(hWndList, 
		       SplitbarLeft + Context->SplitbarBorder, 
			   TreeRect.top - ClientRect.top,
			   ListWidth,
			   ListRect.bottom - ListRect.top,
			   TRUE);
}

BOOLEAN
FilterListOnDelete(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	int Index;
	int Count;
	HTREEITEM hTreeItem;
	HWND hWndTree;
	HWND hWndParent;

	Index = ListViewGetFirstSelected(hWnd);

	if (Index != -1) {

		ListViewGetParam(hWnd, Index, (LPARAM *)&hTreeItem);

		if (hTreeItem != NULL) {

			ListView_DeleteItem(hWnd, Index);

			hWndParent = GetParent(hWnd);
			hWndTree = GetDlgItem(hWndParent, IDC_TREE_FILTER);
			TreeView_EnsureVisible(hWndTree, hTreeItem);

			if (BspIsVistaAbove()) {
				TreeView_SetCheckState(hWndTree, hTreeItem, FALSE);
			} else {
				PostMessage(hWndParent, WM_TVN_UNCHECKITEM, (WPARAM)hTreeItem, 0);
			}


			Count = ListView_GetItemCount(hWnd);

			//
			// If the last item is deleted, start from 0
			//

			if (Count > Index) {
				ListViewSelectSingle(hWnd, Index);	
			} else {
				ListViewSelectSingle(hWnd, 0);	
			}

			return TRUE;
		}
	}

	return FALSE;
}

LRESULT CALLBACK
FilterListProcedure(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp,
	IN UINT_PTR uIdSubclass,
	IN DWORD_PTR dwRefData
	)
{
	BOOLEAN Handled;

	switch (uMsg) {

		case WM_KEYDOWN:

			if ((ULONG)wp == VK_DELETE) {
				Handled = FilterListOnDelete(hWnd, uMsg, wp, lp);
				if (Handled) {
					return TRUE;
				}
			}
	}

	return DefSubclassProc(hWnd, uMsg, wp, lp);
} 

LRESULT 
FilterOnColumnClick(
	IN HWND hWnd,
	IN NMLISTVIEW *lpNmlv
	)
{
	HWND hWndList;
	HWND hWndHeader;
	int ColumnCount;
	int i;
	HDITEM hdi;
	LISTVIEW_OBJECT *Object;
	PDIALOG_OBJECT Dialog;
	PFILTER_CONTEXT Context;

	Dialog = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Dialog, FILTER_CONTEXT);
	Object = Context->ListObject;

	ASSERT(Object != NULL);

    if (Object->SortOrder == SortOrderNone){
        return 0;
    }

	if (Object->LastClickedColumn == lpNmlv->iSubItem) {
		Object->SortOrder = (LIST_SORT_ORDER)!Object->SortOrder;
    } else {
		Object->SortOrder = SortOrderAscendent;
    }
    
	hWndList = GetDlgItem(hWnd, IDC_LIST_FILTER);
    hWndHeader = ListView_GetHeader(hWndList);
    ASSERT(hWndHeader);

    ColumnCount = Header_GetItemCount(hWndHeader);
    
    for (i = 0; i < ColumnCount; i++) {
        hdi.mask = HDI_FORMAT;
        Header_GetItem(hWndHeader, i, &hdi);
        
        if (i == lpNmlv->iSubItem) {
            hdi.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
            if (Object->SortOrder == SortOrderAscendent){
                hdi.fmt |= HDF_SORTUP;
            } else {
                hdi.fmt |= HDF_SORTDOWN;
            }
        } else {
            hdi.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
        } 
        
        Header_SetItem(hWndHeader, i, &hdi);
    }
    
	Object->LastClickedColumn = lpNmlv->iSubItem;
	Object->hWnd = lpNmlv->hdr.hwndFrom;
	Object->CtrlId = lpNmlv->hdr.idFrom;
	ListView_SortItemsEx(hWndList, FilterSortCallback, (LPARAM)Object);

    return 0L;
}

int CALLBACK
FilterSortCallback(
	IN LPARAM First, 
	IN LPARAM Second,
	IN LPARAM Param
	)
{
    WCHAR FirstData[MAX_PATH + 1];
    WCHAR SecondData[MAX_PATH + 1];
	HWND hWnd;
	LIST_DATA_TYPE Type;
    int Result;
	LISTVIEW_OBJECT *Object;
	LISTVIEW_COLUMN *Column;

	Object = (LISTVIEW_OBJECT *)Param;
	FirstData[0] = 0;
	SecondData[0] = 0;

	hWnd = Object->hWnd;
	Column = Object->Column;

    ListView_GetItemText(hWnd, First,  Object->LastClickedColumn, FirstData,  MAX_PATH);
    ListView_GetItemText(hWnd, Second, Object->LastClickedColumn, SecondData, MAX_PATH);

	Type = Column[Object->LastClickedColumn].DataType;

    switch (Type) {

        case DataTypeInt:
            Result = _wtoi(FirstData) - _wtoi(SecondData);
            break;        

        case DataTypeUInt:
            Result = (int)((UINT)_wtoi(FirstData) - (UINT)_wtoi(SecondData));
            break;        
        
		case DataTypeInt64:
            Result = (int)(_wtoi64(FirstData) - _wtoi64(SecondData));
			break;

        case DataTypeUInt64:
			Result =  (int)((ULONG64)_wtoi64(FirstData) - (ULONG64)_wtoi64(SecondData));
			break;

        case DataTypeFloat:
			if (_wtof(FirstData) - _wtof(SecondData) >= 0) {
				Result = -1;
			} else {
				Result = 1;
			}
            break;
        
		case DataTypeTime:
        case DataTypeText:
        case DataTypeBool:
            Result = _wcsicmp(FirstData, SecondData);
            break;

		default:
			ASSERT(0);
    }

	return Object->SortOrder ? Result : -Result;
}

ULONG
FilterScanSelection(
	IN PDIALOG_OBJECT Object,
	OUT PLIST_ENTRY DllListHead
	)
{
	HWND hWndTree;
	HTREEITEM DllItem;
	HTREEITEM ApiItem;
	TVITEMEX Item = {0};
	ULONG ApiNumber;
	PFILTER_NODE Node;
	PBTR_FILTER BtrFilter;
	PCSP_FILTER CspFilter;
	ULONG IsChecked;
	ULONG DllCount;
	BTR_BITMAP Bitmap;

	DllCount = 0;
	InitializeListHead(DllListHead);

	hWndTree = GetDlgItem(Object->hWnd, IDC_TREE_FILTER);
	DllItem = TreeView_GetChild(hWndTree, TVI_ROOT);

	while (DllItem != NULL) {
		
		RtlZeroMemory(&Item, sizeof(Item));
		Item.mask = TVIF_PARAM;
		Item.hItem = DllItem;
		TreeView_GetItem(hWndTree, &Item);

		Node = (PFILTER_NODE)Item.lParam;

		if (!Node->NumberOfSelected) {
			DllItem = TreeView_GetNextSibling(hWndTree, DllItem);
			continue;
		}

		CspFilter = (PCSP_FILTER)SdkMalloc(sizeof(CSP_FILTER));
		RtlZeroMemory(CspFilter, sizeof(CSP_FILTER));

		BtrFilter = Node->Filter;
		CspFilter->Count = 0;
		CspFilter->FilterGuid = BtrFilter->FilterGuid;
		StringCchCopy(CspFilter->FilterName, MAX_PATH, BtrFilter->FilterName);

		BtrInitializeBitMap(&Bitmap, CspFilter->Bits, BtrFilter->ProbesCount);

		//
		// Get first child of DllItem
		//

		ApiNumber = 0;
		ApiItem = TreeView_GetChild(hWndTree, DllItem);

		while (ApiItem != NULL) {

			IsChecked = TreeView_GetCheckState(hWndTree, ApiItem);

			if (IsChecked == 1) {
				BtrSetBit(&Bitmap, ApiNumber);
				CspFilter->Count += 1;
			}

			ApiNumber += 1;
			ApiItem = TreeView_GetNextSibling(hWndTree, ApiItem);
		}
	
		ASSERT(CspFilter->Count == Node->NumberOfSelected);
		InsertTailList(DllListHead, &CspFilter->ListEntry);
		DllCount += 1;

		DllItem = TreeView_GetNextSibling(hWndTree, DllItem);
	}

	return DllCount;
}

ULONG 
FilterExportProbe(
	IN PDIALOG_OBJECT Object,
	IN PWSTR Path
	)
{
	LIST_ENTRY ListHead;
	PCSP_FILTER Filter;
	PLIST_ENTRY ListEntry;
	CSP_FILE_HEADER Head;
	HANDLE FileHandle;
	ULONG Count;
	ULONG Size;
	ULONG Complete;

	InitializeListHead(&ListHead);

	Count = FilterScanSelection(Object, &ListHead);
	if (!Count) {
		MessageBox(Object->hWnd, L"No probes are selected!", L"D Probe", MB_OK);
		return 0;
	}

	DeleteFile(Path);
	FileHandle = CreateFile(Path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		                    CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		return GetLastError();
	}
	
	//
	// Adjust the file pointer to skip header and point to data area
	//

	Size = 0;
	SetFilePointer(FileHandle, sizeof(CSP_FILE_HEADER), NULL, FILE_BEGIN);
	
	while (IsListEmpty(&ListHead) != TRUE) {

		ListEntry = RemoveHeadList(&ListHead);
		Filter = CONTAINING_RECORD(ListEntry, CSP_FILTER, ListEntry);	

		WriteFile(FileHandle, Filter, sizeof(CSP_FILTER), &Complete, NULL);	

		Size += sizeof(CSP_FILTER);
		SdkFree(Filter);
	}
	
	//
	// Finally write the dpc file header
	//

	SetFilePointer(FileHandle, 0, NULL, FILE_BEGIN);

	Head.Signature = CSP_FILE_SIGNATURE;
	Head.Flag = 0;
	Head.Size = Size + sizeof(CSP_FILE_HEADER);
	Head.MajorVersion = 1;
	Head.MinorVersion = 0;
	Head.Type = PROBE_FILTER;
	Head.DllCount = Count;

	WriteFile(FileHandle, &Head, sizeof(CSP_FILE_HEADER), &Complete, NULL);
	CloseHandle(FileHandle);

	return 0;	
}

ULONG 
FilterImportProbe(
	IN PWSTR Path,
	OUT struct _MSP_USER_COMMAND **Command
	)
{
	HANDLE FileHandle;
	HANDLE MapObject;
	PVOID MapAddress;
	CSP_FILE_HEADER Head;
	PCSP_FILTER Filter;
	PBTR_FILTER BtrFilter;
	PBTR_USER_COMMAND BtrCommand;
	PMSP_USER_COMMAND MspCommand;
	BTR_BITMAP Bitmap;
	ULONG Length;
	ULONG Number;
	ULONG BitNumber;
	ULONG Status;
	ULONG Complete;
	ULONG i;

	FileHandle = CreateFile(Path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 
		                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		return GetLastError();
	}

	ReadFile(FileHandle, &Head, sizeof(Head), &Complete, NULL);

	//
	// Check dpc file signature, dllcount, and its file size 
	//

	if (Head.Signature != CSP_FILE_SIGNATURE || !Head.DllCount || Head.Size < sizeof(Head)) {
		MessageBox(NULL, L"Invalid dpc file!", L"D Probe", MB_OK|MB_ICONERROR);
		CloseHandle(FileHandle);
		return ERROR_INVALID_PARAMETER;
	}

	//
	// Map the entire dpc file as memory mapped
	//

	MapObject = CreateFileMapping(FileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
	if (!MapObject) {
		Status = GetLastError();
		CloseHandle(FileHandle);
		return Status;
	}

	MapAddress = (PVOID)MapViewOfFile(MapObject, FILE_MAP_READ, 0, 0, 0);
	if (!MapAddress) {
		Status = GetLastError();
		CloseHandle(MapObject);
		CloseHandle(FileHandle);
		return Status;
	}

	Filter = (PCSP_FILTER)((PUCHAR)MapAddress + sizeof(CSP_FILE_HEADER));
	MspCommand = (PMSP_USER_COMMAND)SdkMalloc(sizeof(MSP_USER_COMMAND));
	RtlZeroMemory(MspCommand, sizeof(MSP_USER_COMMAND));

	MspCommand->CommandType = CommandProbe;
	MspCommand->Type = PROBE_FILTER;
	InitializeListHead(&MspCommand->CommandList);
	InitializeListHead(&MspCommand->FailureList);

	for(Number = 0; Number < Head.DllCount; Number += 1) {

		Length = FIELD_OFFSET(BTR_USER_COMMAND, Probe[Filter->Count]);

		BtrCommand = (PBTR_USER_COMMAND)SdkMalloc(Length);
		BtrCommand->Length = Length; 
		BtrCommand->Type = PROBE_FILTER;
		BtrCommand->Status = 0;
		BtrCommand->FailureCount = 0;
		BtrCommand->Count = Filter->Count;

		//
		// N.B. DllName is a base name, this is ok for btr to work as expected,
		// because dpc file can be imported from another machine which has different
		// dll path, so we decide to use base name to avoid this issue.
		//

		BtrFilter = MspQueryDecodeFilterByGuid(&Filter->FilterGuid);
		if (!BtrFilter) {

			SdkFree(BtrCommand);
			FilterFreeCommand(MspCommand);

			UnmapViewOfFile(MapAddress);
			CloseHandle(MapObject);
			CloseHandle(FileHandle);

			return ERROR_INVALID_PARAMETER;
		}

		//
		// N.B. Here we must fill a valid filter path, not CSP_FILTER.FilterName
		//

		StringCchCopy(BtrCommand->DllName, MAX_PATH, BtrFilter->FilterFullPath);

		BitNumber = 0;
		BtrInitializeBitMap(&Bitmap, Filter->Bits, BtrFilter->ProbesCount);

		for(i = 0; i < Filter->Count; i++) {
			
			RtlZeroMemory(&BtrCommand->Probe[i], sizeof(BTR_USER_PROBE));
			BitNumber = BtrFindFirstSetBit(&Bitmap, BitNumber);
			ASSERT(BitNumber != -1);

			BtrCommand->Probe[i].Activate = TRUE;
			BtrCommand->Probe[i].Number = BitNumber;

			BitNumber += 1;
		}

		InsertTailList(&MspCommand->CommandList, &BtrCommand->ListEntry);
		MspCommand->CommandCount += 1;
		MspCommand->CommandLength += Length;

		Filter += 1;
	}

	*Command = MspCommand;

	UnmapViewOfFile(MapAddress);
	CloseHandle(MapObject);
	CloseHandle(FileHandle);

	return ERROR_SUCCESS;	
}

LRESULT
FilterOnExport(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	OPENFILENAME Ofn;
	BOOL Status;
	PWCHAR Extension;
	WCHAR Buffer[MAX_PATH];	
	PDIALOG_OBJECT Object;

	RtlZeroMemory(&Ofn, sizeof Ofn);
	RtlZeroMemory(Buffer, sizeof(Buffer));

	Ofn.lStructSize = sizeof(Ofn);
	Ofn.hwndOwner = hWnd;
	Ofn.hInstance = SdkInstance;
	Ofn.lpstrFilter = L"D Probe Configuration (*.dpc)\0*.dpc\0\0";
	Ofn.lpstrFile = Buffer;
	Ofn.nMaxFile = sizeof(Buffer); 
	Ofn.lpstrTitle = L"D Probe";
	Ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT; 

	GetSaveFileName(&Ofn);

	StringCchCopy(Buffer, MAX_PATH, _wcslwr(Ofn.lpstrFile));
	Extension = wcsstr(Buffer, L".dpc");
	if (!Extension) {
		StringCchCat(Buffer, MAX_PATH, L".dpc");
	}

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Status = FilterExportProbe(Object, Buffer);

	if (Status != 0) {
		StringCchPrintf(Buffer, MAX_PATH, L"Error 0x%08x occurred !", Status);
		MessageBox(hWnd, Buffer, L"D Probe", MB_OK|MB_ICONERROR); 
	}

	return 0;
}