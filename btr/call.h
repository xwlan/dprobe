//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _CAPTURE_H_
#define _CAPTURE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "btr.h"
#include "decode.h"
#include "probe.h"
#include <emmintrin.h>
#include "thread.h"

#if defined(_M_IX86) 

#pragma pack(push, 4)

typedef struct _BTR_REGISTER {
	ULONG_PTR EFlag;
	ULONG_PTR Edi;
	ULONG_PTR Esi;
	ULONG_PTR Ebp;
	ULONG_PTR Esp;
	ULONG_PTR Ebx;
	ULONG_PTR Edx;
	ULONG_PTR Ecx;
	ULONG_PTR Eax;
} BTR_REGISTER, *PBTR_REGISTER;

#pragma pack(pop)

typedef struct _BTR_TRAPFRAME {
	BTR_REGISTER Registers;
	PBTR_PROBE Object;
	ULONG_PTR ExemptionReturn;
} BTR_TRAPFRAME, *PBTR_TRAPFRAME;

typedef struct _BTR_CALLFRAME {
	PVOID StackPost;
	LIST_ENTRY ListEntry;
	PVOID Probe;
	PVOID Record;
	USHORT Depth;
	USHORT Flag;
	ULONG LastError;
} BTR_CALLFRAME, *PBTR_CALLFRAME;

typedef ULONG64 
(__fastcall *BTR_DISPATCH_CALL32)(
	IN ULONG_PTR Ecx,
	IN ULONG_PTR Edx
	);

VOID __fastcall 
BtrGetResults(
	IN PBTR_CALLFRAME Frame 
	);

BOOLEAN
BtrGetFpuReturn(
	OUT double *Result
	);

VOID
BtrRestoreFpuReturn(
	IN double Result
	);

#define TRAP_EXEMPTION_COOKIE  0x9E3779B9 

#endif  // _M_IX86

#if defined(_M_X64)

typedef struct _BTR_REGISTER {
	
	ULONG64 Home0;
	ULONG64 Home1;
	ULONG64 Home2;
	ULONG64 Home3;

	ULONG64 Rcx;
	ULONG64 Rdx;
	ULONG64 R8;
	ULONG64 R9;
	
	__m128d Xmm0;
	__m128d Xmm1;
	__m128d Xmm2;
	__m128d Xmm3;

	ULONG64 Rax;
	ULONG64 RFlag;

} BTR_REGISTER, *PBTR_REGISTER;

typedef struct _BTR_TRAPFRAME {
	BTR_REGISTER Registers;
	PBTR_PROBE Object;
	ULONG_PTR ExemptionReturn;
	ULONG_PTR StackAlignment;
	PVOID ReturnAddress;
} BTR_TRAPFRAME, *PBTR_TRAPFRAME;

typedef struct _BTR_CALLFRAME {
	__m128d Xmm0Value;
	LIST_ENTRY ListEntry;
	PVOID Probe;
	PVOID Record;
	USHORT Depth;
	USHORT Flag;
	ULONG LastError;
} BTR_CALLFRAME, *PBTR_CALLFRAME;

ULONG_PTR
BtrDispatchCall64(
	IN ULONG_PTR Rcx,
	IN ULONG_PTR Rdx,
	IN ULONG_PTR R8,
	IN ULONG_PTR R9,
	IN ULONG_PTR SP1,
	IN ULONG_PTR SP2,
	IN ULONG_PTR SP3,
	IN ULONG_PTR SP4,
	IN ULONG_PTR SP5,
	IN ULONG_PTR SP6,
	IN ULONG_PTR SP7,
	IN ULONG_PTR SP8,
	IN ULONG_PTR SP9,
	IN ULONG_PTR SP10,
	IN ULONG_PTR SP11,
	IN ULONG_PTR SP12,
	IN ULONG_PTR SP13,
	IN ULONG_PTR SP14,
	IN ULONG_PTR SP15,
	IN ULONG_PTR SP16,
	IN PVOID XmmRegister,
	IN PVOID DestineAddress
	);

VOID 
BtrGetResults(
	IN PBTR_CALLFRAME Frame 
	);

extern PVOID BtrEnterProcedureAddress;

VOID 
BtrEnterProcedure(
	IN PBTR_TRAPFRAME Frame 
	);

#define TRAP_EXEMPTION_COOKIE  0x9E3779B99E3779B9

#endif

//
// N.B. Both x86/x64 support maximum 16 stack arguments
// so x86 in fact support maximum 18, x64 maximum 20 arguments.
//

#define STACK_ARGUMENT_LIMIT  16

typedef struct _BTR_CALL_RETURN {
	LONG SpDelta;
	ULONG LastError;
	ULONG64 Return;
	BOOLEAN IsFpuReturn;
	double FpuReturn;
	__m128d Xmm0Return;
} BTR_CALL_RETURN, *PBTR_CALL_RETURN;

VOID
BtrPushCallFrame(
	IN PBTR_THREAD Thread,
	IN PBTR_CALLFRAME Frame
	);

VOID
BtrPopCallFrame(
	IN PBTR_THREAD Thread,
	IN PBTR_CALLFRAME Frame
	);

VOID
BtrSetExemptionCookie(
	IN ULONG_PTR Cookie,
	OUT PULONG_PTR Value
	);

VOID
BtrClearExemptionCookie(
	IN ULONG_PTR Value 
	);

BOOLEAN
BtrIsExemptionCookieSet(
	IN ULONG_PTR Cookie
	);

PBTR_RECORD_HEADER
BtrExecuteCall(
	IN PBTR_TRAPFRAME TrapFrame,
	IN PBTR_THREAD Thread,
	IN PVOID Destine,
	IN PBTR_RECORD_HEADER Record,
	OUT PBTR_CALL_RETURN Result
	);

PBTR_RECORD_HEADER
BtrCaptureFastRecord(
	IN PBTR_TRAPFRAME Frame,
	IN PBTR_THREAD Thread,
	IN PBTR_FAST_RECORD Record,
	IN ULONG StackDepth,
	IN ULONG StackHash,
	IN PVOID Caller,
	IN PULONG_PTR CallerSp
	);

PBTR_THREAD
BtrIsExemptedCall(
	IN PBTR_TRAPFRAME Frame 
	);

VOID 
BtrCallProcedure(
	IN PBTR_TRAPFRAME Frame 
	);

extern PVOID BtrTrapProcedure;

#ifdef __cplusplus
}
#endif
#endif