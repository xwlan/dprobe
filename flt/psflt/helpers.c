#include "psflt.h"
#include "pshelpers.h"

HMODULE PsNtDllHandle;
RTL_GET_CURRENT_PEB PsRtlGetCurrentPeb;
LDR_LOCK_LOADER_LOCK PsLdrLockLoaderLock;
LDR_UNLOCK_LOADER_LOCK PsLdrUnlockLoaderLock;

ULONG
PsInitializeHelpers(
	VOID
	)
{
	PsNtDllHandle = GetModuleHandle(L"ntdll.dll");
	if (!PsNtDllHandle) {
		return -1;
	}

	PsRtlGetCurrentPeb = (RTL_GET_CURRENT_PEB)GetProcAddress(PsNtDllHandle, "RtlGetCurrentPeb");
	PsLdrLockLoaderLock = (LDR_LOCK_LOADER_LOCK)GetProcAddress(PsNtDllHandle, "LdrLockLoaderLock");
	PsLdrUnlockLoaderLock = (LDR_UNLOCK_LOADER_LOCK)GetProcAddress(PsNtDllHandle, "LdrUnlockLoaderLock");

	if(!PsRtlGetCurrentPeb || !PsLdrLockLoaderLock || !PsLdrUnlockLoaderLock) { 
		return -1;
	}

	return 0;
}

ULONG
PsGetDllRefCountUnsafe(
	IN PVOID DllBase 
	)
{
	PLDR_DATA_TABLE_ENTRY Module; 

	Module = PsGetDllLdrDataEntry(DllBase);
	if (Module) {
		return Module->LoadCount;
	}

	return -1;
}

LDR_DATA_TABLE_ENTRY *
PsGetDllLdrDataEntry(
	IN PVOID DllBase
	)
{
	PPEB Peb;
	PPEB_LDR_DATA Ldr; 
	PLDR_DATA_TABLE_ENTRY Module; 

	Peb = (*PsRtlGetCurrentPeb)();
	Ldr = Peb->Ldr;

	Module = (LDR_DATA_TABLE_ENTRY *)Ldr->InLoadOrderModuleList.Flink;

	__try {

		while (Module != NULL && Module->DllBase != 0) { 
			if ((ULONG_PTR)Module->DllBase == (ULONG_PTR)DllBase) {
				return Module;
			}
			Module = (LDR_DATA_TABLE_ENTRY *)Module->InLoadOrderLinks.Flink; 
		} 
	}
	__except(1) {
		Module = NULL;
	}

	return NULL;
}

ULONG
PsGetDllNameUnsafe(
	IN PVOID DllBase,
	OUT PWCHAR Buffer,
	IN ULONG Size,
	OUT PULONG ResultSize
	)
{
	ASSERT(0);
	return -1;
}

BOOLEAN
PsAcquireLoaderLock(
	VOID
	)
{
	if (PsLdrLockLoaderLock) {
		(*PsLdrLockLoaderLock)(0, NULL, NULL);
		return TRUE;
	}

	return FALSE;
}

VOID
PsReleaseLoaderLock(
	VOID
	)
{
	if (PsLdrUnlockLoaderLock) {
		(*PsLdrUnlockLoaderLock)(0, 0);
	}
}

ULONG
PsConvertBitsToString(
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
			Count = sprintf(Next, "%s|", Bits[Number].Text); 
			Next += Count;
		}
	}

	//
	// Cut the last '|' operator
	//

	if (Buffer != Next) {
		Next -= 1;
		*Next = '\0';
	}

	*ResultSize = (ULONG)((ULONG_PTR)Next - (ULONG_PTR)Buffer);
	return 0;	
}