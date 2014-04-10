//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "call.h"
#include "hal.h"
#include "trap.h"
#include "stktrace.h"
#include "cache.h"
#include "heap.h"
#include "queue.h"


VOID
BtrPushCallFrame(
	IN PBTR_THREAD Thread,
	IN PBTR_CALLFRAME Frame
	)
{
	PBTR_CALLFRAME Top;
	ASSERT(Thread != NULL);

	if (IsListEmpty(&Thread->FrameList)) {

		//
		// N.B. Frame's Depth is 1 aligned with Thread->FrameDepth
		//

		Frame->Depth = 1;

	} else {
		Top = CONTAINING_RECORD(Thread->FrameList.Flink, BTR_CALLFRAME, ListEntry);
		Frame->Depth = Top->Depth + 1;
	}

	InterlockedIncrement(&Thread->FrameDepth);
	InsertHeadList(&Thread->FrameList, &Frame->ListEntry);
}

VOID
BtrPopCallFrame(
	IN PBTR_THREAD Thread,
	IN PBTR_CALLFRAME Frame
	)
{
	PLIST_ENTRY ListEntry;
	PBTR_CALLFRAME Top;
	PBTR_RECORD_HEADER Record;

	ASSERT(Thread != NULL);	
	ASSERT(Thread->FrameDepth != 0);

	if (!Thread->FrameDepth) {
		return;
	}

	ListEntry = RemoveHeadList(&Thread->FrameList);
	Top = CONTAINING_RECORD(ListEntry, BTR_CALLFRAME, ListEntry);

	if (Top != Frame) {
		
		Record = Frame->Record;
		if (Record != NULL && Record->RecordType != RECORD_FAST) {
			BtrFltFreeRecord((PBTR_FILTER_RECORD)Record);
		}

		InterlockedDecrement(&Thread->FrameDepth);

		//
		// Stack was unwinded by exception handling process, adjust
		// the active frame list to its appropriate position
		//

		while (IsListEmpty(&Thread->FrameList) != TRUE) {
			
			ListEntry = RemoveHeadList(&Thread->FrameList);
			Top = CONTAINING_RECORD(ListEntry, BTR_CALLFRAME, ListEntry);

			InterlockedDecrement(&Thread->FrameDepth);

			if (Top == Frame) {
				if (Frame->Record) {
					((PBTR_RECORD_HEADER)Frame->Record)->CallDepth = Top->Depth;
				}
				return;
			}
			else {
				Record = Frame->Record;
				if (Record != NULL && Record->RecordType != RECORD_FAST) {
					BtrFltFreeRecord((PBTR_FILTER_RECORD)Record);
				}
			}
		}	

		//
		// N.B. This indicates there's stack corruption
		// RtlUnwind() can trigger this ASSERT, we should 
		// consider add exception dispatch routines into
		// exemption address list.
		//

		//ASSERT(0);
	} 

	else {
	
		if (Frame->Record) {
			((PBTR_RECORD_HEADER)Frame->Record)->CallDepth = Top->Depth;
		}

		InterlockedDecrement(&Thread->FrameDepth);
	}
}

VOID
BtrSetExemptionCookie(
	IN ULONG_PTR Cookie,
	OUT PULONG_PTR Value
	)
{
	PULONG_PTR StackBase;

	StackBase = HalGetCurrentStackBase();
	*Value = *StackBase;
	*StackBase = Cookie;
}

VOID
BtrClearExemptionCookie(
	IN ULONG_PTR Value 
	)
{
	PULONG_PTR StackBase;

	StackBase = HalGetCurrentStackBase();
	*StackBase = Value;
}

BOOLEAN
BtrIsExemptionCookieSet(
	IN ULONG_PTR Cookie
	)
{
	PULONG_PTR StackBase;

	StackBase = HalGetCurrentStackBase();
	return (*StackBase == Cookie) ? TRUE : FALSE;
}

