;
; Apsara Labs
; lan.john@gmail.com
; Copyright(C) 2009-2012
;

.686
.model flat, c
.code

align 16 

FPU_STATE_MASK equ 3800h

public BspLoadRoutineCode
public BspLoadRoutineEnd
public BspDebuggerThread
public BspDebuggerThreadEnd


BSP_LDR_DATA struct 

	Address					dd ?	
	Executed				dd ?
	LastErrorValue			dd ?
	GetLastErrorPtr			dd ?
	SetLastErrorPtr			dd ?
	CreateThreadPtr			dd ?
	LoadLibraryPtr			dd ?
	CloseHandlePtr			dd ?
	WaitForSingleObjectPtr	dd ?
	FpuStateRequired		dd ?
	FpuState				dq ?  
	DllFullPath				dw 512 dup(?)
	
BSP_LDR_DATA ends

	; Try to acquire lock to ensure exclusive injection
	
BspLoadRoutineCode:

	; embedded ldr data
	LdrData BSP_LDR_DATA <>
	
	; N.B. This assume that the old eip is already push on stack
	; in BspExecuteLoadRoutine.
	
	push eax
	push ebx
	
	; reloate instruction pointer
	; ebx hold LdrData address
	
	call Rip	
Rip:
	pop ebx
	mov eax, offset LdrData - Rip
	add ebx, eax	
	
	mov	 edx, 1
	lea  ecx, dword ptr [ebx + BSP_LDR_DATA.Executed]
	xor	 eax, eax
	lock cmpxchg dword ptr[ecx], edx
	cmp	 eax, 1
	
	jne  ExclusiveInject 
	
	pop  ebx
	pop  eax
	ret
	
ExclusiveInject:

	; save LastErrorValue
	call dword ptr[ebx + BSP_LDR_DATA.GetLastErrorPtr]	
	mov  dword ptr[ebx + BSP_LDR_DATA.LastErrorValue], eax
		
	; save FPU state if required
	fstsw ax
	and ax, FPU_STATE_MASK
	jz NoFpuState 
	fstp qword ptr[ebx + BSP_LDR_DATA.FpuState]
	mov dword ptr[ebx + BSP_LDR_DATA.FpuStateRequired], 1
	
NoFpuState:

	; call CreateThread(LoadLibrary)
	push 0
	push 0
	lea  ecx, dword ptr[ebx + BSP_LDR_DATA.DllFullPath]
	push ecx
	push dword ptr[ebx + BSP_LDR_DATA.LoadLibraryPtr]
	push 0
	push 0
	call dword ptr[ebx + BSP_LDR_DATA.CreateThreadPtr]

	; WaitForSingleObject(ThreadHandle)
	mov  ecx, eax
	push eax
	push -1
	call dword ptr[ebx + BSP_LDR_DATA.WaitForSingleObjectPtr]
	
	; call CloseHandle(ThreadHandle)	
	push ecx
	call dword ptr[ebx + BSP_LDR_DATA.CloseHandlePtr]

	; restore original LastErrorValue 
	push dword ptr[ebx + BSP_LDR_DATA.LastErrorValue]
	call dword ptr[ebx + BSP_LDR_DATA.SetLastErrorPtr]

	; restore FPU state
	cmp dword ptr[ebx + BSP_LDR_DATA.FpuStateRequired], 1
	jne NoFpuStateRestore
	fld qword ptr[ebx + BSP_LDR_DATA.FpuState]	
	
NoFpuStateRestore:

	pop ebx
	pop eax
	ret	
	
	db 6 dup(90h)
	
BspLoadRoutineEnd:

; 
; ULONG CALLBACK
; BspDebuggerThread(
;	IN PVOID Context	
;	);
;

BSP_LDR_DBG	struct 
	DllFullPath          dd ?
	LoadLibraryW         dd ?
	RtlCreateUserThread  dd ?	
	RtlExitUserThread    dd ?	
	ThreadId             dd ?
	NtStatus             dd ?
	Path                 dw 260 dup(?)
	Code                 db 200 dup(90h)
BSP_LDR_DBG ends

