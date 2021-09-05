#pragma once
#pragma warning (disable : 6201)
#pragma warning (disable : 6386)

#include <ntddk.h>

// User mode application should have a struct just like this
// TODO: Share this struct with Driver and Client source
typedef struct _Data
{
    char* Buffer;
} DATA, *PDATA;

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

// More info about IOCTL creation at https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/defining-i-o-control-codes
// TODO Share this IOCTL with user mode source
#define IOCTL_Device_Function CTL_CODE(0x8000, 0x8000, METHOD_IN_DIRECT , FILE_ANY_ACCESS)

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
     *  There are default IRP codes, but we can also create custom IOCTL that will arrive in a IRP_MJ_DEVICE_CONTROL request.
     *
     *  Note: We can link the same function pointer to different IRP codes if we want.
     *
     *  https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/writing-dispatch-routines
     */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = HandleRequestPackage; 
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = HandleRequestPackage;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HandleRequestPackage;

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
    
    NTSTATUS Status = STATUS_SUCCESS;

    if(Irp->Type == IRP_MJ_DEVICE_CONTROL)
    {
        // More information about the IRP stack at https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/i-o-stack-locations
        PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
        
        if(Stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_Device_Function)
        {
            // Just to make sure that our input is valid
            if(Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DATA))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                // Its time to retrieve the data sent from user land (Type3InputBuffer)
                const PDATA Data = static_cast<PDATA>(Stack->Parameters.DeviceIoControl.Type3InputBuffer);
                DbgPrint(Data->Buffer);
            }
        }
        else
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;   
        }
    }

    /*
    *  Everytime we handle a IRP dispatch we must manually complete therequest.
    *
    *  https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/completing-the-irp
    */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);	
    
    return Irp->IoStatus.Status;
}