#pragma once
#include "state.hpp"
#include <cstring>

// ============================================================
// Multi-language support
// ============================================================
inline const char* T(const char* en, const char* zh, const char* ja) {
    switch (g_state.lang) {
        case LANG_ZH: return zh;
        case LANG_JA: return ja;
        default: return en;
    }
}

// Auto-detect language from LANG environment variable
inline Lang detectLang() {
    const char* langEnv = std::getenv("LANG");
    if (langEnv) {
        if (std::strstr(langEnv, "zh")) return LANG_ZH;
        if (std::strstr(langEnv, "ja")) return LANG_JA;
    }
    return LANG_EN;
}

// Angle mode display strings
inline const char* angleModeStr(AngleMode m) {
    switch (m) {
        case DEG: return "DEG";
        case RAD: return "RAD";
        case GRAD: return "GRAD";
    }
    return "DEG";
}
