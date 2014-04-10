;
; Apsara Labs
; lan.john@gmail.com
; Copyright(C) 2009-2012
;

.code

align 16

public BspLoadRoutineCode
public BspLoadRoutineEnd

BSP_LDR_DATA struct 

	Address					dq ?
	Executed				dd ? 
	LastErrorValue			dd ?
	GetLastErrorPtr			dq ?
	SetLastErrorPtr			dq ?
	CreateThreadPtr			dq ?
	LoadLibraryPtr			dq ?
	CloseHandlePtr			dq ?
	WaitForSingleObjectPtr  dq ?
	DllFullPath      dw 512 dup(?)

BSP_LDR_DATA ends

BspLoadRoutineCode:

	; embedded ldr data
	LdrData BSP_LDR_DATA <>
	
	; test execution lock to ensure exclusive injection
	; note that due to x64 call convention, x64 ret are
	; always a single c3, no c2 imm16, the original RIP
	; is pushed on stack by caller, when code arrive here,
	; the stack is already 16 bytes aligned, another point
	; is x64 can use RIP to easily call position independent
	; code, we can directly use LdrData.Field to call routines
	; finally, why we need call CreateThread, not directly
	; LoadLibrary? it's because we need 1, a simple API,
	; LoadLibrary is just too complicated to easily involve
	; problem, 2, we need the new thread has a clean security
	; context, not in the context's hijacked's thread.
	
	push rax
	push rbx
	
	mov	 edx, 1
	;lea  rcx, qword ptr [LdrData.Executed] 
	xor	 eax, eax
	lock cmpxchg dword ptr[LdrData.Executed], edx
	cmp	 eax, 1
	jne  ExclusiveInject 
	
	pop  rbx
	pop  rax
	ret
	
ExclusiveInject:

	; already has rax pushed on stack
	; set up stack for maximum 6 on stack arguments (CreateThread),
	; reserve a 16 byte space for xmm0
	
	sub  rsp, 8 * 6 + 16
	movdqa xmmword ptr[rsp + 8 * 6], xmm0
	
	; save LastErrorValue
	call qword ptr[LdrData.GetLastErrorPtr]	
	mov  dword ptr[LdrData.LastErrorValue], eax
		
	; call CreateThread(LoadLibrary)
	xor  rcx, rcx
	xor  rdx, rdx
	mov  r8,  qword ptr[LdrData.LoadLibraryPtr]
	lea  r9,  qword ptr[LdrData.DllFullPath]
	mov  qword ptr[rsp + 8 * 4], 0       
	mov  qword ptr[rsp + 8 * 4 + 8], 0     
	call qword ptr[LdrData.CreateThreadPtr]

	; WaitForSingleObject
	mov  rbx, rax
	mov  rcx, rbx
	mov  rdx, -1
	call qword ptr[LdrData.WaitForSingleObjectPtr]
	
	; call CloseHandle(ThreadHandle)	
	mov  rcx, rbx
	call qword ptr[LdrData.CloseHandlePtr]

	; restore original LastErrorValue 
	mov  rcx, qword ptr[LdrData.LastErrorValue]
	call qword ptr[LdrData.SetLastErrorPtr]

	; restore SSE register, clean stack	
	movdqa xmm0, xmmword ptr[rsp + 8 * 6]
	add rsp, 8 * 6 + 16
	
	pop rbx
	pop rax

	ret	
	
	db 6 dup(90h)
	 
BspLoadRoutineEnd:

;
; MSP_PREEXECUTE_CONTEXT
;

BSP_PREEXECUTE_CONTEXT struct 
	_Rip dq ?
	_Rbx dq ?
	_Rcx dq ?
	_Rdx dq ?
	_R8  dq ?
	_R9  dq ?
	CompleteEvent			dq ?
	SuccessEvent            dq ?
	ErrorEvent              dq ?
	LoadLibraryAddr         dq ?
	SetEventAddr            dq ?
	WaitForSingleObjectAddr dq ?
	CloseHandleAddr         dq ?
	Path                    dw 260 dup(0)
BSP_PREEXECUTE_CONTEXT ends

;
; [TEMPLATE]
; BspPreExecuteRoutine(
;   VOID
;   );
; 
; rbx, pointer to BSP_PREEXECUTE_CONTEXT structure
;

BspInjectPreExecuteRoutine proc

	; reserved home register space and align with 16 bytes
	; stack pointer
	
	sub rsp, 28h
	
	;
	; LoadLibrary(Path)	
	;	

	lea rcx, qword ptr[rbx + BSP_PREEXECUTE_CONTEXT.Path] 
	call qword ptr[rbx + BSP_PREEXECUTE_CONTEXT.LoadLibraryAddr]
	
	cmp rax, 0
	jz  Failed
	
	;
	; SetEvent(SuccessEvent)
	;
	
	mov rcx, qword ptr[rbx + BSP_PREEXECUTE_CONTEXT.SuccessEvent]
	call qword ptr[rbx + BSP_PREEXECUTE_CONTEXT.SetEventAddr]	
	
Complete:

	;
	; WaitForSingleObject(CompleteEvent, INFINITE)
	;
	
	mov edx, -1 ; INFINITE (dword)
	mov rcx, qword ptr[rbx + BSP_PREEXECUTE_CONTEXT.CompleteEvent]
	call qword ptr[rbx + BSP_PREEXECUTE_CONTEXT.WaitForSingleObjectAddr]
	
	; CloseHandle(SuccessEvent)
	mov rcx, qword ptr[rbx + BSP_PREEXECUTE_CONTEXT.SuccessEvent]
	call qword ptr[rbx + BSP_PREEXECUTE_CONTEXT.CloseHandleAddr]
	
	; CloseHandle(ErrorEvent)
	mov rcx, qword ptr[rbx + BSP_PREEXECUTE_CONTEXT.ErrorEvent]
	call qword ptr[rbx + BSP_PREEXECUTE_CONTEXT.CloseHandleAddr]
	
	; CloseHandle(CompleteEvent)
	mov rcx, qword ptr[rbx + BSP_PREEXECUTE_CONTEXT.CompleteEvent]
	call qword ptr[rbx + BSP_PREEXECUTE_CONTEXT.CloseHandleAddr]
	
	;
	; Restore first 4 parameter registers, note that it's enough to restore
	; the only 4 registers for entrypoint function
	;
	
	mov rcx, qword ptr[rbx + BSP_PREEXECUTE_CONTEXT._Rcx]
	mov rdx, qword ptr[rbx + BSP_PREEXECUTE_CONTEXT._Rdx]
	mov r8,  qword ptr[rbx + BSP_PREEXECUTE_CONTEXT._R8]
	mov r9,  qword ptr[rbx + BSP_PREEXECUTE_CONTEXT._R9]
	
	;
	; restore rsp
	;
	
	add rsp, 28h
	
	;
	; jump to orignal entry point
	;
	
	push qword ptr[rbx + BSP_PREEXECUTE_CONTEXT._Rip]
	push qword ptr[rbx + BSP_PREEXECUTE_CONTEXT._Rbx]
	pop  rbx
	
	ret
	
Failed:

	;
	; SetEvent(ErrorEvent)
	;
	
	mov rcx, qword ptr[rbx + BSP_PREEXECUTE_CONTEXT.ErrorEvent]
	call qword ptr[rbx + BSP_PREEXECUTE_CONTEXT.SetEventAddr]	
	jmp  Complete
	
BspInjectPreExecuteRoutine endp 

END