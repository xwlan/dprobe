;
; lan.john@gmail.com
; Apsara Labs
; Copyright(C) 2009-2012
;

.686
.model flat, c
.code

;
; define offset of BTR_UNLOAD_ARGUMENT
;

WaitForSingleObjectPtr equ 0
CloseHandlePtr         equ 4
FreeLibraryPtr         equ 8
HeapDestroyPtr         equ 12
ThreadHandle           equ 16
DllHandle              equ 20
HeapHandle             equ 24

;
; [TEMPLATE]
; ULONG CALLBACK
; BtrUnloadRoutine(
;	IN PVOID Argument 
;   );
;

public BtrUnloadRoutine32

BtrUnloadRoutine32 proc 

	push ebx
	
	;
	; WaitForSingleObject(ThreadHandle, INFINITE)
	;
	
	mov  ebx, dword ptr[esp + 4]
	push -1	  
	push dword ptr[ebx + ThreadHandle]	
	mov  ecx, dword ptr[ebx + WaitForSingleObjectPtr]
	call ecx
	
	;
	; CloseHandle(ThreadHandle)
	;

	push dword ptr[ebx + ThreadHandle]	
	mov ecx, dword ptr[ebx + CloseHandlePtr]
	call ecx
		
	;
	; while (FreeLibrary(DllHandle))
	;
	
RepeatUntilZero:
	push dword ptr[ebx + DllHandle]
	mov  ecx, dword ptr[ebx + FreeLibraryPtr]
	call ecx
	
	test eax, eax
	jne RepeatUntilZero
	  
	;
	; HeapDestroy(HeapHandle)
	;
	
	mov eax, dword ptr[ebx + HeapHandle]
	mov edx, dword ptr[ebx + HeapDestroyPtr]
	
	; Restore non-volatile register
	
	pop ebx		
	
	; BtrUnloadRoutine and HeapDestroy has exactly same prototype, good.
	
	mov dword ptr[esp + 4], eax	    ; HeapHandle
	jmp edx	
	
	;
	; Never arrive here
	;
	
	int 3
    ret 4

BtrUnloadRoutine32 endp

;
; [TEMPLATE] 
; BtrTrapCode
;

BtrTrapCode proc 

	mov dword ptr[esp - 4], 0 
	mov dword ptr[esp - 8], 12345678h 
	jmp dword ptr[BtrTrapProcedure]
	
BtrTrapCode endp 

extern BtrCallProcedure:proc
public BtrTrapProcedure

BtrTrapProcedure proc 
	; 
	; N.B. The code must generate the BTR_TRAPFRAME
	; structure as following.
	;
	;typedef struct _BTR_TRAPFRAME {
	;	BTR_REGISTER Registers;
	;	PBTR_PROBE Object;
	;	ULONG_PTR ExemptionReturn;
	;} BTR_TRAPFRAME, *PBTR_TRAPFRAME;
	;

	sub esp, 8
	pushad
	pushfd
	push esp
	call BtrCallProcedure
	add esp, 4
	popfd
	popad
	pop esp
	ret
	
BtrTrapProcedure endp 

;
; END OF ASSEMBLER
;

end





















