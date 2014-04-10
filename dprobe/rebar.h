//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _REBAR_H_
#define _REBAR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sdk.h"

typedef enum _REBAR_BAND_ID {
	REBAR_BAND_FINDBAR,
	REBAR_BAND_EDIT,
} REBAR_BAND_ID, *PREBAR_BAND_ID;

typedef struct _REBAR_BAND {
	HWND hWndBand;
	REBAR_BAND_ID BandId;
	HIMAGELIST himlNormal;
	HIMAGELIST himlGrayed;
	LIST_ENTRY ListEntry;
	REBARBANDINFO BandInfo;
} REBAR_BAND, *PREBAR_BAND;

typedef struct _REBAR_OBJECT {
	HWND hWndRebar;
	REBARINFO RebarInfo;
	ULONG NumberOfBands;
	LIST_ENTRY BandList;
} REBAR_OBJECT, *PREBAR_OBJECT;

PREBAR_OBJECT
RebarCreateObject(
	IN HWND hWndParent, 
	IN UINT Id 
	);

HWND
RebarCreateFindbar(
	IN PREBAR_OBJECT Object	
	);

HWND
RebarCreateEdit(
	IN PREBAR_OBJECT Object	
	);

BOOLEAN
RebarSetFindbarImage(
	IN HWND hWndFindbar,
	IN COLORREF Mask,
	OUT HIMAGELIST *Normal,
	OUT HIMAGELIST *Grayed
	);

BOOLEAN
RebarInsertBands(
	IN PREBAR_OBJECT Object
	);

VOID
RebarAdjustPosition(
	IN PREBAR_OBJECT Object
	);

VOID
RebarCurrentEditString(
	IN PREBAR_OBJECT Object,
	OUT PWCHAR Buffer,
	IN ULONG Length
	);

#ifdef __cplusplus
}
#endif

#endif