#pragma once
#include <string> 
#include <TlHelp32.h>
#include <stdint.h>
#include <cstdio> 


DWORD processId;
uint32_t get_process_id(const std::string& image_name)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32 process;
    process.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &process)) {
        do {
            // Using _stricmp for a case-insensitive comparison
            if (_stricmp(image_name.c_str(), process.szExeFile) == 0) {
                CloseHandle(snapshot);
                processId = process.th32ProcessID;
                return processId;
            }
        } while (Process32Next(snapshot, &process));
    }

    CloseHandle(snapshot);
    return 0;
}

