//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2014
//

#ifndef _DBGFLT_H_
#define _DBGFLT_H_

#ifdef __cplusplus 
extern "C" {
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#define _BTR_SDK_
#include "btrsdk.h"
#include <strsafe.h>

#ifndef ASSERT
 #include <assert.h>
 #define ASSERT assert
#endif

#define DbgMajorVersion 1
#define DbgMinorVersion 0

//
// UUID identify DBG filter
//

extern UUID DbgUuid;

typedef enum _DBG_PROBE_ORDINAL {

	_OutputDebugStringW,
	_OutputDebugStringA,

	_DbgPrint,
	_DbgPrintEx,

	_fprintf,
	_fwprintf,
	_vfprintf,
	_vfwprintf,
	
	//
	// vc9 (VS 2008) ...
	//

	_fprintf9,
	_fwprintf9,
	_vfprintf9,
	_vfwprintf9,
	
	_fprintf9d,
	_fwprintf9d,
	_vfprintf9d,
	_vfwprintf9d,

	_fprintf10,
	_fwprintf10,
	_vfprintf10,
	_vfwprintf10,

	_fprintf10d,
	_fwprintf10d,
	_vfprintf10d,
	_vfwprintf10d,

	_fprintf11,
	_fwprintf11,
	_vfprintf11,
	_vfwprintf11,

	_fprintf11d,
	_fwprintf11d,
	_vfprintf11d,
	_vfwprintf11d,

	_fprintf12,
	_fwprintf12,
	_vfprintf12,
	_vfwprintf12,

	_fprintf12d,
	_fwprintf12d,
	_vfprintf12d,
	_vfwprintf12d,

	DBG_PROBE_NUMBER,

} DBG_PROBE_ORDINAL, *PDBG_PROBE_ORDINAL;


typedef struct _DBG_OUTPUTW {
	USHORT Length;
	WCHAR Text[ANYSIZE_ARRAY];
} DBG_OUTPUTW, *PDBG_OUTPUTW;

typedef struct _DBG_OUTPUTA {
	USHORT Length;
	CHAR Text[ANYSIZE_ARRAY];
} DBG_OUTPUTA, *PDBG_OUTPUTA;

typedef VOID (WINAPI *OUTPUTDEBUGSTRINGW)(LPCWSTR lpstr);
typedef VOID (WINAPI *OUTPUTDEBUGSTRINGA)(LPCSTR lpstr);

void WINAPI 
OutputDebugStringWEntry(
    __in_opt  LPCTSTR lpstr
	);

void WINAPI 
OutputDebugStringAEntry(
    __in_opt LPCSTR lpstr
	);

//
// ntdll!DbgPrint
// ntdll!DbgPrintEx
//

//
// N.B. DbgPrint function pointer has different
//      prototype with DbgPrint, because we don't
//      know argument count, so maximum 16 arguments
//      are supported. 
//

typedef ULONG (__cdecl *DBGPRINT)(
	__in PCHAR Format,
	__in ULONG_PTR Sp0,
	__in ULONG_PTR Sp1,
	__in ULONG_PTR Sp2,
	__in ULONG_PTR Sp3,
	__in ULONG_PTR Sp4,
	__in ULONG_PTR Sp5,
	__in ULONG_PTR Sp6,
	__in ULONG_PTR Sp7,
	__in ULONG_PTR Sp8,
	__in ULONG_PTR Sp9,
	__in ULONG_PTR Sp10,
	__in ULONG_PTR Sp11,
	__in ULONG_PTR Sp12,
	__in ULONG_PTR Sp13,
	__in ULONG_PTR Sp14
	);

typedef ULONG (__cdecl *DBGPRINTEX)(
	__in ULONG ComponentId, 
	__in ULONG Level, 
	__in PCHAR Format, 
	__in ULONG_PTR Sp0,
	__in ULONG_PTR Sp1,
	__in ULONG_PTR Sp2,
	__in ULONG_PTR Sp3,
	__in ULONG_PTR Sp4,
	__in ULONG_PTR Sp5,
	__in ULONG_PTR Sp6,
	__in ULONG_PTR Sp7,
	__in ULONG_PTR Sp8,
	__in ULONG_PTR Sp9,
	__in ULONG_PTR Sp10,
	__in ULONG_PTR Sp11,
	__in ULONG_PTR Sp12
	);

ULONG __cdecl
DbgPrintEntry(
    __in PCHAR Format,
	...
    );

ULONG __cdecl
DbgPrintExEntry(
    __in ULONG ComponentId,
    __in ULONG Level,
    __in PCHAR Format,
	...
    );

//
// N.B. These routines are commonly used to output 
//      debug message to log file
//
// fprintf
// fwprintf
// vfprintf
// vfwprintf
//

typedef ULONG (__cdecl *FPRINTF)(
	__in FILE *stream,
	__in const char *,
	__in ULONG_PTR Sp0,
	__in ULONG_PTR Sp1,
	__in ULONG_PTR Sp2,
	__in ULONG_PTR Sp3,
	__in ULONG_PTR Sp4,
	__in ULONG_PTR Sp5,
	__in ULONG_PTR Sp6,
	__in ULONG_PTR Sp7,
	__in ULONG_PTR Sp8,
	__in ULONG_PTR Sp9,
	__in ULONG_PTR Sp10,
	__in ULONG_PTR Sp11,
	__in ULONG_PTR Sp12,
	__in ULONG_PTR Sp13
	);

typedef ULONG (__cdecl *FWPRINTF)(
	__in FILE *stream,
	__in const wchar_t *,
	__in ULONG_PTR Sp0,
	__in ULONG_PTR Sp1,
	__in ULONG_PTR Sp2,
	__in ULONG_PTR Sp3,
	__in ULONG_PTR Sp4,
	__in ULONG_PTR Sp5,
	__in ULONG_PTR Sp6,
	__in ULONG_PTR Sp7,
	__in ULONG_PTR Sp8,
	__in ULONG_PTR Sp9,
	__in ULONG_PTR Sp10,
	__in ULONG_PTR Sp11,
	__in ULONG_PTR Sp12,
	__in ULONG_PTR Sp13
	);

typedef ULONG (__cdecl *VFPRINTF)(
	__in FILE *stream,
	__in const char *,
	__in va_list argptr 
	);

typedef ULONG (__cdecl *VFWPRINTF)(
	__in FILE *stream,
	__in const wchar_t *,
	__in va_list argptr 
	);

int __cdecl 
fprintfEntry( 
	FILE *stream,
	const char *format,
	...
	);

int __cdecl 
fwprintfEntry( 
	FILE *stream,
	const wchar_t *format,
	...
	);

int __cdecl
vfprintfEntry(
	FILE *stream,
	const char *format,
	va_list argptr 
	);

int __cdecl 
vfwprintfEntry(
	FILE *stream,
	const wchar_t *format,
	va_list argptr 
	);

//
// Unified Decode Routines
//

ULONG
DbgStringWDecode(
	__in PBTR_FILTER_RECORD Record,
	__in ULONG MaxCount,
	__out PWCHAR Buffer
	);

ULONG
DbgStringADecode(
	__in PBTR_FILTER_RECORD Record,
	__in ULONG MaxCount,
	__out PWCHAR Buffer
	);

//
// Helpers to format variable string
//

SIZE_T
DbgFormatStringA(
	__in PCHAR Buffer,
	__in SIZE_T Length,
	__in PCHAR Format,
	__in va_list ArgPtr
	);

SIZE_T
DbgFormatStringW(
	__in PWCHAR Buffer,
	__in SIZE_T Length,
	__in PWCHAR Format,
	__in va_list ArgPtr
	);

#ifdef __cplusplus
}
#endif

#endif