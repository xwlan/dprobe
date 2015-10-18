//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "bsp.h"
#include "frame.h"
#include <stdio.h>
#include <psapi.h>
#include <strsafe.h>
#include <time.h>
#include "ntapi.h"

HANDLE BspHeapHandle;
double BspSecondPerHardwareTick;

ULONG BspMajorVersion;
ULONG BspMinorVersion;
ULONG BspPageSize;
ULONG BspProcessorNumber;

//
// Internal Logging 
//

#define BSP_LOG_THRESHOLD 1000

LIST_ENTRY BspLogList;
BSP_LOCK BspLogLock;
ULONG BspLogCount;

//
// Compile time process type flag
//

#if defined (_M_IX86)
BOOLEAN BspIs64Bits = FALSE;

#elif defined (_M_X64)
BOOLEAN BspIs64Bits = TRUE;

#endif

//
// Runtime process type flag
//

BOOLEAN BspIsWow64;

typedef BOOL 
(WINAPI *ISWOW64PROCESS)(
	IN HANDLE, 
	OUT PBOOL
	);

ISWOW64PROCESS BspIsWow64ProcessPtr;

ULONG
BspReadProcessCmdline(
	IN HANDLE ProcessHandle,
	IN PVOID PebAddress,
	OUT PVOID Buffer,
	IN ULONG BufferLength,
	OUT PULONG ResultLength
	);

ULONG
BspQueryModuleEx(
	IN PBSP_PROCESS Process,
	IN HANDLE ProcessHandle
	);

ULONG
BspGetModuleTimeStamp(
	IN PBSP_MODULE Module
	);

ULONG
BspGetModuleVersion(
	IN PBSP_MODULE Module
	);

VOID
BspSetFlag(
	OUT PULONG Flag,
	IN ULONG Bit
	)
{
	*Flag = (*Flag) | Bit;
}

//
//
// Implementation
//

ULONG
BspAllocateQueryBuffer(
	IN SYSTEM_INFORMATION_CLASS InformationClass,
	OUT PVOID *Buffer,
	OUT PULONG BufferLength,
	OUT PULONG ActualLength
	)
{
	PVOID BaseVa;
	ULONG Length;
	ULONG ReturnedLength;
	NTSTATUS Status;

	BaseVa = NULL;
	Length = 0;
	ReturnedLength = 0;

	do {
		if (BaseVa != NULL) {
			VirtualFree(BaseVa, 0, MEM_RELEASE);	
		}

		//
		// Incremental unit is 64K this is aligned with Mm subsystem
		//

		Length += 0x10000;

		BaseVa = VirtualAlloc(NULL, Length, MEM_COMMIT, PAGE_READWRITE);
		if (BaseVa == NULL) {
			Status = GetLastError();
			break;
		}

		Status = (*NtQuerySystemInformation)(InformationClass, 
			                                 BaseVa, 
										     Length, 
										     &ReturnedLength);
		
	} while (Status == STATUS_INFO_LENGTH_MISMATCH);

	if (Status != STATUS_SUCCESS) {
		return (ULONG)Status;
	} 
	
	*Buffer = BaseVa;
	*BufferLength = Length;
	*ActualLength = ReturnedLength;

	return ERROR_SUCCESS;
}

VOID
BspReleaseQueryBuffer(
	IN PVOID StartVa
	)
{
	VirtualFree(StartVa, 0, MEM_RELEASE);
}

ULONG
BspAdjustPrivilege(
    IN PWSTR PrivilegeName, 
    IN BOOLEAN Enable
    ) 
{
    HANDLE Handle;
    TOKEN_PRIVILEGES Privileges;
    BOOLEAN Status;
	
	Status = OpenProcessToken(GetCurrentProcess(),  
		                      TOKEN_ADJUST_PRIVILEGES, 
							  &Handle);
    if (Status != TRUE){
        return GetLastError();
    }
    
    Privileges.PrivilegeCount = 1;

    Status = LookupPrivilegeValue(NULL, PrivilegeName, &Privileges.Privileges[0].Luid);

	if (Status != TRUE) {
		CloseHandle(Handle);
		return GetLastError();
	}

	//
	// N.B. SE_PRIVILEGE_REMOVED must be avoided since this will remove privilege from
	// primary token and never get recovered again for current process. For disable case
	// we should use 0.
	//

    Privileges.Privileges[0].Attributes = (Enable ? SE_PRIVILEGE_ENABLED : 0);
    Status = AdjustTokenPrivileges(Handle, 
		                           FALSE, 
								   &Privileges, 
								   sizeof(Privileges), 
								   NULL, 
								   NULL);
	if (Status != TRUE) {
		CloseHandle(Handle);
		return GetLastError();
	}

    CloseHandle(Handle);

    return ERROR_SUCCESS;
}

BOOLEAN 
BspIsWow64Process(
	IN HANDLE ProcessHandle	
	)
{
    BOOL IsWow64 = FALSE;
   
    if (BspIsWow64ProcessPtr) {
        BspIsWow64ProcessPtr(ProcessHandle, &IsWow64);
    }
    return (BOOLEAN)IsWow64;
}

ULONG
BspInitialize(
	VOID
	)
{
	ULONG Status;
	SYSTEM_INFO Information;
	LARGE_INTEGER HardwareFrequency = {0};
	OSVERSIONINFO Version = {0};
	ULONG Value = 2;

	BspHeapHandle = HeapCreate(0, 0, 0);
	if (BspHeapHandle == NULL) {
		return GetLastError();
	}

	HeapSetInformation(BspHeapHandle, HeapCompatibilityInformation, 
		               &Value, sizeof(Value));

	QueryPerformanceFrequency(&HardwareFrequency);
	BspSecondPerHardwareTick = 1.0 / HardwareFrequency.QuadPart;

	Version.dwOSVersionInfoSize = sizeof(Version);

	GetVersionEx(&Version);
	BspMajorVersion = Version.dwMajorVersion;
	BspMinorVersion = Version.dwMinorVersion;

	GetSystemInfo(&Information);
	BspPageSize = Information.dwPageSize;
	BspProcessorNumber = Information.dwNumberOfProcessors;
	
	//
	// Check whether it's wow64 process
	//

	BspIsWow64ProcessPtr = (ISWOW64PROCESS)GetProcAddress(GetModuleHandle(L"kernel32"), 
		                                                  "IsWow64Process");
	if (!BspIs64Bits) {
		BspIsWow64 = BspIsWow64Process(GetCurrentProcess());
	}

	//
	// Enable debug privilege
	//

	Status = BspAdjustPrivilege(SE_DEBUG_NAME, TRUE);
	if (Status != ERROR_SUCCESS) {
		return Status;
	}

	BspGetSystemRoutine();

	//
	// Initialize logging data
	//

	InitializeListHead(&BspLogList);
	BspInitializeLock(&BspLogLock);
	BspLogCount = 0;

	return ERROR_SUCCESS;
}

