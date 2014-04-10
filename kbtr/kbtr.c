//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#include <ntddk.h> 
#include <string.h>

#include "kbtrtype.h"
#include "kbtrData.h"
#include "kbtrproc.h"

#if DBG
#define DebugTrace(_M)							\
                DbgPrint("dprobe.kbtr.sys: ");	\
                DbgPrint _M;

#else
#define DebugTrace(_M)
#endif

//
// Device driver routine declarations.
//

DRIVER_INITIALIZE DriverEntry;

__drv_dispatchType(IRP_MJ_CREATE)
DRIVER_DISPATCH BtrDispatchCreate;

__drv_dispatchType(IRP_MJ_CLOSE)
DRIVER_DISPATCH BtrDispatchClose;

__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH BtrDispatchControl;

DRIVER_UNLOAD BtrUnloadDriver;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, BtrDispatchCreate)
#pragma alloc_text(PAGE, BtrDispatchClose)
#pragma alloc_text(PAGE, BtrDispatchControl)
#pragma alloc_text(PAGE, BtrUnloadDriver)
#endif

KEVENT BtrEvent;

NTSTATUS
DriverEntry(
    __in PDRIVER_OBJECT   DriverObject,
    __in PUNICODE_STRING  RegistryPath
    )
/*++

Routine Description:
    This routine is called by the Operating System to initialize the driver.

    It creates the device object, fills in the dispatch entry points and
    completes the initialization.

Arguments:
    DriverObject - a pointer to the object that represents this device
    driver.

    RegistryPath - a pointer to our Services key in the registry.

Return Value:
    STATUS_SUCCESS if initialized; an error otherwise.

--*/

{
    NTSTATUS  Status;
    UNICODE_STRING	DeviceName;  
    UNICODE_STRING  SymbolicLinkName; 
    PDEVICE_OBJECT  DeviceObject; 
	HANDLE ThreadHandle;

    UNREFERENCED_PARAMETER(RegistryPath);

    RtlInitUnicodeString(&DeviceName, KBTR_DEVICE_NAME);

    Status = IoCreateDevice(DriverObject, 0, &DeviceName, 
							FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, 
							FALSE, &DeviceObject);        

    if (!NT_SUCCESS(Status)) {
        DebugTrace(("Couldn't create the device object \n"));
        return Status;
    }

    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = BtrDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = BtrDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = BtrDispatchControl;
    DriverObject->DriverUnload = BtrUnloadDriver;

    //
    // Initialize a Unicode String containing the Win32 name
    // for our device.
    //

    RtlInitUnicodeString(&SymbolicLinkName, KBTR_DOSDEVICE_NAME);

    //
    // Create a symbolic link between our device name  and the Win32 name
    //

    Status = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);

    if (!NT_SUCCESS(Status)) {

        //
        // Delete everything that this routine has allocated.
        //

        DebugTrace(("Couldn't create symbolic link \n"));
        IoDeleteDevice(DeviceObject);
    }

	//
	// Create synchronization event object
	//

	KeInitializeEvent(&BtrEvent, NotificationEvent, FALSE);

	//
	// Create system worker thread
	//

	Status = PsCreateSystemThread(&ThreadHandle,
								  THREAD_ALL_ACCESS,
								  NULL,
								  0,
								  NULL,
								  BtrWorkerThread,
								  NULL);
	if (NT_SUCCESS(Status)) {
		ZwClose(ThreadHandle);
	}

    return Status;
}


NTSTATUS
BtrDispatchCreate(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    This routine is called by the I/O system when the SIOCTL is opened or
    closed.

    No action is performed other than completing the request successfully.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    NT status code

--*/

{
    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
BtrDispatchClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    This routine is called by the I/O system when the SIOCTL is opened or
    closed.

    No action is performed other than completing the request successfully.

Arguments:

    DeviceObject - a pointer to the object that represents the device
    that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    NT status code

--*/

{
    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

VOID
BtrUnloadDriver(
    __in PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    This routine is called by the I/O system to unload the driver.

    Any resources previously allocated must be freed.

Arguments:

    DriverObject - a pointer to the object that represents our driver.

Return Value:

    None
--*/

{
    PDEVICE_OBJECT DeviceObject = DriverObject->DeviceObject;
    UNICODE_STRING DosDeviceName;

    PAGED_CODE();

    //
    // Create counted string version of our Win32 device name.
    //

    RtlInitUnicodeString(&DosDeviceName, KBTR_DOSDEVICE_NAME);

    //
    // Delete the link from our device name to a name in the Win32 namespace.
    //

    IoDeleteSymbolicLink(&DosDeviceName);

    if (DeviceObject != NULL) {
        IoDeleteDevice(DeviceObject);
    }
}

NTSTATUS
BtrDispatchControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system to perform a device I/O
    control function.

Arguments:

    DeviceObject - a pointer to the object that represents the device
        that I/O is to be done on.

    Irp - a pointer to the I/O Request Packet for this request.

Return Value:

    NT status code

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION  StackLocation;

    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

	Status = STATUS_SUCCESS;
    StackLocation = IoGetCurrentIrpStackLocation(Irp);

    //
    // Determine which I/O control code was specified.
	//

	switch (StackLocation->Parameters.DeviceIoControl.IoControlCode) {

	case IOCTL_KBTR_METHOD_WAKEUP_THREAD:

		//
		// Wake up the system thread
		//

		KeSetEvent( &BtrEvent, IO_NO_INCREMENT, FALSE );
		DebugTrace(("signal event object \n"));
		Irp->IoStatus.Information = 0;
		break;

	default:

		Status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return Status;
}

VOID
BtrWorkerThread(
	__in PVOID Context
	)
{
	//
	// Test whether it can safely wait on DISPATCH_LEVEL
	//

	DebugTrace(("worker thread is running \n"));

	KeRaiseIrqlToDpcLevel();
	DebugTrace(("raise IRQL to DISPATCH_LEVEL \n"));

	KeWaitForSingleObject( &BtrEvent, 
		                   Executive, 
						   KernelMode, 
						   FALSE, 
						   NULL );

	DebugTrace(("lower IRQL to PASSIVE_LEVEL \n"));
	KeLowerIrql(PASSIVE_LEVEL);

	DebugTrace(("worker thread is terminated \n"));
	PsTerminateSystemThread(STATUS_SUCCESS);
}