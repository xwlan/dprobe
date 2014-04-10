//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2010
//

#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// N.B. The parameters must be aligned on a 32 bits boundary, 64 bits 
// parameters must be aligned on a 64 bits boundary, otherwise, they
// will behave unpredictably on multiprocessor x86 system.
//

BOOLEAN 
AtomicCompareAndSwap(
	IN volatile LONG_PTR *Destine,
	IN LONG_PTR Comparand,
	IN LONG_PTR Exchange
	);

LONG_PTR
AtomicRead(
	IN volatile LONG_PTR *Address
	);

LONG_PTR
AtomicWrite(
	IN volatile LONG_PTR *Address,
	IN LONG_PTR Exchange,
	IN LONG_PTR Comparand
	);

LONG_PTR
AtomicIncrement(
	IN volatile LONG_PTR *Addend
	);

LONG_PTR
AtomicDecrement(
	IN volatile LONG_PTR *Addend
	);

#ifdef __cplusplus
}
#endif

#endif