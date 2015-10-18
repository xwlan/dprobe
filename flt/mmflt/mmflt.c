//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "mmflt.h"

ULONG
MmConvertBitsToString(
	IN ULONG Flags,
	IN BITS_DECODE Bits[],
	IN ULONG BitsSize,
	IN PSTR Buffer,
	IN ULONG BufferSize,
	OUT PULONG ResultSize
	)
{
	ULONG Number;
	ULONG Count;
	PSTR Next;

	Count = 0;
	Next = Buffer;

	//
	// Fast check zero flag.
	//

	if (Flags == 0) {
		*ResultSize = 0;
		return 0;
	}

	//
	// N.B. We don't check buffer overflow here, user must ensure BufferSize
	// is large enough, maximum size can be computed at compile time by OR all
	// string literal, this avoid heap allocation, or large stack buffer.
	//

	for (Number = 0; Number < BitsSize; Number += 1) {
		if ((Flags & Bits[Number].Bit) != 0) {
			Count = sprintf_s(Next, BufferSize - ((ULONG_PTR)Next - (ULONG_PTR)Buffer), 
				               "%s|", Bits[Number].Text); 
			Next += Count;
		}
	}

	//
	// Cut the last '|' operator
	//

	if (Buffer != Next) {
		Next -= 1;
		*Next = '\0';
	} else {
	
		//
		// There's garbage bits
		//

		Buffer[0] = '\0';
	}

	*ResultSize = (ULONG)((ULONG_PTR)Next - (ULONG_PTR)Buffer);
	return 0;	
}

BITS_DECODE RtlHeapBits[] = {
{ HEAP_NO_SERIALIZE, "HEAP_NO_SERIALIZE", sizeof("HEAP_NO_SERIALIZE") },
{ HEAP_GROWABLE, "HEAP_GROWABLE", sizeof("HEAP_GROWABLE") },
{ HEAP_GENERATE_EXCEPTIONS, "HEAP_GENERATE_EXCEPTIONS", sizeof("HEAP_GENERATE_EXCEPTIONS") },
{ HEAP_ZERO_MEMORY, "HEAP_ZERO_MEMORY", sizeof("HEAP_ZERO_MEMORY") },
{ HEAP_REALLOC_IN_PLACE_ONLY, "HEAP_REALLOC_IN_PLACE_ONLY", sizeof("HEAP_REALLOC_IN_PLACE_ONLY") },
{ HEAP_TAIL_CHECKING_ENABLED, "HEAP_TAIL_CHECKING_ENABLED", sizeof("HEAP_TAIL_CHECKING_ENABLED") },
{ HEAP_FREE_CHECKING_ENABLED, "HEAP_FREE_CHECKING_ENABLED", sizeof("HEAP_FREE_CHECKING_ENABLED") },
{ HEAP_DISABLE_COALESCE_ON_FREE, "HEAP_DISABLE_COALESCE_ON_FREE", sizeof("HEAP_DISABLE_COALESCE_ON_FREE") },
{ HEAP_CREATE_ALIGN_16, "HEAP_CREATE_ALIGN_16", sizeof("HEAP_CREATE_ALIGN_16") },
{ HEAP_CREATE_ENABLE_TRACING, "HEAP_CREATE_ENABLE_TRACING", sizeof("HEAP_CREATE_ENABLE_TRACING") },
{ HEAP_CREATE_ENABLE_EXECUTE, "HEAP_CREATE_ENABLE_EXECUTE", sizeof("HEAP_CREATE_ENABLE_EXECUTE") },
};
 
BITS_DECODE MmSectionBits[] = {
{ SEC_FILE, "SEC_FILE", sizeof("SEC_FILE") },
{ SEC_IMAGE, "SEC_IMAGE", sizeof("SEC_IMAGE") },
{ SEC_PROTECTED_IMAGE, "SEC_PROTECTED_IMAGE", sizeof("SEC_PROTECTED_IMAGE") },
{ SEC_RESERVE, "SEC_RESERVE", sizeof("SEC_RESERVE") },
{ SEC_COMMIT, "SEC_COMMIT", sizeof("SEC_COMMIT") },
{ SEC_NOCACHE, "SEC_NOCACHE", sizeof("SEC_NOCACHE") },
{ SEC_WRITECOMBINE, "SEC_WRITECOMBINE", sizeof("SEC_WRITECOMBINE") },
{ SEC_LARGE_PAGES, "SEC_LARGE_PAGES", sizeof("SEC_LARGE_PAGES") },
};

