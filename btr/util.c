//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "btr.h"
#include "heap.h"
#include "hal.h"

#if defined (_M_IX86)
#define GOLDEN_RATIO 0x9E3779B9

#elif defined (_M_X64)
#define GOLDEN_RATIO 0x9E3779B97F4A7C13
#endif

ULONG_PTR
BtrUlongPtrRoundDown(
	IN ULONG_PTR Value,
	IN ULONG_PTR Align
	)
{
	return Value & ~(Align - 1);
}

ULONG
BtrUlongRoundDown(
	IN ULONG Value,
	IN ULONG Align
	)
{
	return Value & ~(Align - 1);
}

ULONG64
BtrUlong64RoundDown(
	IN ULONG64 Value,
	IN ULONG64 Align
	)
{
	return Value & ~(Align - 1);
}

ULONG_PTR
BtrUlongPtrRoundUp(
	IN ULONG_PTR Value,
	IN ULONG_PTR Align
	)
{
	return (Value + Align - 1) & ~(Align - 1);
}

ULONG
BtrUlongRoundUp(
	IN ULONG Value,
	IN ULONG Align
	)
{
	return (Value + Align - 1) & ~(Align - 1);
}

ULONG64
BtrUlong64RoundUp(
	IN ULONG64 Value,
	IN ULONG64 Align
	)
{
	return (Value + Align - 1) & ~(Align - 1);
}

BOOLEAN
BtrIsExecutableAddress(
	IN PVOID Address
	)
{
	SIZE_T Status;
	ULONG Mask;
	MEMORY_BASIC_INFORMATION Mbi = {0};

	Status = VirtualQuery(Address, &Mbi, sizeof(Mbi));
	if (!Status) {
		return FALSE;
	}

	Mask = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

	//
	// The specific address must be committed, and execute page.
	//

	if (!FlagOn(Mbi.State, MEM_COMMIT) || !FlagOn(Mbi.Protect, Mask)) {
		return FALSE;
	}

	return TRUE;
}

BOOLEAN
BtrIsValidAddress(
	IN PVOID Address
	)
{
	CHAR Byte;
	BOOLEAN Status = TRUE;

	__try {
		Byte = *(PCHAR)Address;	
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		Status = FALSE;
	}

	return Status;
}

ULONG
BtrGetByteOffset(
	IN PVOID Address
	)
{
	return (ULONG)((ULONG_PTR)Address & (BtrPageSize - 1));
}

SIZE_T 
BtrConvertUnicodeToUTF8(
	IN PWSTR Source, 
	OUT PCHAR *Destine
	)
{
	int Length;
	int RequiredLength;
	PCHAR Buffer;

	Length = (int)wcslen(Source) + 1;
	RequiredLength = WideCharToMultiByte(CP_UTF8, 0, Source, Length, 0, 0, 0, 0);

	Buffer = (PCHAR)BtrFltMalloc(RequiredLength);
	Buffer[0] = 0;

	WideCharToMultiByte(CP_UTF8, 0, Source, Length, Buffer, RequiredLength, 0, 0);
	*Destine = Buffer;

	return Length;
}

VOID 
BtrConvertUTF8ToUnicode(
	IN PCHAR Source,
	OUT PWCHAR *Destine
	)
{
	int Length;
	PWCHAR Buffer;

	Length = (int)strlen(Source) + 1;
	Buffer = (PWCHAR)BtrFltMalloc(Length * sizeof(WCHAR));
	Buffer[0] = L'0';

	MultiByteToWideChar(CP_UTF8, 0, Source, Length, Buffer, Length);
	*Destine = Buffer;
}

VOID 
DebugTrace(
	IN PSTR Format,
	...
	)
{
#ifdef _DEBUG
	va_list arg;
	char format[512];
	char buffer[512];
	
	va_start(arg, Format);

	StringCchVPrintfA(format, 512, Format, arg);
	StringCchPrintfA(buffer, 512, "[btr]: %s\n", format);
	OutputDebugStringA(buffer);
	
	va_end(arg);
#endif
}

