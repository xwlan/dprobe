//
// lan.john@gmail.com
// Adbgara Labs
// Copyright(C) 2009-2014
//

#include "dbgflt.h"

//
// OutputDebugStringW
//

void WINAPI 
OutputDebugStringWEntry(
    __in_opt  LPCTSTR lpstr
	)
{
	PDBG_OUTPUTW Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	OUTPUTDEBUGSTRINGW Routine;
	size_t Length;
	ULONG UserLength;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	
	BtrFltEnterExemptionRegion();
	(*Routine)(lpstr);
	BtrFltLeaveExemptionRegion();

	BtrFltSetContext(&Context);

	__try {

		Length = lpstr ? (wcslen(lpstr) + 1) : 1;
		UserLength = FIELD_OFFSET(DBG_OUTPUTW, Text[Length]);
		Record = BtrFltAllocateRecord(UserLength, DbgUuid, _OutputDebugStringW);
		if (!Record) {
			return;
		}

		Filter = FILTER_RECORD_POINTER(Record, DBG_OUTPUTW);
		Filter->Length = Length;

		if (Length > 1) {
			StringCchCopyW(Filter->Text, Length, lpstr);
		}

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}
}

//
// OutputDebugStringA
//

void WINAPI 
OutputDebugStringAEntry(
    __in_opt LPCSTR lpstr
	)
{
	PDBG_OUTPUTA Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	OUTPUTDEBUGSTRINGA Routine;
	size_t Length;
	ULONG UserLength;
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	
	BtrFltEnterExemptionRegion();
	(*Routine)(lpstr);
	BtrFltLeaveExemptionRegion();

	BtrFltSetContext(&Context);

	__try {

		Length = lpstr ? (strlen(lpstr) + 1) : 1;
		UserLength = FIELD_OFFSET(DBG_OUTPUTA, Text[Length]);
		Record = BtrFltAllocateRecord(UserLength, DbgUuid, _OutputDebugStringA);
		if (!Record) {
			return;
		}

		Filter = FILTER_RECORD_POINTER(Record, DBG_OUTPUTA);
		Filter->Length = Length;

		if (Length > 1) {
			StringCchCopyA(Filter->Text, Length, lpstr);
		}

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}
}

//
// ntdll!DbgPrint 
//

