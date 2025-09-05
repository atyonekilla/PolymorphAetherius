#pragma once
#include <Windows.h>
#include <vector>

class GhostMemory {
private:
    struct SecureAlloc {
        void* address;
        size_t size;
        DWORD oldProtect;
    };
    static std::vector<SecureAlloc> secureAllocations;

public:
    static void* SecureAllocate(size_t size);
    static void SecureFree(void* ptr);
    static void Cleanup();
    static void SecureErase(std::vector<unsigned char>& data);
};