VOID WINAPI
BtrFormatError(
	IN ULONG ErrorCode,
	IN BOOLEAN SystemError,
	OUT PWCHAR Buffer,
	IN ULONG BufferLength, 
	OUT PULONG ActualLength
	)
{
	Buffer[0] = 0;

	if (SystemError) {

		StringCchPrintf(Buffer, BufferLength, L"0x%08x", ErrorCode);

		if (ActualLength != NULL) {
			*ActualLength = (ULONG)wcslen(Buffer);
		}
		return;
	}

	switch (ErrorCode) {
		case BTR_E_UNEXPECTED:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_UNEXPECTED");
			break;
		
		case BTR_E_INIT_FAILED:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_INIT_FAILED");
			break;
		
		case BTR_E_BAD_PARAMETER:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_BAD_PARAMETER");
			break;
		
		case BTR_E_IO_ERROR:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_IO_ERROR");
			break;
		
		case BTR_E_BAD_MESSAGE:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_BAD_MESSAGE");
			break;
		
		case BTR_E_EXCEPTION:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_EXCEPTION");
			break;

		case BTR_E_BUFFER_LIMITED:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_BUFFER_LIMITED");
			break;
		
		case BTR_E_UNEXPECTED_MESSAGE:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_UNEXPECTED_MESSAGE");
			break;

		case BTR_E_OUTOFMEMORY:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_OUTOFMEMORY");
			break;

		case BTR_E_PORT_BROKEN:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_PORT_BROKEN");
			break;
		
		case BTR_E_INVALID_ARGUMENT:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_INVALID_ARGUMENT");
			break;

		case BTR_E_PROBE_COLLISION:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_PROBE_COLLISION");
			break;
		
		case BTR_E_CANNOT_PROBE:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_CANNOT_PROBE");
			break;
		
		case BTR_E_OUTOF_PROBE:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_OUTOF_PROBE");
			break;
		
		case BTR_E_ACCESSDENIED:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_ACCESSDENIED");
			break;

		case BTR_E_FAILED_LOAD_FILTER:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_FAILED_LOAD_FILTER");
			break;
		
		case BTR_E_FAILED_GET_FILTER_API:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_FAILED_GET_FILTER_API");
			break;

		case BTR_E_FILTER_REGISTRATION:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_FILTER_REGISTRATION");
			break;
		
		case BTR_E_LOADLIBRARY:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_LOADLIBRARY");
			break;

		case BTR_E_GETPROCADDRESS:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_GETPROCADDRESS");
			break;
		
		case BTR_E_UNKNOWN_MESSAGE:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_UNKNOWN_MESSAGE");
			break;

		case BTR_E_DISASTER:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_DISASTER");
			break;
		
		case BTR_S_MORE_DATA:
			StringCchCopy(Buffer, BufferLength, L"BTR_S_MORE_DATA");
			break;

		default:
			StringCchCopy(Buffer, BufferLength, L"BTR_E_UNKNOWN");
			break;
	}

	if (ActualLength != NULL) {
		*ActualLength = (ULONG)wcslen(Buffer);
	}

	return;
}

ULONG 
BtrCompareMemoryPointer(
	IN PVOID Source,
	IN PVOID Destine,
	IN ULONG NumberOfUlongPtr
	)
{
	ULONG Number;

	for(Number = 0; Number < NumberOfUlongPtr; Number += 1) {
		if (*(PULONG_PTR)Source != *(PULONG_PTR)Destine) {
			return Number;
		}
	}

	return Number;
}

ULONG
BtrComputeAddressHash(
	IN PVOID Address,
	IN ULONG Limit
	)
{
	ULONG Hash;

	Hash = (ULONG)((ULONG_PTR)Address ^ (ULONG_PTR)GOLDEN_RATIO);
	Hash = (Hash >> 24) ^ (Hash >> 16) ^ (Hash >> 8) ^ (Hash);

	return Hash % Limit;
}