BITS_DECODE MmMappingBits[]={
{ FILE_MAP_COPY, "FILE_MAP_COPY", sizeof("FILE_MAP_COPY") },
{ FILE_MAP_WRITE, "FILE_MAP_WRITE", sizeof("FILE_MAP_WRITE") },
{ FILE_MAP_READ, "FILE_MAP_READ", sizeof("FILE_MAP_READ") },
{ FILE_MAP_EXECUTE, "FILE_MAP_EXECUTE", sizeof("FILE_MAP_EXECUTE") },
};

BITS_DECODE MmAllocationBits[] = {
{ MEM_COMMIT, "MEM_COMMIT", sizeof("MEM_COMMIT") },
{ MEM_RESERVE, "MEM_RESERVE", sizeof("MEM_RESERVE") },
{ MEM_DECOMMIT, "MEM_DECOMMIT", sizeof("MEM_DECOMMIT") },
{ MEM_RELEASE, "MEM_RELEASE", sizeof("MEM_RELEASE") },
{ MEM_FREE, "MEM_FREE", sizeof("MEM_FREE") },
{ MEM_PRIVATE, "MEM_PRIVATE", sizeof("MEM_PRIVATE") },
{ MEM_MAPPED, "MEM_MAPPED", sizeof("MEM_MAPPED") },
{ MEM_RESET, "MEM_RESET", sizeof("MEM_RESET") },
{ MEM_TOP_DOWN, "MEM_TOP_DOWN", sizeof("MEM_TOP_DOWN") },
{ MEM_WRITE_WATCH, "MEM_WRITE_WATCH", sizeof("MEM_WRITE_WATCH") },
{ MEM_PHYSICAL, "MEM_PHYSICAL", sizeof("MEM_PHYSICAL") },
{ MEM_ROTATE, "MEM_ROTATE", sizeof("MEM_ROTATE") },
{ MEM_IMAGE, "MEM_IMAGE", sizeof("MEM_IMAGE") },
{ MEM_LARGE_PAGES, "MEM_LARGE_PAGES", sizeof("MEM_LARGE_PAGES") },
{ MEM_4MB_PAGES, "MEM_4MB_PAGES", sizeof("MEM_4MB_PAGES") },
};

BITS_DECODE MmProtectionBits[] = {
{ PAGE_NOACCESS, "PAGE_NOACCESS", sizeof("PAGE_NOACCESS") },
{ PAGE_READONLY, "PAGE_READONLY", sizeof("PAGE_READONLY") },
{ PAGE_READWRITE, "PAGE_READWRITE", sizeof("PAGE_READWRITE") },
{ PAGE_WRITECOPY, "PAGE_WRITECOPY", sizeof("PAGE_WRITECOPY") },
{ PAGE_EXECUTE, "PAGE_EXECUTE", sizeof("PAGE_EXECUTE") },
{ PAGE_EXECUTE_READ, "PAGE_EXECUTE_READ", sizeof("PAGE_EXECUTE_READ") },
{ PAGE_EXECUTE_READWRITE, "PAGE_EXECUTE_READWRITE", sizeof("PAGE_EXECUTE_READWRITE") },
{ PAGE_EXECUTE_WRITECOPY, "PAGE_EXECUTE_WRITECOPY", sizeof("PAGE_EXECUTE_WRITECOPY") },
{ PAGE_GUARD, "PAGE_GUARD", sizeof("PAGE_GUARD") },
{ PAGE_WRITECOMBINE, "PAGE_WRITECOMBINE", sizeof("PAGE_WRITECOMBINE") },
};

typedef enum MM_BITS_SIZE {

	RtlHeapMaxCch = sizeof("NO_SERIALIZE|GROWABLE|GENERATE_EXCEPTIONS|ZERO_MEMORY|\
							REALLOC_IN_PLACE_ONLY|TAIL_CHECKING_ENABLED|FREE_CHECKING_ENABLED|\
							DISABLE_COALESCE_ON_FREE|CREATE_ALIGN_16|CREATE_ENABLE_TRACING|CREATE_ENABLE_EXECUTE"),
	RtlHeapArraySize = _countof(RtlHeapBits),

	MmSectionMaxCch = sizeof("SEC_FILE|SEC_IMAGE|SEC_PROTECTED_IMAGE|SEC_COMMIT|SEC_NOCACHE|\
							SEC_WRITECOMBINE|SEC_LARGE_PAGES"),
	MmSectionArraySize = _countof(MmSectionBits),

	MmAllocationMaxCch = sizeof("MEM_COMMIT|MEM_RESERVE|MEM_DECOMMIT|MEM_RELEASE|MEM_FREE|MEM_PRIVATE|\
								MEM_MAPPED|MEM_RESET|MEM_TOP_DOWN|MEM_WRITE_WATCH|MEM_PHYSICAL|MEM_ROTATE|\
								MEM_IMAGE|MEM_LARGE_PAGES|MEM_4MB_PAGES"),
	MmAllocationArraySize = _countof(MmAllocationBits),

	MmProtectionMaxCch = sizeof("PAGE_NOACCESS|PAGE_READONLY|PAGE_READWRITE|PAGE_WRITECOPY|\
								PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|\
								PAGE_EXECUTE_WRITECOPY|PAGE_GUARD|PAGE_WRITECOMBINE"),
	MmProtectionArraySize = _countof(MmProtectionBits),

	MmMappingMaxCch = sizeof("FILE_MAP_COPY|FILE_MAP_WRITE|FILE_MAP_READ|FILE_MAP_EXECUTE"),
	MmMappingArraySize = _countof(MmMappingBits),

} MM_BITS_SIZE, *PMM_BITS_SIZSE;

