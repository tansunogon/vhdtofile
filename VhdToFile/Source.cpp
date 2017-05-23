#include <Windows.h>
#include <VirtDisk.h>
#include <stdio.h>

// console application entry point
int wmain(int argc, wchar_t* argv[], wchar_t* envp[])
{
    if (argc != 4
        || (wcscmp(argv[1], L"-t") != 0) && (wcscmp(argv[1], L"-f") != 0))
    {
        wprintf(L"for small files\n");
        wprintf(L"vhd to   file: -t vhd_file_name file_name\n");
        wprintf(L"vhd from file: -f vhd_file_name file_name\n");
        return -1;
    }

    wchar_t* vhd_file_name = argv[2];
    wchar_t* file_name = argv[3];

    if (wcscmp(argv[1], L"-t") == 0)
    {
        // open vhd file
        HANDLE vhd_handle;
        VIRTUAL_STORAGE_TYPE virtual_storage_type = {};
        DWORD ret = OpenVirtualDisk(&virtual_storage_type,
                                    vhd_file_name,
                                    VIRTUAL_DISK_ACCESS_ATTACH_RO
                                    | VIRTUAL_DISK_ACCESS_GET_INFO,
                                    OPEN_VIRTUAL_DISK_FLAG_NONE,
                                    NULL,        // Parameters
                                    &vhd_handle);
        if (ret != ERROR_SUCCESS || vhd_handle == INVALID_HANDLE_VALUE)
        {
            wprintf(L"can't open vhd file\n");
            return -1;
        }

        // attach vhd file
        ret = AttachVirtualDisk(vhd_handle,
                                NULL,                 // SecurityDescriptor
                                ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY
                                | ATTACH_VIRTUAL_DISK_FLAG_NO_LOCAL_HOST,
                                0,                    // ProviderSpecificFlags
                                NULL,                 // Parameters
                                NULL);                // Overlapped
        if (ret != ERROR_SUCCESS)
        {
            wprintf(L"can't attach vhd file\n");
            CloseHandle(vhd_handle);
            return -1;
        }
        // get file size
        ULONGLONG file_size;
        {
            GET_VIRTUAL_DISK_INFO vinfo;
            vinfo.Version = GET_VIRTUAL_DISK_INFO_SIZE;
            ULONG vinfo_size = sizeof vinfo;
            ret = GetVirtualDiskInformation(vhd_handle,
                                            &vinfo_size,
                                            &vinfo,
                                            NULL); // SizeUsed
            if (ret != ERROR_SUCCESS)
            {
                wprintf(L"can't get vhd file size\n");
                CloseHandle(vhd_handle);
                return -1;
            }
            file_size = vinfo.Size.VirtualSize;
        }

        // open file
        HANDLE file_handle = CreateFileW(file_name,
                                         GENERIC_WRITE,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                                         NULL,           // SecurityAttributes
                                         CREATE_ALWAYS,
                                         FILE_ATTRIBUTE_NORMAL,
                                         NULL);          // TemplateFile
        if (file_handle == INVALID_HANDLE_VALUE)
        {
            CloseHandle(vhd_handle);
            wprintf(L"can't open file\n");
            return -1;
        }


        PBYTE buf = new BYTE[file_size];
        DWORD bytes_read;
        OVERLAPPED over = {};
        if (!ReadFile(vhd_handle, buf, file_size, &bytes_read, &over))
        {
            if (!GetOverlappedResult(vhd_handle, &over, &bytes_read, TRUE))
            {
                wprintf(L"read error\n");
            }
        }
        if (!WriteFile(file_handle, buf, file_size, &bytes_read, NULL))
        {
            wprintf(L"write error\n");
        }

        delete[] buf;
        CloseHandle(vhd_handle);
        CloseHandle(file_handle);
        return 0;
    }
    else if (wcscmp(argv[1], L"-f") == 0)
    {
        // open vhd file
        HANDLE vhd_handle;
        VIRTUAL_STORAGE_TYPE virtual_storage_type = {};
        DWORD ret = OpenVirtualDisk(&virtual_storage_type,
                                    vhd_file_name,
                                    VIRTUAL_DISK_ACCESS_ATTACH_RW
                                    | VIRTUAL_DISK_ACCESS_GET_INFO,
                                    OPEN_VIRTUAL_DISK_FLAG_NONE,
                                    NULL,        // Parameters
                                    &vhd_handle);
        if (ret != ERROR_SUCCESS || vhd_handle == INVALID_HANDLE_VALUE)
        {
            wprintf(L"can't open vhd file\n");
            return -1;
        }

        // attach vhd file
        ret = AttachVirtualDisk(vhd_handle,
                                NULL,                 // SecurityDescriptor
                                ATTACH_VIRTUAL_DISK_FLAG_NO_LOCAL_HOST,
                                0,                    // ProviderSpecificFlags
                                NULL,                 // Parameters
                                NULL);                // Overlapped
        if (ret != ERROR_SUCCESS)
        {
            wprintf(L"can't attach vhd file\n");
            CloseHandle(vhd_handle);
            return -1;
        }
        // get file size
        ULONGLONG vhd_file_size;
        {
            GET_VIRTUAL_DISK_INFO vinfo;
            vinfo.Version = GET_VIRTUAL_DISK_INFO_SIZE;
            ULONG vinfo_size = sizeof vinfo;
            ret = GetVirtualDiskInformation(vhd_handle,
                                            &vinfo_size,
                                            &vinfo,
                                            NULL); // SizeUsed
            if (ret != ERROR_SUCCESS)
            {
                wprintf(L"can't get vhd file size\n");
                CloseHandle(vhd_handle);
                return -1;
            }
            vhd_file_size = vinfo.Size.VirtualSize;
        }

        // open file
        HANDLE file_handle = CreateFileW(file_name,
                                         GENERIC_READ,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                                         NULL,           // SecurityAttributes
                                         OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL,
                                         NULL);          // TemplateFile
        if (file_handle == INVALID_HANDLE_VALUE)
        {
            CloseHandle(vhd_handle);
            wprintf(L"can't open file\n");
            return -1;
        }

        DWORD file_size = GetFileSize(file_handle, NULL);
        if (file_size == -1)
        {
            wprintf(L"can't get file size\n");
            CloseHandle(vhd_handle);
            CloseHandle(file_handle);
            return -1;
        }
        if (file_size > vhd_file_size)
        {
            wprintf(L"file size is too large\n");
            CloseHandle(vhd_handle);
            CloseHandle(file_handle);
            return -1;
        }

        PBYTE buf = new BYTE[vhd_file_size];
        DWORD bytes_read;
        OVERLAPPED over = {};
        if (!ReadFile(file_handle, buf, file_size, &bytes_read, NULL))
        {
            wprintf(L"write error\n");
        }
        if (!WriteFile(vhd_handle, buf, vhd_file_size, &bytes_read, &over))
        {
            if (!GetOverlappedResult(vhd_handle, &over, &bytes_read, TRUE))
            {
                wprintf(L"read error\n");
            }
        }

        delete[] buf;
        CloseHandle(vhd_handle);
        CloseHandle(file_handle);
        return 0;
    }
    else
    {
        return -1;
    }
}