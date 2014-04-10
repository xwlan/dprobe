//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "psflt.h"
#include "pshelpers.h"
#include <psapi.h>
	
#pragma comment(lib, "psapi" )
 
//
// Clear noisy lower bits in module handle
//

#define PTR_64KB_ALIGNMENT(_P)  (PVOID)(((ULONG_PTR)_P) & (ULONG_PTR)(~(ULONG_PTR)(1024 * 64 - 1)))

//
// Bits Vector
//

BITS_DECODE _GetModuleHandleExW_Bits[] = {
{ GET_MODULE_HANDLE_EX_FLAG_PIN, "FLAG_PIN", sizeof("FLAG_PIN") }, 
{ GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, "FLAG_UNCHANGED_REFCOUNT", sizeof("FLAG_UNCHANGED_REFCOUNT") }, 
{ GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, "FLAG_FROM_ADDRESS", sizeof("FLAG_FROM_ADDRESS") },
}; 

BITS_DECODE _LoadLibraryEx_Bits[] = {
{ DONT_RESOLVE_DLL_REFERENCES, "DONT_RESOLVE_DLL_REFERENCES", sizeof("DONT_RESOLVE_DLL_REFERENCES") },
{ LOAD_WITH_ALTERED_SEARCH_PATH, "LOAD_WITH_ALTERED_SEARCH_PATH", sizeof("LOAD_WITH_ALTERED_SEARCH_PATH") },
{ LOAD_LIBRARY_AS_DATAFILE, "LOAD_LIBRARY_AS_DATAFILE", sizeof("LOAD_LIBRARY_AS_DATAFILE") },
{ LOAD_IGNORE_CODE_AUTHZ_LEVEL, "LOAD_IGNORE_CODE_AUTHZ_LEVEL", sizeof("LOAD_IGNORE_CODE_AUTHZ_LEVEL") },
{ LOAD_LIBRARY_AS_IMAGE_RESOURCE, "LOAD_LIBRARY_AS_IMAGE_RESOURCE", sizeof("LOAD_LIBRARY_AS_IMAGE_RESOURCE") },
{ LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE, "LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE", sizeof("LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE") },
};

//
// Size Enum
//

typedef enum _PS_BITS_SIZE {
	
	_GetModuleHandleExW_MaxCch = sizeof("FLAG_PIN|FLAG_UNCHANGED_REFCOUNT|FLAG_FROM_ADDRESS"),
	_GetModuleHandleExW_ArraySize = _countof(_GetModuleHandleExW_Bits),
	_GetModuleHandleExA_MaxCch = _GetModuleHandleExW_MaxCch,
	_GetModuleHandleExA_ArraySize = _GetModuleHandleExW_ArraySize,

	_LoadLibraryEx_MaxCch = sizeof("DONT_RESOLVE_DLL_REFERENCES|LOAD_LIBRARY_AS_DATAFILE|\
								   LOAD_WITH_ALTERED_SEARCH_PATH|LOAD_IGNORE_CODE_AUTHZ_LEVEL|\
								   LOAD_LIBRARY_AS_IMAGE_RESOURCE|LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE"),

	_LoadLibraryEx_ArraySize = _countof(_LoadLibraryEx_Bits),

} PS_BITS_SIZE, *PPS_BITS_SIZE;

//
// GetModuleHandle 
//

HMODULE WINAPI 
GetModuleHandleEnter(
	__in  LPCTSTR lpModuleName
	)
{
	HMODULE Return;
	PPS_GETMODULEHANDLE Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	GETMODULEHANDLE Routine;
	size_t Length;
	ULONG UserLength;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(lpModuleName);
	BtrFltSetContext(&Context);

	__try {

		Length = lpModuleName ? (wcslen(lpModuleName) + 1) : 1;

		//
		// N.B. Don't pass sizeof(PS_GETMODULEHANDLE) + Length to BtrFltAllocateRecord,
		// it's wrong, because of structure packing and alignment, it will corrupt memory.
		// use FIELD_OFFSET() macro to allocate variable sized structure.
		//

		UserLength = FIELD_OFFSET(PS_GETMODULEHANDLE, Name[Length]);
		Record = BtrFltAllocateRecord(UserLength, PsUuid, _GetModuleHandle);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_GETMODULEHANDLE);
		Filter->lpModuleName = lpModuleName;
		Filter->NameLength = (USHORT)Length;

		if (Length > 1) {
			StringCchCopyW(Filter->Name, Length, lpModuleName);
		}

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

ULONG
GetModuleHandleDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PPS_GETMODULEHANDLE Data;

	if (Record->ProbeOrdinal != _GetModuleHandle) {
		return S_FALSE;
	}	

	Data = (PPS_GETMODULEHANDLE)Record->Data;

	if (Data->NameLength > 1) {
		StringCchPrintf(Buffer, MaxCount, L"Name: '%s', Handle: 0x%x", 
						Data->Name, (ULONG_PTR)Record->Base.Return);
	} else {
		StringCchPrintf(Buffer, MaxCount, L"Name: NULL, Handle: 0x%x", 
						(ULONG_PTR)Record->Base.Return);
	}

	return S_OK;
}

//
// GetModuleHandleExA
//

