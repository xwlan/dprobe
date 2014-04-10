//
// Apsaras Labs
// lan.john@gmail.com
// Copyright(C) 2009-2010
//

#include "psp.h"
#include "manualprobe.h"

VOID
PspFreeManualProbe(
	IN PPSP_MANUAL_PROBE Object
	)
{
	ASSERT(Object != NULL);

	if (Object->Cache != NULL) {
		BspFree(Object->Cache);
	}
	BspFree(Object);
}

PPSP_MANUAL_PROBE
PspCreateManualProbe(
	IN PWSTR ModuleName,
	IN PWSTR MethodName,
	IN PWSTR UndMethodName
	)
{
	PPSP_MANUAL_PROBE Object;

	Object = (PPSP_MANUAL_PROBE)BspMalloc(sizeof(PSP_MANUAL_PROBE));
	Object->State = ProbeCreating;

	StringCchCopy(Object->ModuleName, MAX_MODULE_NAME_LENGTH, ModuleName);
	StringCchCopy(Object->MethodName, MAX_FUNCTION_NAME_LENGTH, MethodName);
	StringCchCopy(Object->UndMethodName, MAX_FUNCTION_NAME_LENGTH, UndMethodName);

	return Object;
}