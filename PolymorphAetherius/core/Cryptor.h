#pragma once
#include <vector>
#include <string>
#include "Cryptor.h"
class NanoObfuscator {
private:

    static uint64_t runtimeSeed;

    static uint64_t GenerateRuntimeSeed();

public:
    template<typename T>
    static void PolymorphEncryptDecrypt(T* data, size_t size, const std::string& key);
};    


    uint64_t NanoObfuscator::runtimeSeed = 0;


class QuantumDecryptor {
public:
    static bool DecryptInMemory(const std::string& key, std::vector<unsigned char>& data);
};