#include "MemoryManager.h"
#include <Windows.h>

std::vector<GhostMemory::SecureAlloc> GhostMemory::secureAllocations;

void* GhostMemory::SecureAllocate(size_t size) {
    void* mem = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (mem) {
        secureAllocations.push_back({ mem, size, PAGE_READWRITE });
    }
    return mem;
}

void GhostMemory::SecureFree(void* ptr) {
    for (auto it = secureAllocations.begin(); it != secureAllocations.end(); ++it) {
        if (it->address == ptr) {
            SecureZeroMemory(ptr, it->size);
            VirtualFree(ptr, 0, MEM_RELEASE);
            secureAllocations.erase(it);
            break;
        }
    }
}

void GhostMemory::Cleanup() {
    for (auto& alloc : secureAllocations) {
        SecureZeroMemory(alloc.address, alloc.size);
        VirtualFree(alloc.address, 0, MEM_RELEASE);
    }
    secureAllocations.clear();
}

void GhostMemory::SecureErase(std::vector<unsigned char>& data) {
    if (!data.empty()) {
        SecureZeroMemory(data.data(), data.size());
        data.clear();
        data.shrink_to_fit();
    }
}