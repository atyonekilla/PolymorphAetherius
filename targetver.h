#pragma once

#if !defined(_WIN64) && !defined(_WIN32)
#define _WIN32_WINNT 0x0A00  // Windows 10
#define WIN32_LEAN_AND_MEAN
#endif

#include <SDKDDKVer.h>