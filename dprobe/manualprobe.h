//
// Apsaras Labs
// lan.john@gmail.com
// Copyright(C) 2009-2010
//

#ifndef _MANUALPROBE_H_
#define _MANUALPROBE_H_

#include "psp.h"

PPSP_MANUAL_PROBE
PspCreateManualProbe(
	IN PWSTR ModuleName,
	IN PWSTR MethodName,
	IN PWSTR UndMethodName
	);

VOID
PspFreeManualProbe(
	IN PPSP_MANUAL_PROBE Object
	);

#endif