PBTR_RECORD_HEADER
BtrCaptureFastRecord(
	IN PBTR_TRAPFRAME Frame,
	IN PBTR_THREAD Thread,
	IN PBTR_FAST_RECORD Record,
	IN ULONG StackDepth,
	IN ULONG StackHash,
	IN PVOID Caller,
	IN PULONG_PTR CallerSp
	)
{
	PBTR_PROBE Probe;
	PBTR_REGISTER Register;
	PULONG_PTR StackBase;
	ULONG_PTR Count;
	ULONG Number;

	ASSERT(Thread != NULL);

	Probe = Frame->Object;
	Register = &Frame->Registers;

	//
	// N.B. Sequence is filled when write record,
	// CallDepth and Duration are filled after 
	// execute target routine.
	//

	Record->Base.TotalLength = sizeof(BTR_FAST_RECORD);
	Record->Base.RecordFlag = 0;
	Record->Base.RecordType = RECORD_FAST;
	Record->Base.ReturnType = Probe->ReturnType;
	Record->Base.StackDepth = StackDepth;
	Record->Base.CallDepth = -1;
	Record->Base.Address = Probe->Address;
	Record->Base.Caller = Caller;
	Record->Base.Sequence = -1;
	Record->Base.ObjectId = Probe->ObjectId;
	Record->Base.StackHash = StackHash;
	Record->Base.ProcessId = HalCurrentProcessId();
	Record->Base.ThreadId = HalCurrentThreadId(); 
	Record->Base.LastError = 0;
	Record->Base.Duration = 0;
	
#if defined (_M_IX86)

	Record->Argument.Ecx = Register->Ecx;
	Record->Argument.Edx = Register->Edx;
	
	//
	// Skip return address
	//

	CallerSp += 1;

#elif defined (_M_X64)

	Record->Argument.Rcx = Register->Rcx;
	Record->Argument.Rdx = Register->Rdx;
	Record->Argument.R8 = Register->R8;
	Record->Argument.R9 = Register->R9;
	Record->Argument.Xmm0 = Register->Xmm0;
	Record->Argument.Xmm1 = Register->Xmm1;
	Record->Argument.Xmm2 = Register->Xmm2;
	Record->Argument.Xmm3 = Register->Xmm3;

	//
	// Skip home register region
	//

	CallerSp += 4;

#endif

	StackBase = HalGetCurrentStackBase();
	Count = (StackBase - CallerSp) / sizeof(ULONG_PTR);
	Count = min(Count, FAST_STACK_ARGUMENT);

	for (Number = 0; Number < Count; Number += 1) {
		Record->Argument.Stack[Number] = CallerSp[Number];
	}
	
	return &Record->Base;
}

PBTR_THREAD
BtrIsExemptedCall(
	IN PBTR_TRAPFRAME Frame 
	)
{
	ULONG_PTR Caller;
	PBTR_THREAD Thread;

	Caller = *(PULONG_PTR)(Frame + 1);
	if (BtrIsExemptedAddress(Caller)) {
		return NULL;
	}

	if (BtrIsRuntimeThread(HalCurrentThreadId())) {
		return NULL;
	}

	if (BtrIsExemptionCookieSet(TRAP_EXEMPTION_COOKIE)) {
		return NULL;
	}

	if (BtrIsUnloading() || !HalIsAcspValid()) {
		return NULL;
	}

	Thread = BtrGetCurrentThread();
	ASSERT(Thread != NULL);

	if (FlagOn(Thread->ThreadFlag, BTR_FLAG_EXEMPTION)) {
		return NULL;
	}

	return Thread;
}

#if defined (_M_IX86)

BOOLEAN
BtrGetFpuReturn(
	OUT double *Result
	)
{
	BOOLEAN Status = FALSE;

	__asm {
		fstsw   ax
        and     ax, 0x3800
        cmp     ax, 0	
        jz      NoFpuReturn 
		mov     ecx, dword ptr[Result]
        fstp    qword ptr [ecx]
		mov     byte ptr[Status], 1
	}

NoFpuReturn:
	return Status;
}

VOID
BtrRestoreFpuReturn(
	IN double Result
	)
{
	__asm {
		fld qword ptr[Result]
	}
}

__declspec(naked) 
LONG __fastcall
BtrBalanceEsp(
	IN LONG Delta
	)
{
	__asm {
		mov edx, dword ptr[esp]
		sub esp, ecx
		mov dword ptr[esp], edx
		ret
	}
}

__declspec(naked)
VOID __fastcall
BtrGetResults(
	IN PBTR_CALLFRAME Frame
	)
{
	__asm {
		lea eax, dword ptr[esp + 4] 
		mov dword ptr[ecx + BTR_CALLFRAME.StackPost], eax 
		ret
	}
}

#endif

#if defined (_M_IX86)

