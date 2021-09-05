#pragma once
#pragma warning(disable: 4245)

#include <iostream>
#include <Windows.h>

// TODO: Share this struct with Driver and Client source
typedef struct _Data
{
    char* Buffer;
} DATA, *PDATA;

#define IOCTL_Device_Function CTL_CODE(0x8000, 0x8000, METHOD_IN_DIRECT , FILE_ANY_ACCESS)

int main()
{
    // We start by acquiring an handle for our device
    HANDLE hDevice = CreateFile("\\\\?\\GLOBALROOT\\Device\\Example1"
        , GENERIC_WRITE
        , FILE_SHARE_WRITE
        , NULL
        , OPEN_EXISTING
        , 0
        , NULL);

    if(hDevice == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to open device (error: " << GetLastError() << ")" << std::endl;
        return 1;
    }

    DATA Data;
    Data.Buffer = "Hello Driver!";

    DWORD ReturnedBytes;
    BOOL Result = DeviceIoControl(hDevice
        , IOCTL_Device_Function
        , &Data
        , sizeof(Data)
        , nullptr
        , 0
        , &ReturnedBytes
        , nullptr);

    if(Result)
    {
        std::cerr << "Unable to send data. " << GetLastError() << std::endl;
        return 1;
    }
    
    return 0;
}