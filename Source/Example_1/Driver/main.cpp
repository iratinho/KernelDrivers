#pragma once
#pragma warning (disable : 6201)
#pragma warning (disable : 6386)

#include <ntddk.h>

/*
 *  This driver acts as an example on how to handle data sent from user mode application and as
 * the minimum initial setup for a software driver
 *
 *  This can be divided in a couple of logical steps:
 *
 *   1 - We will create a new DeviceObject so that user mode applications can get an handle to our driver using
 *    the CreateFile function or other functions that allow us to get handles to driver objects.
 *
 *   2 - Register dispatch routines to handle native and custom IRP codes (https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/writing-dispatch-routines)
 *
 */

void DriverUnload(_In_ _DRIVER_OBJECT* DriverObject);
NTSTATUS HandleRequestPackage(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);

UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\Example1");
UNICODE_STRING SymLinkName = RTL_CONSTANT_STRING(L"\\??\\Example1");

extern "C"
NTSTATUS DriverEntry(_In_ _DRIVER_OBJECT* DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    /*
     * The IoCreateDevice creates a new device object that our driver object will be stored in the _DRIVER_OBJECT structure
     *
     * Creating this device object will allow user mode applications to open an handle to our driver, notice that we gave a name
     * for our device object, a device object can be named or unnamed but in order to interact with user mode applications the
     * driver must supply a valid MS-DOS device name and we do that by creating a symbolic link by using the IoCreateSymbolicLink
     *
     * https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/named-device-objects
     * https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-ms-dos-device-names
     * 
     */
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS CreateDeviceStatus = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, FALSE, FALSE, &DeviceObject);
    NTSTATUS CreateSymLinkStatus = IoCreateSymbolicLink(&SymLinkName, &DeviceName);

    /*
     *  Normally we would clean up resources in the Driver unload routine, but if anything fails in the DriverEntry function
     * the unload routine will never be called, so make sure to handle all errors and destroy any created objects from this
     * function
     */
    if (!NT_SUCCESS(CreateDeviceStatus))
    {
        KdPrint(("Failed to device object."));
        
        IoDeleteDevice(DeviceObject);
        return CreateDeviceStatus;
    }

    if (!NT_SUCCESS(CreateSymLinkStatus))
    {
        KdPrint(("Failed to Symbolic link for the device object."));

        IoDeleteSymbolicLink(&SymLinkName);
        IoDeleteDevice(DeviceObject);
        return CreateSymLinkStatus;
    }

    // This is the driver unload routine that we must provide, this will be called when the driver is destroyed.
    DriverObject->DriverUnload = DriverUnload;
    
    /*
     *  Now its time to create dispatch routines, everytime there is a I/O request package (IRP) the I/O manager will access the dispatch
     *  table for the driver and will call the routines linked with the IRP. In code this can be accessed from DriverObject->MajorFunction[IRP_CODE].
     *  There are default IRP codes, but we can also create custom IRP and we will do that in this example.
     *
     *  Note: We can link the same function pointer to different IRP codes if we want.
     *
     *  https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/writing-dispatch-routines
     */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = HandleRequestPackage; 
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = HandleRequestPackage;

    return STATUS_SUCCESS;
}

void DriverUnload(_In_ _DRIVER_OBJECT* DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    IoDeleteSymbolicLink(&SymLinkName);
    IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS HandleRequestPackage(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    
    /*
    *  Everytime we handle a IRP dispatch we must manually complete the
    *  request. This code shows how to complete a basic request.
    *
    *  https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/completing-the-irp
    */
    if(Irp->Type == IRP_MJ_CREATE || Irp->Type == IRP_MJ_CLOSE)
    {
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);	
    }

    // TODO: Handle custom IRP code

    return Irp->IoStatus.Status;
}