PBTR_RECORD_HEADER
BtrExecuteCall(
	IN PBTR_TRAPFRAME TrapFrame,
	IN PBTR_THREAD Thread,
	IN PVOID Destine,
	IN PBTR_RECORD_HEADER Record,
	OUT PBTR_CALL_RETURN Result
	)
{
	PULONG_PTR StackBase;
	PULONG_PTR CallerSp;
	BTR_CALLFRAME CallFrame;
	ULONG_PTR Number;
	ULONG_PTR Count;
	ULONG64 Return;
	PBTR_REGISTER Register;
	PBTR_PROBE Probe;
	PULONG_PTR Argument;

	//
	// Caller stack pointer refer to its return address, we need skip
	// a machine word to locate the first stack argument
	//

	Register = &TrapFrame->Registers;
	Probe = TrapFrame->Object;

	//
	// Push call frame to current thread's virtual stack
	//

	CallFrame.StackPost = NULL;
	CallFrame.Record = Record;
	CallFrame.Probe = Probe;
	CallFrame.LastError = 0;
	CallFrame.Flag = 0;

	BtrPushCallFrame(Thread, &CallFrame);

	//
	// Skip return address
	//

	CallerSp = (PULONG_PTR)(TrapFrame + 1);
	CallerSp += 1;

	StackBase = HalGetCurrentStackBase();
	Count = (StackBase - CallerSp) / sizeof(ULONG_PTR);
	Count = min(Count, STACK_ARGUMENT_LIMIT);
	
	//
	// N.B. _alloca() ensure Argument is at lowest address of current stack,
	// after return, it ensure that even the esp is adjusted, the local variable
	// won't be corrupted, furthermore, we can easily compute the esp delta 
	// to adjust it back (if required), for x64, stack pointer is not an issue.
	//

	Argument = (PULONG_PTR)_alloca(STACK_ARGUMENT_LIMIT * sizeof(ULONG_PTR));

	RtlZeroMemory(Argument, STACK_ARGUMENT_LIMIT * sizeof(ULONG_PTR));
	for(Number = 0; Number < STACK_ARGUMENT_LIMIT; Number += 1) {
		if (Number < Count) {
			Argument[Number] = CallerSp[Number];
		} else {
			Argument[Number] = 0;
		}
	}

	//
	// N.B. BtrGetResults MUST immediately follow the call to routine,
	// because we need immediately save the register values after the call.
	//

	__try {

		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);

		Return = ((BTR_DISPATCH_CALL32)Destine)(Register->Ecx, Register->Edx); 
		BtrGetResults(&CallFrame);

		Result->SpDelta = PtrToLong(CallFrame.StackPost) - PtrToLong(Argument);
		BtrBalanceEsp(Result->SpDelta);

		//
		// N.B. Immediately get FPU result, this can avoid our code dirty FPU stack.
		//

		if (Probe->ReturnType == ReturnFloat || Probe->ReturnType == ReturnDouble) {
			Result->IsFpuReturn = BtrGetFpuReturn(&Result->FpuReturn);
		}
		
		SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);

		if (Probe->Type == PROBE_FAST) {
			CallFrame.LastError = GetLastError();
		}

	}
	__finally {

		BtrPopCallFrame(Thread, &CallFrame);
	}

	Record = CallFrame.Record;
	if (!Record) {
		Result->LastError = CallFrame.LastError;
		Result->Return = Return;
		Register->Eax = (ULONG)Return;
		Register->Edx = (ULONG)(Return >> 32);
		return NULL;
	}

	Record->CallDepth = CallFrame.Depth;
	Record->ReturnType = Probe->ReturnType;
	Record->LastError = CallFrame.LastError;

	Register->Eax = (ULONG)Return;
	Register->Edx = (ULONG)(Return >> 32);

	if (Result->IsFpuReturn) {
		Record->FpuReturn = Result->FpuReturn;
		SetFlag(Record->RecordFlag, BTR_FLAG_FPU_RETURN);
	} else { 
		Record->Return = Return;
	}

	return Record;
}

#endif

#if defined (_M_IX86)