BOOL WINAPI 
GetModuleHandleExAEnter(
	__in  DWORD dwFlags,
	__in  PSTR lpModuleName,
	__out HMODULE* phModule
	)
{
	BOOL Return;
	PPS_GETMODULEHANDLEEXA Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	GETMODULEHANDLEEXA Routine;
	size_t Length;
	ULONG UserLength;
	CHAR Buffer[MAX_PATH];
	BOOLEAN UseBuffer = FALSE;

	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(dwFlags, lpModuleName, phModule);
	BtrFltSetContext(&Context);

	__try {

		//
		// Failure case
		//

		if (!Return) { 
			Length = 1;
		}

		else if (dwFlags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS) {
			
			//
			// lpModuleName is a pointer to module address space
			//

			Buffer[0] = 0;

			BtrFltEnterExemptionRegion();
			Length = GetModuleBaseNameA(GetCurrentProcess(), *phModule, Buffer, MAX_PATH);
			BtrFltLeaveExemptionRegion();

			if (!Length) {
				Length = 1;
			} else {
				Length += 1;
				UseBuffer = TRUE;
			}
		}

		//
		// It's name string
		//

		else {
			Length = lpModuleName ? (strlen(lpModuleName) + 1) : 1;
		}

		UserLength = FIELD_OFFSET(PS_GETMODULEHANDLEEXA, Name[Length]);

		Record = BtrFltAllocateRecord(UserLength, PsUuid, _GetModuleHandleExA);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_GETMODULEHANDLEEXA);
		Filter->dwFlags = dwFlags;
		Filter->lpModuleName = lpModuleName;

		if (phModule) 
			Filter->phModule = *phModule;
		else 
			Filter->phModule = NULL;

		Filter->NameLength = (USHORT)Length;

		if (Length > 1) {

			if (!UseBuffer) {
				StringCchCopyA(Filter->Name, Length, lpModuleName);
			} else {
				StringCchCopyA(Filter->Name, Length, Buffer);
			}

		} else {
			Filter->Name[0] = 0;
		}

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

ULONG
GetModuleHandleExADecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PPS_GETMODULEHANDLEEXA Data;
	CHAR Name[MAX_PATH];
	CHAR Flag[_GetModuleHandleExA_MaxCch];
	ULONG Length;

	if (Record->ProbeOrdinal != _GetModuleHandleExA) {
		return S_FALSE;
	}	

	Data = (PPS_GETMODULEHANDLEEXA)Record->Data;
	if (Data->NameLength > 1) {
		StringCchCopyA(Name, MAX_PATH, Data->Name);
	} else {
		StringCchCopyA(Name, MAX_PATH, "NULL");
	}

	PsConvertBitsToString(Data->dwFlags, 
		                  _GetModuleHandleExW_Bits, 
						  _GetModuleHandleExA_ArraySize,
						  Flag, _GetModuleHandleExA_MaxCch, &Length);
						  	
	if (Length) {
		StringCchPrintf(Buffer, MaxCount, L"Flag: %S, Name: %S, Handle: 0x%x", 
						Flag, Name, Data->phModule);
	} else {
		StringCchPrintf(Buffer, MaxCount, L"Flag: 0x%x, Name: %S, Handle: 0x%x", 
						Data->dwFlags, Name, Data->phModule);
	}

	return S_OK;
}

ULONG
GetModuleHandleExWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PPS_GETMODULEHANDLEEXW Data;
	CHAR Flag[_GetModuleHandleExW_MaxCch];
	WCHAR Name[MAX_PATH];
	ULONG Length;

	if (Record->ProbeOrdinal != _GetModuleHandleExW) {
		return S_FALSE;
	}	

	Data = (PPS_GETMODULEHANDLEEXW)Record->Data;
	if (Data->NameLength > 1) {
		StringCchCopy(Name, MAX_PATH, Data->Name);
	} else {
		StringCchCopy(Name, MAX_PATH, L"NULL");
	}
	
	PsConvertBitsToString(Data->dwFlags, 
		                  _GetModuleHandleExW_Bits, 
						  _GetModuleHandleExW_ArraySize,
						  Flag, _GetModuleHandleExW_MaxCch, &Length);
						  	
	if (Length) {
		StringCchPrintf(Buffer, MaxCount, L"Flag: %S, Name: %s, Handle: 0x%x", 
						Flag, Name, Data->phModule);
	} else {
		StringCchPrintf(Buffer, MaxCount, L"Flag: 0x%x, Name: %s, Handle: 0x%x", 
						Data->dwFlags, Name, Data->phModule);
	}
	return S_OK;
}


//
// GetModuleHandleExW
//