BOOLEAN
BspIsVistaAbove(
	VOID
	)
{
	if (BspMajorVersion >= 6) {
		return TRUE;
	}

	return FALSE;
}

BOOLEAN
BspIsWindowsXPAbove(
	VOID
	)
{
	if (BspMajorVersion > 5) {
		return TRUE;
	}

	if (BspMajorVersion == 5 &&BspMinorVersion >= 1) {
		return TRUE;
	}

	return FALSE;
}

PVOID
BspMalloc(
	IN ULONG Length
	)
{
	PVOID BaseVa;

	ASSERT(BspHeapHandle != NULL);
	BaseVa = HeapAlloc(BspHeapHandle, HEAP_ZERO_MEMORY, Length);
	return BaseVa;
}

VOID
BspFree(
	IN PVOID BaseVa
	)
{
	HeapFree(BspHeapHandle, 0, BaseVa);
}

PBSP_PROCESS
BspQueryProcessById(
	IN PLIST_ENTRY ListHead,
	IN ULONG ProcessId
	)
{
	PLIST_ENTRY ListEntry;
	PBSP_PROCESS Process;
	
	ListEntry = ListHead->Flink;
	while (ListEntry != ListHead) {
		Process = CONTAINING_RECORD(ListEntry, BSP_PROCESS, ListEntry);
		if (Process->ProcessId == ProcessId) {	
			return Process;
		}
		ListEntry = ListEntry->Flink;
	}
	return NULL;
}

ULONG
BspQueryProcessByName(
	IN PWSTR Name,
	OUT PLIST_ENTRY ListHead
	)
{
	LIST_ENTRY ListHead2;
	PLIST_ENTRY ListEntry;
	PBSP_PROCESS Process;
	ULONG Number;

	Number = 0;
	InitializeListHead(ListHead);
	InitializeListHead(&ListHead2);

	BspQueryProcessList(&ListHead2);

	while (IsListEmpty(&ListHead2) != TRUE) {

		ListEntry = RemoveHeadList(&ListHead2);
		Process = CONTAINING_RECORD(ListEntry, BSP_PROCESS, ListEntry);

		if (!_wcsicmp(Name, Process->Name)) {	
			InsertTailList(ListHead, ListEntry);
			Number += 1;
		} else {
			BspFree(Process);
		}
	}

	return Number;
}

VOID
BspFreeProcess(
	IN PBSP_PROCESS Process
	)
{
	if (Process->Name) {
		BspFree(Process->Name);
	}

	if (Process->FullPath) {
		BspFree(Process->FullPath);
	}

	if (Process->CommandLine) {
		BspFree(Process->CommandLine);
	}

	BspFree(Process);
}

VOID
BspFreeProcessList(
	IN PLIST_ENTRY ListHead
	)
{
	PLIST_ENTRY ListEntry;
	PBSP_PROCESS Process;

	while (IsListEmpty(ListHead) != TRUE) {
		ListEntry = RemoveHeadList(ListHead);
		Process = CONTAINING_RECORD(ListEntry, BSP_PROCESS, ListEntry);
		BspFreeProcess(Process);
	}
}

//
// N.B. This routine require debug privilege enabled
//

ULONG
BspQueryProcessInformation(
	IN ULONG ProcessId,
	OUT PSYSTEM_PROCESS_INFORMATION *Information
	)
{
	ULONG Status;
	PVOID StartVa;
	ULONG Length;
	ULONG ReturnedLength;
	ULONG Offset;

	HANDLE ProcessHandle;
	SYSTEM_PROCESS_INFORMATION *Entry;

	ASSERT(ProcessId != 0 && ProcessId != 4);

	//
	// Ensure it's querying system process
	//

	if (ProcessId == 0 || ProcessId == 4) {
		return ERROR_INVALID_PARAMETER;
	}

	StartVa = NULL;
	Length = 0;
	ReturnedLength = 0;

	Status = BspAllocateQueryBuffer(SystemProcessInformation, 
		                            &StartVa,
								    &Length,
									&ReturnedLength);

	if (Status != ERROR_SUCCESS) {
		return Status;
	}

	Offset = 0;
	Status = 0;

	while (Offset < ReturnedLength) {
		
		Entry = (SYSTEM_PROCESS_INFORMATION *)((PCHAR)StartVa + Offset);
		Offset += Entry->NextEntryOffset;

		//
		// Skip all non queried process id
		//

		if (HandleToUlong(Entry->UniqueProcessId) != ProcessId) {
			continue;
		}

		ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, 
			                        HandleToUlong(Entry->UniqueProcessId));
		if (!ProcessHandle) {
			Status = GetLastError();
			break;
		}

		CloseHandle(ProcessHandle);

		//
		// Allocate and copy SYSTEM_PROCESS_INFORMATION block
		//

		Length = FIELD_OFFSET(SYSTEM_PROCESS_INFORMATION, Threads[Entry->NumberOfThreads]);
		*Information  = (PSYSTEM_PROCESS_INFORMATION)BspMalloc(Length);
		RtlCopyMemory(*Information, Entry, Length);
		break;
	}
	
	BspReleaseQueryBuffer(StartVa);
	return Status;
}

//
// N.B. ProcessParameters is what we're interested to
// fetch command line.
//

typedef struct _CURDIR {
     UNICODE_STRING DosPath;
     PVOID Handle;
} CURDIR, *PCURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
     ULONG MaximumLength;
     ULONG Length;
     ULONG Flags;
     ULONG DebugFlags;
     PVOID ConsoleHandle;
     ULONG ConsoleFlags;
     PVOID StandardInput;
     PVOID StandardOutput;
     PVOID StandardError;
     CURDIR CurrentDirectory;
     UNICODE_STRING DllPath;
     UNICODE_STRING ImagePathName;
     UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _PEB {
    UCHAR InheritedAddressSpace;
    UCHAR ReadImageFileExecOptions;
    UCHAR BeingDebugged;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    union
    {
        struct
        {
            UCHAR ImageUsesLargePages:1;
            UCHAR IsProtectedProcess:1;
            UCHAR IsLegacyProcess:1;
            UCHAR IsImageDynamicallyRelocated:1;
            UCHAR SkipPatchingUser32Forwarders:1;
            UCHAR SpareBits:3;
        };
        UCHAR BitField;
    };
#else
    BOOLEAN SpareBool;
#endif
    HANDLE Mutant;
    PVOID ImageBaseAddress;
    PVOID Ldr;
    struct _RTL_USER_PROCESS_PARAMETERS *ProcessParameters;
    PVOID SubSystemData;
    PVOID ProcessHeap;
} PEB, *PPEB;