VOID 
BtrCallProcedure(
	IN PBTR_TRAPFRAME Frame 
	)
{
	PBTR_THREAD Thread;
	PBTR_PROBE Object;
	PULONG_PTR AdjustedEsp;
	PBTR_REGISTER Register;
	PBTR_TRAP Trap;
	PBTR_RECORD_HEADER Record;
	ULONG_PTR Caller;
	PULONG_PTR CallerSp;
	ULONG Status;
	BOOLEAN RequireExemption; 
	ULONG Depth, Hash;
	PVOID Destine;
	SYSTEMTIME Time;
	LARGE_INTEGER Start;
	LARGE_INTEGER End;
	BTR_CALL_RETURN Result = {0};
	BTR_FAST_RECORD FastRecord;

	ASSERT(Frame->Object != NULL);

	Object = Frame->Object;
	Trap = Object->Trap;

	//
	// Check exemption condition, if it's not exempted, current thread
	// object is returned.
	//

	Thread = BtrIsExemptedCall(Frame);
	if (!Thread) {
		AdjustedEsp = (PULONG_PTR)&Frame->Object;
		*AdjustedEsp = (ULONG_PTR)&Frame->ExemptionReturn;
		Frame->ExemptionReturn = (ULONG_PTR)Trap->HijackedCode;
		return;
	}

	RequireExemption = FALSE;
	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	InterlockedIncrement(&Object->References);

	//
	// Capture and queue stack trace
	//

	Register = &Frame->Registers;
	BtrCaptureStackTrace((PVOID)Register->Ebp, Object->Address, &Hash, &Depth);

	CallerSp = (PULONG_PTR)(Frame + 1);
	Caller = *CallerSp;

	if (Object->Type == PROBE_FAST) {
		Destine = (PVOID)Trap->HijackedCode;
		Record = BtrCaptureFastRecord(Frame, Thread, &FastRecord, Depth, Hash, (PVOID)Caller, CallerSp);
		ASSERT(Record != NULL);
	} 
	else if (Object->Type == PROBE_FILTER) {
		Destine = (PVOID)Object->FilterCallback;	
		Record = NULL;
	} else {
		ASSERT(0);
	}

	//
	// Execute original call or its registered callback
	//

	GetLocalTime(&Time);
	QueryPerformanceCounter(&Start);

	Record = BtrExecuteCall(Frame, Thread, Destine, Record, &Result);

	//
	// N.B. Here assume return registers are captured, even record ptr is null
	//

	if (Record != NULL) {

		QueryPerformanceCounter(&End);
		Record->Duration = (ULONG)(End.QuadPart - Start.QuadPart);
		SystemTimeToFileTime(&Time, &Record->FileTime);

		if (Object->Type == PROBE_FILTER) {
			Record->StackDepth = Depth;
			Record->Address = Object->Address;
			Record->Caller = (PVOID)Caller;
			Record->StackHash = Hash;
			Record->ObjectId = Object->ObjectId;
			Record->ProcessId = HalCurrentProcessId();
			Record->ThreadId = HalCurrentThreadId(); 
		}

		Status = BtrQueueSharedCacheRecord(Thread, Record, Object);
		if (Status != S_OK) {
			RequireExemption = TRUE;
		}

		if (Record->RecordType != RECORD_FAST) {

			//
			// fast record don't use thread buffer and heap
			//

			if ((PBTR_RECORD_HEADER)Thread->Buffer == Record) {
				Thread->BufferBusy = FALSE;			
			} else {
				BtrFreeLookaside(RecordHeap, Record);
			}
		}
	} 

	//
	// N.B. SpDelta < 0 indicates a stack corruption.
	//

#if defined (_DEBUG)
	if (Result.SpDelta < 0) {
		__debugbreak();
	}
#endif

	if (Result.SpDelta < 0) {
		Result.SpDelta = 0;
	}

	AdjustedEsp = (PULONG_PTR)(Frame + 1);
	AdjustedEsp = (PULONG_PTR)((ULONG_PTR)AdjustedEsp + Result.SpDelta);

	//
	// Make POP ESP to update ESP to point to adjusted ESP 
	//

	*(PULONG_PTR)&Frame->Object = (ULONG_PTR)AdjustedEsp; 
	*AdjustedEsp = Caller;

	InterlockedDecrement(&Object->References);

	if (Result.IsFpuReturn) {
		BtrRestoreFpuReturn(Result.FpuReturn);
	}

	//
	// N.B. If there's no error occurs during the call, and
	// the thread is not in rundown state, clear its exemption flag,
	// it's possible that after the call, the thread enters rundown
	// state, e.g. if the probe object is for LdrShutdownThread,
	// BtrOnThreadDetach() is called before this procedure return,
	// current thread should never enter runtime any more.
	//

	if (!RequireExemption && !FlagOn(Thread->ThreadFlag, BTR_FLAG_RUNDOWN)) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	}

	return;
}