PVOID WINAPI
HeapAllocEnter(
	IN HANDLE hHeap,
	IN DWORD dwFlags,
	IN SIZE_T dwBytes 
	)
{
	PBTR_FILTER_RECORD Record;
	PMM_HEAPALLOC Filter;
	HEAPALLOC_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	PVOID Address;

	BtrFltGetContext(&Context);
	Routine = (HEAPALLOC_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Address = (*Routine)(hHeap, dwFlags, dwBytes);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(MM_HEAPALLOC),
		                          MmGuid, _HeapAlloc);

	Filter = FILTER_RECORD_POINTER(Record, MM_HEAPALLOC);
	Filter->hHeap = hHeap;
	Filter->dwFlags = dwFlags;
	Filter->dwBytes = dwBytes;
	Filter->Block = Address;

	BtrFltQueueRecord(Record);
	return Address;
}

ULONG
HeapAllocDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PMM_HEAPALLOC Data;
	CHAR Flag[RtlHeapMaxCch];
	ULONG Length;

	if (Record->ProbeOrdinal != _HeapAlloc) {
		return S_FALSE;
	}	

	Data = (PMM_HEAPALLOC)Record->Data;
	MmConvertBitsToString(Data->dwFlags, RtlHeapBits, RtlHeapArraySize,
		                  Flag, RtlHeapMaxCch, &Length);

	if (Data->dwFlags != 0) {
		StringCchPrintf(Buffer, MaxCount - 1, 
			L"Handle: 0x%x, Flag: 0x%x (%S), Size: 0x%x, Block: 0x%x", 
			Data->hHeap, Data->dwFlags, Flag, Data->dwBytes, Data->Block);
	} else {
		StringCchPrintf(Buffer, MaxCount - 1, 
			L"Handle: 0x%x, Flag: 0x%x, Size: 0x%x, Block: 0x%x", 
			Data->hHeap, Data->dwFlags, Data->dwBytes, Data->Block);
	}

	return S_OK;
}

BOOL WINAPI
HeapFreeEnter(
	IN HANDLE hHeap,
	IN ULONG dwFlags,
	IN PVOID lpMem 
	)
{
	PBTR_FILTER_RECORD Record;
	PMM_HEAPFREE Filter;
	HEAPFREE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (HEAPFREE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(hHeap, dwFlags, lpMem);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(MM_HEAPFREE),
		                          MmGuid, _HeapFree);

	Filter = FILTER_RECORD_POINTER(Record, MM_HEAPFREE);
	Filter->hHeap = hHeap;
	Filter->dwFlags = dwFlags;
	Filter->lpMem = lpMem;

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
HeapFreeDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PMM_HEAPFREE Data;
	CHAR Flag[RtlHeapMaxCch];
	ULONG Length;

	if (Record->ProbeOrdinal != _HeapFree) {
		return S_FALSE;
	}	

	Data = FILTER_RECORD_POINTER(Record, MM_HEAPFREE);;
	MmConvertBitsToString(Data->dwFlags, RtlHeapBits, RtlHeapArraySize,
		                  Flag, RtlHeapMaxCch, &Length);

	if (Data->dwFlags != 0) {
		StringCchPrintf(Buffer, MaxCount - 1, 
			L"Handle: 0x%x, Flag: 0x%x (%S), Block: 0x%x", 
			Data->hHeap, Data->dwFlags, Flag, Data->lpMem);
	} else {
		StringCchPrintf(Buffer, MaxCount - 1, 
			L"Handle: 0x%x, Flag: 0x%x, Block: 0x%x", 
			Data->hHeap, Data->dwFlags, Data->lpMem);
	}

	return S_OK;
}