ULONG
BspQueryProcessList(
	OUT PLIST_ENTRY ListHead
	)
{
	ULONG Status;
	NTSTATUS NtStatus;
	PVOID StartVa;
	ULONG Length;
	ULONG ReturnedLength;
	ULONG Offset;
	USHORT CmdlineLength = 0;

	PROCESS_BASIC_INFORMATION  ProcessInformation;
	HANDLE ProcessHandle;
	PBSP_PROCESS Process;
	SYSTEM_PROCESS_INFORMATION *Entry;
	ULONG_PTR Address;
	WCHAR Buffer[1024];
	SIZE_T Complete;

	ASSERT(ListHead != NULL);

	InitializeListHead(ListHead);

	StartVa = NULL;
	Length = 0;
	ReturnedLength = 0;

	Status = BspAllocateQueryBuffer(SystemProcessInformation, 
		                            &StartVa,
								    &Length,
									&ReturnedLength);

	if (Status != ERROR_SUCCESS) {
		return Status;
	}

	Offset = 0;

	while (Offset < ReturnedLength) {
		
		Entry = (SYSTEM_PROCESS_INFORMATION *)((PCHAR)StartVa + Offset);
		Offset += Entry->NextEntryOffset;

		if (Entry->UniqueProcessId == (HANDLE)0 || Entry->UniqueProcessId == (HANDLE)4) {
			continue;
		}

		ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, HandleToUlong(Entry->UniqueProcessId));
		if (ProcessHandle == NULL) {

			//
			// Some process can not be debugged, e.g. audiodiag.exe on Vista/2008
			//

			if (Entry->NextEntryOffset == 0) {
				Status = ERROR_SUCCESS;
				break;
			} else {
				continue;
			}			
		}
	
		Process = (PBSP_PROCESS)BspMalloc(sizeof(BSP_PROCESS));
		if (Process == NULL) {
			Status = GetLastError();
			break;
		}

		Process->Name = BspMalloc(Entry->ImageName.Length + sizeof(WCHAR));
		wcscpy(Process->Name, Entry->ImageName.Buffer);

		Process->ProcessId = HandleToUlong(Entry->UniqueProcessId);
		Process->SessionId = Entry->SessionId;
		Process->ParentId  = HandleToUlong(Entry->InheritedFromUniqueProcessId);
		Process->KernelTime = Entry->KernelTime;
		Process->UserTime = Entry->UserTime;
		Process->VirtualBytes = Entry->VirtualSize;
		Process->PrivateBytes = Entry->PrivatePageCount;
		Process->WorkingSetBytes = Entry->WorkingSetSize;
		Process->ThreadCount = Entry->NumberOfThreads;
		Process->KernelHandleCount = Entry->HandleCount;
		Process->ReadTransferCount = Entry->ReadTransferCount;
		Process->WriteTransferCount = Entry->WriteTransferCount;
		Process->OtherTransferCount = Entry->OtherTransferCount;

		Process->GdiHandleCount = GetGuiResources(ProcessHandle, GR_GDIOBJECTS);
		Process->UserHandleCount = GetGuiResources(ProcessHandle, GR_USEROBJECTS);

		Process->FullPath = BspMalloc(MAX_PATH + sizeof(WCHAR));
		GetModuleFileNameEx(ProcessHandle, NULL, Process->FullPath, MAX_PATH - 1);

		//
		// Special case for smss.exe
		//

		/*if (_wcsicmp(L"smss.exe", Process->Name) == 0) {
			GetSystemDirectory(&SystemRoot[0], MAX_PATH - 1);
			wcsncat(SystemRoot, L"\\", MAX_PATH - 1);
			wcsncat(SystemRoot, Process->Name, MAX_PATH - 1);
			wcsncpy(Process->FullPath, SystemRoot, MAX_PATH - 1);
		}*/

		Length = 0;
		NtStatus = (*NtQueryInformationProcess)(ProcessHandle,
											    0,
											    &ProcessInformation,
											    sizeof(PROCESS_BASIC_INFORMATION),
											    &Length);		
		//
		// N.B. PEB is the primary key data structure we're interested.
		// We need command line since many processes can only be distingushed
		// from command line parameter, e.g. svchost, dllhost etc.
		//

		if (NtStatus == STATUS_SUCCESS) {

			RtlZeroMemory(Buffer, sizeof(Buffer));

			Process->PebAddress = ProcessInformation.PebBaseAddress;
			Address = (ULONG_PTR)Process->PebAddress + FIELD_OFFSET(PEB, ProcessParameters);
			ReadProcessMemory(ProcessHandle, (PVOID)Address, Buffer, sizeof(PVOID), &Complete); 

			//
			// Get RTL_USER_PROCESS_PARAMETER address
			//

			Address = *(PULONG_PTR)Buffer;
			Address = Address + FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, CommandLine);
			ReadProcessMemory(ProcessHandle, (PVOID)Address, Buffer, sizeof(UNICODE_STRING), &Complete);

			//
			// Get CommandLine UNICODE_STRING data
			//

			Address = (ULONG_PTR)((PUNICODE_STRING)Buffer)->Buffer;
			CmdlineLength = ((PUNICODE_STRING)Buffer)->Length;
			
			if (CmdlineLength > sizeof(WCHAR)) {
				
				CmdlineLength = min(CmdlineLength, sizeof(Buffer));

				Buffer[0] = 0;
				ReadProcessMemory(ProcessHandle, (PVOID)Address, Buffer, CmdlineLength, &Complete);

				Process->CommandLine = BspMalloc(CmdlineLength + sizeof(WCHAR));
				wcscpy(Process->CommandLine, Buffer);

			} else {
				
				Process->CommandLine = NULL;
			}
		}
		
		CloseHandle(ProcessHandle);
		ProcessHandle = NULL;

		InsertTailList(ListHead, &Process->ListEntry);

		if (Entry->NextEntryOffset == 0) {
			Status = ERROR_SUCCESS;
			break;
		}
	}
	
	BspReleaseQueryBuffer(StartVa);
	return Status;
}