#endif

#if defined (_M_X64)

PBTR_RECORD_HEADER
BtrExecuteCall(
	IN PBTR_TRAPFRAME TrapFrame,
	IN PBTR_THREAD Thread,
	IN PVOID Destine,
	IN PBTR_RECORD_HEADER Record,
	OUT PBTR_CALL_RETURN Result
	)
{
	PULONG_PTR StackBase;
	PULONG_PTR CallerSp;
	BTR_CALLFRAME CallFrame;
	ULONG_PTR Number;
	ULONG_PTR Count;
	ULONG64 Return;
	PBTR_REGISTER Register;
	PBTR_PROBE Probe;
	ULONG_PTR Argument[STACK_ARGUMENT_LIMIT];

	//
	// Caller stack pointer refer to its return address, we need skip
	// a machine word to locate the first stack argument
	//

	Register = &TrapFrame->Registers;
	Probe = TrapFrame->Object;

	//
	// Push call frame to current thread's virtual stack
	//

	CallFrame.Record = Record;
	CallFrame.Probe = Probe;
	CallFrame.LastError = 0;
	CallFrame.Flag = 0;

	BtrPushCallFrame(Thread, &CallFrame);

	//
	// Skip home register reserved region
	//

	CallerSp = (PULONG_PTR)(TrapFrame + 1);
	CallerSp += 4;

	//
	// ASSERT it's 16 bytes aligned
	//

	ASSERT(((ULONG_PTR)CallerSp & 0x0f) == 0);

	StackBase = HalGetCurrentStackBase();
	Count = (StackBase - CallerSp) / sizeof(ULONG_PTR);
	Count = min(Count, STACK_ARGUMENT_LIMIT);
	
	RtlZeroMemory(Argument, STACK_ARGUMENT_LIMIT * sizeof(ULONG_PTR));
	for(Number = 0; Number < STACK_ARGUMENT_LIMIT; Number += 1) {
		if (Number < Count) {
			Argument[Number] = CallerSp[Number];
		} else {
			Argument[Number] = 0;
		}
	}

	//
	// N.B. BtrGetResults MUST be immediately follow the call to routine,
	// because we need immediately save the register values after the call.
	//

	__try {

		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);

		Return = BtrDispatchCall64(Register->Rcx, Register->Rdx, Register->R8, Register->R9,
								   Argument[0], Argument[1], Argument[2], Argument[3], 
								   Argument[4], Argument[5], Argument[6], Argument[7], 
								   Argument[8], Argument[9], Argument[10], Argument[11], 
								   Argument[12], Argument[13], Argument[14], Argument[15], 
								   (PVOID)&Register->Xmm0, Destine );
		BtrGetResults(&CallFrame);
		SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);

		if (Probe->Type == PROBE_FAST) {
			CallFrame.LastError = GetLastError();
		}

	}
	__finally {

		BtrPopCallFrame(Thread, &CallFrame);
	}

	Record = CallFrame.Record;
	if (!Record) {

		Result->LastError = CallFrame.LastError;
		Result->Return = Return;
		Result->Xmm0Return = CallFrame.Xmm0Value;
		return NULL;
	}

	Record->ReturnType = Probe->ReturnType;
	Record->LastError = CallFrame.LastError;

	Register->Rax = Return;
	Register->Xmm0 = CallFrame.Xmm0Value;

	if (Probe->ReturnType == ReturnFloat || Probe->ReturnType == ReturnDouble) {
		Record->FpuReturn = Register->Xmm0.m128d_f64[0];
		SetFlag(Record->RecordFlag, BTR_FLAG_FPU_RETURN);
	} else {
		Record->Return = Register->Rax;
	}

	return Record;
}