LPVOID WINAPI 
HeapReAllocEnter(
	__in HANDLE hHeap,
	__in DWORD dwFlags,
	__in LPVOID lpMem,
	__in SIZE_T dwBytes
	)
{
	PBTR_FILTER_RECORD Record;
	PMM_HEAPREALLOC Filter;
	HEAPREALLOC_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	PVOID Address;

	BtrFltGetContext(&Context);
	Routine = (HEAPREALLOC_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Address = (*Routine)(hHeap, dwFlags, lpMem, dwBytes);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(MM_HEAPREALLOC),
		                          MmGuid, _HeapReAlloc);

	Filter = FILTER_RECORD_POINTER(Record, MM_HEAPREALLOC);
	Filter->hHeap = hHeap;
	Filter->dwFlags = dwFlags;
	Filter->lpMem = lpMem;
	Filter->dwBytes = dwBytes;
	Filter->Block = Address;

	BtrFltQueueRecord(Record);
	return Address;
}

ULONG 
HeapReAllocDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PMM_HEAPREALLOC Data;
	CHAR Flag[RtlHeapMaxCch];
	ULONG Length;

	if (Record->ProbeOrdinal != _HeapReAlloc) {
		return S_FALSE;
	}	

	Data = (PMM_HEAPREALLOC)Record->Data;
	MmConvertBitsToString(Data->dwFlags, RtlHeapBits, RtlHeapArraySize,
		                  Flag, RtlHeapMaxCch, &Length);

	if (Data->dwFlags != 0) {
		StringCchPrintf(Buffer, MaxCount - 1, 
			L"Handle: 0x%x, Flag: 0x%x (%S), Size: 0x%x, Old: 0x%x, Block: 0x%x", 
			Data->hHeap, Data->dwFlags, Flag, Data->dwBytes, Data->lpMem, Data->Block);
	} else {
		StringCchPrintf(Buffer, MaxCount - 1, 
			L"Handle: 0x%x, Flag: 0x%x, Size: 0x%x, Old: 0x%x, Block: 0x%x", 
			Data->hHeap, Data->dwFlags, Flag, Data->dwBytes, Data->lpMem, Data->Block);
	}

	return S_OK;
}

HANDLE WINAPI
HeapCreateEnter( 
	IN DWORD flOptions,
	IN SIZE_T dwInitialSize,
	IN SIZE_T dwMaximumSize
	)
{
	PBTR_FILTER_RECORD Record;
	PMM_HEAPCREATE Filter;
	HEAPCREATE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	PVOID Handle;	

	BtrFltGetContext(&Context);
	Routine = (HEAPCREATE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Handle = (*Routine)(flOptions, dwInitialSize, dwMaximumSize); 
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(MM_HEAPCREATE),
		                          MmGuid, _HeapCreate);

	Filter = FILTER_RECORD_POINTER(Record, MM_HEAPCREATE);
	Filter->flOptions = flOptions;
	Filter->dwInitialSize = dwInitialSize;
	Filter->dwMaximumSize = dwMaximumSize;
	Filter->Handle = Handle;
	
	BtrFltQueueRecord(Record);
	return Handle;
}

ULONG
HeapCreateDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PMM_HEAPCREATE Data;
	CHAR Flag[RtlHeapMaxCch];
	ULONG Length;

	if (Record->ProbeOrdinal != _HeapCreate) {
		return S_FALSE;
	}	

	Data = (PMM_HEAPCREATE)Record->Data;
	MmConvertBitsToString(Data->flOptions, RtlHeapBits, RtlHeapArraySize,
		                  Flag, RtlHeapMaxCch, &Length);
	if (Data->flOptions != 0) {
		StringCchPrintf(Buffer, MaxCount, 
			L"Flags: 0x%x (%S), InitialSize: 0x%x, MaximumSize: 0x%x, Handle=0x%x", 
			Data->flOptions, Flag, Data->dwInitialSize, Data->dwMaximumSize, Data->Handle);
	} else {
		StringCchPrintf(Buffer, MaxCount, 
			L"Flags: 0x%x, InitialSize: 0x%x, MaximumSize: 0x%x, Handle=0x%x", 
			Data->flOptions, Data->dwInitialSize, Data->dwMaximumSize, Data->Handle);
	}

	return S_OK;
}

BOOL WINAPI
HeapDestroyEnter( 
    IN HANDLE hHeap 
    ) 
{
	PBTR_FILTER_RECORD Record;
	PMM_HEAPDESTROY Filter;
	HEAPDESTROY_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (HEAPDESTROY_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(hHeap);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(MM_HEAPDESTROY),
		                          MmGuid, _HeapDestroy);

	Filter = FILTER_RECORD_POINTER(Record, MM_HEAPDESTROY);
	Filter->hHeap = hHeap;

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
HeapDestroyDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PMM_HEAPDESTROY Data;

	if (Record->ProbeOrdinal != _HeapDestroy) {
		return S_FALSE;
	}	

	Data = (PMM_HEAPDESTROY)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, L"Handle: 0x%x", Data->hHeap);
	return S_OK;
}


