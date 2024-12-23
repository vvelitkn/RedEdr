#pragma once

#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <mutex>

#include "ranges.h"
#include "json.hpp"
#include "process.h"
#include "mem_static.h"


std::wstring GetProcessName(HANDLE hProcess);
BOOL InitProcessQuery();
DWORD FindProcessIdByName(const std::wstring& processName);

struct ProcessAddrInfoRet {
    std::wstring name;
    PVOID base_addr;

    // Original?
    PVOID allocation_base;
    size_t region_size;
    DWORD allocation_protect;

    DWORD state;
    DWORD protect;
    DWORD type;

    std::wstring stateStr;
    std::wstring protectStr;
    std::wstring typeStr;
};
ProcessAddrInfoRet ProcessAddrInfo(HANDLE hProcess, DWORD address);


struct ProcessPebInfoRet {
    std::wstring image_path;
    std::wstring commandline;
    std::wstring working_dir;
    DWORD parent_pid;
    DWORD is_debugged;
    DWORD is_protected_process;
    DWORD is_protected_process_light;
    PVOID image_base;
};
ProcessPebInfoRet ProcessPebInfo(HANDLE hProcess);


struct ProcessLoadedDll {
    PVOID dll_base;
    ULONG size;
    std::wstring name;
};
std::vector<ProcessLoadedDll> ProcessEnumerateModules(HANDLE hProcess);


std::vector<MemoryRegion> EnumerateModuleSections(HANDLE hProcess, LPVOID moduleBase);