BOOL WINAPI 
GetModuleHandleExWEnter(
	__in  DWORD dwFlags,
	__in  PWSTR lpModuleName,
	__out HMODULE* phModule
	)
{
	BOOL Return;
	PPS_GETMODULEHANDLEEXW Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	GETMODULEHANDLEEXW Routine;
	size_t Length;
	ULONG UserLength;
	WCHAR Buffer[MAX_PATH];
	BOOLEAN UseBuffer = FALSE;

	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(dwFlags, lpModuleName, phModule);
	BtrFltSetContext(&Context);

	__try {

		//
		// Failure case
		//

		if (!Return) { 
			Length = 1;
		}

		else if (dwFlags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS) {
			
			//
			// lpModuleName is a pointer to module address space
			//

			Buffer[0] = 0;

			BtrFltEnterExemptionRegion();
			Length = GetModuleBaseNameW(GetCurrentProcess(), *phModule, Buffer, MAX_PATH);
			BtrFltLeaveExemptionRegion();

			if (!Length) {
				Length = 1;
			} else {
				Length += 1;
				UseBuffer = TRUE;
			}
		}

		//
		// It's name string
		//

		else {
			Length = lpModuleName ? (wcslen(lpModuleName) + 1) : 1; 
		}

		UserLength = FIELD_OFFSET(PS_GETMODULEHANDLEEXW, Name[Length]);

		Record = BtrFltAllocateRecord(UserLength, PsUuid, _GetModuleHandleExW);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_GETMODULEHANDLEEXW);
		Filter->dwFlags = dwFlags;
		Filter->lpModuleName = lpModuleName;

		if (Filter->phModule)
			Filter->phModule = *phModule;
		else 
			Filter->phModule = NULL;

		Filter->NameLength = (USHORT)Length;

		if (Length > 1) {
			
			if (!UseBuffer) {
				StringCchCopyW(Filter->Name, Length, lpModuleName);
			} else {
				StringCchCopyW(Filter->Name, Length, Buffer);
			}

		} else {
			Filter->Name[0] = 0;
		}

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

//
// GetModuleFileName
//

DWORD WINAPI 
GetModuleFileNameEnter(
	__in  HMODULE hModule,
	__out PWSTR lpFilename,
	__in  DWORD nSize
	)
{
	DWORD Return;
	PPS_GETMODULEFILENAME Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	GETMODULEFILENAME Routine;
	size_t Length;
	ULONG UserLength;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(hModule, lpFilename, nSize);
	BtrFltSetContext(&Context);

	//
	// MSDN:
	//
	// lpFilename 
	// A pointer to a buffer that receives the fully-qualified path of the module. 
	// If the length of the path exceeds the size that the nSize parameter specifies, 
	// the function succeeds, and the string is truncated to nSize characters and 
	// CAN NOT BE NULL TERMINIATED.
	//

	__try {

		if (Return != 0) {

			//
			// Success, but possible truncated
			//

			if (Return != nSize) {

				//
				// No truncation, safe call wcslen
				//
			
				Length = wcslen(lpFilename) + 1;
			} else {

				//
				// Truncated, copy truncated part
				//

				Length = Return + 1;
			}
		} else {
		
			//
			// Failed, no string copy
			//

			Length = 1;
		}

		UserLength = FIELD_OFFSET(PS_GETMODULEFILENAME, Name[Length]);
		Record = BtrFltAllocateRecord(UserLength, PsUuid, _GetModuleFileName);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_GETMODULEFILENAME);
		Filter->hModule = hModule;
		Filter->lpFilename = lpFilename;
		Filter->nSize = nSize;
		Filter->NameLength = (USHORT)Length;

		if (Length > 1) {
			StringCchCopyW(Filter->Name, Length, lpFilename);
		}

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

ULONG
GetModuleFileNameDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PPS_GETMODULEFILENAME Data;

	if (Record->ProbeOrdinal != _GetModuleFileName) {
		return S_FALSE;
	}	

	Data = (PPS_GETMODULEFILENAME)Record->Data;
	if (Data->NameLength > 1) {
		StringCchPrintf(Buffer, MaxCount, L"Module: 0x%x '%s', Name Size: 0x%x", 
						Data->hModule, Data->Name, (ULONG)Record->Base.Return);
	} else {

		//
		// The call failed or invalid parameter
		//

		StringCchPrintf(Buffer, MaxCount, L"Module: 0x%x (BUFFER LIMITED)", Data->hModule);
	}
	return S_OK;
}

//
// GetProcAddress
//

FARPROC WINAPI 
GetProcAddressEnter(
	__in  HMODULE hModule,
	__in  PSTR lpProcName
	)
{
	FARPROC Return;
	PPS_GETPROCADDRESS Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	GETPROCADDRESS Routine;
	size_t Length;
	ULONG UserLength;
	PLDR_DATA_TABLE_ENTRY Module; 
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(hModule, lpProcName);
	BtrFltSetContext(&Context);

	__try {

		//
		// N.B. lpProcName can be an ordinal, we need verify this
		//

		if (HIWORD(PtrToLong(lpProcName)) == 0) {
			Length = 0;
		}
		else {
			Length = strlen(lpProcName) + 1; 
		} 

		UserLength = FIELD_OFFSET(PS_GETPROCADDRESS, Name[Length]);
		Record = BtrFltAllocateRecord(UserLength,PsUuid, _GetProcAddress);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_GETPROCADDRESS);
		Filter->hModule = hModule;
		Filter->lpProcName = lpProcName;
		Filter->NameLength = (USHORT)Length;

		if (Length > 0) {
			StringCchCopyA(Filter->Name, Length, lpProcName);
		}

		Module = PsGetDllLdrDataEntry(PTR_64KB_ALIGNMENT(hModule));
		if (Module != NULL && Module->BaseDllName.Buffer != NULL 
			               && Module->BaseDllName.Length > 0) {

			StringCchCopyW(Filter->Module, MAX_PATH, Module->BaseDllName.Buffer);
		}

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

ULONG
GetProcAddressDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PPS_GETPROCADDRESS Data;

	if (Record->ProbeOrdinal != _GetProcAddress) {
		return S_FALSE;
	}	

	Data = (PPS_GETPROCADDRESS)Record->Data;
	if (Data->NameLength != 0) {
		StringCchPrintf(Buffer, MaxCount, L"Module: 0x%x '%s', Function: '%S', Address: 0x%x", 
						Data->hModule, Data->Module, Data->Name, (ULONG_PTR)Record->Base.Return);
	} else {
		StringCchPrintf(Buffer, MaxCount, L"Module: 0x%x '%s', Function: ordinal %u, Address: 0x%x", 
						Data->hModule, Data->Module, (USHORT)Data->lpProcName, (ULONG_PTR)Record->Base.Return);
	}
	return S_OK;
}

//
// LoadLibraryExW
//

HMODULE WINAPI 
LoadLibraryExEnter(
	__in  PWSTR lpFileName,
	HANDLE  hFile,
	__in  DWORD dwFlags
	)
{
	HMODULE Return;
	PPS_LOADLIBRARYEX Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	LOADLIBRARYEX Routine;
	size_t Length;
	ULONG UserLength;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(lpFileName, hFile, dwFlags);
	BtrFltSetContext(&Context);

	__try {

		Length = lpFileName ? (wcslen(lpFileName) + 1) : 1;
		UserLength = FIELD_OFFSET(PS_LOADLIBRARYEX, Name[Length]);
		Record = BtrFltAllocateRecord(UserLength, PsUuid, _LoadLibraryEx);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_LOADLIBRARYEX);
		Filter->lpFileName = lpFileName;
		Filter->hFile = hFile;
		Filter->dwFlags = dwFlags;
		Filter->NameLength = (USHORT)Length;

		if (lpFileName) {
			StringCchCopyW(Filter->Name, Length, lpFileName);
		}

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

ULONG
LoadLibraryExDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PPS_LOADLIBRARYEX Data;
	ULONG Length = 0;
	CHAR Flag[_LoadLibraryEx_MaxCch];

	if (Record->ProbeOrdinal != _LoadLibraryEx) {
		return S_FALSE;
	}	

	Data = (PPS_LOADLIBRARYEX)Record->Data;
	PsConvertBitsToString(Data->dwFlags,  _LoadLibraryEx_Bits,
						  _LoadLibraryEx_ArraySize, Flag,_LoadLibraryEx_MaxCch,
						  &Length );

	if (Length) {
		StringCchPrintf(Buffer, MaxCount, L"Name: '%s', Flag: %S, File: 0x%x, Address: 0x%x", 
						Data->Name, Flag, Data->hFile, (ULONG_PTR)Record->Base.Return);
	} else {
		StringCchPrintf(Buffer, MaxCount, L"Name: '%s', Flag: 0x%x, File: 0x%x, Address: 0x%x", 
						Data->Name, Data->dwFlags, Data->hFile, (ULONG_PTR)Record->Base.Return);
	}

	return S_OK;
}

//
// FreeLibrary
//

BOOL WINAPI 
FreeLibraryEnter(
	__in  HMODULE hModule
	)
{
	BOOL Return;
	PPS_FREELIBRARY Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	FREELIBRARY Routine;
	DWORD Length = 0;
	ULONG UserLength = 0;
	WCHAR BaseName[MAX_PATH];
	LDR_DATA_TABLE_ENTRY *Module;
	
	//
	// N.B. Before call any non-btr API, first enter exemption region
	// to avoid reentrance, and leave the exemption region after completion,
	// once thread is in exemption region, no any probe callback can be 
	// invoked, runtime directly invoke original target routine.
	// note that what callback can call what API is still an issue, user
	// should ensure the API call won't incur semantic changes to runtime state.
	//

	BtrFltEnterExemptionRegion();

	__try {

		Module = PsGetDllLdrDataEntry(PTR_64KB_ALIGNMENT(hModule));
		if (Module != NULL && Module->BaseDllName.Buffer != NULL 
			&& Module->BaseDllName.Length > 0) {
				StringCchCopyW(BaseName, MAX_PATH, Module->BaseDllName.Buffer);
				Length = Module->BaseDllName.Length + 1;
		}
	}
	__except(1) {

		//
		// At lease has a null termination.
		//

		Length = 1;
	}

	BtrFltLeaveExemptionRegion();

	//
	// Now, after leaving exemption region, if Routine call B internally,
	// which is also being probed (hooked), B's callback can be executed
	// appropriately without missing any records. This mechanism support
	// safely call tree without losing accuracy.
	//

	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(hModule);
	BtrFltSetContext(&Context);

	__try {

		UserLength = FIELD_OFFSET(PS_FREELIBRARY, Name[Length]);
		Record = BtrFltAllocateRecord(UserLength,PsUuid, _FreeLibrary);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_FREELIBRARY);
		Filter->hModule = hModule;
		Filter->NameLength = (USHORT)Length;
		Filter->Name[0] = 0;

		if (Length > 1) {
			StringCchCopyW(Filter->Name, Length, BaseName);
			Filter->Name[Length - 1] = 0;
		}

		BtrFltEnterExemptionRegion();
		Filter->References = (USHORT)PsGetDllRefCountUnsafe(PTR_64KB_ALIGNMENT(hModule));
		BtrFltLeaveExemptionRegion();

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

ULONG
FreeLibraryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PPS_FREELIBRARY Data;
	WCHAR Name[MAX_PATH];

	if (Record->ProbeOrdinal != _FreeLibrary) {
		return S_FALSE;
	}	

	Data = (PPS_FREELIBRARY)Record->Data;
	memcpy(Name, Data->Name, Data->NameLength * 2);
	Name[Data->NameLength] = 0;

	StringCchPrintf(Buffer, MaxCount, L"Name: '%s', Handle: 0x%x, RefCount: 0x%x", 
					Name, Data->hModule, Data->References);
	return S_OK;
}

//
// Thread routines
//

HANDLE WINAPI 
CreateThreadEnter(
	__in   LPSECURITY_ATTRIBUTES lpThreadAttributes,
	__in   SIZE_T dwStackSize,
	__in   LPTHREAD_START_ROUTINE lpStartAddress,
	__in   LPVOID lpParameter,
	__in   DWORD dwCreationFlags,
	__out  LPDWORD lpThreadId
	)
{
	PPS_THREAD_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	CREATETHREAD Routine;
	HANDLE Return;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(lpThreadAttributes, dwStackSize,
		                lpStartAddress, lpParameter,
						dwCreationFlags, lpThreadId);
	BtrFltSetContext(&Context);

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_THREAD_RECORD), PsUuid, _CreateThread);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_THREAD_RECORD);
		Filter->Ordinal = _CreateThread;
		Filter->Create.CreationFlag = dwCreationFlags;
		Filter->Create.ThreadId = 0;
		Filter->Create.Parameter = lpParameter;
		Filter->Create.StartAddress = lpStartAddress;

		if (lpThreadId != NULL && Return != NULL) { 
			Filter->Create.ThreadId = *lpThreadId;
		}

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