ULONG
BspQueryModule(
	IN ULONG ProcessId,
	IN BOOLEAN IncludeVersion, 
	OUT PLIST_ENTRY ListHead
	)
{
	ULONG Status;
	HANDLE Handle;
	MODULEENTRY32 Entry;
    PBSP_MODULE Module;

	InitializeListHead(ListHead);

	if (ProcessId == 0 || ProcessId == 4) {
		return ERROR_SUCCESS;
	}

    Handle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessId);
    if (Handle == INVALID_HANDLE_VALUE){
        Status = GetLastError();
		return Status;
    }	
	 
	Entry.dwSize = sizeof(Entry);

	if (!Module32First(Handle, &Entry)){
        CloseHandle(Handle);
        Status = GetLastError();
		return Status;
    }
    
	do {
		
		Module = (PBSP_MODULE)BspMalloc(sizeof(BSP_MODULE));

		Module->BaseVa = Entry.modBaseAddr;
		Module->Size = Entry.modBaseSize;

		Module->Name = BspMalloc((ULONG)(wcslen(Entry.szModule) + 1) * sizeof(WCHAR));
		wcscpy(Module->Name, Entry.szModule);
		wcslwr(Module->Name);

		Module->FullPath = BspMalloc((ULONG)(wcslen(Entry.szExePath) + 1) * sizeof(WCHAR));
		wcscpy(Module->FullPath, Entry.szExePath);
		wcslwr(Module->Name);
		
		if (IncludeVersion == TRUE) {
			BspGetModuleVersion(Module);
		}

		InsertTailList(ListHead, &Module->ListEntry);

    } while (Module32Next(Handle, &Entry));

	CloseHandle(Handle);
	return ERROR_SUCCESS;
}

ULONG
BspGetModuleVersion(
	IN PBSP_MODULE Module
	)
{
	BOOLEAN Status;
	ULONG Length;
	ULONG Handle;
	PVOID FileVersion;
	PVOID Value;
	WCHAR SubBlock[64];

	struct {
		USHORT Language;
		USHORT CodePage;
	} *Translate;	

	Length = GetFileVersionInfoSize(Module->FullPath, &Handle);
	if (Length == 0) {
		return GetLastError();
	}

	FileVersion = BspMalloc(Length);

	GetFileVersionInfo(Module->FullPath, Handle, Length, FileVersion); 
	Status = VerQueryValue(FileVersion, L"\\VarFileInfo\\Translation", &Translate, &Length);
	if (Status != TRUE) {
		return GetLastError();
	}

	wsprintf(SubBlock,
             L"\\StringFileInfo\\%04x%04x\\ProductVersion",
             Translate[0].Language, 
			 Translate[0].CodePage);

	Value = NULL;
	Status = VerQueryValue(FileVersion, SubBlock, &Value, &Length);						
	if (Status != TRUE) {
		return GetLastError();
	}
	if (Value != NULL) {
		Module->Version = BspMalloc((Length + 1) *sizeof(WCHAR));
		wcscpy(Module->Version, Value);
	}

	wsprintf(SubBlock,
		     L"\\StringFileInfo\\%04x%04x\\CompanyName",
			 Translate[0].Language, 
			 Translate[0].CodePage);
	
	Value = NULL;
	Status = VerQueryValue(FileVersion, SubBlock, &Value, &Length);						
	if (Status != TRUE) {
		return GetLastError();
	}
	if (Value != NULL) {
		Module->Company = BspMalloc((Length + 1) * sizeof(WCHAR));
		wcscpy(Module->Company, Value);
	}

	wsprintf(SubBlock,
		     L"\\StringFileInfo\\%04x%04x\\FileDescription",
			 Translate[0].Language, 
			 Translate[0].CodePage);
	
	Value = NULL;
	Status = VerQueryValue(FileVersion, SubBlock, &Value, &Length);						
	if (Status != TRUE) {
		return GetLastError();
	}
	if (Value != NULL) {
		Module->Description = BspMalloc((Length + 1) * sizeof(WCHAR));
		wcscpy(Module->Description, Value);
	}
	
	BspGetModuleTimeStamp(Module);	
	BspFree(FileVersion);

	return ERROR_SUCCESS;
}

ULONG
BspGetModuleTimeStamp(
	IN PBSP_MODULE Module
	)
{
    HANDLE FileHandle;
	HANDLE FileMappingHandle;
	PVOID MappedVa;
	ULONG64 LinkerTime;
	WCHAR Value[MAX_PATH];
	ULONG Length;
	struct tm *time;

	ASSERT(Module != NULL);

	FileHandle = CreateFile(Module->FullPath,
						    GENERIC_READ,
							FILE_SHARE_READ,
						    NULL, 
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL);
	
	if (FileHandle == INVALID_HANDLE_VALUE) {
		return GetLastError();
	}

    FileMappingHandle = CreateFileMapping(FileHandle, 
		                                  NULL,
										  PAGE_READONLY,
										  0, 
										  0, 
										  NULL);
    
	if (FileHandle == INVALID_HANDLE_VALUE) {
		return GetLastError();
	}
	
	MappedVa = MapViewOfFile(FileMappingHandle, FILE_MAP_READ, 0, 0, 0);
	if (MappedVa == NULL){
		return GetLastError();
	}

	//
	// N.B. Linker time is required, which is real time when the file was generated.
	//

	LinkerTime = GetTimestampForLoadedLibrary(MappedVa);
	
	time = localtime(&LinkerTime);
	time->tm_year += 1900;
	time->tm_mon += 1;

	Length = wsprintf(Value, L"%04d/%02d/%02d %02d:%02d", 
					  time->tm_year,time->tm_mon,time->tm_mday,time->tm_hour, time->tm_min);

	Module->TimeStamp = BspMalloc((Length + 1) * sizeof(WCHAR));
	wcscpy(Module->TimeStamp, Value);

	UnmapViewOfFile(MappedVa);
	CloseHandle(FileHandle);
	CloseHandle(FileMappingHandle);
	
	return ERROR_SUCCESS;
}