VOID 
BtrCallProcedure(
	IN PBTR_TRAPFRAME Frame 
	)
{
	PBTR_THREAD Thread;
	PBTR_PROBE Object;
	PULONG_PTR AdjustedEsp;
	PBTR_REGISTER Register;
	PBTR_TRAP Trap;
	PBTR_RECORD_HEADER Record;
	ULONG_PTR Caller;
	PULONG_PTR CallerSp;
	ULONG Status;
	BOOLEAN RequireExemption; 
	ULONG Depth, Hash;
	PVOID Destine;
	BTR_CALL_RETURN Result = {0};
	SYSTEMTIME Time;
	LARGE_INTEGER Start;
	LARGE_INTEGER End;
	BTR_FAST_RECORD FastRecord;

	ASSERT(Frame->Object != NULL);

	Object = Frame->Object;
	Trap = Object->Trap;

	//
	// Check whether it's call to be exempted
	//

	Thread = BtrIsExemptedCall(Frame);
	if (!Thread ) {
		AdjustedEsp = (PULONG_PTR)&Frame->Object;
		*AdjustedEsp = (ULONG_PTR)&Frame->StackAlignment;
		Frame->StackAlignment = (ULONG_PTR)Trap->HijackedCode;
		return;
	}

	//
	// Mark current thread's probe exemption flag and increase reference count
	//

	RequireExemption = FALSE;
	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	_InterlockedIncrement(&Object->References);

	//
	// Capture stack trace
	//

	Register = &Frame->Registers;
	BtrCaptureStackTrace(NULL, Object->Address, &Hash, &Depth);

	//
	// under any case, we should get a record here, else the sequence number will
	// be broken here, it's impossible to fix any subsequent record's sequence number.
	// for exception case, we can allocate a dummy record and report the exception.
	//

	CallerSp = (PULONG_PTR)(Frame + 1);
	Caller = *CallerSp;

	if (Object->Type == PROBE_FAST) {
		Destine = (PVOID)Trap->HijackedCode;
		Record = BtrCaptureFastRecord(Frame, Thread, &FastRecord, Depth, Hash, (PVOID)Caller, CallerSp);
		ASSERT(Record != NULL);
	} 
	else if (Object->Type == PROBE_FILTER) {
		Destine = (PVOID)Object->FilterCallback;	
		Record = NULL;
	} else {
		ASSERT(0);
	}

	//
	// Execute target routine or callback
	//

	GetLocalTime(&Time);
	QueryPerformanceCounter(&Start);

	Record = BtrExecuteCall(Frame, Thread, Destine, Record, &Result);

	if (Record) {

		//
		// Write record to shared cache
		//

		QueryPerformanceCounter(&End);
		Record->Duration = (ULONG)(End.QuadPart - Start.QuadPart);
		SystemTimeToFileTime(&Time, &Record->FileTime);

		if (Object->Type == PROBE_FILTER) {
			Record->StackDepth = Depth;
			Record->Address = Object->Address;
			Record->Caller = (PVOID)Caller;
			Record->StackHash = Hash;
			Record->ObjectId = Object->ObjectId;
			Record->ProcessId = HalCurrentProcessId();
			Record->ThreadId = HalCurrentThreadId(); 
		}

		Status = BtrQueueSharedCacheRecord(Thread, Record, Object);
		if (Status != S_OK) {
			RequireExemption = TRUE;
		}

		//
		// Free record buffer
		//

		if (Record->RecordType != RECORD_FAST) {

			//
			// fast record don't use thread buffer and heap
			//

			if ((PBTR_RECORD_HEADER)Thread->Buffer == Record) {
				Thread->BufferBusy = FALSE;			
			} else {
				BtrFreeLookaside(RecordHeap, Record);
			}
		}
	} 
	else {
	
		//
		// Assume BtrExecuteCall already fill return registers.
		//
	}

	//
	// AMD64 has no stack delta, caller own the stack frame
	//

	AdjustedEsp = (PULONG_PTR)&Frame->Object;
	*AdjustedEsp = (ULONG_PTR)&Frame->ReturnAddress;	

	//
	// Clear current thread's probe exemption flag and decrease reference count
	//

	InterlockedDecrement(&Object->References);

	//
	// N.B. If there's no error occurs during the call, and
	// the thread is not in rundown state, clear its exemption flag,
	// it's possible that after the call, the thread enters rundown
	// state, e.g. if the probe object is for LdrShutdownThread,
	// BtrOnThreadDetach() is called before this procedure return,
	// current thread should never enter runtime any more.
	//

	if (!RequireExemption && !FlagOn(Thread->ThreadFlag, BTR_FLAG_RUNDOWN)) {
		ClearFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);
	}

	return;
}

#endif