HANDLE WINAPI 
CreateRemoteThreadEnter(
	__in  HANDLE hProcess,
	__in  LPSECURITY_ATTRIBUTES lpThreadAttributes,
	__in  SIZE_T dwStackSize,
	__in  LPTHREAD_START_ROUTINE lpStartAddress,
	__in  LPVOID lpParameter,
	__in  DWORD dwCreationFlags,
	__out LPDWORD lpThreadId
	)
{
	PPS_THREAD_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	CREATEREMOTETHREAD Routine;
	HANDLE Return;
	WCHAR Name[MAX_PATH];
	ULONG ProcessId;
	
	Name[0] = 0;
	ProcessId = 0;

	BtrFltEnterExemptionRegion();

	if (hProcess != LongToHandle(-1)) {
		ProcessId = GetProcessId(hProcess);
		if (ProcessId != GetCurrentProcessId()) {
			GetProcessImageFileName(hProcess, Name, MAX_PATH);
		} else {
			GetModuleFileName(NULL, Name, MAX_PATH);
		}
	} else {
		ProcessId = GetCurrentProcessId();
		GetModuleFileName(NULL, Name, MAX_PATH);
	}

	BtrFltLeaveExemptionRegion();

	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(hProcess, lpThreadAttributes, dwStackSize,
		                lpStartAddress, lpParameter,
						dwCreationFlags, lpThreadId);
	BtrFltSetContext(&Context);

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_THREAD_RECORD), PsUuid, _CreateRemoteThread);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_THREAD_RECORD);
		Filter->Ordinal = _CreateRemoteThread;
		Filter->CreateRemote.CreationFlag = dwCreationFlags;
		Filter->CreateRemote.ThreadId = 0;
		Filter->CreateRemote.Parameter = lpParameter;
		Filter->CreateRemote.StartAddress = lpStartAddress;
		
		if (lpThreadId != NULL && Return != NULL) { 
			Filter->CreateRemote.ThreadId = *lpThreadId;
		}

		Filter->CreateRemote.ProcessId = ProcessId;
		StringCchCopy(Filter->CreateRemote.Name, MAX_PATH, Name);

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

