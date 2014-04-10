//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#ifndef _MAIN_H_
#define _MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

extern HWND hWndMain;

BOOLEAN
MainInitialize(
	VOID
	);

int
MainMessagePump(
	VOID
	);

#ifdef __cplusplus
}
#endif

#endif