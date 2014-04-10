//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009 - 2011
//

#include "btr.h"
#include "hal.h"
#include "thread.h"

typedef struct _BTR_UNLOAD_ARGUMENT {
	PVOID WaitForSingleObjectPtr;
	PVOID CloseHandlePtr;
	PVOID FreeLibraryPtr;
	PVOID HeapDestroyPtr;
	HANDLE ThreadHandle;
	HANDLE DllHandle;
	HANDLE HeapHandle;
} BTR_UNLOAD_ARGUMENT, *PBTR_UNLOAD_ARGUMENT;

#if defined (_M_IX86)

CHAR BtrUnloadRoutine[] = {
	
	0x53,				     // push ebx
	0x8b, 0x5c, 0x24, 0x08,	 // mov  ebx, dword ptr[esp + 8]
	0x6a, 0xff,              // push -1	  
	0xff, 0x73, 0x10,		 // push dword ptr[ebx + ThreadHandle]	
	0x8b, 0x0b,				 // mov  ecx, dword ptr[ebx + WaitForSingleObjectPtr]
	0xff, 0xd1,				 // call ecx
					

	0xff, 0x73, 0x10,		 //	push dword ptr[ebx + ThreadHandle]	
	0x8b, 0x4b, 0x04,		 // mov ecx, dword ptr[ebx + CloseHandlePtr]
	0xff, 0xd1,				 // call ecx

							 // RepeatUntilZero:	
	0xff, 0x73, 0x14,		 // push dword ptr[ebx + DllHandle]
	0x8b, 0x4b, 0x08,		 // mov  ecx, dword ptr[ebx + FreeLibraryPtr]
	0xff, 0xd1,				 // call ecx
					
	0x85, 0xc0,				 // test eax, eax
	0x75, 0xf4,				 // jne RepeatUntilZero
					  
	0x8b, 0x43, 0x18,		 // mov eax, dword ptr[ebx + HeapHandle]
	0x8b, 0x53, 0x0c,		 // mov edx, dword ptr[ebx + HeapDestroyPtr]
					
	0x5b,					 // pop ebx		
					
	0x89, 0x44, 0x24, 0x04,  // mov dword ptr[esp + 4], eax
	0xff, 0xe2,				 // jmp edx	
					
	0xcc,                    //	int 3
	0xc2, 0x04, 0x00         // ret 4
};

#elif defined (_M_X64)

CHAR BtrUnloadRoutine[] = {

	0x53,							//	push rbx
	0x48, 0x83, 0xec, 0x20,			//	sub	rsp,	20h
	0x48, 0x8b, 0xd9,				//	mov  rbx,	rcx	; Load Argument into rbx
	0xba, 0xff, 0xff, 0xff, 0xff,	//	mov	edx,	-1	; INFINITE (dword)
	0x48, 0x8b, 0x4b, 0x20,			//	mov  rcx,	qword ptr[rbx + ThreadHandle]	
	0x4c, 0x8b, 0x03,				//	mov  r8,	qword ptr[rbx + WaitForSingleObjectPtr]
	0x41, 0xff, 0xd0,				//	call r8
					
	0x48, 0x8b, 0x4b, 0x20,			//	mov  rcx,	qword ptr[rbx + ThreadHandle]	
	0x4c, 0x8b, 0x43, 0x08,			//	mov  r8,	qword ptr[rbx + CloseHandlePtr]
	0x41, 0xff, 0xd0,				//	call r8

									//  RepeatUntilZero:
	0x48, 0x8b, 0x4b, 0x28,			//	mov  rcx,	qword ptr[rbx + DllHandle]
	0x4c, 0x8b, 0x43, 0x10,			//	mov  r8,	qword ptr[rbx + FreeLibraryPtr]
	0x41, 0xff, 0xd0,				//	call r8
					
	0x85, 0xc0,						//	test eax, eax  ; BOOL (dword)
	0x75, 0xf1,						//	jne RepeatUntilZero
					  
	0x48, 0x8b, 0x4b, 0x30,			//	mov rcx,	qword ptr[rbx + HeapHandle]
	0x4c, 0x8b, 0x43, 0x18,			//	mov r8,		qword ptr[rbx + HeapDestroyPtr]
					
	0x48, 0x83, 0xc4, 0x20,			//	add rsp,	20h
	0x5b,							//	pop rbx		
					
	0x41, 0xff, 0xe0,				//	jmp r8	
					
	0xcc,							//	int 3
	0xc2, 0x08, 0x00				//	ret 8
};

#endif

VOID
BtrExecuteUnloadRoutine(
	VOID
	)
{
	HANDLE HeapHandle;
	HANDLE DllHandle;
	HANDLE ThreadHandle;
	PBTR_UNLOAD_ARGUMENT Argument;
	LPTHREAD_START_ROUTINE UnloadRoutine;
	
	HeapHandle = HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 0, 0);
	Argument = (PBTR_UNLOAD_ARGUMENT)HeapAlloc(HeapHandle, 0, sizeof(BTR_UNLOAD_ARGUMENT));

	//
	// N.B. schedule thread is the last thread to exit, it ensure that all allocated 
	// resource include thread, handle, memory are freed and exit.
	//

	Argument->ThreadHandle = BtrSystemThread[ThreadSchedule];
	Argument->DllHandle = (HANDLE)BtrDllBase;
	Argument->HeapHandle = HeapHandle;

	DllHandle = GetModuleHandle(L"kernel32.dll");
	Argument->WaitForSingleObjectPtr = (PVOID)GetProcAddress(DllHandle, "WaitForSingleObject");
	Argument->CloseHandlePtr = (PVOID)GetProcAddress(DllHandle, "CloseHandle");
	Argument->FreeLibraryPtr = (PVOID)GetProcAddress(DllHandle, "FreeLibrary");
	Argument->HeapDestroyPtr = (PVOID)GetProcAddress(DllHandle, "HeapDestroy");

	//
	// Convert the allocated heap block as thread routine
	//

	UnloadRoutine = (LPTHREAD_START_ROUTINE)HeapAlloc(HeapHandle, 0, sizeof(BtrUnloadRoutine));
	RtlCopyMemory(UnloadRoutine, BtrUnloadRoutine, sizeof(BtrUnloadRoutine));

	ThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UnloadRoutine, Argument, 0, NULL);
	CloseHandle(ThreadHandle);
}