HANDLE WINAPI 
OpenThreadEnter(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  DWORD dwThreadId
	)
{
	PPS_THREAD_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	OPENTHREAD Routine;
	HANDLE Return;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(dwDesiredAccess, bInheritHandle, dwThreadId);
	BtrFltSetContext(&Context);

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_THREAD_RECORD), PsUuid, _OpenThread);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_THREAD_RECORD);
		Filter->Ordinal = _OpenThread;
		Filter->Open.ThreadId = dwThreadId;

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

DWORD WINAPI 
SuspendThreadEnter(
	__in HANDLE hThread
	)
{
	PPS_THREAD_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	SUSPENDTHREAD Routine;
	DWORD Return;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(hThread);
	BtrFltSetContext(&Context);

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_THREAD_RECORD), PsUuid, _SuspendThread);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_THREAD_RECORD);
		Filter->Ordinal = _SuspendThread;
		Filter->Suspend.ThreadHandle = hThread;
		Filter->Suspend.ThreadId = 0;
		Filter->Suspend.SuspendCount = Return;

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

DWORD WINAPI 
ResumeThreadEnter(
	__in HANDLE hThread
	)
{
	PPS_THREAD_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	RESUMETHREAD Routine;
	DWORD Return;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(hThread);
	BtrFltSetContext(&Context);

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_THREAD_RECORD), PsUuid, _ResumeThread);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_THREAD_RECORD);
		Filter->Ordinal = _ResumeThread;
		Filter->Resume.ThreadHandle = hThread;
		Filter->Resume.SuspendCount = Return;
		Filter->Resume.ThreadId = 0;

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

VOID WINAPI 
ExitThreadEnter(
	__in DWORD dwExitCode
	)
{
	PPS_THREAD_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	EXITTHREAD Routine;

	BtrFltGetContext(&Context);
	Routine = Context.Destine;

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_THREAD_RECORD), PsUuid, _ExitThread);
		if (!Record) {
			(*Routine)(dwExitCode);
			BtrFltSetContext(&Context);
			return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_THREAD_RECORD);
		Filter->Ordinal = _ExitThread;
		Filter->Exit.ExitCode = dwExitCode;

		BtrFltEnterExemptionRegion();
		Filter->Exit.ThreadId = GetCurrentThreadId();
		BtrFltLeaveExemptionRegion();

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	(*Routine)(dwExitCode);
	BtrFltSetContext(&Context);
	return;
}

BOOL WINAPI 
TerminateThreadEnter(
	__in HANDLE hThread,
	__in DWORD dwExitCode
	)
{
	PPS_THREAD_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	TERMINATETHREAD Routine;
	BOOL Return;
			
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	
	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_THREAD_RECORD), PsUuid, _TerminateThread);
		if (!Record) {
			Return = (*Routine)(hThread, dwExitCode);
			BtrFltSetContext(&Context);
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_THREAD_RECORD);
		Filter->Ordinal = _TerminateThread;
		Filter->Terminate.ExitCode = dwExitCode;
		Filter->Terminate.ThreadId = 0;

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	Return = (*Routine)(hThread, dwExitCode);
	BtrFltSetContext(&Context);
	return Return;
}

