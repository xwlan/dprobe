//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "driver.h"

HANDLE BspDriverHandle;

ULONG
BspConnectDriver(
	IN PWSTR DriverName,
	IN PWSTR DosDeviceName, 
	OUT PHANDLE DeviceHandle
	)
{
	ULONG Status;
	HANDLE FileHandle;
	WCHAR DeviceName[MAX_PATH];
	WCHAR DriverLocation[MAX_PATH];

	StringCchPrintf(DeviceName, MAX_PATH, L"\\\\.\\%ws", DosDeviceName);

    FileHandle = CreateFile(DeviceName, GENERIC_READ | GENERIC_WRITE, 0,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	//
	// Succeed to open the device object
	//

	if (FileHandle != INVALID_HANDLE_VALUE) {
		*DeviceHandle = FileHandle;
		return S_OK;
	}

	//
	// Device name exist, but failed to open
	//

    Status = GetLastError();
    if (Status != ERROR_FILE_NOT_FOUND) {
        return Status;
    }

    //
    // The driver is not started yet so let us the install the driver.
    // First setup full path to driver name.
    //

    Status = BspBuildDriverName(DriverName, DriverLocation, sizeof(DriverLocation));
	if (Status != S_OK) {
		return Status;
	}

    Status = BspManageDriver(DosDeviceName, DriverLocation, OperationInstall); 
	if (Status != S_OK) {
        BspManageDriver(DosDeviceName, DriverLocation, OperationRemove);
        return Status;
    }

    FileHandle = CreateFile(DeviceName, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
        return Status;
    }

	*DeviceHandle = FileHandle;
	return Status;
}

ULONG
BspInstallDriver(
	IN SC_HANDLE ScmHandle,
	IN PWSTR ServiceName,
	IN PWSTR DriverPath 
	)
{
	ULONG Status;
    SC_HANDLE ServiceHandle;

    ServiceHandle = CreateService(ScmHandle,              // handle of service control manager database
	                              ServiceName,            // address of name of service to start
	                              ServiceName,            // address of display name
	                              SERVICE_ALL_ACCESS,     // type of access to service
	                              SERVICE_KERNEL_DRIVER,  // type of service
	                              SERVICE_DEMAND_START,   // when to start service
	                              SERVICE_ERROR_NORMAL,   // severity if service fails to start
	                              DriverPath,             // address of name of binary file
	                              NULL,                   // service does not belong to a group
	                              NULL,                   // no tag requested
	                              NULL,                   // no dependency names
                                  NULL,                   // use LocalSystem account
                                  NULL                    // no password for service account
                                  );

    if (!ServiceHandle) {
        Status = GetLastError();
        if (Status == ERROR_SERVICE_EXISTS) {
            return S_OK;
        } else {
            return Status;
        }
    }

    CloseServiceHandle(ServiceHandle);
    return S_OK;
}

ULONG
BspManageDriver(
    IN PWSTR ServiceName,
    IN PWSTR DriverPath,
    IN DRIVER_OPERATION Operation 
    )
{
	ULONG Status;
    SC_HANDLE ScmHandle;

	ASSERT(DriverPath != NULL);
	ASSERT(ServiceName != NULL);

    ScmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!ScmHandle) {
		Status = GetLastError();
        return Status;
    }

    switch (Operation) {

        case OperationInstall:

            Status = BspInstallDriver(ScmHandle, ServiceName, DriverPath);
			if (Status != S_OK) {
				break;
			}
            Status = BspStartDriver(ScmHandle, ServiceName);
            break;

        case OperationRemove:

            Status = BspStopDriver(ScmHandle, ServiceName);
			if (Status != S_OK) {
				break;
			}
            Status = BspUninstallDriver(ScmHandle, ServiceName);
            break;

        default:
			Status = S_FALSE;
            break;
    }

    if (ScmHandle) {
        CloseServiceHandle(ScmHandle);
    }

    return Status;
}   

ULONG
BspUninstallDriver(
    IN SC_HANDLE ScmHandle,
    IN PWSTR ServiceName
    )
{
	ULONG Status;
    SC_HANDLE ServiceHandle;

    //
    // Open the handle to the existing service.
    //

    ServiceHandle = OpenService(ScmHandle, ServiceName, SERVICE_ALL_ACCESS);
    if (!ServiceHandle) {
		Status = GetLastError();
        return Status;
    }

    //
    // Mark the service for deletion from the service control manager database.
    //

    if (DeleteService(ServiceHandle)) {
	    CloseServiceHandle(ServiceHandle);
		Status = S_OK;
    } else {
		Status = GetLastError();
	    CloseServiceHandle(ServiceHandle);
    }

    return Status;
} 

ULONG
BspStartDriver(
    IN SC_HANDLE ScmHandle,
    IN PWSTR ServiceName 
    )
{
	ULONG Status;
    SC_HANDLE ServiceHandle;

    ServiceHandle = OpenService(ScmHandle, ServiceName, SERVICE_ALL_ACCESS);
	if (!ServiceHandle) { 
		Status = GetLastError();
        return Status;
    }

	if (!StartService(ServiceHandle, 0, NULL)) { 

        Status = GetLastError();
        if (Status == ERROR_SERVICE_ALREADY_RUNNING) {
		    CloseServiceHandle(ServiceHandle);
            Status = S_OK;

        } else {
		    CloseServiceHandle(ServiceHandle);
        }
	} 
	else {
		Status = S_OK;
	}

    return Status;
}   

ULONG
BspStopDriver(
    IN SC_HANDLE ScmHandle,
    IN PWSTR ServiceName
    )
{
	ULONG Status;
    SC_HANDLE ServiceHandle;
    SERVICE_STATUS ServiceStatus;

    ServiceHandle = OpenService(ScmHandle, ServiceName, SERVICE_ALL_ACCESS);
    if (!ServiceHandle) {
		Status = GetLastError();
        return Status;
    }

	if (ControlService(ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus)) {
	    CloseServiceHandle(ServiceHandle);
		Status = S_OK;

    } else {
		Status = GetLastError();
	    CloseServiceHandle(ServiceHandle);
    }

	return Status;
} 

ULONG
BspBuildDriverName(
	IN PWSTR DriverBaseName,
    IN PWCHAR FullPathName,
    IN ULONG BufferLength
    )
{
	ULONG Status;
    HANDLE FileHandle;
	ULONG LocationLength;

    LocationLength = GetCurrentDirectory(BufferLength, FullPathName);
    if (!LocationLength) {
		Status = GetLastError();
        return Status;
    }

    //
    // Setup full path driver file name
    //

	StringCchCat(FullPathName, BufferLength, L"\\");
    StringCchCat(FullPathName, BufferLength, DriverBaseName);

    //
    // Ensure driver file is in the specified directory.
    //

    FileHandle = CreateFile(FullPathName, GENERIC_READ, 0, NULL, 
		                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		Status = GetLastError();
		return Status;
	}

    CloseHandle(FileHandle);
    return S_OK;
} 