LPVOID WINAPI 
VirtualAllocExEnter(
	IN HANDLE hProcess,
	IN LPVOID lpAddress,
	IN SIZE_T dwSize,
	IN DWORD flAllocationType,
	IN DWORD flProtect
	)
{
	PBTR_FILTER_RECORD Record;
	PMM_VIRTUALALLOCEX Filter;
	VIRTUALALLOCEX_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	PVOID ReturnedAddress;	

	BtrFltGetContext(&Context);
	Routine = (VIRTUALALLOCEX_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	ReturnedAddress = (*Routine)(hProcess, lpAddress, dwSize,
		                         flAllocationType, flProtect);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(MM_VIRTUALALLOCEX),
		                          MmGuid, _VirtualAllocEx);

	Filter = FILTER_RECORD_POINTER(Record, MM_VIRTUALALLOCEX);
	Filter->ProcessHandle = hProcess;
	Filter->Address = lpAddress;
	Filter->Size = dwSize;
	Filter->AllocationType = flAllocationType;
	Filter->ProtectType = flProtect;
	Filter->ReturnedAddress = ReturnedAddress;

	BtrFltQueueRecord(Record);
	return ReturnedAddress;
}

ULONG 
VirtualAllocExDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PMM_VIRTUALALLOCEX Data;
	CHAR AllocationFlag[MmAllocationMaxCch];
	CHAR ProtectionFlag[MmProtectionMaxCch];
	ULONG Length;

	if (Record->ProbeOrdinal != _VirtualAllocEx) {
		return S_FALSE;
	}	

	Data = (PMM_VIRTUALALLOCEX)Record->Data;
	MmConvertBitsToString(Data->AllocationType, MmAllocationBits, MmAllocationArraySize,
		                  AllocationFlag, MmAllocationMaxCch, &Length);
	
	MmConvertBitsToString(Data->ProtectType, MmProtectionBits, MmProtectionArraySize,
		                  ProtectionFlag, MmProtectionMaxCch, &Length);
	
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"Process: 0x%x, Desired Address: 0x%x, Size: 0x%x, Allocation: %S,"
		L"Protect: %S, Returned Address: 0x%x", 
		Data->ProcessHandle, Data->Address, Data->Size, AllocationFlag, 
		ProtectionFlag, Data->ReturnedAddress);

	return S_OK;
}

BOOL WINAPI 
VirtualFreeExEnter(
	IN HANDLE hProcess,
	IN LPVOID lpAddress,
	IN SIZE_T dwSize,
	IN DWORD dwFreeType
	)
{
	PBTR_FILTER_RECORD Record;
	PMM_VIRTUALFREEEX Filter;
	VIRTUALFREEEX_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (VIRTUALFREEEX_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(hProcess, lpAddress, dwSize, dwFreeType);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(MM_VIRTUALFREEEX),
		                          MmGuid, _VirtualFreeEx);

	Filter = FILTER_RECORD_POINTER(Record, MM_VIRTUALFREEEX);
	Filter->ProcessHandle = hProcess;
	Filter->Address = lpAddress;
	Filter->Size = dwSize;
	Filter->FreeType = dwFreeType;
	Filter->Status = Status;
	Filter->LastErrorStatus = GetLastError();

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG 
VirtualFreeExDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PMM_VIRTUALFREEEX Data;
	CHAR Flag[MmAllocationMaxCch];
	ULONG Length;

	if (Record->ProbeOrdinal != _VirtualFreeEx) {
		return S_FALSE;
	}	

	Data = (PMM_VIRTUALFREEEX)Record->Data;
	MmConvertBitsToString(Data->FreeType, MmAllocationBits, MmAllocationArraySize,
		                  Flag, MmAllocationMaxCch, &Length);

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"Process: 0x%x, Address: 0x%x, Size: 0x%x, FreeType: %S",
		Data->ProcessHandle, Data->Address, Data->Size, Flag);
	return S_OK;
}

