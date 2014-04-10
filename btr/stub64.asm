;
; lan.john@gmail.com
; Apsara Labs
; Copyright(C) 2009-2012
;

.code

align 16

;
; MUST SAME AS DEFINE IN TRAP.H
;

STACK_ARGUMENT_LIMIT   equ 16
HOME_NUM               equ 4
HOME_SIZE              equ 8*HOME_NUM

BTR_CALLFRAME struct 
	Xmm0LowPart  dq ?
	Xmm0HighPart dq ?
	Flink        dq ?
	Blink        dq ?
	ProbePtr     dq ?
	RecordPtr    dq ?
	Depth        dw ?
	Flags        dw ?
	LastError    dd ?
BTR_CALLFRAME ends 

;
; define offset of BTR_UNLOAD_ARGUMENT
;

WaitForSingleObjectPtr equ 0
CloseHandlePtr         equ 8
FreeLibraryPtr         equ 16 
HeapDestroyPtr         equ 24
ThreadHandle           equ 32 
DllHandle              equ 40
HeapHandle             equ 48

; VOID
; BtrGetResults(
;	IN PBTR_CALLFRAME Frame 
;	);
;

BtrGetResults proc 
	movdqa xmmword ptr[rcx + BTR_CALLFRAME.Xmm0LowPart], xmm0
	ret
BtrGetResults endp 

;
; ULONG_PTR
; BtrDispatchCall64(
;	IN ULONG_PTR Rcx,
;	IN ULONG_PTR Rdx,
;	IN ULONG_PTR R8,
;	IN ULONG_PTR R9,
;	IN ULONG_PTR SP1,
;	IN ULONG_PTR SP2,
;	IN ULONG_PTR SP3,
;	IN ULONG_PTR SP4,
;	IN ULONG_PTR SP5,
;	IN ULONG_PTR SP6,
;	IN ULONG_PTR SP7,
;	IN ULONG_PTR SP8,
;	IN ULONG_PTR SP9,
;	IN ULONG_PTR SP10,
;	IN ULONG_PTR SP11,
;	IN ULONG_PTR SP12
;	IN ULONG_PTR SP13
;	IN ULONG_PTR SP14
;	IN ULONG_PTR SP15
;	IN ULONG_PTR SP16
;	IN PVOID XmmRegister,
;	IN PVOID DestineAddress
;	);
;

BtrDispatchCall64 proc
	; load SSE registers	
	mov r10, qword ptr[rsp + 8 + HOME_SIZE + STACK_ARGUMENT_LIMIT * 8]
	movdqa xmm0, xmmword ptr[r10 + 00h]
	movdqa xmm1, xmmword ptr[r10 + 10h]
	movdqa xmm2, xmmword ptr[r10 + 20h]
	movdqa xmm3, xmmword ptr[r10 + 30h]
	mov r10, qword ptr[rsp + 8 + HOME_SIZE + STACK_ARGUMENT_LIMIT * 8 + 8]
	jmp r10
	
BtrDispatchCall64 endp

;
; [TEMPLATE]
; ULONG CALLBACK
; BtrUnloadRoutine64(
;	IN PVOID Argument 
;   );
;

public BtrUnloadRoutine64

BtrUnloadRoutine64 proc 

	push rbx
	sub	 rsp,	20h    ; reserved home register space
	
	;
	; WaitForSingleObject(ThreadHandle, INFINITE)
	;
	
	mov  rbx,	rcx	; Load Argument into rbx
	mov	 edx,	-1	; INFINITE (dword)
	mov  rcx,	qword ptr[rbx + ThreadHandle]	
	mov  r8,	qword ptr[rbx + WaitForSingleObjectPtr]
	call r8
	
	;
	; CloseHandle(ThreadHandle)
	;

	mov  rcx,	qword ptr[rbx + ThreadHandle]	
	mov  r8,	qword ptr[rbx + CloseHandlePtr]
	call r8
		
	;
	; while (FreeLibrary(DllHandle))
	;
	