DWORD WINAPI 
TlsAllocEnter(
	VOID
	)
{
	PPS_THREAD_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	TLSALLOC Routine;
	DWORD Return;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)();
	BtrFltSetContext(&Context);

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_THREAD_RECORD), PsUuid, _TlsAlloc);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_THREAD_RECORD);
		Filter->Ordinal = _TlsAlloc;
		Filter->TlsAlloc.Index = Return;
		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

BOOL WINAPI 
TlsFreeEnter(
	__in DWORD dwTlsIndex
	)
{
	PPS_THREAD_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	TLSFREE Routine;
	BOOL Return;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(dwTlsIndex);
	BtrFltSetContext(&Context);

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_THREAD_RECORD), PsUuid, _TlsFree);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_THREAD_RECORD);
		Filter->Ordinal = _TlsFree;
		Filter->TlsFree.Index = dwTlsIndex;
		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

LPVOID WINAPI 
TlsGetValueEnter(
	__in  DWORD dwTlsIndex
	)
{
	PPS_THREAD_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	TLSGETVALUE Routine;
	PVOID Return;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(dwTlsIndex);
	BtrFltSetContext(&Context);

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_THREAD_RECORD), PsUuid, _TlsGetValue);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_THREAD_RECORD);
		Filter->Ordinal = _TlsGetValue;
		Filter->TlsGet.Index = dwTlsIndex;
		Filter->TlsGet.Value = Return;
		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

BOOL WINAPI 
TlsSetValueEnter(
	__in  DWORD dwTlsIndex,
	__in  LPVOID lpTlsValue
	)
{
	PPS_THREAD_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	TLSSETVALUE Routine;
	BOOL Return;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(dwTlsIndex, lpTlsValue);
	BtrFltSetContext(&Context);

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_THREAD_RECORD), PsUuid, _TlsSetValue);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_THREAD_RECORD);
		Filter->Ordinal = _TlsSetValue;
		Filter->TlsGet.Index = dwTlsIndex;
		Filter->TlsGet.Value = lpTlsValue;
		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

BOOL WINAPI 
GetThreadContextEnter(
	__in HANDLE hThread,
	__in LPCONTEXT lpContext
	)
{
	PPS_THREAD_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	GETTHREADCONTEXT Routine;
	BOOL Return;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(hThread, lpContext);
	BtrFltSetContext(&Context);

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_THREAD_RECORD), PsUuid, _GetThreadContext);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_THREAD_RECORD);
		Filter->Ordinal = _GetThreadContext;
		Filter->GetContext.ThreadHandle = hThread;
		Filter->GetContext.ContextFlag = 0;
		Filter->GetContext.Xip = 0;
		Filter->GetContext.Xsp = 0;

		if (lpContext) {
			Filter->GetContext.ContextFlag = lpContext->ContextFlags;
		}

		if (Return && (lpContext->ContextFlags & CONTEXT_CONTROL)) {
#if defined (_M_IX86)
			Filter->GetContext.Xip = lpContext->Eip;
			Filter->GetContext.Xsp = lpContext->Esp;
#elif defined(_M_X64)
			Filter->GetContext.Xip = lpContext->Rip;
			Filter->GetContext.Xsp = lpContext->Rsp;
#endif
		}

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

BOOL WINAPI 
SetThreadContextEnter(
	__in HANDLE hThread,
	__in const CONTEXT* lpContext
	)
{
	PPS_THREAD_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	SETTHREADCONTEXT Routine;
	BOOL Return;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(hThread, lpContext);
	BtrFltSetContext(&Context);

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_THREAD_RECORD), PsUuid, _SetThreadContext);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_THREAD_RECORD);
		Filter->Ordinal = _SetThreadContext;
		Filter->GetContext.ThreadHandle = hThread;
		Filter->GetContext.ContextFlag = 0;
		Filter->GetContext.Xip = 0;
		Filter->GetContext.Xsp = 0;

		if (lpContext) {
			Filter->GetContext.ContextFlag = lpContext->ContextFlags;
		}

		if (lpContext->ContextFlags & CONTEXT_CONTROL) {
#if defined (_M_IX86)
			Filter->GetContext.Xip = lpContext->Eip;
			Filter->GetContext.Xsp = lpContext->Esp;
#elif defined(_M_X64)
			Filter->GetContext.Xip = lpContext->Rip;
			Filter->GetContext.Xsp = lpContext->Rsp;
#endif
		}

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

BOOL WINAPI 
QueueUserWorkItemEnter(
	__in  LPTHREAD_START_ROUTINE Function,
	__in  PVOID ItemContext,
	__in  ULONG Flags
	)
{
	PPS_THREAD_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	QUEUEUSERWORKITEM Routine;
	BOOL Return;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(Function, ItemContext, Flags);
	BtrFltSetContext(&Context);

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_THREAD_RECORD), PsUuid, _QueueUserWorkItem);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_THREAD_RECORD);
		Filter->Ordinal = _QueueUserWorkItem;
		Filter->QueueWorkItem.Function = Function;
		Filter->QueueWorkItem.Context = ItemContext;
		Filter->QueueWorkItem.Flag = Flags;
		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

DWORD WINAPI 
QueueUserAPCEnter(
	__in  PAPCFUNC pfnAPC,
	__in  HANDLE hThread,
	__in  ULONG_PTR dwData
	)
{
	PPS_THREAD_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	QUEUEUSERAPC Routine;
	DWORD Return;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(pfnAPC, hThread, dwData);
	BtrFltSetContext(&Context);

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_THREAD_RECORD), PsUuid, _QueueUserAPC);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_THREAD_RECORD);
		Filter->Ordinal = _QueueUserAPC;
		Filter->QueueApc.ApcRoutine = pfnAPC;
		Filter->QueueApc.ThreadHandle = hThread;
		Filter->QueueApc.Data = dwData;
		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

