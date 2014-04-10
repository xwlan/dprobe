//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009 - 2011
//

#include "btr.h"
#include "heap.h"
#include "hal.h"
#include "filter.h"
#include "util.h"
#include "probe.h"

LIST_ENTRY BtrFilterList;

PBTR_FILTER
BtrQueryFilter(
	IN PWSTR FilterName
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_FILTER Object;

	ListEntry = BtrFilterList.Flink;
	while (ListEntry != &BtrFilterList) {

		Object = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);
		if (_wcsicmp(Object->FilterName, FilterName) == 0) {
			return Object;
		}

		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

ULONG
BtrCreateFilter(
	IN PWSTR FilterFullPath,
	OUT PBTR_FILTER *Result
	)
{
	HMODULE DllHandle;
	PBTR_FILTER FilterObject;
	MODULEINFO ModuleInfo = {0};
	BTR_FLT_GETOBJECT ExportRoutine;
	PVOID Buffer;

	*Result = NULL;

	DllHandle = LoadLibrary(FilterFullPath);
	if (!DllHandle) {
		return BTR_E_FAILED_LOAD_FILTER;
	}

	//
	// Once the filter dll is loaded, ensure that it's unloaded
	// whenever there's error occured during the whole operation
	//

	ExportRoutine = (BTR_FLT_GETOBJECT)GetProcAddress(DllHandle,"BtrFltGetObject");
	if (!ExportRoutine) {
		FreeLibrary(DllHandle);
		return BTR_E_FAILED_GET_FILTER_API;
	}

	__try {
		
		FilterObject = (*ExportRoutine)(FILTER_MODE_CAPTURE);	

	} __except (EXCEPTION_EXECUTE_HANDLER) {
		FreeLibrary(DllHandle);
		return BTR_E_UNEXPECTED;
	}

	if (!FilterObject) {
		FreeLibrary(DllHandle);
		return BTR_E_FILTER_REGISTRATION;
	}

	Buffer = BtrMalloc(MAX_FILTER_PROBE / 8);
	BtrInitializeBitMap(&FilterObject->BitMap, Buffer, MAX_FILTER_PROBE);

	StringCchCopy(FilterObject->FilterName, MAX_PATH, FilterFullPath);

	//
	// Query filter dll base and its size, this is reserved for unload operation which
	// require to check whether any thread is executing any filter dll
	//

	(*BtrGetModuleInformation)(GetCurrentProcess(), DllHandle, 
		                       &ModuleInfo, sizeof(ModuleInfo));

	FilterObject->BaseOfDll = ModuleInfo.lpBaseOfDll;
	FilterObject->SizeOfImage = ModuleInfo.SizeOfImage;

	//
	// Chain the new filter object into global filter list
	//

	InsertHeadList(&BtrFilterList, &FilterObject->ListEntry);

	*Result = FilterObject;
	return S_OK;
}

ULONG
BtrUnregisterFilters(
	VOID
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_FILTER FilterObject;
	HMODULE FilterHandle;
	WCHAR Path[MAX_PATH];

	while (IsListEmpty(&BtrFilterList) != TRUE) {
		
		ListEntry = RemoveHeadList(&BtrFilterList);
		FilterObject = CONTAINING_RECORD(ListEntry, BTR_FILTER, ListEntry);
		FilterHandle = GetModuleHandle(FilterObject->FilterFullPath);

		//
		// Call each filter's unregister callback to release filter resources
		//

		__try {

			if (FilterObject->UnregisterCallback) {
				(*FilterObject->UnregisterCallback)();	
			}
		
		}
		__except(EXCEPTION_EXECUTE_HANDLER) {
	
			//
			// Ignore this exception if any
			//
			
		}

		if (FilterObject->BitMap.Buffer != NULL) {
			BtrFree(FilterObject->BitMap.Buffer);
		}
		
		StringCchCopy(Path, MAX_PATH, FilterObject->FilterFullPath);

		//
		// N.B. Don't free filter object because it belongs to filter
		//

		while(FilterHandle = GetModuleHandle(Path)) {
			
			//
			// Not known whether FreeLibrary once is enough, repeatly until
			// it return error
			//

			FreeLibrary(FilterHandle);
		}
	}

	return S_OK;
}

ULONG
BtrInitializeFlt(
	VOID
	)
{
	InitializeListHead(&BtrFilterList);
	return S_OK;
}

VOID
BtrAcquireLoaderLock(
	VOID
	)
{
	return;
}

VOID
BtrReleaseLoaderLock(
	VOID
	)
{
	return;
}