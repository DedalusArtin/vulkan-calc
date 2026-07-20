#pragma once
#include "state.hpp"
#include <cstring>
#include <cstdio>
#include <memory>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

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
#ifdef _MSC_VER
static std::string execShell(const char* cmd) {
    FILE* f = _popen(cmd, "r");
    if (!f) return "";
    char buf[256];
    std::string result;
    if (fgets(buf, sizeof(buf), f)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
        result = buf;
    }
    _pclose(f);
    return result;
}
#else
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
#endif

// Helper: read all output from a shell command
#ifdef _MSC_VER
static std::string execShellAll(const char* cmd) {
    FILE* f = _popen(cmd, "r");
    if (!f) return "";
    std::string result;
    char linebuf[4096];
    while (fgets(linebuf, sizeof(linebuf), f)) {
        result += linebuf;
    }
    _pclose(f);
    return result;
}
#else
inline std::string execShellAll(const char* cmd) {
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "";
    std::string result;
    char linebuf[4096];
    while (fgets(linebuf, sizeof(linebuf), pipe.get())) {
        result += linebuf;
    }
    return result;
}
#endif

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

#ifdef _WIN32
    // 2. Windows: use Win32 API (avoids _popen issues with 2>/dev/null redirect)
    LANGID langId = GetUserDefaultUILanguage();
    WORD primaryLang = PRIMARYLANGID(langId);
    if (primaryLang == LANG_CHINESE) return LANG_ZH;
    if (primaryLang == LANG_JAPANESE) return LANG_JA;
#else
    // 2. On WSL/Linux, probe Windows locale via PowerShell
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
    std::string winLang = execShell(
        "powershell.exe -NoProfile -Command "
        "\"Get-WinSystemLocale | Select-Object -ExpandProperty Name\" 2>/dev/null"
    );
    if (!winLang.empty()) {
        if (winLang.find("zh") != std::string::npos) return LANG_ZH;
        if (winLang.find("ja") != std::string::npos) return LANG_JA;
    }
#endif

    // 3. Default to English
    return LANG_EN;
}