BOOL WINAPI 
VirtualProtectExEnter(
	IN  HANDLE hProcess,
	IN  LPVOID lpAddress,
	IN  SIZE_T dwSize,
	IN  DWORD flNewProtect,
	OUT PDWORD lpflOldProtect
	)
{
	PBTR_FILTER_RECORD Record;
	PMM_VIRTUALPROTECTEX Filter;
	VIRTUALPROTECTEX_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (VIRTUALPROTECTEX_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(hProcess, lpAddress, dwSize, flNewProtect, lpflOldProtect);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(MM_VIRTUALPROTECTEX),
		                          MmGuid, _VirtualProtectEx);

	Filter = FILTER_RECORD_POINTER(Record, MM_VIRTUALPROTECTEX);
	Filter->ProcessHandle = hProcess;
	Filter->Address = lpAddress;
	Filter->Size = dwSize;
	Filter->NewProtect = flNewProtect;
	Filter->OldProtect = (ULONG_PTR)lpflOldProtect;
	Filter->Status = Status;
	Filter->LastErrorStatus = GetLastError();

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG 
VirtualProtectExDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PMM_VIRTUALPROTECTEX Data;
	CHAR New[MmProtectionMaxCch];
	CHAR Old[MmProtectionMaxCch];
	ULONG Length;

	if (Record->ProbeOrdinal != _VirtualProtectEx) {
		return S_FALSE;
	}	

	Data = (PMM_VIRTUALPROTECTEX)Record->Data;
	MmConvertBitsToString(Data->NewProtect, MmProtectionBits, MmProtectionArraySize,
		                  New, MmProtectionMaxCch, &Length);
	MmConvertBitsToString((ULONG)Data->OldProtect, MmProtectionBits, MmProtectionArraySize,
		                  Old, MmProtectionMaxCch, &Length);
	
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"Process: 0x%x, Address: 0x%x, Size: 0x%x, New: %S, Old: %S, LastError: 0x%x",
		Data->ProcessHandle, Data->Address, Data->Size, New, Old, Record->Base.LastError);

	return S_OK;
}

HANDLE WINAPI 
CreateFileMappingEnter(
	IN HANDLE hFile,
	IN LPSECURITY_ATTRIBUTES lpAttributes,
	IN DWORD flProtect,
	IN DWORD dwMaximumSizeHigh,
	IN DWORD dwMaximumSizeLow,
	IN LPCWSTR lpName
	)
{
	PBTR_FILTER_RECORD Record;
	PMM_CREATEFILEMAPPING Filter;
	CREATEFILEMAPPING_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	HANDLE FileMappingHandle;

	BtrFltGetContext(&Context);
	Routine = (CREATEFILEMAPPING_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	FileMappingHandle = (*Routine)(hFile, lpAttributes, flProtect,
		                           dwMaximumSizeHigh, dwMaximumSizeLow,
								   lpName);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(MM_CREATEFILEMAPPING),
		                          MmGuid, _CreateFileMapping);

	Filter = FILTER_RECORD_POINTER(Record, MM_CREATEFILEMAPPING);
	Filter->FileHandle = hFile;
	Filter->Security = lpAttributes;
	Filter->Protect = flProtect;
	Filter->MaximumSizeHigh = dwMaximumSizeHigh;
	Filter->MaximumSizeLow = dwMaximumSizeLow;

	if (lpName != NULL) {
		StringCchCopy(Filter->Name, MAX_PATH, lpName);
	}

	Filter->LastErrorStatus = GetLastError();
	Filter->FileMappingHandle = FileMappingHandle;

	BtrFltQueueRecord(Record);
	return FileMappingHandle;
}

ULONG 
CreateFileMappingDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PMM_CREATEFILEMAPPING Data;
	CHAR ProtectionFlag[MmProtectionMaxCch];
	CHAR SectionFlag[MmSectionMaxCch];
	ULONG ProtectionLength;
	ULONG SectionLength;

	if (Record->ProbeOrdinal != _CreateFileMapping) {
		return S_FALSE;
	}	

	Data = (PMM_CREATEFILEMAPPING)Record->Data;
	MmConvertBitsToString(Data->Protect, MmProtectionBits, MmProtectionArraySize,
		                  ProtectionFlag, MmProtectionMaxCch, &ProtectionLength);
	
	MmConvertBitsToString(Data->Protect, MmSectionBits, MmSectionArraySize,
		                  SectionFlag, MmSectionMaxCch, &SectionLength);

	//
	// Complement the OR sign
	//

	if (SectionLength != 0 && ProtectionLength != 0) {
		ProtectionFlag[ProtectionLength] = '|';
		ProtectionFlag[ProtectionLength + 1] = '\0';
	}

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"File: 0x%x, SA: 0x%x, Protect: %S%S, MaxSizeHigh: 0x%x, MaxSizeLow: 0x%x,\
		 Name=%s, Handle: 0x%x, LastError: 0x%x",
		Data->FileHandle, Data->Security, ProtectionFlag, SectionFlag, Data->MaximumSizeHigh, Data->MaximumSizeLow,
		Data->Name, Data->FileMappingHandle, Record->Base.LastError);

	return S_OK;
}

