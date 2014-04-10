//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _MSPDEFS_H_
#define _MSPDEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#pragma warning(disable : 4995)
#pragma warning(disable : 4996)

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <assert.h>
#include "list.h"
#include "bitmap.h"
#include "btr.h"

//
// MSP Status
//

#define	MSP_STATUS_OK					0x00000000	
#define	MSP_STATUS_IPC_BROKEN			0xE1000000
#define	MSP_STATUS_BUFFER_LIMIT			0xE1000001
#define	MSP_STATUS_STOPPED				0xE1000002
#define	MSP_STATUS_INIT_FAILED			0xE1000003
#define	MSP_STATUS_EXCEPTION			0xE1000004
#define	MSP_STATUS_WOW64_ERROR			0xE1000005  
#define	MSP_STATUS_INVALID_PARAMETER	0xE1000006
#define MSP_STATUS_NO_FILTER            0xE1000007
#define MSP_STATUS_FILTER_FAILURE       0xE1000008
#define MSP_STATUS_LOG_FAILURE          0xE1000009
#define MSP_STATUS_ACCESS_DENIED        0xE1000010
#define MSP_STATUS_NO_UUID              0xE1000011
#define MSP_STATUS_NO_DECODE            0xE1000012
#define MSP_STATUS_BAD_RECORD           0xE1000013 
#define MSP_STATUS_OUTOFMEMORY          0xE1000014
#define MSP_STATUS_SETFILEPOINTER       0xE1000015
#define MSP_STATUS_NO_RECORD            0xE1000016
#define MSP_STATUS_BUFFER_TOO_SMALL     0xE1000017
#define MSP_STATUS_NO_SEQUENCE          0xE1000018
#define MSP_STATUS_MAPVIEWOFFILE        0xE1000019
#define MSP_STATUS_CREATEFILEMAPPING    0xE1000020
#define MSP_STATUS_CREATEFILE           0xE1000021
#define MSP_STATUS_NOT_IMPLEMENTED      0xE1000022
#define MSP_STATUS_BUCKET_FULL          0xE1000023
#define MSP_STATUS_TIMEOUT              0xE1000024
#define MSP_STATUS_BAD_FILE             0xE1000025
#define MSP_STATUS_BAD_CHECKSUM         0xE1000026
#define MSP_STATUS_NOT_FOUND            0xE1000027
#define MSP_STATUS_REQUIRE_32BITS       0xE1000028
#define MSP_STATUS_REQUIRE_64BITS       0xE1000029
#define	MSP_STATUS_ERROR				0xFFFFFFFF

//
// MSP Mode
//

typedef enum _MSP_MODE {
	MSP_MODE_LIB,   
	MSP_MODE_DLL,  
} MSP_MODE, *PMSP_MODE;

//
// Misc Macros
//

#define MSP_CACHE_SIZE 0x400000
#define	MSP_PAGE_SIZE  0x1000

//
// Flag Macros
//

#define FlagOn(F,SF)  ((F) & (SF))     
#define SetFlag(F,SF)   { (F) |= (SF);  }
#define ClearFlag(F,SF) { (F) &= ~(SF); }

//
// Debug Routine
//

VOID __cdecl
DebugTrace(
	IN PSTR Format,
	...
	);
	
#ifndef ASSERT
#ifdef _DEBUG
	#define ASSERT assert
#else
	#define ASSERT
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif