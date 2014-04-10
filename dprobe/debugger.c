//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#include "bsp.h"
#include "debugger.h"

ULONG
PspHijackThread(
	IN HANDLE ProcessHandle,
	IN HANDLE ThreadHandle,
	IN PVOID BaseVa,
	IN PVOID LoadLibraryVa,
	IN PWSTR DllFullPath
	)
{
	CONTEXT Context;
	PVOID ReturnAddress;
	ULONG CompleteBytes;
	ULONG Status;
	PPSP_HIJACK_BLOCK Hijack;

	Context.ContextFlags = CONTEXT_ALL;
	Status = GetThreadContext(ThreadHandle, &Context);
	if (Status != TRUE) {
		return GetLastError();
	}

#if defined (_M_IX86)
	ReadProcessMemory(ProcessHandle, Context.Esp, &ReturnAddress, sizeof(PVOID), &CompleteBytes);
#endif

	//Hijack = PspCreateHijackBlock(BaseVa, ReturnAddress, DllFullPath, LoadLibraryVa);

	//
	// Write hijack block into address space of target process
	//

	Status = WriteProcessMemory(ProcessHandle, 
		                        BaseVa, 
								Hijack, 
								sizeof(PSP_HIJACK_BLOCK), 
								&CompleteBytes); 

	BspFree(Hijack);

	if (Status != TRUE) {
		return GetLastError();
	}
	
	//
	// Hijack the original return address of remote thread
	//

#if defined (_M_IX86)

	Status = WriteProcessMemory(ProcessHandle,
							    (PVOID)Context.Esp,
								&BaseVa,
								sizeof(PVOID),
								&CompleteBytes);
#elif defined (_M_X64) 

	Status = WriteProcessMemory(ProcessHandle,
							    (PVOID)Context.Rsp,
								&BaseVa,
								sizeof(PVOID),
								&CompleteBytes);
#endif

	if (Status != TRUE) {
		return GetLastError();
	}

	return ERROR_SUCCESS;
}

PSP_DEBUG_CONTEXT PspDebugContext;

ULONG
PspDebugOnCreateProcess(
	IN PDEBUG_EVENT Event
	)
{
	PspDebugContext.ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Event->dwProcessId);
	PspDebugInsertThread(Event);

	DebugTrace("CREATE_PROCESS_DEBUG_EVENT, PID = %x", Event->dwProcessId);
	return DBG_CONTINUE;
}

ULONG
PspDebugOnExitProcess(
	IN PDEBUG_EVENT Event
	)
{
	DebugTrace("EXIT_PROCESS_DEBUG_EVENT");
	return DBG_CONTINUE;
}

PPSP_DEBUG_THREAD
PspDebugInsertThread(
	IN PDEBUG_EVENT Event
	)
{
	PPSP_DEBUG_THREAD Thread;
	CONTEXT Context;

	Thread = (PPSP_DEBUG_THREAD)BspMalloc(sizeof(PSP_DEBUG_THREAD));
	Thread->ThreadId = Event->dwThreadId;
	Thread->ThreadHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, Event->dwThreadId);

	Context.ContextFlags = CONTEXT_FULL;
	GetThreadContext(Thread->ThreadHandle, &Thread->Context);

	if (Event->u.CreateThread.lpStartAddress != PspDebugContext.DbgUiRemoteBreakin) {
		Thread->BreakInThread = FALSE;
	} else {
		Thread->BreakInThread = TRUE;
	}

	InsertHeadList(&PspDebugContext.ThreadList, &Thread->ListEntry);
	return Thread;
}

