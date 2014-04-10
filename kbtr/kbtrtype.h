//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#ifndef _KBTR_TYPE_H_
#define _KBTR_TYPE_H_

#include "kbtr.h"

#define DRIVER_FUNC_INSTALL     0x01
#define DRIVER_FUNC_REMOVE      0x02

//
// Device Extension
//

typedef struct _KBTR_DEVICE_EXTENSION {
	LIST_ENTRY PrimaryBufferList;
	LIST_ENTRY SecondaryBufferList;
	HANDLE CompleteEvent;
} KBTR_DEVICE_EXTENSION, *PKBTR_DEVICE_EXTENSION;

#endif  // KBTR_TYPE_H_