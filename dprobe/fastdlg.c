//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "fastdlg.h"
#include "sdk.h"
#include "mspprocess.h"
#include "btr.h"
#include "splitbar.h"
#include "csp.h"
#include "msputility.h"

#define FAST_TREE_STYLE (TVS_INFOTIP | TVS_HASLINES | TVS_LINESATROOT |\
	                        TVS_HASBUTTONS | TVS_CHECKBOXES)

#define FAST_SELECT_STRING L"Total select %u probes"
#define FAST_TITLE_STRING  L"Fast Probe (%ws:%u)"

#define CHECKED   2 
#define UNCHECKED 1

#define IS_CHECKED(_V) \
	(((_V) >> 12) == 2 ? TRUE : FALSE)

#define IS_UNCHECKED(_V) \
	(((_V) >> 12) == 1 ? TRUE : FALSE)


LISTVIEW_COLUMN FastColumn[2] = {
	{ 160, L"Module",    LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
	{ 200, L"Function",  LVCFMT_LEFT, 0 , TRUE, TRUE, BLACK, WHITE, BLACK, DataTypeText },
};

INT_PTR
FastDialog(
	IN HWND hWnd,
	IN PBSP_PROCESS Process,
	OUT struct _MSP_USER_COMMAND **Command
	)

{
	DIALOG_OBJECT Object = {0};
	FAST_CONTEXT Context = {0};
	INT_PTR Return;

	DIALOG_SCALER_CHILD Children[6] = {
		{ IDC_TREE_FAST, AlignRight, AlignBottom },
		{ IDC_LIST_FAST, AlignBoth, AlignBottom },
		{ IDOK,     AlignBoth, AlignBoth },
		{ IDCANCEL, AlignBoth, AlignBoth },
		{ IDC_BUTTON_EXPORT, AlignBoth, AlignBoth },
		{ IDC_STATIC, AlignNone, AlignBoth }
	};

	DIALOG_SCALER Scaler = {
		{0,0}, {0,0}, {0,0}, 6, Children
	};

	*Command = NULL;

	InitializeListHead(&Context.ModuleList);

	Context.Process = Process;
	Object.Context = &Context;
	Object.hWndParent = hWnd;
	Object.ResourceId = IDD_DIALOG_FAST;
	Object.Procedure = FastProcedure;
	Object.Scaler = &Scaler;

	Return = DialogCreate(&Object);
	*Command = Context.Command;
	return Return;
}

ULONG
FastInsertModule(
	IN PDIALOG_OBJECT Object
	)
{
	ULONG Status;
	HWND hWnd;
	HWND hWndTree;
	PLIST_ENTRY ListHead;
	PLIST_ENTRY ListEntry;
	TVINSERTSTRUCT Insert = {0};
	HTREEITEM hTreeItem;
	WCHAR Buffer[MAX_PATH];
	PFAST_CONTEXT Context;
	PBSP_PROCESS Process;
	LONG Style;
	ULONG Count;
	PBSP_EXPORT_DESCRIPTOR Api;
	PBSP_MODULE Module;
	PFAST_NODE Node;

	hWnd = Object->hWnd;
	Context = SdkGetContext(Object, FAST_CONTEXT);
	Process = Context->Process;

	hWndTree = GetDlgItem(hWnd, IDC_TREE_FAST);
	TreeView_SetUnicodeFormat(hWndTree, TRUE);

	Style = GetWindowLong(hWndTree, GWL_STYLE);
	SetWindowLong(hWndTree, GWL_STYLE, Style | FAST_TREE_STYLE); 

	//
	// Query loaded module list for target process
	//

	ListHead = &Context->ModuleList;
	InitializeListHead(ListHead);

	Status = BspQueryModule(Process->ProcessId, TRUE, ListHead);
	if (Status != S_OK) {
		return Status;
	}

	ListEntry = ListHead->Flink;
	while (ListEntry != ListHead) {

		Module = CONTAINING_RECORD(ListEntry, BSP_MODULE, ListEntry);

		//
		// Skip runtime and filters dll
		//

		if (Module->FullPath) {
			if (MspIsRuntimeFilterDll(Module->FullPath)) {
				ListEntry = ListEntry->Flink;
				RemoveEntryList(&Module->ListEntry);
				BspFreeModule(Module);
				continue;
			}
		}

		Status = BspEnumerateDllExport(Module->FullPath, Module, &Count, &Api);
		if (Status != S_OK || Count == 0 || Api == NULL) {
			ListEntry = ListEntry->Flink;
			RemoveEntryList(&Module->ListEntry);
			BspFreeModule(Module);
			continue;
		}

		Node = (PFAST_NODE)SdkMalloc(sizeof(FAST_NODE));
		RtlZeroMemory(Node, sizeof(FAST_NODE));
		Node->Module = Module;
		Node->Api = Api;
		Node->ApiCount = Count;

		//
		// At node object as module's context, this is useful when do clean up,
		// we don't need to enumerate all the tree nodes
		//

		Module->Context = Node;

		Insert.hParent = NULL; 
		Insert.hInsertAfter = TVI_ROOT; 
		Insert.item.mask = TVIF_TEXT | TVIF_PARAM; 

		StringCchPrintf(Buffer, MAX_PATH, L"%ws", _wcslwr(Module->Name));
		Insert.item.pszText = Buffer;
		Insert.item.lParam = (LPARAM)Node; 
		hTreeItem = TreeView_InsertItem(hWndTree, &Insert);

		FastInsertApi(hWndTree, hTreeItem, Process->ProcessId, 
			             Context, Module, Api, Count);

		ListEntry = ListEntry->Flink;
	}
    
	return 0;
}

ULONG
FastInsertApi(
	IN HWND hWndTree,
	IN HTREEITEM hParent,
	IN ULONG ProcessId,
	IN PFAST_CONTEXT Context,
	IN PBSP_MODULE Module,
	IN PBSP_EXPORT_DESCRIPTOR Api,
	IN ULONG ApiCount
	)
{
	TVINSERTSTRUCT Insert = {0};
	HTREEITEM hInserted;
	WCHAR Buffer[MAX_PATH];
	ULONG Number;
	PWCHAR Unicode;
	BOOLEAN IsProbeActivated;
	BOOLEAN IsProcessActivated;
	PVOID Address;

	//
	// Save the export list in module's Context 
	//

	hInserted = hParent;
	IsProcessActivated = MspIsProcessActivated(ProcessId);

	for (Number = 0; Number < ApiCount; Number += 1) {

		IsProbeActivated = FALSE;

		Insert.hParent = hParent; 
		Insert.hInsertAfter = hInserted; 
		Insert.item.mask = TVIF_TEXT | TVIF_PARAM; 

		BspConvertUTF8ToUnicode(Api[Number].Name, &Unicode);
		BspUnDecorateSymbolName(Unicode, Buffer, MAX_PATH - 1, UNDNAME_COMPLETE);
		Insert.item.pszText = Buffer;

		//
		// Lookup process's probe table to get probe state
		//

		if (IsProcessActivated) {

			//
			// N.B. For forwarded entry like kernel32!HeapAlloc is forwarded
			// to ntdll!RtlAllocateHeap, its Rva is 0, we must check rva here
			//

			if (Api[Number].Rva != 0) {
				Address = (PVOID)((ULONG_PTR)Module->BaseVa + Api[Number].Rva);
			} else {
				Address = BspGetDllForwardAddress(ProcessId, Api[Number].Forward);
			}

			IsProbeActivated = MspIsFastProbeActivated(ProcessId, Address, 
														  Module->Name, 
														  Unicode);
		} 

		//
		// Attach original probe's activation state into lparam
		//

		Insert.item.lParam = (LPARAM)IsProbeActivated;
		hInserted = TreeView_InsertItem(hWndTree, &Insert);

		if (IsProbeActivated) {
			
			if (BspIsVistaAbove()) {
				TreeView_SetCheckState(hWndTree, hInserted, TRUE);
			} else {
				PostMessage(GetParent(hWndTree), WM_TVN_CHECKITEM, (WPARAM)hInserted, 0);
			}
		}

		BspFree(Unicode);
	}

	return 0;
}

LRESULT
FastOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PFAST_CONTEXT Context;
	PBSP_PROCESS Process;
	WCHAR Buffer[MAX_PATH];
	HWND hWndStatic;
	HWND hWndTree;
	HWND hWndList;
	RECT ListRect, TreeRect;
	LVCOLUMN Column = {0};
	PLISTVIEW_OBJECT ListObject;
	int i;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, FAST_CONTEXT);
	
	hWndList = GetDlgItem(hWnd, IDC_LIST_FAST);
	hWndTree = GetDlgItem(hWnd, IDC_TREE_FAST);

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
    ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_FULLROWSELECT, 
		                                LVS_EX_FULLROWSELECT);
    ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_INFOTIP, 
		                                LVS_EX_INFOTIP);

	Column.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT; 
	  
	for (i = 0; i < 2; i++) { 
        Column.iSubItem = i;
		Column.pszText = FastColumn[i].Title;	
		Column.cx = FastColumn[i].Width;     
		Column.fmt = FastColumn[i].Align;
		ListView_InsertColumn(hWndList, i, &Column);
    } 
	
	ListObject = (PLISTVIEW_OBJECT)SdkMalloc(sizeof(LISTVIEW_OBJECT));
	ListObject->hWnd = hWndList;
	ListObject->Column = FastColumn;
	ListObject->Count = 2;
	ListObject->SortOrder = SortOrderDescendent;
	ListObject->LastClickedColumn = 0;

	Context->ListObject = ListObject;

	//
	// Subclass list control to get chance to handle WM_CHAR, WM_KEYDOWN 
	// keyboard events.
	//

	SetWindowSubclass(hWndList, FastListProcedure, 0, 0);

	FastInsertModule(Object);

	hWndStatic = GetDlgItem(hWnd, IDC_STATIC);
	StringCchPrintf(Buffer, MAX_PATH, FAST_SELECT_STRING, 
		            Context->NumberOfSelected);
	SetWindowText(hWndStatic, Buffer);

	Process = Context->Process;
	StringCchPrintf(Buffer, MAX_PATH, FAST_TITLE_STRING,
		            Process->Name, Process->ProcessId);
	SetWindowText(hWnd, Buffer);
	SdkSetMainIcon(hWnd);

	if (Context->NumberOfSelected != 0) {
		ListViewSelectSingle(hWndList, 0);
		SetFocus(hWndList);
	}
	return TRUE;
}