BspDebuggerThread proc

	push ebp
	mov  ebp, esp
	
	mov  ebx, eax
	
	; RtlCreateUserThread(LoadLibrary)
	lea  eax, dword ptr[ebx + BSP_LDR_DBG.ThreadId]
	push eax
	push 0
	push dword ptr[ebx + BSP_LDR_DBG.DllFullPath] 
	push dword ptr[ebx + BSP_LDR_DBG.LoadLibraryW]
	push 0
	push 0
	push 0
	push 0
	push 0
	push -1 
	call dword ptr[ebx + BSP_LDR_DBG.RtlCreateUserThread]
	
	; save NTSTATUS	
	mov  dword ptr[ebx + BSP_LDR_DBG.NtStatus], eax
	
	; RtlExitUserThread
	push 0
	call dword ptr[ebx+ BSP_LDR_DBG.RtlExitUserThread]
	; Clear DbgContext as DbgUiRemoteBreakIn
	; mov eax, dword ptr[ebx + BSP_LDR_DBG.DbgContext]
	; mov dword ptr[esp + 8], eax
	
	; Execute ntdll!DbgUiRemoteBreakIn
	;mov eax, dword ptr[ebx + BSP_LDR_DBG.DbgUiRemoteBreakIn]
	;pop ebx
	;jmp eax
	int 3

BspDebuggerThread endp

BspDebuggerThreadEnd proc
	ret
BspDebuggerThreadEnd endp
	

BSP_LOADER struct 
	LoadLibrary        dd ?
	RtlExitUserThread  dd ?
	Path               dw 260 dup(?)
	LoaderThread       db 100 dup(90h)
BSP_LOADER ends

BspLoaderThread proc

	push ebp
	mov  ebp, esp
	
	mov  ebx, dword ptr[ebp+8] 

	; LoadLibrary(Path)	
	lea  ecx, dword ptr[ebx + BSP_LOADER.Path]
	push ecx	
	call dword ptr[ebx + BSP_LOADER.LoadLibrary]
	
	; RtlExitUserThread
	push 0
	call dword ptr[ebx + BSP_LOADER.RtlExitUserThread]
	int 3
	ret 4

BspLoaderThread endp

; UserApc data
BSP_APC_DATA struct 
	DelayInterval     dq ?
	NtDelayExecution  dd ?
	Path              dw 512 dup(?)
	Code              db 512 dup(?)
BSP_APC_DATA ends

BspAlertUserThread PROC

	push eax
	push edx
	
	mov  edx, dword ptr[esp + 8]	
	push edx
	push 1
	call dword ptr[edx + 4]
	
	pop  edx
	pop  eax
	
	; clean pushed BSP_APC_DATA pointer on stack,
	; now the stack is orignal return address
	add  esp, 4
	ret
	
BspAlertUserThread ENDP 

BSP_PREEXECUTE_CONTEXT struct 
	XFlags	dd ?
	Xdi		dd ?
	Xsi		dd ?
	Xbp		dd ?
	Xsp		dd ?
	Xbx		dd ?
	Xdx		dd ?
	Xcx		dd ?
	Xax		dd ?
	Xip		dd ?
	CompleteEvent			dd ?
	SuccessEvent            dd ?
	ErrorEvent              dd ?
	LoadLibraryAddr         dd ?
	SetEventAddr            dd ?
	WaitForSingleObjectAddr dd ?
	CloseHandleAddr         dd ?
	Path                    dw 260 dup(0)
BSP_PREEXECUTE_CONTEXT ends

;
; BspPreExecuteRoutine(
;   VOID
;   );
; 
; ebx, pointer to BSP_PREEXECUTE_CONTEXT structure
;

BspInjectPreExecuteRoutine proc

	;
	; LoadLibrary(Runtime)	
	;	

	lea  ecx, dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Path] 
	push ecx
	call dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.LoadLibraryAddr]
	
	cmp eax, 0
	jz  Failed
	
	push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.SuccessEvent]
	call dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.SetEventAddr]	
	
Complete:

	;
	; WaitForSingleObject(CompleteEvent, INFINITE)
	;
	
	push -1
	push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.CompleteEvent]
	call dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.WaitForSingleObjectAddr]
	
	push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.SuccessEvent]
	call dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.CloseHandleAddr]
	
	push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.ErrorEvent]
	call dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.CloseHandleAddr]
	
	push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.CompleteEvent]
	call dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.CloseHandleAddr]
	
	;
	; Restore GPR registers
	;
	
	mov eax, dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xax]
	mov ecx, dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xcx]
	mov edx, dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xdx]
	mov esi, dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xsi]
	mov edi, dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xdi]
	mov ebp, dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xbp]
	mov esp, dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xsp]
	
	push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.XFlags]
	popfd
	
	;
	; jump to orignal entry point
	;
	
	push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xip]
	push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.Xbx]
	pop  ebx
	ret
	
Failed:
	push dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.ErrorEvent]
	call dword ptr[ebx + BSP_PREEXECUTE_CONTEXT.SetEventAddr]	
	jmp  Complete
	
BspInjectPreExecuteRoutine endp 


END