//
// Process Object
//

HANDLE WINAPI 
OpenProcessEnter(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  DWORD dwProcessId
	)
{
	PPS_PROCESS_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	OPENPROCESS Routine;
	HANDLE Return;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(dwDesiredAccess, bInheritHandle, dwProcessId);
	BtrFltSetContext(&Context);

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_PROCESS_RECORD), PsUuid, _OpenProcess);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_PROCESS_RECORD);
		Filter->Ordinal = _OpenProcess;
		Filter->Open.ProcessId = dwProcessId;
		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

BOOL WINAPI
CreateProcessWEnter(
    __in_opt    LPCWSTR lpApplicationName,
    __inout_opt LPWSTR lpCommandLine,
    __in_opt    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    __in_opt    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    __in        BOOL bInheritHandles,
    __in        DWORD dwCreationFlags,
    __in_opt    LPVOID lpEnvironment,
    __in_opt    LPCWSTR lpCurrentDirectory,
    __in        LPSTARTUPINFOW lpStartupInfo,
    __out       LPPROCESS_INFORMATION lpProcessInformation
	)
{
	PPS_PROCESS_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	CREATEPROCESSW Routine;
	BOOL Return;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Return = (*Routine)(lpApplicationName,
		                lpCommandLine,
						lpProcessAttributes,
						lpThreadAttributes,
						bInheritHandles,
						dwCreationFlags,
						lpEnvironment,
						lpCurrentDirectory,
						lpStartupInfo,
						lpProcessInformation);
	BtrFltSetContext(&Context);

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_PROCESS_RECORD), PsUuid, _CreateProcess);
		if (!Record) {
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_PROCESS_RECORD);
		Filter->Ordinal = _CreateProcess;
		Filter->Create.CreationFlag = dwCreationFlags;
		
		if (lpApplicationName != NULL) {
			StringCchCopy(Filter->Create.Name, MAX_PATH, lpApplicationName);
		}

		if (lpCommandLine != NULL) {
			StringCchCopy(Filter->Create.Cmdline, MAX_PATH, lpCommandLine);
		}

		if (lpCurrentDirectory != NULL) {
			StringCchCopy(Filter->Create.WorkDir, MAX_PATH, lpCurrentDirectory);
		}

		if (lpProcessInformation != NULL) {
			memcpy(&Filter->Create.Info, lpProcessInformation, sizeof(PROCESS_INFORMATION));
		}

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Return;
}

VOID WINAPI 
ExitProcessEnter(
	__in UINT uExitCode
	)
{
	PPS_PROCESS_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	EXITPROCESS Routine;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_PROCESS_RECORD), PsUuid, _ExitProcess);
		if (!Record) {
			(*Routine)(uExitCode);
			BtrFltSetContext(&Context);
			return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_PROCESS_RECORD);
		Filter->Ordinal = _ExitProcess;
		Filter->Exit.ExitCode = uExitCode;
		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	(*Routine)(uExitCode);
	BtrFltSetContext(&Context);
	return;
}

BOOL WINAPI 
TerminateProcessEnter(
	__in  HANDLE hProcess,
	__in  UINT uExitCode
	)
{
	PPS_PROCESS_RECORD Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	TERMINATEPROCESS Routine;
	BOOL Return;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;

	__try {

		Record = BtrFltAllocateRecord(sizeof(PS_PROCESS_RECORD), PsUuid, _TerminateProcess);
		if (!Record) {
			Return = (*Routine)(hProcess, uExitCode);
			BtrFltSetContext(&Context);
			return Return;
		}

		Filter = FILTER_RECORD_POINTER(Record, PS_PROCESS_RECORD);
		Filter->Ordinal = _TerminateProcess;
		Filter->Terminate.ProcessHandle = hProcess;
		Filter->Terminate.ExitCode = uExitCode;
		Filter->Terminate.Name[0] = 0;

		if (hProcess != LongToHandle(-1)) {
			BtrFltEnterExemptionRegion();
			GetProcessImageFileName(hProcess, Filter->Terminate.Name, MAX_PATH);
			BtrFltLeaveExemptionRegion();
		}

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	Return = (*Routine)(hProcess, uExitCode);
	BtrFltSetContext(&Context);
	return Return;
}

ULONG
PsProcessDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PPS_PROCESS_RECORD PsRecord;

	Buffer[0] = 0;
	PsRecord = (PPS_PROCESS_RECORD)&Record->Data;

	switch (Record->ProbeOrdinal) {

		case _CreateProcess:
			{
				StringCchPrintf(Buffer, MaxCount, L"Name: '%s', Cmdline: '%s', WorkDir: '%s', PID: %u ", 
								PsRecord->Create.Name, 
								PsRecord->Create.Cmdline, 
								PsRecord->Create.WorkDir,
								PsRecord->Create.Info.dwProcessId);
			}
			break;

		case _OpenProcess:
			{
				StringCchPrintf(Buffer, MaxCount, L"PID: %u ", 
								PsRecord->Open.ProcessId);
			}
			break;

		case _ExitProcess:
			{
				StringCchPrintf(Buffer, MaxCount, L"ExitCode: 0x%x", 
								PsRecord->Exit.ExitCode);
			}
			break;

		case _TerminateProcess:
			{
				StringCchPrintf(Buffer, MaxCount, L"Name: '%s', Handle: 0x%x, ExitCode: 0x%x ", 
								PsRecord->Terminate.Name, 
								PsRecord->Terminate.ProcessHandle, 
								PsRecord->Terminate.ExitCode);
			}
			break;

		default:
			return S_FALSE;
	}	

	return S_OK;
}

