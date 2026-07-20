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
// Extension API: a plugin extension descriptor
// ============================================================
struct CalcExtension {
    const char* name;          // Display name (Chinese)
    const char* (*func)(const char* input);  // Callback: input string → result string
    const char* desc;          // Description
    const char* exampleInput;  // Example input placeholder
    const char* shortName;     // Short unique name
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

    // ============================================================
    // Advanced tab index (0-6)
    // ============================================================
    int advTab = 0;  // 0=微积分, 1=傅里叶, 2=复变函数, 3=域着色, 4=概率统计, 5=3D曲面, 6=扩展

    // ============================================================
    // Derivative graph improvements
    // ============================================================
    bool showSecondDeriv = false;     // Show f''(x) on graph
    std::vector<double> derivZeroPts; // Zero points of derivative (to mark on graph)
    std::vector<double> derivExtremePts; // Extreme points

    // ============================================================
    // Probability & Statistics state
    // ============================================================
    // Permutation/Combination
    long long probN = 5;
    long long probK = 3;
    std::string probPermResult;
    std::string probCombResult;
    std::string probFactResult;

    // Binomial
    long long binomN = 10;
    long long binomK = 5;
    double binomP = 0.5;
    std::string binomResult;

    // Normal distribution
    double normX = 1.0;
    double normMu = 0.0;
    double normSigma = 1.0;
    std::string normCdfResult;
    std::string normInvResult;

    // Poisson
    double poisLambda = 3.0;
    long long poisK = 2;
    std::string poisResult;

    // Descriptive statistics
    char statsDataBuf[512] = "1,2,3,4,5,6,7,8,9,10";
    std::string statsResult;

    // Histogram
    std::vector<double> histData;
    int histNBins = 10;
    std::vector<double> histCounts;
    double histMin = 0, histMax = 0;
    bool histDirty = true;

    // ============================================================
    // Volume calculation state
    // ============================================================
    int volType = 0; // 0=旋转体, 1=球体, 2=圆柱, 3=圆锥, 4=长方体
    // Revolution
    double revA = 0, revB = 3.141592653589793;
    // Sphere
    double sphereR = 5.0;
    // Cylinder
    double cylR = 3.0, cylH = 5.0;
    // Cone
    double coneR = 3.0, coneH = 5.0;
    // Box
    double boxA = 3.0, boxB = 4.0, boxC = 5.0;
    std::string volResult;
    std::string volFormula;

    // ============================================================
    // Surface integral state
    // ============================================================
    int surfType = 0; // 0=球面, 1=柱面, 2=平面, 3=自定义
    char surfFunc[256] = "x*y";
    double surfA = 0, surfB = 3.141592653589793;
    double surfC = 0, surfD = 3.141592653589793;
    int mcSamples = 5000;
    std::string surfResult;

    // ============================================================
    // 3D Surface state
    // ============================================================
    char func3D[256] = "sin(sqrt(x^2+y^2))";
    float rot3DX = 30.0f, rot3DY = -30.0f;
    float zoom3D = 1.0f;
    double range3DMin = -5.0, range3DMax = 5.0;
    bool surf3DDirty = true;
    int gridRes = 40;

    // ============================================================
    // Extension API state
    // ============================================================
    bool showExtPanel = false;
    char extInput[256] = "";
    std::string extResult;
    int extSelected = 0;
    std::vector<CalcExtension> extensions; // Populated at init
};

// Global instance
extern CalcState g_state;