LRESULT
FastOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PFAST_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, FAST_CONTEXT);

	FastBuildCommand(Object, Context);

	FastCleanUp(hWnd);
	EndDialog(hWnd, IDOK);
	return 0;
}

LRESULT
FastOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PFAST_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, FAST_CONTEXT);

	FastCleanUp(hWnd);
	EndDialog(hWnd, IDCANCEL);
	return 0;
}

VOID
FastCleanUp(
	IN HWND hWnd
	)
{
	PDIALOG_OBJECT Object;
	PFAST_CONTEXT Context;
	PLIST_ENTRY ListEntry;
	PBSP_MODULE Module;
	PFAST_NODE Node;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, FAST_CONTEXT);

	while (IsListEmpty(&Context->ModuleList) != TRUE) {

		ListEntry = RemoveHeadList(&Context->ModuleList);
		Module = CONTAINING_RECORD(ListEntry, BSP_MODULE, ListEntry);

		if (Module->Context) {
			Node = (PFAST_NODE)Module->Context;
			BspFree(Node->Api);
			SdkFree(Node);
		}

		BspFreeModule(Module);
	}

	if (Context->hBoldFont) {
		DeleteFont(Context->hBoldFont);
	}

	if (Context->ListObject) {
		SdkFree(Context->ListObject);
	}
}