ULONG
PsThreadDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PPS_THREAD_RECORD PsRecord;

	Buffer[0] = 0;
	PsRecord = (PPS_THREAD_RECORD)&Record->Data;

	switch (Record->ProbeOrdinal) {

		case _CreateThread:
			{
				StringCchPrintf(Buffer, MaxCount, L"ThreadProc: 0x%p, Parameter: 0x%p, Flag: 0x%x, TID: %u", 
								PsRecord->Create.StartAddress, 
								PsRecord->Create.Parameter, 
								PsRecord->Create.CreationFlag,
								PsRecord->Create.ThreadId);
			}
			break;

		case _CreateRemoteThread:
			{
				StringCchPrintf(Buffer, MaxCount, L"PID: %u, EXE: '%s', ThreadProc: 0x%p, Parameter: 0x%p, Flag: 0x%x, TID: %u", 
								PsRecord->CreateRemote.ProcessId, 
								PsRecord->CreateRemote.Name, 
								PsRecord->CreateRemote.StartAddress, 
								PsRecord->CreateRemote.Parameter, 
								PsRecord->CreateRemote.CreationFlag,
								PsRecord->CreateRemote.ThreadId);
			}
			break;

		case _OpenThread:
			{
				StringCchPrintf(Buffer, MaxCount, L"TID: %u ", 
								PsRecord->Open.ThreadId);
			}
			break;

		case _ExitThread:
			{
				StringCchPrintf(Buffer, MaxCount, L"TID: %u, ExitCode=0x%x", 
								PsRecord->Exit.ThreadId, 
								PsRecord->Exit.ExitCode);
			}
			break;

		case _TerminateThread:
			{
				StringCchPrintf(Buffer, MaxCount, L"TID: %u, ExitCode:0x%x ", 
								PsRecord->Terminate.ThreadId, 
								PsRecord->Terminate.ExitCode);
			}
			break;

		case _SuspendThread:
			{
				StringCchPrintf(Buffer, MaxCount, L"TID: %u, SuspendCount: %u ", 
								PsRecord->Suspend.ThreadId, 
								PsRecord->Suspend.SuspendCount); 
			}
			break;

		case _ResumeThread:
			{
				StringCchPrintf(Buffer, MaxCount, L"TID: %u, SuspendCount: %u ", 
								PsRecord->Resume.ThreadId, 
								PsRecord->Resume.SuspendCount); 
			}
			break;

		case _GetThreadContext:
			{
				if (PsRecord->GetContext.ContextFlag & CONTEXT_CONTROL) {
					StringCchPrintf(Buffer, MaxCount, L"Handle: 0x%x, Flag: 0x%x, IP: 0x%p, SP: 0x%p", 
									PsRecord->GetContext.ThreadHandle, 
									PsRecord->GetContext.ContextFlag,
									PsRecord->GetContext.Xip, 
									PsRecord->GetContext.Xsp); 
				} else {
					StringCchPrintf(Buffer, MaxCount, L"Handle: 0x%x, Flag: 0x%x",
									PsRecord->GetContext.ThreadHandle, 
									PsRecord->GetContext.ContextFlag);
				}
			}
			break;

		case _SetThreadContext:
			{
				if (PsRecord->GetContext.ContextFlag & CONTEXT_CONTROL) {
					StringCchPrintf(Buffer, MaxCount, L"Handle: 0x%x, Flag: 0x%x, IP: 0x%p, SP: 0x%p", 
									PsRecord->GetContext.ThreadHandle, 
									PsRecord->GetContext.ContextFlag,
									PsRecord->GetContext.Xip, 
									PsRecord->GetContext.Xsp); 
				} else {
					StringCchPrintf(Buffer, MaxCount, L"Handle: 0x%x, Flag: 0x%x",
									PsRecord->GetContext.ThreadHandle, 
									PsRecord->GetContext.ContextFlag);
				}
			}
			break;

		case _TlsAlloc:
			{
				StringCchPrintf(Buffer, MaxCount, L"Index: 0x%x", 
								PsRecord->TlsAlloc.Index); 
			}
			break;

		case _TlsFree:
			{
				StringCchPrintf(Buffer, MaxCount, L"Index: 0x%x", 
								PsRecord->TlsFree.Index); 
			}
			break;

		case _TlsSetValue:
			{
				StringCchPrintf(Buffer, MaxCount, L"Index: 0x%x, Value: 0x%p", 
								PsRecord->TlsSet.Index, PsRecord->TlsSet.Value); 
			}
			break;

		case _TlsGetValue:
			{
				StringCchPrintf(Buffer, MaxCount, L"Index: 0x%x, Value: 0x%p", 
								PsRecord->TlsGet.Index, PsRecord->TlsGet.Value); 
			}
			break;

		case _QueueUserWorkItem:
			{
				StringCchPrintf(Buffer, MaxCount, L"Function: 0x%p, Context: 0x%p, Flag:0x%x ", 
								PsRecord->QueueWorkItem.Function, 
								PsRecord->QueueWorkItem.Context, 
								PsRecord->QueueWorkItem.Flag);
			}
			break;

		case _QueueUserAPC:
			{
				StringCchPrintf(Buffer, MaxCount, L"ApcRoutine: 0x%p, Handle: 0x%p, Data: 0x%p", 
								PsRecord->QueueApc.ApcRoutine, 
								PsRecord->QueueApc.ThreadHandle, 
								PsRecord->QueueApc.Data);
			}
			break;

		default:
			return S_FALSE;
	}	

	return S_OK;
}