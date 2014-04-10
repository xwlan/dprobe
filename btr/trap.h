//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _TRAP_H_
#define _TRAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "btr.h"
#include "decode.h"
#include "probe.h"

#pragma pack(push, 1)
#if defined(_M_IX86) 

typedef struct _BTR_TRAP{

	ULONG TrapFlag;
	PVOID Probe;                
	PVOID TrapProcedure;
	PVOID BackwardAddress;
	ULONG HijackedLength;
	CHAR OriginalCopy[16];
	CHAR HijackedCode[32];

	UCHAR MovEsp4[4];
	ULONG Zero;
	UCHAR MovEsp8[4];
	PVOID Object;
	UCHAR Jmp[2];
	PVOID Procedure;
	UCHAR Ret;

} BTR_TRAP, *PBTR_TRAP;

#elif defined (_M_X64) 

typedef struct _BTR_TRAP{

	ULONG TrapFlag;
	PVOID Probe;                
	PVOID TrapProcedure;
	PVOID BackwardAddress;
	ULONG HijackedLength;
	CHAR OriginalCopy[16];
	CHAR HijackedCode[32];

	UCHAR MovRsp8[5];
	ULONG Zero1;
	UCHAR MovRsp10[5];
	ULONG Zero2;
	UCHAR MovRsp18[4];
	ULONG ObjectLowPart;
	CHAR MovRsp1C[4];
	ULONG ObjectHighPart;
	UCHAR Jmp[2];
	ULONG Rip;
	PVOID Procedure;
	UCHAR Ret;

} BTR_TRAP, *PBTR_TRAP;		

#endif
#pragma pack(pop)

typedef struct _BTR_TRAP_PAGE {
	LIST_ENTRY ListEntry;
	PVOID StartVa;
	PVOID EndVa;
	BTR_BITMAP BitMap;
	BTR_TRAP Object[ANYSIZE_ARRAY];
} BTR_TRAP_PAGE, *PBTR_TRAP_PAGE;


ULONG
BtrInitializeTrap(
	VOID
	);

VOID
BtrUninitializeTrap(
	VOID
	);

PBTR_TRAP_PAGE
BtrAllocateTrapPage(
	IN ULONG_PTR SourceAddress
	);

VOID
BtrInitializeTrapPage(
	IN PBTR_TRAP_PAGE TrapPage 
	);

PBTR_TRAP_PAGE
BtrScanTrapPageList(
	IN ULONG_PTR SourceAddress
	);

BOOLEAN
BtrIsTrapPage(
	IN PVOID Address
	);

PBTR_TRAP
BtrAllocateTrap(
	IN ULONG_PTR SourceAddress
	);

VOID
BtrFreeTrap(
	IN PBTR_TRAP Trap
	);

ULONG
BtrFuseTrap(
	IN PVOID Address, 
	IN PBTR_DECODE_CONTEXT Decode,
	IN PBTR_TRAP Trap
	);

ULONG
BtrFuseTrapLite(
	IN PVOID Address, 
	IN PVOID Destine,
	IN PBTR_TRAP Trap
	);

extern LIST_ENTRY BtrTrapPageList;
extern BTR_TRAP BtrTrapTemplate;

#ifdef __cplusplus
}
#endif
#endif