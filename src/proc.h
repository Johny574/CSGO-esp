#pragma once

#include <windows.h>
#include <wchar.h>
#include <TlHelp32.h>


class ProcH
{
public:

    HANDLE Proc;
    DWORD ID;

    ProcH(const WCHAR* name)
    {
        ID = GetProcessID(name);
        Proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ID);
    };

    uintptr_t GetModuleBaseAddress(const wchar_t* modName)
    {
        uintptr_t modBaseAddr = 0;
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, ID);
        if (hSnap != INVALID_HANDLE_VALUE)
        {
            MODULEENTRY32 modEntry;
            modEntry.dwSize = sizeof(modEntry);
            if (Module32First(hSnap, &modEntry))
            {
                do
                {
                    if (!_wcsicmp(modEntry.szModule, modName))
                    {
                        modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                        break;
                    }
                } while (Module32Next(hSnap, &modEntry));
            }
        }
        CloseHandle(hSnap);
        return modBaseAddr;
    }

    DWORD GetProcessID(const WCHAR* name)
    {
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

        if (Process32First(snapshot, &entry) == TRUE)
        {
            do
            {
                if (_wcsicmp(entry.szExeFile, name) == 0)
                {
                    CloseHandle(snapshot);  // Don't forget to close the handle!
                    return entry.th32ProcessID;
                }
            } while (Process32Next(snapshot, &entry));
        }

        CloseHandle(snapshot);
        return 0;
    }
};