VOID
BspFreeModuleList(
	IN PLIST_ENTRY ListHead
	)
{
    PLIST_ENTRY ListEntry;
	PBSP_MODULE Module;
	
	__try {
		while (IsListEmpty(ListHead) != TRUE) {
			ListEntry = RemoveHeadList(ListHead);
			Module = CONTAINING_RECORD(ListEntry, BSP_MODULE, ListEntry);
			BspFreeModule(Module);
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		
	}
}

VOID
BspFreeModule(
	IN PBSP_MODULE Module 
	)
{
	__try {
		if (Module->Name)
			BspFree(Module->Name);

		if (Module->FullPath)
			BspFree(Module->FullPath);

		if (Module->Version) 
			BspFree(Module->Version);

		if (Module->TimeStamp)
			BspFree(Module->TimeStamp);

		if (Module->Company)
			BspFree(Module->Company);

		if (Module->Description)
			BspFree(Module->Description);

		BspFree(Module);

	} __except (EXCEPTION_EXECUTE_HANDLER) {
		
	}
}

ULONG
BspEnumerateDllExport(
	IN PWSTR ModuleName,
	IN PBSP_MODULE Module, 
	OUT PULONG Count,
	OUT PBSP_EXPORT_DESCRIPTOR *Descriptor
	)
{
	PULONG Names;
	PUCHAR Base; 
	HANDLE FileHandle;
	HANDLE ViewHandle;
	ULONG Length, i;
	PULONG Routines;
	PUSHORT Ordinal;
	PIMAGE_DATA_DIRECTORY Directory;
	PIMAGE_SECTION_HEADER SectionHeader;
	PIMAGE_EXPORT_DIRECTORY Export;
	PIMAGE_NT_HEADERS NtHeader;
	PBSP_EXPORT_DESCRIPTOR Buffer;
	ULONG NumberOfSections;
	ULONG NamesOffset, FunctionsOffset, OrdinalsOffset;
	PCHAR Pointer;

	*Count = 0;
	*Descriptor = NULL;

	Base = BspMapImageFile(ModuleName, &FileHandle, &ViewHandle);
	if (!Base){
		return ERROR_INVALID_HANDLE;
	}

	NtHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)Base + ((PIMAGE_DOS_HEADER)Base)->e_lfanew);

#if defined (_M_IX86)
	
	if (NtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
		BspUnmapImageFile(Base, FileHandle, ViewHandle);
		return ERROR_INVALID_HANDLE;
	}

#elif defined (_M_X64)
	
	if (NtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) {
		BspUnmapImageFile(Base, FileHandle, ViewHandle);
		return ERROR_INVALID_HANDLE;
	}

#endif

	Directory = &NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (!Directory->VirtualAddress) {

		//
		// There's no export information
		//

		BspUnmapImageFile(Base, FileHandle, ViewHandle);
		return ERROR_NOT_FOUND;
	}

	//DebugTrace("BSP: Walking export of %S ...", ModuleName);

	SectionHeader = IMAGE_FIRST_SECTION(NtHeader);
	NumberOfSections = NtHeader->FileHeader.NumberOfSections;

	Export = (PIMAGE_EXPORT_DIRECTORY)(Base + BspRvaToOffset(Directory->VirtualAddress, SectionHeader, NumberOfSections));
	NamesOffset = BspRvaToOffset(Export->AddressOfNames, SectionHeader, NumberOfSections);
	FunctionsOffset = BspRvaToOffset(Export->AddressOfFunctions, SectionHeader, NumberOfSections);
	OrdinalsOffset = BspRvaToOffset(Export->AddressOfNameOrdinals, SectionHeader, NumberOfSections);

	//
	// Compute virtual address of name array
	//

	Names    = (PULONG)((ULONG_PTR)Base + (ULONG_PTR)NamesOffset); 
	Routines = (PULONG)((ULONG_PTR)Base + (ULONG_PTR)FunctionsOffset);
	Ordinal  = (PUSHORT)((ULONG_PTR)Base + (ULONG_PTR)OrdinalsOffset);

	//
	// Allocate export descriptor buffer and copy export names to
	// buffer.
	//

	Buffer = (BSP_EXPORT_DESCRIPTOR *)BspMalloc(Export->NumberOfNames * sizeof(BSP_EXPORT_DESCRIPTOR));
	
	for(i = 0; i < Export->NumberOfNames; i++) {

		//
		// N.B. PE specification has a bug that claim Ordinal should subtract
		// OrdinalBase to get real index into function address table, experiment
		// indicates the Ordinal[i] is real index, don't subtract OrdinalBase.
		//

		Buffer[i].Rva = Routines[Ordinal[i]];
		if (Buffer[i].Rva >= Directory->VirtualAddress && 
			Buffer[i].Rva < Directory->VirtualAddress + Directory->Size) {
				
				//
				// This is a forwarder, Rva points to a Ascii string of DLL forwarder
				//

				Pointer = (PUCHAR)((ULONG_PTR)Base + BspRvaToOffset(Names[i], SectionHeader, NumberOfSections));
				Length = (ULONG)strlen(Pointer);
				Buffer[i].Name = (char *)BspMalloc(Length + 1);
				strcpy(Buffer[i].Name, Pointer);

				Pointer = (PUCHAR)(ULONG_PTR)Base + BspRvaToOffset(Buffer[i].Rva, SectionHeader, NumberOfSections);
				Length = (ULONG)strlen(Pointer);
				Buffer[i].Forward = (char *)BspMalloc(Length + 1);
				strcpy(Buffer[i].Forward, Pointer);

				Buffer[i].Ordinal = Ordinal[i];
				Buffer[i].Rva = 0;
				Buffer[i].Va = 0;
				continue;
		}

		Buffer[i].Ordinal = Ordinal[i];

		if (Module != NULL) {
			Buffer[i].Va = (ULONG_PTR)((ULONG_PTR)Module->BaseVa + (ULONG)Buffer[i].Rva);
		} else {
			Buffer[i].Va = (ULONG_PTR)((ULONG_PTR)NtHeader->OptionalHeader.ImageBase + (ULONG)Buffer[i].Rva);
		}

		Pointer = (PUCHAR)((ULONG_PTR)Base + BspRvaToOffset(Names[i], SectionHeader, NumberOfSections));
		Length = (ULONG)strlen(Pointer);
		Buffer[i].Name = (char *)BspMalloc(Length + 1);
		strcpy(Buffer[i].Name, Pointer);

		Buffer[i].Forward = NULL;
	}

	*Count = Export->NumberOfNames;
	*Descriptor = Buffer;

	BspUnmapImageFile(Base, FileHandle, ViewHandle);
	return ERROR_SUCCESS;
}

PVOID
BspGetDllForwardAddress(
	IN ULONG ProcessId,
	IN PSTR DllForward
	)
{
	HANDLE Handle;
	PVOID Address;
	CHAR DllName[MAX_PATH];
	CHAR ApiName[MAX_PATH];
	PWCHAR Unicode;

	BspGetForwardDllName(DllForward, DllName, MAX_PATH);
	BspGetForwardApiName(DllForward, ApiName, MAX_PATH);

	if (ProcessId == GetCurrentProcessId()) {

		Handle = GetModuleHandleA(DllName);
		if (!Handle) {
			return NULL;
		}

		Address = GetProcAddress(Handle, ApiName);
		return Address;
	}

	BspConvertUTF8ToUnicode(DllName, &Unicode);
	Address = BspGetRemoteApiAddress(ProcessId, Unicode, ApiName);

	BspFree(Unicode);
	return Address;
}