LPVOID WINAPI 
MapViewOfFileEnter(
	IN HANDLE hFileMappingObject,
	IN DWORD dwDesiredAccess,
	IN DWORD dwFileOffsetHigh,
	IN DWORD dwFileOffsetLow,
	IN SIZE_T dwNumberOfBytesToMap
	)
{
	PBTR_FILTER_RECORD Record;
	PMM_MAPVIEWOFFILE Filter;
	MAPVIEWOFFILE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	PVOID MappedAddress;

	BtrFltGetContext(&Context);
	Routine = (MAPVIEWOFFILE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	MappedAddress = (*Routine)(hFileMappingObject, dwDesiredAccess,
                               dwFileOffsetHigh, dwFileOffsetLow,
							   dwNumberOfBytesToMap);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(MM_MAPVIEWOFFILE),
		                          MmGuid, _MapViewOfFile);

	Filter = FILTER_RECORD_POINTER(Record, MM_MAPVIEWOFFILE);
	Filter->FileMappingHandle = hFileMappingObject;
	Filter->DesiredAccess = dwDesiredAccess;
	Filter->FileOffsetHigh = dwFileOffsetHigh;
	Filter->FileOffsetLow = dwFileOffsetLow;
	Filter->NumberOfBytesToMap = dwNumberOfBytesToMap;
	Filter->MappedAddress = MappedAddress;
	Filter->LastErrorStatus = GetLastError();

	BtrFltQueueRecord(Record);
	return MappedAddress;
}

ULONG
MapViewOfFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PMM_MAPVIEWOFFILE Data;
	CHAR Flag[MmMappingMaxCch];
	ULONG Length;

	if (Record->ProbeOrdinal != _MapViewOfFile) {
		return S_FALSE;
	}	

	Data = (PMM_MAPVIEWOFFILE)Record->Data;
	MmConvertBitsToString(Data->DesiredAccess, MmMappingBits, MmMappingArraySize,
		                  Flag, MmMappingMaxCch, &Length);
	
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"Handle: 0x%x, Access: %S, OffsetHigh: 0x%x, OffsetLow: 0x%x, Size: 0x%x, Address: 0x%x",
		Data->FileMappingHandle, Flag, Data->FileOffsetHigh, Data->FileOffsetLow,
		Data->NumberOfBytesToMap, Data->MappedAddress);

	return S_OK;
}

BOOL WINAPI 
UnmapViewOfFileEnter(
	IN LPCVOID lpBaseAddress
	)
{
	PBTR_FILTER_RECORD Record;
	PMM_UNMAPVIEWOFFILE Filter;
	UNMAPVIEWOFFILE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (UNMAPVIEWOFFILE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(lpBaseAddress);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(MM_UNMAPVIEWOFFILE),
		                          MmGuid, _UnmapViewOfFile);

	Filter = FILTER_RECORD_POINTER(Record, MM_UNMAPVIEWOFFILE);
	Filter->MappedAddress = (PVOID)lpBaseAddress;
	Filter->LastErrorStatus = GetLastError();
	Filter->Status = Status;

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG 
UnmapViewOfFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PMM_UNMAPVIEWOFFILE Data;

	if (Record->ProbeOrdinal != _UnmapViewOfFile) {
		return S_FALSE;
	}	

	Data = (PMM_UNMAPVIEWOFFILE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"Address: 0x%x",Data->MappedAddress);

	return S_OK;
}

HANDLE WINAPI 
OpenFileMappingEnter(
	IN DWORD dwDesiredAccess,
	IN BOOL bInheritHandle,
	IN LPCWSTR lpName
	)
{
	PBTR_FILTER_RECORD Record;
	PMM_OPENFILEMAPPING Filter;
	OPENFILEMAPPING_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	HANDLE FileMappingHandle;

	BtrFltGetContext(&Context);
	Routine = (OPENFILEMAPPING_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	FileMappingHandle = (*Routine)(dwDesiredAccess, bInheritHandle, lpName);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(MM_OPENFILEMAPPING),
		                          MmGuid, _OpenFileMapping);

	Filter = FILTER_RECORD_POINTER(Record, MM_OPENFILEMAPPING);
	Filter->DesiredAccess = dwDesiredAccess;
	Filter->InheritHandle = bInheritHandle;
	Filter->FileMappingHandle = FileMappingHandle;
	Filter->LastErrorStatus = GetLastError();

	if (lpName != NULL) {
		StringCchCopy(Filter->Name, MAX_PATH, lpName);
	}

	BtrFltQueueRecord(Record);
	return FileMappingHandle;
}

ULONG 
OpenFileMappingDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PMM_OPENFILEMAPPING Data;
	CHAR Flag[MmMappingMaxCch];
	ULONG Length;

	if (Record->ProbeOrdinal != _OpenFileMapping) {
		return S_FALSE;
	}	

	Data = (PMM_OPENFILEMAPPING)Record->Data;
	MmConvertBitsToString(Data->DesiredAccess, MmMappingBits, MmMappingArraySize,
		                  Flag, MmMappingMaxCch, &Length);

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"Name: '%s', Handle: 0x%x, InheritHandle: %u, Access: %S, LastError: 0x%x",
		Data->Name, Data->FileMappingHandle, Data->InheritHandle, Flag, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
