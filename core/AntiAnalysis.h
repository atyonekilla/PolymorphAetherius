#pragma once

class AntiAnalysis {
public:
    static bool IsDebugged();
    static void EvadeSandbox();
    static bool CheckBlacklistedModules();
};