VOID
BspGetForwardDllName(
	IN PSTR Forward,
	OUT PCHAR DllName,
	IN ULONG Length
	)
{
	PCHAR Dot;

	Dot = strchr(Forward, '.');
	if (!Dot) {
		DllName[0] = '\0';
		return;
	}

	*Dot = '\0';
	StringCchCopyA(DllName, Length, Forward);
	*Dot = '.';
}

VOID
BspGetForwardApiName(
	IN PSTR Forward,
	OUT PCHAR ApiName,
	IN ULONG Length
	)
{
	PCHAR Dot;

	Dot = strchr(Forward, '.');
	if (!Dot) {
		ApiName[0] = '\0';
		return;
	}

	StringCchCopyA(ApiName, Length, Dot + 1);
}

PBSP_MODULE
BspGetModuleByName(
	IN PLIST_ENTRY ListHead,
	IN PWSTR ModuleName
	)
{
	PLIST_ENTRY ListEntry;
	PBSP_MODULE Module;
	
	ListEntry = ListHead->Flink;
	while (ListEntry != ListHead) {
		Module = CONTAINING_RECORD(ListEntry, BSP_MODULE, ListEntry);
		if (!wcsicmp(Module->Name, ModuleName)){
			return Module;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

ULONG_PTR
BspGetModuleBase(
	IN PWSTR ModuleName
	)
{
	return 0;
}

ULONG_PTR
BspGetRoutineAddress(
	IN PWSTR ModuleName,
	IN PWSTR RoutineName
	)
{
	return 0;
}

PBSP_MODULE
BspGetModuleByAddress(
	IN PLIST_ENTRY ListHead,
	IN PVOID StartVa
	)
{
	PLIST_ENTRY ListEntry;
	PBSP_MODULE Module;
	
	ListEntry = ListHead->Flink;
	while (ListEntry != ListHead) {
		Module = CONTAINING_RECORD(ListEntry, BSP_MODULE, ListEntry);
		if ((ULONG_PTR)Module->BaseVa <= (ULONG_PTR)StartVa && 
			(ULONG_PTR)Module->BaseVa + (ULONG_PTR)Module->Size > (ULONG_PTR)StartVa) {	
			return Module;
		}
		ListEntry = ListEntry->Flink;
	}
	return NULL;
}

ULONG
BspEnumerateDllExportByName(
	IN ULONG ProcessId,
	IN PWSTR ModuleName,
	IN PWSTR ApiName,
	OUT PULONG Count,
	OUT PLIST_ENTRY ApiList
	)
{
	ULONG Status;
	PBSP_MODULE Module;
	ULONG ApiCount = 0;
	PBSP_EXPORT_DESCRIPTOR Api;
	PBSP_EXPORT_DESCRIPTOR Descriptor;
	ULONG i;
	ULONG Number = 0;
	LIST_ENTRY ModuleListHead;
	CHAR AnsiPattern[MAX_PATH];
	CHAR AnsiApiName[MAX_PATH];
	PCHAR Position;

	Status = BspQueryModule(ProcessId, FALSE, &ModuleListHead);
	if (Status != ERROR_SUCCESS) {
		return Status;
	}

	Module = BspGetModuleByName(&ModuleListHead, ModuleName);
	if (Module == NULL) {
		return ERROR_NOT_FOUND;
	}

	BspEnumerateDllExport(Module->FullPath,
	                Module,
					&ApiCount,
					&Api);

	WideCharToMultiByte(CP_ACP, 0, ApiName, -1, AnsiPattern, 256, NULL, NULL);
	strlwr(AnsiPattern);

	for(i = 0; i < ApiCount; i++) {

		strncpy(AnsiApiName, Api[i].Name, MAX_PATH - 1);
		strlwr(AnsiApiName);
		
		Position = strstr(AnsiApiName, AnsiPattern);
		
		if (Position != NULL) {

			Descriptor = (BSP_EXPORT_DESCRIPTOR *)BspMalloc(sizeof(BSP_EXPORT_DESCRIPTOR));
			
			Descriptor->Rva = Api[i].Rva;
			Descriptor->Va = Api[i].Va;
			Descriptor->Ordinal = Api[i].Ordinal;
			Descriptor->Name = (PCHAR)BspMalloc((ULONG)strlen(Api[i].Name) + 1);
			strcpy(Descriptor->Name, Api[i].Name);

			if (Api[i].Forward != NULL){
				Descriptor->Forward = (PCHAR)BspMalloc((ULONG)strlen(Api[i].Forward) + 1);
				strcpy(Descriptor->Forward, Api[i].Forward);
			}

			Number += 1;
			InsertTailList(ApiList, &Descriptor->Entry);
		}
	}
	
	*Count = Number;

	if (ApiCount > 0) {
		BspFreeExportDescriptor(Api, ApiCount);	
	}

	if (IsListEmpty(ApiList)) {
		return ERROR_NOT_FOUND;
	}

	return ERROR_SUCCESS;
}

VOID
BspFreeExportDescriptor(
	IN PBSP_EXPORT_DESCRIPTOR Descriptor,
	IN ULONG Count
	)
{
	ULONG i;
	
	ASSERT(Descriptor != NULL);

	for(i = 0; i < Count; i++) {
		BspFree(Descriptor[i].Name);
		if(Descriptor[i].Forward != NULL) {
			BspFree(Descriptor[i].Forward);
		}
	}

	BspFree(Descriptor);
}

ULONG
BspAttachProcess(
	IN ULONG ProcessId
	)
{
	ULONG ErrorCode;
	BOOLEAN Status;

	Status = DebugActiveProcess(ProcessId);
	if (Status != TRUE) {
		ErrorCode = GetLastError();
		return ErrorCode;
	}

	return ERROR_SUCCESS;
}

VOID
BspInitializeLock(
	IN PBSP_LOCK Lock
	)
{
	InitializeCriticalSection(Lock);
}

VOID
BspAcquireLock(
	IN PBSP_LOCK Lock
	)
{
	EnterCriticalSection(Lock);
}

VOID
BspReleaseLock(
	IN PBSP_LOCK Lock
	)
{
	LeaveCriticalSection(Lock);
}

VOID
BspDestroyLock(
	IN PBSP_LOCK Lock
	)
{
	DeleteCriticalSection(Lock);
}

BOOLEAN
BspTryAcquireLock(
	IN PBSP_LOCK Lock	
	)
{
	return (BOOLEAN)TryEnterCriticalSection(Lock);
}

double
BspComputeMilliseconds(
	IN ULONG Duration
	)
{
	return (Duration * BspSecondPerHardwareTick) * 1000;
}

ULONG
BspRoundUp(
	IN ULONG Value,
	IN ULONG Unit 
	)
{
	return (((ULONG)(Value) + (ULONG)(Unit) - 1) & ~((ULONG)(Unit) - 1)); 
}

ULONG_PTR
BspRoundUpUlongPtr(
	IN ULONG_PTR Value,
	IN ULONG_PTR Unit 
	)
{
	return (((ULONG_PTR)(Value) + (ULONG_PTR)(Unit) - 1) & ~((ULONG_PTR)(Unit) - 1)); 
}

ULONG 
BspRoundDown(
	IN ULONG Value,
	IN ULONG Unit
	)
{
	return ((ULONG)(Value) & ~((ULONG)(Unit) - 1));
}

ULONG64
BspRoundUpUlong64(
	IN ULONG64 Value,
	IN ULONG64 Unit
	)
{
	return (((ULONG64)(Value) + (ULONG64)(Unit) - 1) & ~((ULONG64)(Unit) - 1)); 
}

ULONG64
BspRoundDownUlong64(
	IN ULONG64 Value,
	IN ULONG64 Unit
	)
{
	return ((ULONG64)(Value) & ~((ULONG64)(Unit) - 1));
}

BOOLEAN
BspIsUlong64Aligned(
	IN ULONG64 Value,
	IN ULONG64 Unit
	)
{
	return ((Value & (Unit - 1)) == 0) ? TRUE : FALSE;
}

PVOID
BspAllocateGuardedChunk(
	IN ULONG NumberOfPages	
	)
{
	PVOID Address;
	
	Address = (PVOID)VirtualAlloc(NULL, BspPageSize * (NumberOfPages + 2), MEM_RESERVE, PAGE_READWRITE);
	if (!Address) {
		return NULL;
	}

	Address = VirtualAlloc((PCHAR)Address + BspPageSize, BspPageSize * NumberOfPages, MEM_COMMIT, PAGE_READWRITE);
	return Address;
}

VOID
BspFreeGuardedChunk(
	IN PVOID Address
	)
{
	//
	// N.B. We should do sanity check againt the address, ensure that address is page aligned,
	// and its previous page is in reserve state, via VirtualQuery.
	//

	VirtualFree((PCHAR)Address - BspPageSize, 0, MEM_RELEASE);
	return;
}

PVOID
BspQueryTebAddress(
	IN HANDLE ThreadHandle
	)
{
	NTSTATUS Status;	
	THREAD_BASIC_INFORMATION BasicInformation = {0};

	Status = (*NtQueryInformationThread)(ThreadHandle,
										 0,
									     &BasicInformation,
									     sizeof(THREAD_BASIC_INFORMATION),
									     NULL);
	if (NT_SUCCESS(Status)) {
		return BasicInformation.TebBaseAddress;	
	}

	return NULL;
}

PVOID
BspQueryPebAddress(
	IN HANDLE ProcessHandle 
	)
{
	ULONG Length = 0;
	NTSTATUS NtStatus;
	PROCESS_BASIC_INFORMATION Information = {0};

	NtStatus = (*NtQueryInformationProcess)(ProcessHandle,
										    0,
										    &Information,
										    sizeof(PROCESS_BASIC_INFORMATION),
										    &Length);		
	if (NtStatus == STATUS_SUCCESS) {
		return Information.PebBaseAddress;
	}

	return NULL;
}

SIZE_T 
BspConvertUnicodeToUTF8(
	IN PWSTR Source, 
	OUT PCHAR *Destine
	)
{
	int Length;
	int RequiredLength;
	PCHAR Buffer;

	if (!Source) {
		*Destine = NULL;
		return 0;
	}

	Length = (int)wcslen(Source) + 1;
	RequiredLength = WideCharToMultiByte(CP_UTF8, 0, Source, Length, 0, 0, 0, 0);

	Buffer = (PCHAR)BspMalloc(RequiredLength);
	Buffer[0] = 0;

	WideCharToMultiByte(CP_UTF8, 0, Source, Length, Buffer, RequiredLength, 0, 0);
	*Destine = Buffer;

	return Length;
}

VOID 
BspConvertUTF8ToUnicode(
	IN PCHAR Source,
	OUT PWCHAR *Destine
	)
{
	ULONG Length;
	PWCHAR Buffer;

	if (!Source) {
		*Destine = NULL;
		return;
	}

	Length = (ULONG)strlen(Source) + 1;
	Buffer = (PWCHAR)BspMalloc(Length * sizeof(WCHAR));
	Buffer[0] = L'0';

	MultiByteToWideChar(CP_UTF8, 0, Source, Length, Buffer, Length);
	*Destine = Buffer;
}

VOID
BspWriteLogEntry(
	IN BSP_LOG_LEVEL Level,
	IN BSP_LOG_SOURCE Source,
	IN PWSTR Message
	)
{
	PBSP_LOG_ENTRY LogEntry;
	PLIST_ENTRY ListEntry;

	BspAcquireLock(&BspLogLock);

	if (BspLogCount >= BSP_LOG_THRESHOLD) {

		//
		// If the log queue is full, remove the oldest log entry
		//

		ASSERT(IsListEmpty(&BspLogList) != TRUE);
		ListEntry = RemoveHeadList(&BspLogList);
		LogEntry = CONTAINING_RECORD(ListEntry, BSP_LOG_ENTRY, ListEntry);
		BspFree(LogEntry);
		BspLogCount -= 1;

	}

	LogEntry = (PBSP_LOG_ENTRY)BspMalloc(sizeof(BSP_LOG_ENTRY));
	LogEntry->Level = Level;
	LogEntry->Source = Source;
	GetSystemTime(&LogEntry->TimeStamp);
	StringCchCopy(LogEntry->Message, MAX_PATH, Message);
	InsertTailList(&BspLogList, &LogEntry->ListEntry);

	BspLogCount += 1;

	BspReleaseLock(&BspLogLock);
}

VOID
BspCleanLogEntry(
	VOID
	)
{
	PLIST_ENTRY ListEntry;	
	PBSP_LOG_ENTRY LogEntry;

	BspAcquireLock(&BspLogLock);

	while (IsListEmpty(&BspLogList) != TRUE) {
		ListEntry = RemoveHeadList(&BspLogList);
		LogEntry = CONTAINING_RECORD(ListEntry, BSP_LOG_ENTRY, ListEntry);
		BspFree(LogEntry);
	}

	BspReleaseLock(&BspLogLock);
}

VOID
BspGetWorkDirectory(
	IN PWSTR ImagePath,
	OUT PWCHAR Buffer
	)
{
	_wsplitpath(ImagePath, NULL, Buffer, NULL, NULL);
}

ULONG
BspCaptureMemoryDump(
	IN ULONG ProcessId,
	IN BOOLEAN Full,
	IN PWSTR Path,
	IN PEXCEPTION_POINTERS Pointers,
	IN BOOLEAN Local
	)
{
	#define BSP_MINIDUMP_FULL  MiniDumpWithFullMemory        | \
							   MiniDumpWithHandleData        | \
							   MiniDumpWithThreadInfo        | \
							   MiniDumpWithUnloadedModules   | \
							   MiniDumpWithFullAuxiliaryState

	#define BSP_MINIDUMP_MINI (MiniDumpNormal | MiniDumpWithUnloadedModules)

	HANDLE ProcessHandle;
	HANDLE FileHandle;
	MINIDUMP_TYPE Type;
	ULONG Status;
	MINIDUMP_EXCEPTION_INFORMATION Exception = {0};

	if (!Local) {
		ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, ProcessId);
		if (!ProcessHandle) {
			Status = GetLastError();
			return Status;
		}
	} else {
		ProcessHandle = GetCurrentProcess();
	}

	FileHandle = CreateFile(Path,
	                        GENERIC_READ | GENERIC_WRITE,
	                        FILE_SHARE_READ | FILE_SHARE_WRITE,
	                        NULL,
	                        CREATE_ALWAYS,
	                        FILE_ATTRIBUTE_NORMAL,
	                        NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		if (!Local) {
			CloseHandle(ProcessHandle);
		}
		return Status;
	}

	Type = Full ? (BSP_MINIDUMP_FULL) : (BSP_MINIDUMP_MINI);

	if (Local) {

		Exception.ThreadId = GetCurrentThreadId();
		Exception.ExceptionPointers = Pointers; 
		Exception.ClientPointers = FALSE;
		Status = MiniDumpWriteDump(ProcessHandle, ProcessId, FileHandle, Type, &Exception, NULL, NULL);

	} else {
		
		Status = MiniDumpWriteDump(ProcessHandle, ProcessId, FileHandle, Type, NULL, NULL, NULL);
	}


	if (!Status) {
		Status = GetLastError();
	} else {
		Status = ERROR_SUCCESS;
	}

	CloseHandle(FileHandle);
	return Status;
}

ULONG
BspGetFullPathName(
	IN PWSTR BaseName,
	IN PWCHAR Buffer,
	IN USHORT Length,
	OUT PUSHORT ActualLength
	)
{
	HMODULE ModuleHandle;
	PWCHAR Slash;

	ModuleHandle = GetModuleHandle(NULL);
	GetModuleFileName(ModuleHandle, Buffer, Length);
	Slash = wcsrchr(Buffer, L'\\');
	wcscpy(Slash + 1, BaseName);
	*ActualLength = (USHORT)wcslen(Buffer);

	return ERROR_SUCCESS;
}

ULONG
BspGetProcessPath(
	IN PWCHAR Buffer,
	IN USHORT Length,
	OUT PUSHORT ActualLength
	)
{
	HMODULE ModuleHandle;
	PWCHAR Slash;

	ModuleHandle = GetModuleHandle(NULL);
	GetModuleFileName(ModuleHandle, Buffer, Length);

	Slash = wcsrchr(Buffer, L'\\');
	Slash[1] = 0;

	*ActualLength = (USHORT)wcslen(Buffer);
	return ERROR_SUCCESS;
}

BOOLEAN
BspGetModuleInformation(
	IN ULONG ProcessId,
	IN PWSTR DllName,
	IN BOOLEAN FullPath,
	OUT HMODULE *DllHandle,
	OUT PULONG_PTR Address,
	OUT SIZE_T *Size
	)
{ 
	BOOLEAN Found = FALSE;
	MODULEENTRY32 Module; 
	HANDLE Handle = INVALID_HANDLE_VALUE; 

	Handle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessId); 
	if (Handle == INVALID_HANDLE_VALUE) { 
		return FALSE;
	} 

	Module.dwSize = sizeof(MODULEENTRY32); 

	if (!Module32First(Handle, &Module)) { 
		CloseHandle(Handle);
		return FALSE; 
	} 

	do { 

		if (FullPath) {
			Found = !_wcsicmp(DllName, Module.szExePath);
		} else {
			Found = !_wcsicmp(DllName, Module.szModule);
		}

		if (Found) {

			if (DllHandle) {
				*DllHandle = Module.hModule;
			}

			if (Address) {
				*Address = (ULONG_PTR)Module.modBaseAddr;	
			}

			if (Size) {
				*Size = Module.modBaseSize;
			}

			CloseHandle(Handle);
			return TRUE;
		}

	} while (Module32Next(Handle, &Module));

	CloseHandle(Handle); 
	return FALSE; 
} 

PVOID
BspGetRemoteApiAddress(
	IN ULONG ProcessId,
	IN PWSTR DllName,
	IN PSTR ApiName
	)
{ 
	HMODULE DllHandle;
	ULONG_PTR Address;
	HMODULE LocalDllHandle;
	ULONG_PTR LocalAddress;
	SIZE_T Size;
	ULONG Status;
	WCHAR Buffer[MAX_PATH];

	ASSERT(GetCurrentProcessId() != ProcessId);
	
	LocalDllHandle = GetModuleHandle(DllName);
	if (!LocalDllHandle) {
		return NULL;
	}
	
	LocalAddress = (ULONG_PTR)GetProcAddress(LocalDllHandle, ApiName);
	if (!LocalAddress) {
		return NULL;
	}

	StringCchCopy(Buffer, MAX_PATH, DllName);
	wcslwr(Buffer);

	if (!wcsstr(Buffer, L".dll")) {
		StringCchPrintf(Buffer, MAX_PATH, L"%s.dll", DllName);
	}

	Status = BspGetModuleInformation(ProcessId, Buffer, FALSE, &DllHandle, &Address, &Size);
	if (!Status) {
		return NULL;
	}

	return (PVOID)((ULONG_PTR)DllHandle + (LocalAddress - (ULONG_PTR)LocalDllHandle));	
}

VOID
BspUnDecorateSymbolName(
	IN PWSTR DecoratedName,
	IN PWSTR UnDecoratedName,
	IN ULONG UndecoratedLength,
	IN ULONG Flags
	)
{
	CHAR DecoratedBuffer[MAX_PATH];
	CHAR UnDecoratedBuffer[MAX_PATH];

	StringCchPrintfA(DecoratedBuffer, MAX_PATH, "%S", DecoratedName);
	__unDName(UnDecoratedBuffer, DecoratedBuffer, MAX_PATH, malloc, free, UNDNAME_NAME_ONLY);
	StringCchPrintfW(UnDecoratedName, UndecoratedLength, L"%S", UnDecoratedBuffer);
}