PPSP_DEBUG_THREAD
PspDebugLookupThread(
	IN ULONG ThreadId 
	)
{
	PPSP_DEBUG_THREAD Thread;
	PLIST_ENTRY ListEntry;

	ListEntry = PspDebugContext.ThreadList.Flink;
	while (ListEntry != &PspDebugContext.ThreadList) {
		Thread = CONTAINING_RECORD(ListEntry, PSP_DEBUG_THREAD, ListEntry);
		if (Thread->ThreadId == ThreadId) {
			return Thread;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

ULONG
PspDebugOnCreateThread(
	IN PDEBUG_EVENT Event
	)
{
	DebugTrace("CREATE_THREAD_DEBUG_EVENT ClientId = %x.%x", 
		       Event->dwProcessId, Event->dwThreadId);

	PspDebugInsertThread(Event);
	return DBG_CONTINUE;
}

ULONG
PspDebugOnExitThread(
	IN PDEBUG_EVENT Event
	)
{
	PPSP_DEBUG_THREAD Thread;

	DebugTrace("EXIT_THREAD_DEBUG_EVENT ClientId = %x.%x", 
		       Event->dwProcessId, Event->dwThreadId);

	Thread = PspDebugLookupThread(Event->dwThreadId);
	if (Thread) {
		RemoveEntryList(&Thread->ListEntry);
		CloseHandle(Thread->ThreadHandle);
		BspFree(Thread);
	}

	return DBG_CONTINUE;
}

ULONG
PspDebugOnLoadDll(
	IN PDEBUG_EVENT Event
	)
{
	DebugTrace("LOAD_DLL_DEBUG_EVENT");
	return DBG_CONTINUE;
}

ULONG
PspDebugOnUnloadDll(
	IN PDEBUG_EVENT Event
	)
{
	DebugTrace("UNLOAD_DLL_DEBUG_EVENT");
	return DBG_CONTINUE;
}

ULONG
PspDebugInsertBreak(
	IN PPSP_DEBUG_CONTEXT Pdc
	)
{
	ULONG OldProtect;
	ULONG Status = S_OK;
	CHAR Trap = 0xcc;
	
	
	VirtualProtectEx(Pdc->ProcessHandle, Pdc->SystemCallReturn, 4096, 
			         PAGE_EXECUTE_READWRITE, &OldProtect);

	ReadProcessMemory(Pdc->ProcessHandle, Pdc->SystemCallReturn, &Pdc->CodeByte, sizeof(CHAR), NULL);
	WriteProcessMemory(Pdc->ProcessHandle, Pdc->SystemCallReturn, &Trap, sizeof(CHAR), NULL);
	
	VirtualProtectEx(Pdc->ProcessHandle, Pdc->SystemCallReturn, 4096, 
			         OldProtect, &OldProtect);
	return Status;
}

ULONG
PspDebugDeleteBreak(
	IN PPSP_DEBUG_CONTEXT Pdc
	)
{
	ULONG OldProtect;
	ULONG Status = S_OK;
	CHAR Trap = 0xcc;
	
	VirtualProtectEx(Pdc->ProcessHandle, Pdc->SystemCallReturn, sizeof(ULONG_PTR), 
			         PAGE_EXECUTE_READWRITE, &OldProtect);

	WriteProcessMemory(Pdc->ProcessHandle, Pdc->SystemCallReturn, &Pdc->CodeByte, sizeof(CHAR), NULL);
	
	VirtualProtectEx(Pdc->ProcessHandle, Pdc->SystemCallReturn, sizeof(ULONG_PTR), 
			         OldProtect, NULL);
	return Status;
}

#if defined (_M_IX86)

ULONG
PspDebugLoadLibrary(
	VOID
	)
{
	CONTEXT CallContext;
	ULONG Arguments[2];
	ULONG ReturnBreak;

	PspDebugContext.DllFullPath = VirtualAllocEx(PspDebugContext.ProcessHandle, 
												 NULL, 4096, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	WriteProcessMemory(PspDebugContext.ProcessHandle, PspDebugContext.DllFullPath, 
					   PspRuntimeFullPath, wcslen(PspRuntimeFullPath) * 2, NULL);

	//
	// Copy call context from debug break context
	//

	RtlCopyMemory(&CallContext, &PspDebugContext.Context, sizeof(CONTEXT));

	CallContext.Eip = PspDebugContext.LoadLibraryPtr;
	CallContext.Esp = CallContext.Esp - sizeof(ULONG) * 2;

	Arguments[0] = (ULONG_PTR)PspDebugContext.DllFullPath + 1024; 
	Arguments[1] = PspDebugContext.DllFullPath;

	ReturnBreak = 0xCCCCCCCC;
	WriteProcessMemory(PspDebugContext.ProcessHandle, Arguments[0], &ReturnBreak, sizeof(ULONG), NULL);
	WriteProcessMemory(PspDebugContext.ProcessHandle, CallContext.Esp, &Arguments[0], sizeof(ULONG) * 2, NULL);

	SetThreadContext(PspDebugContext.ThreadHandle, &CallContext);
	PspDebugContext.ReturnAddress = Arguments[0];

	return S_OK;
}

#endif

#if defined (_M_X64)

ULONG
PspDebugLoadLibrary(
	VOID
	)
{
	return S_OK;
}

#endif

ULONG
PspDebugOnException(
	IN PDEBUG_EVENT Event
	)
{
	HANDLE ThreadHandle;
	CONTEXT Context;
	EXCEPTION_RECORD *Record;
	PLIST_ENTRY ListEntry;
	PPSP_DEBUG_THREAD Thread;
	PPSP_DEBUG_THREAD HijackedThread;

	Context.ContextFlags = CONTEXT_FULL;

	//
	// Don't care about non break point exception 	
	//

	Record = &Event->u.Exception;
	if (Record->ExceptionCode != EXCEPTION_BREAKPOINT) {
		return DBG_EXCEPTION_NOT_HANDLED;
	}

	//
	// Insert debugbreak 
	//

	if (PspDebugContext.State == PSP_DEBUG_STATE_NULL) {
		PspDebugInsertBreak(&PspDebugContext);
		PspDebugContext.State = PSP_DEBUG_INIT_BREAK;
		return DBG_CONTINUE;
	}

	//
	// Expect KiFastSystemCallRet is hit
	//

	if (PspDebugContext.SystemCallReturn == (ULONG_PTR)Record->ExceptionAddress) {

		//
		// Hijack current thread, suspend all other threads and reset their context
		//

		Context.ContextFlags = CONTEXT_FULL;
		HijackedThread = PspDebugLookupThread(Event->dwThreadId);	

		if (HijackedThread->BreakInThread) {
			
			//
			// DbgUiRemoteBreakIn can not be used to call LoadLibrary safely, redirect to
			// DbgBreakPoint to execute the ret instruction
			//

			GetThreadContext(HijackedThread->ThreadHandle, &Context);
#if defined (_M_IX86)
			Context.Eip = PspDebugContext.DbgBreakPointRet;	
#elif defined (_M_X64)
			Context.Rip = PspDebugContext.DbgBreakPointRet;	
#endif
			SetThreadContext(HijackedThread->ThreadHandle, &Context);
			return DBG_CONTINUE;

		}

		//
		// Delete the hijacked thread from thread list and save its context data
		//

		RemoveEntryList(&HijackedThread->ListEntry);
		PspDebugContext.ThreadHandle = HijackedThread->ThreadHandle;
		PspDebugContext.ThreadId = HijackedThread->ThreadId;

		PspDebugContext.Context.ContextFlags = CONTEXT_FULL;
		GetThreadContext(HijackedThread->ThreadHandle, &PspDebugContext.Context);

#if defined (_M_IX86)
		PspDebugContext.Context.Eip -= 1;
#elif defined (_M_X64)
		PspDebugContext.Context.Rip -= 1;
#endif

		BspFree(HijackedThread);

		while (IsListEmpty(&PspDebugContext.ThreadList) != TRUE) {
				
			ListEntry = RemoveHeadList(&PspDebugContext.ThreadList);
			Thread = CONTAINING_RECORD(ListEntry, PSP_DEBUG_THREAD, ListEntry);

			GetThreadContext(Thread->ThreadHandle, &Context);

#if defined (_M_IX86)
			if (Context.Eip == PspDebugContext.SystemCallReturn + 1) {
				Context.Eip -= 1;
#elif defined (_M_X64)
			if (Context.Rip == PspDebugContext.SystemCallReturn + 1) {
				Context.Rip -= 1;
#endif
				SetThreadContext(Thread->ThreadHandle, &Context);
				DebugTrace("TID %x hit breakpoint", Thread->ThreadId);
			}
		
			CloseHandle(Thread->ThreadHandle);
			BspFree(Thread);
		}

		//
		// Remove breakpoint of KiFastSystemCallRet
		//

		PspDebugDeleteBreak(&PspDebugContext);

		//
		// Execute LoadLibraryW on the hijacked thread
		//

		PspDebugLoadLibrary();
		PspDebugContext.State = PSP_DEBUG_INTERCEPT_BREAK;

		return DBG_CONTINUE;
	}

	if (PspDebugContext.ReturnAddress == (ULONG_PTR)Record->ExceptionAddress) {

		ASSERT(PspDebugContext.State == PSP_DEBUG_INTERCEPT_BREAK);

		GetThreadContext(PspDebugContext.ThreadHandle, &Context);

#if defined (_M_IX86)
		if (Context.Eax != 0) {
#elif defined (_M_X64)
		if (Context.Rax != 0) {
#endif
			PspDebugContext.Success = TRUE;
		} else {
			PspDebugContext.Success = FALSE;
		}

		SetThreadContext(PspDebugContext.ThreadHandle, &PspDebugContext.Context);
		return DBG_COMPLETE;
	}

	DebugTrace("DBG_EXCEPTION_NOT_HANDLED, TID=%d", Event->dwThreadId);
	return DBG_EXCEPTION_NOT_HANDLED;
}

ULONG
PspDebugOnDebugPrint(
	IN PDEBUG_EVENT Event
	)
{
	DebugTrace("OUTPUT_DEBUG_STRING_EVENT");
	return DBG_CONTINUE;
}

VOID
PspDebugInitialize(
	IN ULONG ProcessId	
	)
{
	RtlZeroMemory(&PspDebugContext, sizeof(PspDebugContext));

	PspDebugContext.State = PSP_DEBUG_STATE_NULL;
	PspDebugContext.ProcessId = ProcessId;
	InitializeListHead(&PspDebugContext.ThreadList);
	
	PspDebugContext.LoadLibraryPtr = BspGetRemoteApiAddress(ProcessId, 
		                                                    L"kernel32.dll", 
															"LoadLibraryW");

	PspDebugContext.SystemCallReturn = BspGetRemoteApiAddress(ProcessId, 
		                                                      L"ntdll.dll", 
															  "KiFastSystemCallRet");

	PspDebugContext.DbgUiRemoteBreakin = BspGetRemoteApiAddress(ProcessId, 
		                                                      L"ntdll.dll", 
															  "DbgUiRemoteBreakin");

	PspDebugContext.DbgBreakPointRet = BspGetRemoteApiAddress(ProcessId, 
		                                                      L"ntdll.dll", 
															  "DbgBreakPoint");
	PspDebugContext.DbgBreakPointRet += 1;
}

ULONG CALLBACK
PspDebugProcedure(
	IN PVOID Context 
	)
{
	ULONG Status;
	ULONG ProcessId;
	DEBUG_EVENT DebugEvent;

	ProcessId = (PVOID)Context;
	Status = DebugActiveProcess(ProcessId);
	if (Status != TRUE) {
		return GetLastError();
	}

	Status = DebugSetProcessKillOnExit(FALSE);
	if (Status != TRUE) {
		Status = GetLastError();
		DebugActiveProcessStop(ProcessId);	
		return Status;
	}

	PspDebugInitialize(ProcessId);

	while (TRUE) {

		Status = WaitForDebugEvent(&DebugEvent, INFINITE);
		if (Status != TRUE) {
			break;
		}

		switch (DebugEvent.dwDebugEventCode) {

			case CREATE_PROCESS_DEBUG_EVENT:
				Status = PspDebugOnCreateProcess(&DebugEvent);	
				break;

			case EXIT_PROCESS_DEBUG_EVENT:
				Status = PspDebugOnExitProcess(&DebugEvent);	
				break;

			case CREATE_THREAD_DEBUG_EVENT:
				Status = PspDebugOnCreateThread(&DebugEvent);	
				break;

			case EXIT_THREAD_DEBUG_EVENT: 
				Status = PspDebugOnExitThread(&DebugEvent);	
				break;

			case LOAD_DLL_DEBUG_EVENT:
				Status = PspDebugOnLoadDll(&DebugEvent);	
				break;

			case UNLOAD_DLL_DEBUG_EVENT:
				Status = PspDebugOnUnloadDll(&DebugEvent);	
				break;

			case OUTPUT_DEBUG_STRING_EVENT:
				Status = PspDebugOnDebugPrint(&DebugEvent);	
				break;
			
			case EXCEPTION_DEBUG_EVENT: 
				Status = PspDebugOnException(&DebugEvent);	
				break;
		}

		if (Status == DBG_COMPLETE) {
			ContinueDebugEvent(DebugEvent.dwProcessId,DebugEvent.dwThreadId, DBG_CONTINUE);
			break;
		} else {
			ContinueDebugEvent(DebugEvent.dwProcessId,DebugEvent.dwThreadId, Status);
		}
	}

	DebugActiveProcessStop(PspDebugContext.ProcessId);
	CloseHandle(PspDebugContext.ProcessHandle);
	CloseHandle(PspDebugContext.ThreadHandle);

	return PspDebugContext.Success ? S_OK : S_FALSE;
}

//
// ntdll!KiFastSystemCall:
// 77d75e70 8bd4 mov edx,esp
// 77d75e72 0f34 sysenter
// 
// ntdll!KiFastSystemCallRet:
// 77d75e74 c3 ret
//

PVOID
PspGetSystemCallReturnAddress(
	VOID
	)
{
	HMODULE Module;
	PVOID ReturnAddress;

	Module = LoadLibrary(L"ntdll.dll");

	if (Module != NULL) {
		ReturnAddress = GetProcAddress(Module, "KiFastSystemCallRet");
		ASSERT(ReturnAddress != NULL);
		return ReturnAddress;
	}

	return NULL;
}

ULONG
PspLoadLibrary(
	IN ULONG ProcessId,
	IN PWSTR DllFullPath
	)
{
	HANDLE ProcessHandle;
	HANDLE ThreadHandle;
	ULONG ThreadId;
	ULONG Status;
	PPSP_HIJACK_BLOCK Block;
	PVOID LoadLibraryVa;
	BOOLEAN Hijacked;
	CONTEXT Context;
	ULONG CurrentEip;
	ULONG CompleteBytes;
	ULONG ThreadNumber;
	ULONG HijackNumber;
	LIST_ENTRY ThreadListHead;
	LIST_ENTRY HijackListHead;
	PLIST_ENTRY ListEntry;
	PPSP_HIJACK_THREAD Thread;

	ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
	if (!ProcessHandle) {
		Status = GetLastError();
		return Status;
	}

	LoadLibraryVa = GetProcAddress(LoadLibrary(L"kernel32.dll"), "LoadLibraryW");
	if (!LoadLibraryVa) {
		Status = GetLastError();
		CloseHandle(ProcessHandle);
		return Status;
	}

StartHijack:

	(*NtSuspendProcess)(ProcessHandle);

	InitializeListHead(&ThreadListHead);
	InitializeListHead(&HijackListHead);
	Context.ContextFlags = CONTEXT_ALL;

	HijackNumber = 0;
	ThreadNumber = PspScanHijackableThreads(ProcessId, &ThreadListHead);
	ASSERT(ThreadNumber != 0);
	
	while (IsListEmpty(&ThreadListHead) != TRUE) {

		ListEntry = RemoveHeadList(&ThreadListHead);
		Thread = CONTAINING_RECORD(ListEntry, PSP_HIJACK_THREAD, ListEntry);

		GetThreadContext(Thread->ThreadHandle, &Context);

#if defined (_M_IX86)

		ReadProcessMemory(ProcessHandle, Context.Eip, 
			              &CurrentEip, sizeof(PVOID), &CompleteBytes);

#elif defined (_M_X64)

		ReadProcessMemory(ProcessHandle, Context.Rip, 
			              &CurrentEip, sizeof(PVOID), &CompleteBytes);

#endif

		if ((UCHAR)CurrentEip == (UCHAR)0xc3) {
			InsertTailList(&HijackListHead, &Thread->ListEntry);	
			HijackNumber += 1;
		} else {
			CloseHandle(Thread->ThreadHandle);
			BspFree(Thread);
		}
	}

	if (IsListEmpty(&HijackListHead)) {
		(*NtResumeProcess)(ProcessHandle);
		SwitchToThread();
		goto StartHijack;
	}

	Block = (PPSP_HIJACK_BLOCK)VirtualAllocEx(ProcessHandle, 
				                              NULL, 
											  sizeof(PSP_HIJACK_BLOCK) * HijackNumber, 
											  MEM_COMMIT, 
											  PAGE_EXECUTE_READWRITE);

	if (!Block) {
		Status = GetLastError();
		PspCleanHijackThreads(&HijackListHead);
		CloseHandle(ProcessHandle);
		return Status;
	}

	while (IsListEmpty(&HijackListHead) != TRUE) {

		ListEntry = RemoveHeadList(&HijackListHead);
		Thread = CONTAINING_RECORD(ListEntry, PSP_HIJACK_THREAD, ListEntry);

		Status = PspHijackThread(ProcessHandle, 
					             Thread->ThreadHandle, 
								 Block, 
								 LoadLibraryVa, 
								 DllFullPath);

		CloseHandle(Thread->ThreadHandle);
		BspFree(Thread);
		Block += 1;
	}

	(*NtResumeProcess)(ProcessHandle);
	CloseHandle(ProcessHandle);
	return Status;
}

VOID
PspCleanHijackThreads(
	__in PLIST_ENTRY HijackListHead
	)
{
	PLIST_ENTRY ListEntry;
	PPSP_HIJACK_THREAD Thread;

	while (IsListEmpty(HijackListHead) != TRUE) {
		ListEntry = RemoveHeadList(HijackListHead);
		Thread = CONTAINING_RECORD(ListEntry, PSP_HIJACK_THREAD, ListEntry);
		CloseHandle(Thread->ThreadHandle);
		BspFree(Thread);
	}
}

ULONG
PspQueryMostBusyThread(
	IN ULONG ProcessId,
	OUT PULONG ThreadId
	)
{
	ULONG Status;
	HANDLE Toolhelp;
	HANDLE ThreadHandle;
	THREADENTRY32 Entry;
	FILETIME CreationTime;
	FILETIME ExitTime;
	FILETIME KernelTime;
	FILETIME UserTime;
	FILETIME MaxUserTime;
	ULONG BusyThreadId;

    Entry.dwSize = sizeof(THREADENTRY32);

    Toolhelp = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, ProcessId);
    if (Toolhelp== INVALID_HANDLE_VALUE){
        Status = GetLastError();
		return Status;
    }

    if (!Thread32First(Toolhelp, &Entry)){
		Status = GetLastError();
        CloseHandle(Toolhelp);
        return Status;
    }

	BusyThreadId = 0;
	MaxUserTime.dwLowDateTime = 0;
	MaxUserTime.dwHighDateTime = 0;

	do {
		if (Entry.th32OwnerProcessID == ProcessId) { 
			
			ThreadHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, Entry.th32ThreadID);
			GetThreadTimes(ThreadHandle, &CreationTime, &ExitTime, &KernelTime, &UserTime);

			if (CompareFileTime(&UserTime, &MaxUserTime) >= 0) {
				MaxUserTime.dwLowDateTime = UserTime.dwLowDateTime;
				MaxUserTime.dwHighDateTime = UserTime.dwHighDateTime;
				BusyThreadId = Entry.th32ThreadID;
			}
			
			CloseHandle(ThreadHandle);
		} 

    } while (Thread32Next(Toolhelp, &Entry));

    CloseHandle(Toolhelp);
	*ThreadId = BusyThreadId;

    return ERROR_SUCCESS;
}
	
ULONG
PspScanHijackableThreads(
	IN ULONG ProcessId,
	OUT PLIST_ENTRY ThreadListHead
	)
{
	ULONG Status;
	HANDLE Toolhelp;
	HANDLE ThreadHandle;
	THREADENTRY32 Entry;
	ULONG ThreadNumber;
	PPSP_HIJACK_THREAD Thread;
	ULONG AccessMask;

	ThreadNumber = 0;
    Entry.dwSize = sizeof(THREADENTRY32);

    Toolhelp = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, ProcessId);
    if (Toolhelp== INVALID_HANDLE_VALUE){
        return ThreadNumber;
    }

    if (!Thread32First(Toolhelp, &Entry)){
        CloseHandle(Toolhelp);
        return ThreadNumber;
    }

	AccessMask = THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_QUERY_INFORMATION;

	do {
		if (Entry.th32OwnerProcessID == ProcessId) { 

			ThreadHandle = OpenThread(AccessMask, FALSE, Entry.th32ThreadID);
			if (ThreadHandle) {

				CONTEXT Context;

				Context.ContextFlags = CONTEXT_ALL;
				if (!GetThreadContext(ThreadHandle, &Context)) {
					CloseHandle(ThreadHandle);
					continue;
				}

				Thread = (PPSP_HIJACK_THREAD)BspMalloc(sizeof(PSP_HIJACK_THREAD));
				Thread->ThreadHandle = ThreadHandle;
				Thread->ThreadId = Entry.th32ThreadID;
				RtlCopyMemory(&Thread->Context, &Context, sizeof(Context));

				InsertTailList(ThreadListHead, &Thread->ListEntry);
				ThreadNumber += 1;
			}
		} 

    } while (Thread32Next(Toolhelp, &Entry));

    CloseHandle(Toolhelp);
    return ThreadNumber;
}