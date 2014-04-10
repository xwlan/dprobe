//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#ifndef _FIND_H_
#define _FIND_H_

#include "dprobe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FIND_NOT_FOUND (ULONG)-1

struct _MSP_DTL_OBJECT;
struct _FIND_CONTEXT;

typedef ULONG 
(*FIND_CALLBACK)(
   IN struct _FIND_CONTEXT *Object
);

typedef struct _FIND_CONTEXT {
	HWND hWndList;
	BOOLEAN FindForward;
	ULONG PreviousIndex;
	WCHAR FindString[MAX_PATH];
    struct _MSP_DTL_OBJECT *DtlObject;
    FIND_CALLBACK FindCallback;
} FIND_CONTEXT, *PFIND_CONTEXT;

BOOLEAN
FindIsComplete(
	IN struct _FIND_CONTEXT *Context,
	IN struct _MSP_DTL_OBJECT *DtlObject,
	IN ULONG Current
	);

#ifdef __cplusplus
}
#endif

#endif