LRESULT
FastOnCommand(
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
			return FastOnOk(hWnd, uMsg, wp, lp);

		case IDCANCEL:
			return FastOnCancel(hWnd, uMsg, wp, lp);

		case IDC_BUTTON_EXPORT:
			return FastOnExport(hWnd, uMsg, wp, lp);
	}

	return 0;
}

VOID
FastCheckChildren(
	IN HWND hWnd,
	IN PFAST_CONTEXT Context,
	IN PFAST_NODE Node,
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

					FastInsertListItem(hWnd, hWndTree, Node,
						               Handle, Buffer, TRUE);

					Node->NumberOfSelected += 1;
					Context->NumberOfSelected += 1;

					StringCchPrintf(Buffer, MAX_PATH, FAST_SELECT_STRING, 
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

					FastInsertListItem(hWnd, hWndTree, Node,
						               Handle, Buffer, FALSE);

					Node->NumberOfSelected -= 1;
					Context->NumberOfSelected -= 1;

					StringCchPrintf(Buffer, MAX_PATH, FAST_SELECT_STRING, 
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
FastOnItemChanged(
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
	PFAST_CONTEXT Context;
	WCHAR Buffer[MAX_PATH];
	PFAST_NODE Node;
	TVITEM Item = {0};

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, FAST_CONTEXT);

	Checked = INDEXTOSTATEIMAGEMASK(CHECKED);
	Unchecked = INDEXTOSTATEIMAGEMASK(UNCHECKED);

	Parent = TreeView_GetParent(hWndTree, hItem);

	if (IS_CHECKED(NewState) && IS_UNCHECKED(OldState)) {

		if (Parent != NULL) {

			Item.mask = TVIF_PARAM;
			Item.hItem = Parent;
			TreeView_GetItem(hWndTree, &Item);

			Node = (PFAST_NODE)Item.lParam;

			Node->NumberOfSelected += 1;
			Context->NumberOfSelected += 1;

			if (Node->NumberOfSelected == Node->ApiCount) {
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

			FastInsertListItem(hWnd, hWndTree, Node, hItem, Buffer, TRUE);

			StringCchPrintf(Buffer, MAX_PATH, FAST_SELECT_STRING, 
				            Context->NumberOfSelected);
			SetWindowText(GetDlgItem(hWnd, IDC_STATIC), Buffer);

		} 
		
		else {
			
			Node = (PFAST_NODE)Param;

			if (Node->NumberOfSelected != Node->ApiCount) {
				FastCheckChildren(hWnd, Context, Node, hWndTree, hItem, TRUE);
			}
		}
	}

	if (IS_UNCHECKED(NewState) && IS_CHECKED(OldState)) {

		if (Parent != NULL) {

			Item.mask = TVIF_PARAM;
			Item.hItem = Parent;
			TreeView_GetItem(hWndTree, &Item);

			Node = (PFAST_NODE)Item.lParam;
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

			FastInsertListItem(hWnd, hWndTree, Node, hItem, NULL, FALSE);

			StringCchPrintf(Buffer, MAX_PATH, FAST_SELECT_STRING, 
				            Context->NumberOfSelected);
			SetWindowText(GetDlgItem(hWnd, IDC_STATIC), Buffer);

		} 
		
		else {

			Node = (PFAST_NODE)Param;

			//
			// N.B. This can happen only if initially all children are checked,
			// user then uncheck the parent node, if the user unchecked a child
			// node, the NumberOfSelected would be ApiCount - 1
			//

			if (Node->NumberOfSelected == Node->ApiCount) {
				FastCheckChildren(hWnd, Context, Node, hWndTree, hItem, FALSE);
			}
		}
	}

	TreeView_SelectItem(hWndTree, hItem);
	return 0;
}

LRESULT
FastOnNotify(
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

	hWndTree = GetDlgItem(hWnd, IDC_TREE_FAST);

	if (IDC_TREE_FAST == LOWORD(wp)) {

		switch (lpnmhdr->code)  {

			case TVN_GETINFOTIP:
				hWndTree = GetDlgItem(hWnd, IDC_TREE_FAST);
				FastOnInfoTip(hWnd, hWndTree, (LPNMTVGETINFOTIP)lp);
				break;

			case TVN_ITEMCHANGED:
				{
					NMTVITEMCHANGE *lpnmic;

					lpnmic = (NMTVITEMCHANGE *)lp;
					FastOnItemChanged(hWnd, hWndTree, lpnmic->hItem,
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

	if (IDC_LIST_FAST == LOWORD(wp)) {
		
		switch (lpnmhdr->code)  {

			case LVN_COLUMNCLICK:
				return FastOnColumnClick(hWnd, (LPNMLISTVIEW)lp);

		}
	}

	return 0;
}

PWSTR FastInfoTipFormat = 
		L"                    \n"
		L"  Module      : %s  \n"
		L"  Version     : %s  \n"
		L"  TimeStamp   : %s  \n"
		L"  Description : %s  \n"
		L"  Company     : %s  \n"
		L"\n";

LRESULT
FastOnInfoTip(
	IN HWND hWnd,
	IN HWND hWndTree,
	IN LPNMTVGETINFOTIP lp
	)
{
	HTREEITEM Parent;
	PFAST_NODE Node;
	PBSP_MODULE Module;
	WCHAR Version[MAX_PATH] = L"N/A";
	WCHAR TimeStamp[MAX_PATH] = L"N/A";
	WCHAR Description[MAX_PATH] = L"N/A";
	WCHAR Company[MAX_PATH] = L"N/A";

	Parent = TreeView_GetParent(hWndTree, lp->hItem);

	if (!Parent) {

		Node = (PFAST_NODE)lp->lParam;
		Module = Node->Module;

		if (Module->Version) {
			wcscpy_s(Version, MAX_PATH, Module->Version);
		}
		if (Module->TimeStamp) {
			wcscpy_s(TimeStamp, MAX_PATH, Module->TimeStamp);
		}
		if (Module->Description) {
			wcscpy_s(Description, MAX_PATH, Module->Description);
		}
		if (Module->Company) {
			wcscpy_s(Company, MAX_PATH, Module->Company);
		}

		StringCchPrintf(lp->pszText, lp->cchTextMax, FastInfoTipFormat, 
						Module->Name, Version, TimeStamp,Description, Company);
	}
	return 0;	
}

VOID
FastInsertListItem(
	IN HWND hWnd,
	IN HWND hWndTree,
	IN PFAST_NODE Node,
	IN HTREEITEM hTreeItem,
	IN PWCHAR Buffer,
	IN BOOLEAN Insert
	)
{
	HWND hWndList;
	PBSP_MODULE Module;
	LVITEM lvi = {0};
	LVFINDINFO lvfi = {0};
	int index;

	hWndList = GetDlgItem(hWnd, IDC_LIST_FAST);

	if (Insert) {

		ASSERT(Node != NULL);
		ASSERT(Buffer != NULL);

		Module = Node->Module;
		index = ListView_GetItemCount(hWndList);

		lvi.mask = LVIF_PARAM | LVIF_TEXT;
		lvi.lParam = (LPARAM)hTreeItem;
		lvi.iItem = index;

		lvi.iSubItem = 0;
		lvi.pszText = _wcslwr(Module->Name);
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

INT_PTR CALLBACK
FastProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	INT_PTR Status = FALSE;

	switch (uMsg) {

		case WM_COMMAND:
			return FastOnCommand(hWnd, uMsg, wp, lp);

		case WM_INITDIALOG:
			FastOnInitDialog(hWnd, uMsg, wp, lp);
			SdkCenterWindow(hWnd);
			Status = TRUE;
			break;

		case WM_NOTIFY:
			return FastOnNotify(hWnd, uMsg, wp, lp);

		case WM_CTLCOLORSTATIC:
			return FastOnCtlColorStatic(hWnd, uMsg, wp, lp);
		
		case WM_MOUSEMOVE:
			return FastOnMouseMove(hWnd, uMsg, wp, lp);

		case WM_LBUTTONDOWN:
			return FastOnLButtonDown(hWnd, uMsg, wp, lp);

		case WM_LBUTTONUP:
			return FastOnLButtonUp(hWnd, uMsg, wp, lp);

		case WM_TVN_ITEMCHANGED:
			return FastOnWmItemChanged(hWnd, uMsg, wp, lp);

		case WM_TVN_UNCHECKITEM:
			return FastOnWmUncheckItem(hWnd, uMsg, wp, lp);
		
		case WM_TVN_CHECKITEM:
			return FastOnWmCheckItem(hWnd, uMsg, wp, lp);
	}
	
	return Status;
}


LRESULT
FastOnWmItemChanged(
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
	hWndTree = GetDlgItem(hWnd, IDC_TREE_FAST);

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

	FastOnItemChanged(hWnd, hWndTree, State->hItem, State->NewState, State->OldState, State->lParam);
	SdkFree(State);

	return 0L;
}

//
// N.B. Only XP/2003 requires this routine
//

LRESULT
FastOnWmUncheckItem(
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

	hWndTree = GetDlgItem(hWnd, IDC_TREE_FAST);

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

	FastOnItemChanged(hWnd, hWndTree, hItem, NewState, OldState, 0);
	
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
FastOnWmCheckItem(
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

	hWndTree = GetDlgItem(hWnd, IDC_TREE_FAST);

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

	TreeView_SetCheckState(hWndTree, hItem, TRUE);

	Item.mask = TVIF_STATE;
	Item.stateMask = TVIS_STATEIMAGEMASK ;
	Item.hItem = hItem;
	TreeView_GetItem(hWndTree, &Item);

	NewState = Item.state;

	//
	// N.B. This message can only be triggered to child item, NULL lParam
	//

	FastOnItemChanged(hWnd, hWndTree, hItem, NewState, OldState, 0);
	return 0L;
}

LRESULT
FastOnCtlColorStatic(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PFAST_CONTEXT Context;
	UINT Id;
	HFONT hFont;
	LOGFONT LogFont;
	RECT Rect;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PFAST_CONTEXT)Object->Context;
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
FastBuildCommand(
	IN PDIALOG_OBJECT Object,
	IN PFAST_CONTEXT Context
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
	WCHAR Buffer[MAX_PATH];
	PLIST_ENTRY ListEntry;
	ULONG Number;
	PFAST_NODE Node;
	PBSP_MODULE Module;
	PBSP_EXPORT_DESCRIPTOR Api;
	PFAST_ENTRY Entry;
	LIST_ENTRY DllList;

	InitializeListHead(&DllList);

	hWndTree = GetDlgItem(Object->hWnd, IDC_TREE_FAST);

	DllItem = TreeView_GetChild(hWndTree, TVI_ROOT);

	Checked = INDEXTOSTATEIMAGEMASK(CHECKED);
	Unchecked = INDEXTOSTATEIMAGEMASK(UNCHECKED);

	while (DllItem != NULL) {
		
		RtlZeroMemory(&Item, sizeof(Item));
		Item.mask = TVIF_PARAM;
		Item.hItem = DllItem;
		TreeView_GetItem(hWndTree, &Item);

		Node = (PFAST_NODE)Item.lParam;
		Module = Node->Module;
		Api = Node->Api;

		InitializeListHead(&Node->FastList);

		//
		// Get first child of DllItem
		//

		ApiItem = TreeView_GetChild(hWndTree, DllItem);
		ApiNumber = 0;

		while (ApiItem != NULL) {

			ULONG IsChecked;

			RtlZeroMemory(&Item, sizeof(Item));
			Item.mask = TVIF_PARAM | TVIF_TEXT;
			Item.hItem = ApiItem;
			Item.pszText = Buffer;
			Item.cchTextMax = MAX_PATH;
			TreeView_GetItem(hWndTree, &Item);

			IsChecked = TreeView_GetCheckState(hWndTree, ApiItem);

			if (IsChecked == 0 && Item.lParam != 0) {

				//
				// This probe should be deactivated
				//

				Entry = (PFAST_ENTRY)SdkMalloc(sizeof(FAST_ENTRY));
				RtlZeroMemory(Entry, sizeof(FAST_ENTRY));
				Entry->Activate = FALSE;
				StringCchCopyA(Entry->ApiName, MAX_PATH, Api[ApiNumber].Name);

				InsertTailList(&Node->FastList, &Entry->ListEntry);
				Node->Count += 1;
			} 
			
			if (IsChecked == 1 && Item.lParam == 0) {

				//
				// This probe should be activated
				//

				Entry = (PFAST_ENTRY)SdkMalloc(sizeof(FAST_ENTRY));
				RtlZeroMemory(Entry, sizeof(FAST_ENTRY));
				Entry->Activate = TRUE;
				StringCchCopyA(Entry->ApiName, MAX_PATH, Api[ApiNumber].Name);

				InsertTailList(&Node->FastList, &Entry->ListEntry);
				Node->Count += 1;
			}

			ApiNumber += 1;
			ApiItem = TreeView_GetNextSibling(hWndTree, ApiItem);
		}
	
		//
		// Chain the fast list if the module has action to do
		//

		if (Node->Count != 0) {
			InsertTailList(&DllList, &Node->ListEntry);
		} 

		DllItem = TreeView_GetNextSibling(hWndTree, DllItem);
	}

	//
	// Copy scanned result into MSP_USER_COMMAND's command list
	//

	Command = NULL;

	if (IsListEmpty(&DllList) != TRUE) {

		Command = (PMSP_USER_COMMAND)SdkMalloc(sizeof(MSP_USER_COMMAND));
		Command->CommandType = CommandProbe;
		Command->ProcessId = Context->Process->ProcessId;
		Command->Type = PROBE_FAST;
		Command->Status = 0;
		Command->CommandCount = 0;
		Command->CommandLength = 0;
		Command->FailureCount = 0;

		InitializeListHead(&Command->CommandList);
		InitializeListHead(&Command->FailureList);

		Command->Callback = NULL;
		Command->CallbackContext = NULL;
	}

	while (IsListEmpty(&DllList) != TRUE) {
	
		ListEntry = RemoveHeadList(&DllList);
		Node = CONTAINING_RECORD(ListEntry, FAST_NODE, ListEntry);	
		Module = Node->Module;

		Length = FIELD_OFFSET(BTR_USER_COMMAND, Probe[Node->Count]);
		BtrCommand = (PBTR_USER_COMMAND)SdkMalloc(Length);
		BtrCommand->Length = Length;
		BtrCommand->Type = PROBE_FAST;
		BtrCommand->Count = Node->Count;

		StringCchCopyW(BtrCommand->DllName, MAX_PATH, Module->Name);
	
		for(Number = 0; Number < Node->Count; Number += 1) {

			Entry = CONTAINING_RECORD(RemoveHeadList(&Node->FastList), 
				                      FAST_ENTRY, ListEntry);	

			BtrCommand->Probe[Number].Activate = Entry->Activate;
			StringCchCopyA(BtrCommand->Probe[Number].ApiName, MAX_PATH, Entry->ApiName);

			SdkFree(Entry);
		}

		InsertTailList(&Command->CommandList, &BtrCommand->ListEntry);
		Command->CommandCount += 1;
		Command->CommandLength += BtrCommand->Length;
	}

	Context->Command = Command;
}

VOID
FastFreeCommand(
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

LRESULT CALLBACK
FastListProcedure(
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
				Handled = FastListOnDelete(hWnd, uMsg, wp, lp);
				if (Handled) {
					return TRUE;
				}
			}
	}

	return DefSubclassProc(hWnd, uMsg, wp, lp);
} 

LRESULT 
FastOnColumnClick(
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
	PFAST_CONTEXT Context;

	Dialog = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Dialog, FAST_CONTEXT);
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
    
	hWndList = GetDlgItem(hWnd, IDC_LIST_FAST);
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
	ListView_SortItemsEx(hWndList, FastSortCallback, (LPARAM)Object);

    return 0L;
}

int CALLBACK
FastSortCallback(
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

BOOLEAN
FastListOnDelete(
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
			hWndTree = GetDlgItem(hWndParent, IDC_TREE_FAST);

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

LRESULT 
FastOnLButtonDown(
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
	PFAST_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, FAST_CONTEXT);

	hWndTree = GetDlgItem(hWnd, IDC_TREE_FAST);
	hWndList = GetDlgItem(hWnd, IDC_LIST_FAST);

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
FastOnLButtonUp(
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
	PFAST_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, FAST_CONTEXT);

	if (!Context->DragMode) {
		return 0;
	}

	hWndTree = GetDlgItem(hWnd, IDC_TREE_FAST);
	hWndList = GetDlgItem(hWnd, IDC_LIST_FAST);

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

	FastAdjustPosition(Object);
	ReleaseCapture();
	return 0;
}

LRESULT 
FastOnMouseMove(
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
	PFAST_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = SdkGetContext(Object, FAST_CONTEXT);

	pt.x = GET_X_LPARAM(lp);
	pt.y = GET_Y_LPARAM(lp);
	ClientToScreen(hWnd, &pt);
	
	hWndTree = GetDlgItem(hWnd, IDC_TREE_FAST);
	hWndList = GetDlgItem(hWnd, IDC_LIST_FAST);

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
FastAdjustPosition(
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
	PFAST_CONTEXT Context;
	LONG SplitbarLeft;

	Context = SdkGetContext(Object, FAST_CONTEXT);

	hWnd = Object->hWnd;
	hWndTree = GetDlgItem(hWnd, IDC_TREE_FAST);
	hWndList = GetDlgItem(hWnd, IDC_LIST_FAST);
	
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
 
ULONG
FastScanSelection(
	IN PDIALOG_OBJECT Object,
	OUT PLIST_ENTRY DllListHead
	)
{
	HWND hWndTree;
	HTREEITEM DllItem;
	HTREEITEM ApiItem;
	TVITEMEX Item = {0};
	ULONG ApiNumber;
	ULONG Length;
	PFAST_NODE Node;
	PBSP_MODULE Module;
	PBSP_EXPORT_DESCRIPTOR Api;
	PCSP_FAST Fast;
	ULONG IsChecked;
	ULONG DllCount;
	ULONG Count;

	DllCount = 0;
	InitializeListHead(DllListHead);

	hWndTree = GetDlgItem(Object->hWnd, IDC_TREE_FAST);
	DllItem = TreeView_GetChild(hWndTree, TVI_ROOT);

	while (DllItem != NULL) {
		
		RtlZeroMemory(&Item, sizeof(Item));
		Item.mask = TVIF_PARAM;
		Item.hItem = DllItem;
		TreeView_GetItem(hWndTree, &Item);

		Node = (PFAST_NODE)Item.lParam;

		if (!Node->NumberOfSelected) {
			DllItem = TreeView_GetNextSibling(hWndTree, DllItem);
			continue;
		}

		Module = Node->Module;
		Api = Node->Api;

		Length = FIELD_OFFSET(CSP_FAST, ApiName[Node->NumberOfSelected]);
		Fast = (PCSP_FAST)SdkMalloc(Length);
		StringCchCopy(Fast->DllName, MAX_PATH, Module->Name);
		Fast->ApiCount = Node->NumberOfSelected;

		//
		// Get first child of DllItem
		//

		Count = 0;
		ApiNumber = 0;
		ApiItem = TreeView_GetChild(hWndTree, DllItem);

		while (ApiItem != NULL) {

			IsChecked = TreeView_GetCheckState(hWndTree, ApiItem);

			if (IsChecked == 1) {
				StringCchCopyA(Fast->ApiName[Count], MAX_PATH, Api[ApiNumber].Name);
				Count += 1;
			}

			ApiNumber += 1;
			ApiItem = TreeView_GetNextSibling(hWndTree, ApiItem);
		}
	
		ASSERT(Count == Node->NumberOfSelected);
		InsertTailList(DllListHead, &Fast->ListEntry);
		DllCount += 1;

		DllItem = TreeView_GetNextSibling(hWndTree, DllItem);
	}

	return DllCount;
}

ULONG 
FastExportProbe(
	IN PDIALOG_OBJECT Object,
	IN PWSTR Path
	)
{
	LIST_ENTRY ListHead;
	PCSP_FAST Fast;
	PLIST_ENTRY ListEntry;
	CSP_FILE_HEADER Head;
	HANDLE FileHandle;
	ULONG Count;
	ULONG Size;
	ULONG Length;
	ULONG Complete;

	InitializeListHead(&ListHead);

	Count = FastScanSelection(Object, &ListHead);
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
		Fast = CONTAINING_RECORD(ListEntry, CSP_FAST, ListEntry);	

		Length = FIELD_OFFSET(CSP_FAST, ApiName[Fast->ApiCount]);
		WriteFile(FileHandle, Fast, Length, &Complete, NULL);	

		Size += Length;
		SdkFree(Fast);
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
	Head.Type = PROBE_FAST;
	Head.DllCount = Count;

	WriteFile(FileHandle, &Head, sizeof(CSP_FILE_HEADER), &Complete, NULL);
	CloseHandle(FileHandle);

	return 0;	
}

ULONG 
FastImportProbe(
	IN PWSTR Path,
	OUT struct _MSP_USER_COMMAND **Command
	)
{
	HANDLE FileHandle;
	HANDLE MapObject;
	PVOID MapAddress;
	CSP_FILE_HEADER Head;
	PCSP_FAST Fast;
	PBTR_USER_COMMAND BtrCommand;
	PMSP_USER_COMMAND MspCommand;
	ULONG Length;
	ULONG Number;
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

	Fast = (PCSP_FAST)((PUCHAR)MapAddress + sizeof(CSP_FILE_HEADER));
	MspCommand = (PMSP_USER_COMMAND)SdkMalloc(sizeof(MSP_USER_COMMAND));
	RtlZeroMemory(MspCommand, sizeof(MSP_USER_COMMAND));

	MspCommand->CommandType = CommandProbe;
	MspCommand->Type = PROBE_FAST;
	InitializeListHead(&MspCommand->CommandList);
	InitializeListHead(&MspCommand->FailureList);

	for(Number = 0; Number < Head.DllCount; Number += 1) {

		Length = FIELD_OFFSET(BTR_USER_COMMAND, Probe[Fast->ApiCount]);

		BtrCommand = (PBTR_USER_COMMAND)SdkMalloc(Length);
		BtrCommand->Length = Length; 
		BtrCommand->Type = PROBE_FAST;
		BtrCommand->Status = 0;
		BtrCommand->FailureCount = 0;
		BtrCommand->Count = Fast->ApiCount;

		//
		// N.B. DllName is a base name, this is ok for btr to work as expected,
		// because dpc file can be imported from another machine which has different
		// dll path, so we decide to use base name to avoid this issue.
		//

		StringCchCopy(BtrCommand->DllName, MAX_PATH, Fast->DllName);

		for(i = 0; i < Fast->ApiCount; i++) {
			RtlZeroMemory(&BtrCommand->Probe[i], sizeof(BTR_USER_PROBE));
			StringCchCopyA(BtrCommand->Probe[i].ApiName, MAX_PATH, Fast->ApiName[i]);
			BtrCommand->Probe[i].Activate = TRUE;
		}

		InsertTailList(&MspCommand->CommandList, &BtrCommand->ListEntry);
		MspCommand->CommandCount += 1;
		MspCommand->CommandLength += Length;

		//
		// Move to next fast entry
		//

		Length = FIELD_OFFSET(CSP_FAST, ApiName[Fast->ApiCount]);
		Fast = (PCSP_FAST)((PUCHAR)Fast + Length);
	}

	*Command = MspCommand;

	UnmapViewOfFile(MapAddress);
	CloseHandle(MapObject);
	CloseHandle(FileHandle);

	return ERROR_SUCCESS;	
}

LRESULT
FastOnExport(
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
	Status = FastExportProbe(Object, Buffer);

	if (Status != 0) {
		StringCchPrintf(Buffer, MAX_PATH, L"Error 0x%08x occurred !", Status);
		MessageBox(hWnd, Buffer, L"D Probe", MB_OK|MB_ICONERROR); 
	}

	return 0;
}