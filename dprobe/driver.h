//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _DRIVER_H_
#define _DRIVER_H_

#include "dprobe.h"
#include "kbtr.h"

typedef enum _DRIVER_OPERATION {
	OperationInstall,
	OperationRemove,
} DRIVER_OPERATION, *PDRIVER_OPERATION;

#define BTR_DRIVER_NAME     L"dprobe.kbtr.sys"
#define BTR_DOSDEVICE_NAME  L"kbtr"

extern HANDLE BspDriverHandle;

ULONG
BspConnectDriver(
	IN PWSTR DriverName,
	IN PWSTR DosDeviceName, 
	OUT PHANDLE DeviceHandle
	);

ULONG
BspManageDriver(
    IN PWSTR ServiceName,
    IN PWSTR DriverPath,
    IN DRIVER_OPERATION Operation 
    );

ULONG
BspInstallDriver(
	IN SC_HANDLE ScmHandle,
	IN PWSTR ServiceName,
	IN PWSTR DriverPath 
	);

ULONG
BspUninstallDriver(
    IN SC_HANDLE ScmHandle,
    IN PWSTR ServiceName
    );

ULONG
BspStartDriver(
    IN SC_HANDLE ScmHandle,
    IN PWSTR ServiceName
    );

ULONG
BspStopDriver(
    IN SC_HANDLE ScmHandle,
    IN PWSTR ServiceName
    );

ULONG
BspBuildDriverName(
	IN PWSTR DriverBaseName,
    IN PWCHAR FullPathName,
    IN ULONG BufferLength
    );

#endif