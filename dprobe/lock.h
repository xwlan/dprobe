//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2010
//

#ifndef _LOCK_H_
#define _LOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

typedef struct _SPINLOCK {
	volatile ULONG DECLSPEC_ALIGN(8) Acquired;
	volatile ULONG OwnerThreadId;
} SPINLOCK, *PSPINLOCK;

typedef SPINLOCK   BTR_SPINLOCK;
typedef PSPINLOCK  PBTR_SPINLOCK;

VOID
BtrInitSpinLock(
	IN PBTR_SPINLOCK Lock
	);

VOID
BtrAcquireSpinLock(
	IN PBTR_SPINLOCK Lock
	);

VOID
BtrReleaseSpinLock(
	IN PBTR_SPINLOCK Lock
	);

#ifdef __cplusplus
}
#endif

#endif