ULONG __cdecl
DbgPrintEntry(
    __in PCHAR Format,
	...
	)
{
	PDBG_OUTPUTA Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	DBGPRINT Routine;
	size_t Length;
	ULONG UserLength;
	va_list arg;
	PULONG_PTR Sp;
	ULONG Result;
	char format[512];
	char buffer[512];
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;

	//
	// N.B. Maximum support 16 arguments
	//

	Sp = (PULONG_PTR)&Format + 1;
	Result = (*Routine)(Format, Sp[0], Sp[1], Sp[2], Sp[3], Sp[4],
			   Sp[5], Sp[6], Sp[7], Sp[8], Sp[9], 
			   Sp[10], Sp[11], Sp[12], Sp[13], Sp[14]);

	BtrFltSetContext(&Context);

	__try {

		va_start(arg, Format);
		StringCchVPrintfA(format, 512, Format, arg);
		StringCchPrintfA(buffer, 512, "%s", format);
		Length = strlen(buffer) + 1;
		va_end(arg);

		UserLength = FIELD_OFFSET(DBG_OUTPUTA, Text[Length]);
		Record = BtrFltAllocateRecord(UserLength, DbgUuid, _DbgPrint);
		if (!Record) {
			return Result;
		}

		Filter = FILTER_RECORD_POINTER(Record, DBG_OUTPUTA);
		Filter->Length = Length;
		StringCchCopyA(Filter->Text, Length, buffer);

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Result;
}

ULONG __cdecl
DbgPrintExEntry(
	__in ULONG ComponentId,
	__in ULONG Level,
    __in PCHAR Format,
	...
	)
{
	PDBG_OUTPUTA Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	DBGPRINTEX Routine;
	size_t Length;
	ULONG UserLength;
	va_list arg;
	ULONG Result;
	PULONG_PTR Sp;
	char format[512];
	char buffer[512];
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;

	//
	// N.B. Maximum support 16 arguments
	//

	Sp = (PULONG_PTR)&Format + 1;
	Result = (*Routine)(ComponentId, Level, Format, Sp[0], Sp[1], Sp[2], Sp[3], Sp[4],
			   Sp[5], Sp[6], Sp[7], Sp[8], Sp[9], Sp[10], Sp[11], Sp[12]);
			   
	BtrFltSetContext(&Context);

	__try {

		va_start(arg, Format);
		StringCchVPrintfA(format, 512, Format, arg);
		StringCchPrintfA(buffer, 512, "%s", format);
		Length = strlen(buffer) + 1;
		va_end(arg);

		UserLength = FIELD_OFFSET(DBG_OUTPUTA, Text[Length]);
		Record = BtrFltAllocateRecord(UserLength, DbgUuid, _DbgPrintEx);
		if (!Record) {
			return Result;
		}

		Filter = FILTER_RECORD_POINTER(Record, DBG_OUTPUTA);
		Filter->Length = Length;
		StringCchCopyA(Filter->Text, Length, buffer);

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Result;
}

int __cdecl 
fprintfEntry( 
	FILE *stream,
	const char *format,
	...
	)
{
	PDBG_OUTPUTA Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	FPRINTF Routine;
	size_t Length;
	ULONG UserLength;
	va_list argptr;
	PULONG_PTR Sp;
	ULONG Result;
	char buffer[512];
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;

	//
	// N.B. Maximum support 16 arguments
	//

	Sp = (PULONG_PTR)&format + 1;
	Result = (*Routine)(stream, format, Sp[0], Sp[1], Sp[2], Sp[3], Sp[4],
			   Sp[5], Sp[6], Sp[7], Sp[8], Sp[9], 
			   Sp[10], Sp[11], Sp[12], Sp[13]);

	BtrFltSetContext(&Context);

	__try {

		va_start(argptr, format);
		StringCchVPrintfA(buffer, 512, format, argptr);
		Length = strlen(buffer) + 1;
		va_end(argptr);

		UserLength = FIELD_OFFSET(DBG_OUTPUTA, Text[Length]);
		Record = BtrFltAllocateRecord(UserLength, DbgUuid, _fprintf);
		if (!Record) {
			return Result;
		}

		Filter = FILTER_RECORD_POINTER(Record, DBG_OUTPUTA);
		Filter->Length = Length;
		StringCchCopyA(Filter->Text, Length, buffer);

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Result;
}

int __cdecl 
fwprintfEntry( 
	FILE *stream,
	const wchar_t *format,
	...
	)
{
	PDBG_OUTPUTW Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	FWPRINTF Routine;
	size_t Length;
	ULONG UserLength;
	va_list argptr;
	PULONG_PTR Sp;
	ULONG Result;
	WCHAR buffer[512];
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;

	//
	// N.B. Maximum support 16 arguments
	//

	Sp = (PULONG_PTR)&format + 1;
	Result = (*Routine)(stream, format, Sp[0], Sp[1], Sp[2], Sp[3], Sp[4],
			   Sp[5], Sp[6], Sp[7], Sp[8], Sp[9], 
			   Sp[10], Sp[11], Sp[12], Sp[13]);

	BtrFltSetContext(&Context);

	__try {

		va_start(argptr, format);
		StringCchVPrintfW(buffer, 512, format, argptr);
		Length = wcslen(buffer) + 1;
		va_end(argptr);

		UserLength = FIELD_OFFSET(DBG_OUTPUTW, Text[Length]);
		Record = BtrFltAllocateRecord(UserLength, DbgUuid, _fwprintf);
		if (!Record) {
			return Result;
		}

		Filter = FILTER_RECORD_POINTER(Record, DBG_OUTPUTW);
		Filter->Length = Length;
		StringCchCopyW(Filter->Text, Length, buffer);

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Result;
}

int __cdecl
vfprintfEntry(
	FILE *stream,
	const char *format,
	va_list argptr 
	)
{
	PDBG_OUTPUTA Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	VFPRINTF Routine;
	size_t Length;
	ULONG UserLength;
	ULONG Result;
	char buffer[512];
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Result = (*Routine)(stream, format, argptr);
	BtrFltSetContext(&Context);

	__try {

		StringCchVPrintfA(buffer, 512, format, argptr);
		Length = strlen(buffer) + 1;

		UserLength = FIELD_OFFSET(DBG_OUTPUTA, Text[Length]);
		Record = BtrFltAllocateRecord(UserLength, DbgUuid, _vfprintf);
		if (!Record) {
			return Result;
		}

		Filter = FILTER_RECORD_POINTER(Record, DBG_OUTPUTA);
		Filter->Length = Length;
		StringCchCopyA(Filter->Text, Length, buffer);

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Result;
}

int __cdecl 
vfwprintfEntry(
	FILE *stream,
	const wchar_t *format,
	va_list argptr 
	)
{
	PDBG_OUTPUTW Filter;
	PBTR_FILTER_RECORD Record;
	BTR_PROBE_CONTEXT Context;
	VFWPRINTF Routine;
	size_t Length;
	ULONG UserLength;
	ULONG Result;
	WCHAR buffer[512];
	
	BtrFltGetContext(&Context);
	Routine = Context.Destine;
	Result = (*Routine)(stream, format, argptr);
	BtrFltSetContext(&Context);

	__try {

		StringCchVPrintfW(buffer, 512, format, argptr);
		Length = wcslen(buffer) + 1;

		UserLength = FIELD_OFFSET(DBG_OUTPUTW, Text[Length]);
		Record = BtrFltAllocateRecord(UserLength, DbgUuid, _vfwprintf);
		if (!Record) {
			return Result;
		}

		Filter = FILTER_RECORD_POINTER(Record, DBG_OUTPUTW);
		Filter->Length = Length;
		StringCchCopyW(Filter->Text, Length, buffer);

		BtrFltQueueRecord(Record);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (Record) {
			BtrFltFreeRecord(Record);
		}		
	}

	return Result;
}

ULONG
DbgStringWDecode(
	__in PBTR_FILTER_RECORD Record,
	__in ULONG MaxCount,
	__out PWCHAR Buffer
	)
{
	PDBG_OUTPUTW Data;

	if (Record->ProbeOrdinal >= DBG_PROBE_NUMBER) {
		return S_FALSE;
	}	

	Data = (PDBG_OUTPUTW)Record->Data;

	if (Data->Length > 1) {
		StringCchPrintf(Buffer, MaxCount, L"%s", Data->Text);
	} else {
		StringCchPrintf(Buffer, MaxCount, L"<null>"); 
	}

	return S_OK;
}

ULONG
DbgStringADecode(
	__in PBTR_FILTER_RECORD Record,
	__in ULONG MaxCount,
	__out PWCHAR Buffer
	)
{
	PDBG_OUTPUTA Data;

	if (Record->ProbeOrdinal >= DBG_PROBE_NUMBER) {
		return S_FALSE;
	}	

	Data = (PDBG_OUTPUTA)Record->Data;

	if (Data->Length > 1) {
		StringCchPrintf(Buffer, MaxCount, L"%S", Data->Text);
	} else {
		StringCchPrintf(Buffer, MaxCount, L"<null>"); 
	}

	return S_OK;
}
