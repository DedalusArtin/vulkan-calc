#pragma once
#include "state.hpp"
#include <cstring>
#include <cstdio>
#include <memory>

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

// Helper: run a shell command and return first line (or empty)
inline std::string execShell(const char* cmd) {
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "";
    char buf[256];
    if (fgets(buf, sizeof(buf), pipe.get())) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
        return buf;
    }
    return "";
}

// Helper: run a shell command and return ALL output (for JSON responses etc.)
inline std::string execShellAll(const char* cmd) {
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "";
    std::string result;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf) - 1, pipe.get())) > 0) {
        buf[n] = '\0';
        result += buf;
    }
    return result;
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

// Auto-detect language from environment + WSL Windows locale
inline Lang detectLang() {
    // 1. Check environment variables (highest priority)
    const char* envChecks[] = {
        std::getenv("LANG"),
        std::getenv("LANGUAGE"),
        std::getenv("LC_ALL"),
    };
    for (auto env : envChecks) {
        if (env) {
            if (std::strstr(env, "zh")) return LANG_ZH;
            if (std::strstr(env, "ja")) return LANG_JA;
        }
    }

    // 2. On WSL, probe Windows locale via PowerShell
    //    Windows Chinese system returns "zh-CN"
    std::string winLocale = execShell(
        "powershell.exe -NoProfile -Command "
        "\"[System.Globalization.CultureInfo]::CurrentUICulture.Name\" 2>/dev/null"
    );
    if (!winLocale.empty()) {
        if (winLocale.find("zh") != std::string::npos ||
            winLocale.find("ZH") != std::string::npos) return LANG_ZH;
        if (winLocale.find("ja") != std::string::npos ||
            winLocale.find("JA") != std::string::npos) return LANG_JA;
    }

    // 3. Also try wmic (older Windows systems)
    std::string winLang = execShell(
        "powershell.exe -NoProfile -Command "
        "\"Get-WinSystemLocale | Select-Object -ExpandProperty Name\" 2>/dev/null"
    );
    if (!winLang.empty()) {
        if (winLang.find("zh") != std::string::npos) return LANG_ZH;
        if (winLang.find("ja") != std::string::npos) return LANG_JA;
    }

    // 4. Default to English
    return LANG_EN;
}
