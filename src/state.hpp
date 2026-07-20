#pragma once
#include "math_engine.hpp"
#include <string>
#include <vector>
#include <deque>

// ============================================================
// Language
// ============================================================
enum Lang { LANG_EN, LANG_ZH, LANG_JA };

// ============================================================
// Angle mode
// ============================================================
enum AngleMode { DEG, RAD, GRAD };

// ============================================================
// Number base (display only for now)
// ============================================================
enum NumBase { DEC, HEX, BIN, OCT };

// ============================================================
// History entry
// ============================================================
struct HistoryEntry {
    std::string expression;
    std::string result;
};

// ============================================================
// Global calculator state
// ============================================================
struct CalcState {
    // Language
    Lang lang = LANG_ZH; // Default to Chinese on WSL

    // Display
    std::string display = "";  // Current expression being typed
    std::string result = "";   // Last result
    std::string error = "";
    bool hasResult = false;    // After =, next operator continues

    // Mode
    AngleMode angleMode = DEG;
    NumBase numBase = DEC;

    // Ans
    std::string lastAns = "0";

    // History (max 20)
    std::deque<HistoryEntry> history;

    // Keyboard flash feedback timer
    int keyFlashTimer = 0;

    // Manual input buffer (for InputText LCD)
    char inputBuf[1024] = "";

    // Font size: 0=small, 1=medium, 2=large
    int fontSize = 1;

    // Panel visibility
    bool showHistory = false;
    bool showConstants = false;
    bool showAdvanced = false;

    // Expression evaluator (from math_engine)
    ExpressionEvaluator evaluator;

    // ============================================================
    // Legacy state for Fourier / Complex / Domain coloring
    // (kept intact for the Advanced modal)
    // ============================================================
    int fourierTerms = 5;
    int fourierType = 0;
    std::vector<PlotData> fourierPlots;

    std::string complexExpr = "z";
    double cxMin = -3, cxMax = 3, cyMin = -3, cyMax = 3;
    std::vector<PlotGenerator::DomainColorPoint> domainPts;
    bool domainDirty = true;

    // Plot / calculus state
    double evalX = 0;
    double plotXMin = -10, plotXMax = 10, plotYMin = -10, plotYMax = 10;
    std::vector<PlotData> plots;
    std::vector<PlotData> integPlot;
    enum PlotMode { PLOT_NONE, PLOT_FUNC, PLOT_DERIV, PLOT_INTEG } plotMode = PLOT_NONE;
    double intA = 0, intB = 6.2831853;
    std::string intResult;
    double complexRe = 0, complexIm = 0;
    double cauchyRadius = 1.0;
    std::string complexResult;
};

// Global instance
extern CalcState g_state;