RepeatUntilZero:
	mov  rcx,	qword ptr[rbx + DllHandle]
	mov  r8,	qword ptr[rbx + FreeLibraryPtr]
	call r8
	
	test eax, eax  ; BOOL (dword)
	jne RepeatUntilZero
	  
	;
	; HeapDestroy(HeapHandle)
	;
	
	mov rcx,	qword ptr[rbx + HeapHandle]
	mov r8,		qword ptr[rbx + HeapDestroyPtr]
	
	; Clean home register space, restore rbx
	
	add rsp,	20h
	pop rbx		
	
	; BtrUnloadRoutine and HeapDestroy has exactly same prototype, good.
	
	jmp r8	
	
	;
	; Never arrive here
	;
	
	int 3
    ret 8

BtrUnloadRoutine64 endp


;
; N.B. BtrTrapProcedure
; required to use masm directives to generate unwind information 
; to capture good stack trace, otherwise RtlVirtualWind don't 
; understand how to walk the stack.
;

extern BtrCallProcedure:proc
public BtrTrapProcedure

BtrTrapProcedure proc FRAME
	; 
	; N.B. The code must generate the BTR_TRAPFRAME
	; structure as following.
	;
	;typedef struct _BTR_TRAPFRAME {
	;	BTR_REGISTER Registers;
	;	PBTR_PROBE Object;
	;	ULONG_PTR ExemptionReturn;
	;	ULONG_PTR StackAlignment;
	;	ULONG_PTR ReturnAddress;
	;} BTR_TRAPFRAME, *PBTR_TRAPFRAME;
	;

	sub rsp, 18h
	.allocstack 18h
	
	;
	; registers
	;
	
	pushfq
	.allocstack 8
	
	push rax
	.pushreg rax
	
	sub rsp, 40h
	.allocstack 40h
	
	movdqa xmmword ptr[rsp + 00h], xmm0
    .savexmm128 xmm0, 00h
    
	movdqa xmmword ptr[rsp + 10h], xmm1
    .savexmm128 xmm0, 10h
    
	movdqa xmmword ptr[rsp + 20h], xmm2
    .savexmm128 xmm0, 20h
    
	movdqa xmmword ptr[rsp + 30h], xmm3
    .savexmm128 xmm0, 30h
    
	push r9
	.pushreg r9
	
	push r8
	.pushreg r8
	
	push rdx
	.pushreg rdx
	
	push rcx
	.pushreg rcx
	
	sub rsp, 20h
	.allocstack 20h
	
	.endprolog
	
	;
	; Clear HOME register region
	;
	
	mov qword ptr[rsp + 00h], 0
	mov qword ptr[rsp + 08h], 0
	mov qword ptr[rsp + 10h], 0
	mov qword ptr[rsp + 18h], 0
	
	;
	; load pointer of trap frame and
	; execute BtrCallProcedure 
	;
	
	mov rcx, rsp
	call BtrCallProcedure
	
	add rsp, 20h
	pop rcx
	pop rdx
	pop r8
	pop r9
	
	movdqa xmm0, xmmword ptr[rsp + 00h]
	movdqa xmm1, xmmword ptr[rsp + 10h]
	movdqa xmm2, xmmword ptr[rsp + 20h]
	movdqa xmm3, xmmword ptr[rsp + 30h]
	
	add rsp, 40h
	pop rax
	popfq
	
	; 
	; N.B after 'pop rsp', rsp point to destine address
	;
	
	pop rsp
	ret
	
BtrTrapProcedure endp 

;
; [TEMPLATE]
; BtrTrapCode
;

BtrTrapCode proc

	mov qword ptr[rsp - 08h], 0 
	mov qword ptr[rsp - 10h], 0 
	mov dword ptr[rsp - 18h], 12345678h 
	mov dword ptr[rsp - 14h], 87654321h
	jmp qword ptr[0]
	
BtrTrapCode endp

;
; END OF ASSEMBLER
;

end