FlushViewOfFileEnter(
	IN LPCVOID lpBaseAddress,
	IN SIZE_T dwNumberOfBytesToFlush
	)
{
	PBTR_FILTER_RECORD Record;
	PMM_FLUSHVIEWOFFILE Filter;
	FLUSHVIEWOFFILE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (FLUSHVIEWOFFILE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(lpBaseAddress, dwNumberOfBytesToFlush);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(MM_FLUSHVIEWOFFILE),
		                          MmGuid, _FlushViewOfFile);

	Filter = FILTER_RECORD_POINTER(Record, MM_FLUSHVIEWOFFILE);
	Filter->BaseAddress = (PVOID)lpBaseAddress;
	Filter->NumberOfBytesToFlush = dwNumberOfBytesToFlush;
	Filter->LastErrorStatus = GetLastError();
	Filter->Status = Status;

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG 
FlushViewOfFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PMM_FLUSHVIEWOFFILE Data;

	if (Record->ProbeOrdinal != _FlushViewOfFile) {
		return S_FALSE;
	}	

	Data = (PMM_FLUSHVIEWOFFILE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, L"Address: 0x%x, Size: 0x%x", 
					Data->BaseAddress, Data->NumberOfBytesToFlush);

	return S_OK;
}

//
// VirtualQueryEx
//

SIZE_T WINAPI 
VirtualQueryExEnter(
  __in  HANDLE hProcess,
  __in  LPCVOID lpAddress,
  __out PMEMORY_BASIC_INFORMATION lpBuffer,
  __in  SIZE_T dwLength
)
{
	PBTR_FILTER_RECORD Record;
	PMM_VIRTUALQUERYEX Filter;
	VIRTUALQUERYEX_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	SIZE_T Return;

	BtrFltGetContext(&Context);
	Routine = (VIRTUALQUERYEX_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Return = (*Routine)(hProcess, lpAddress, lpBuffer, dwLength);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(MM_VIRTUALQUERYEX),
		                          MmGuid, _VirtualQueryEx);

	Filter = FILTER_RECORD_POINTER(Record, MM_VIRTUALQUERYEX);
	Filter->hProcess = hProcess;
	Filter->lpAddress = (PVOID)lpAddress;
	Filter->lpBuffer = lpBuffer;
	Filter->dwLength = dwLength;

	if (lpBuffer && Return) {
		memcpy(&Filter->Mbi, lpBuffer, Return);
	}

	BtrFltQueueRecord(Record);
	return Return;
}


ULONG 
VirtualQueryExDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PMM_VIRTUALQUERYEX Data;
	CHAR AllocationProtectFlag[MmProtectionMaxCch];
	CHAR ProtectionFlag[MmProtectionMaxCch];
	CHAR StateFlag[MmAllocationMaxCch];
	CHAR TypeFlag[MmAllocationMaxCch];
	PMEMORY_BASIC_INFORMATION Info;
	ULONG Length;

	if (Record->ProbeOrdinal != _VirtualQueryEx) {
		return S_FALSE;
	}	

	Data = (PMM_VIRTUALQUERYEX)Record->Data;
	if (!Record->Base.Return) {
		StringCchPrintf(Buffer, MaxCount - 1, 
			L"Process: 0x%x, Address: 0x%x, Buffer: 0x%x, Returned Size: 0, LastError: 0x%x",
			Data->hProcess, Data->lpAddress, Data->lpBuffer, (DWORD)Record->Base.LastError);
		return S_OK;
	}

	Info = &Data->Mbi;
	
	MmConvertBitsToString(Info->AllocationProtect, MmProtectionBits, MmProtectionArraySize,
		                  AllocationProtectFlag, MmProtectionMaxCch, &Length);

	MmConvertBitsToString(Info->Protect, MmProtectionBits, MmProtectionArraySize,
		                  ProtectionFlag, MmProtectionMaxCch, &Length);

	MmConvertBitsToString(Info->State, MmAllocationBits, MmAllocationArraySize,
		                  StateFlag, MmAllocationMaxCch, &Length);

	MmConvertBitsToString(Info->Type, MmAllocationBits, MmAllocationArraySize,
		                  TypeFlag, MmAllocationMaxCch, &Length);

	StringCchPrintf(Buffer, MaxCount - 1, 
		L"Process: 0x%x, Address: 0x%x, Information: \
		 { Base: 0x%x, AllocBase: 0x%x, AllocProtect: %S, Size: 0x%x, State: %S, Protect: %S, Type: %S }", 
		Data->hProcess, Data->lpAddress, Info->BaseAddress, Info->AllocationBase,
		AllocationProtectFlag, Info->RegionSize, StateFlag, ProtectionFlag, TypeFlag);

	return S_OK;
}