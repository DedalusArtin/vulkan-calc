#include "calc_logic.hpp"
#include "locale.hpp"
#include <string>
#include <cmath>
#include <numbers>
#include <random>
#include <sstream>
#include <map>
#include <algorithm>
#include <functional>
#include <ctime>

// Apply angle mode to trig function calls in expression
std::string applyAngleMode(const std::string& expr, AngleMode mode) {
    if (mode == RAD) return expr;

    double factor = (mode == DEG) ? (std::numbers::pi / 180.0) : (std::numbers::pi / 200.0);
    std::ostringstream factorStr;
    factorStr << "*(" << factor << ")";

    std::string result = expr;

    // Process sin, cos, tan
    const char* funcs[] = {"sin", "cos", "tan"};
    for (const auto& func : funcs) {
        size_t pos = 0;
        size_t flen = std::strlen(func);
        while ((pos = result.find(func, pos)) != std::string::npos) {
            // Check it's a function call: followed by '('
            size_t parenPos = pos + flen;
            if (parenPos < result.size() && result[parenPos] == '(') {
                // Find matching closing paren
                int depth = 0;
                size_t end = parenPos;
                while (end < result.size()) {
                    if (result[end] == '(') depth++;
                    else if (result[end] == ')') {
                        depth--;
                        if (depth == 0) break;
                    }
                    end++;
                }
                if (end < result.size()) {
                    // Insert factor before the closing paren
                    result.insert(end, factorStr.str());
                    pos = end + factorStr.str().size();
                } else {
                    pos = parenPos + 1;
                }
            } else {
                pos += flen;
            }
        }
    }
    return result;
}

// Preprocess expression: replace function aliases and convert constants to numeric values
void calcPreprocessExpression(std::string& expr) {
    // Replace π character with "pi" text (which the evaluator already handles)
    {
        size_t pos = 0;
        while ((pos = expr.find("π", pos)) != std::string::npos) {
            expr.replace(pos, 3, "pi");  // π is 3 bytes in UTF-8
            pos += 2;
        }
    }

    // Replace function aliases (user-facing → evaluator-supported)
    struct AliasMap {
        const char* alias;
        const char* standard;
    };
    const AliasMap aliases[] = {
        {"arcsin(", "asin("},
        {"arccos(", "acos("},
        {"arctan(", "atan("},
        {"arcsinh(", "asinh("},
        {"arccosh(", "acosh("},
        {"arctanh(", "atanh("},
        {"sinh(",   "sinh("},
        {"cosh(",   "cosh("},
        {"tanh(",   "tanh("},
        {"log2(",   "log2("},
        {"log10(",  "log("},  // log10 → log (base 10 evaluator)
        {"loge(",   "ln("},   // natural log
        {"cbrt(",   "cbrt("},
        {"floor(",  "floor("},
        {"ceil(",   "ceil("},
        {"round(",  "round("},
    };
    for (const auto& a : aliases) {
        size_t pos = 0;
        while ((pos = expr.find(a.alias, pos)) != std::string::npos) {
            expr.replace(pos, strlen(a.alias), a.standard);
            pos += strlen(a.standard);
        }
    }

    // Replace standalone "pi" with high-precision numeric value
    // This ensures sin(pi) → sin(3.141592653589793) which evaluates precisely
    {
        size_t pos = 0;
        while ((pos = expr.find("pi", pos)) != std::string::npos) {
            bool standalone = true;
            if (pos > 0 && std::isalpha((unsigned char)expr[pos-1])) standalone = false;
            size_t endPos = pos + 2;
            if (endPos < expr.size() && std::isalpha((unsigned char)expr[endPos])) standalone = false;
            if (standalone) {
                expr.replace(pos, 2, "3.141592653589793");
                pos += 16;
            } else {
                pos += 2;
            }
        }
    }

    // Replace standalone "e" (Euler's number, not scientific notation like 1e5) with numeric value
    {
        size_t pos = 0;
        while ((pos = expr.find('e', pos)) != std::string::npos) {
            bool isEuler = true;
            if (pos > 0) {
                char prev = expr[pos-1];
                if (std::isdigit((unsigned char)prev) || prev == '.') {
                    isEuler = false; // scientific notation like 1e5
                }
            }
            size_t endPos = pos + 1;
            if (endPos < expr.size()) {
                char next = expr[endPos];
                if (std::isalpha((unsigned char)next) || next == '(') {
                    isEuler = false; // part of function name
                }
            }
            if (isEuler) {
                expr.replace(pos, 1, "2.718281828459045");
                pos += 17;
            } else {
                pos += 1;
            }
        }
    }
}

// Set keyboard flash feedback
void calcTriggerKeyFlash() {
    g_state.keyFlashTimer = 8; // ~8 frames of highlight
}

// Evaluate current expression
void calcEvaluate() {
    if (g_state.display.empty()) return;

    std::string expr = g_state.display;

    // Preprocess: replace function aliases and constants with numeric values
    calcPreprocessExpression(expr);

    // Apply factorial
    calcApplyFactorial(expr);

    // Apply percentage
    calcApplyPercent(expr);

    // Apply angle mode
    expr = applyAngleMode(expr, g_state.angleMode);

    // Evaluate
    g_state.evaluator.setExpression(expr);
    if (g_state.evaluator.valid()) {
        // For constant expressions, evaluate at x=0
        double val = g_state.evaluator.evaluate(0);
        if (std::isfinite(val)) {
            // Format result
            std::ostringstream oss;
            oss << std::setprecision(12);
            // Remove trailing zeros
            oss << val;
            std::string resStr = oss.str();
            // Clean up
            auto dot = resStr.find('.');
            if (dot != std::string::npos) {
                auto last = resStr.find_last_not_of('0');
                if (last > dot) {
                    resStr = resStr.substr(0, last + 1);
                } else if (last == dot) {
                    resStr = resStr.substr(0, dot);
                }
            }

            // Store result
            g_state.lastAns = resStr;
            g_state.result = resStr;
            g_state.error.clear();
            g_state.hasResult = true;

            // Add to history
            HistoryEntry entry;
            entry.expression = g_state.display;
            entry.result = resStr;
            g_state.history.push_front(entry);
            if (g_state.history.size() > 20) {
                g_state.history.pop_back();
            }
        } else {
            g_state.error = T("Infinity or NaN", "无穷大或无效数值", "無限大または無効な値");
            g_state.result.clear();
        }
    } else {
        g_state.error = g_state.evaluator.lastError();
        g_state.result.clear();
    }
}

// Clear all
void calcClear() {
    g_state.display.clear();
    g_state.result.clear();
    g_state.error.clear();
    g_state.hasResult = false;
    g_state.inputBuf[0] = '\0';
    g_state.inputCursorPos = 0;
}

// Insert text at cursor position in display buffer
static void calcInsertAtCursor(const std::string& text) {
    int pos = g_state.inputCursorPos;
    if (pos < 0) pos = 0;
    if (pos > (int)g_state.display.size()) pos = (int)g_state.display.size();
    g_state.display.insert(pos, text);
    g_state.inputCursorPos = pos + (int)text.size();
}

// Delete last character (or at cursor position)
void calcDelete() {
    if (g_state.hasResult) {
        calcClear();
        return;
    }
    if (!g_state.display.empty()) {
        int pos = g_state.inputCursorPos;
        if (pos > 0) {
            g_state.display.erase(pos - 1, 1);
            g_state.inputCursorPos = pos - 1;
        }
    }
}

// Toggle angle mode
void calcToggleAngleMode() {
    switch (g_state.angleMode) {
        case DEG: g_state.angleMode = RAD; break;
        case RAD: g_state.angleMode = GRAD; break;
        case GRAD: g_state.angleMode = DEG; break;
    }
}

// Insert Ans value
void calcInsertAns() {
    if (g_state.hasResult) {
        g_state.display.clear();
        g_state.inputCursorPos = 0;
        g_state.hasResult = false;
    }
    calcInsertAtCursor(g_state.lastAns);
}

// Insert a named constant (returns numeric value as string)
void calcInsertConstant(const std::string& name) {
    if (g_state.hasResult) {
        g_state.display.clear();
        g_state.inputCursorPos = 0;
        g_state.hasResult = false;
    }

    if (name == "pi") {
        calcInsertAtCursor("3.141592653589793");
    } else if (name == "e") {
        calcInsertAtCursor("2.718281828459045");
    } else if (name == "c") {
        calcInsertAtCursor("299792458");
    } else if (name == "h") {
        calcInsertAtCursor("6.62607015e-34");
    } else if (name == "G") {
        calcInsertAtCursor("6.67430e-11");
    } else if (name == "NA") {
        calcInsertAtCursor("6.02214076e23");
    }
}

// Random number
void calcInsertRandom() {
    if (g_state.hasResult) {
        g_state.display.clear();
        g_state.inputCursorPos = 0;
        g_state.hasResult = false;
    }
    static std::mt19937_64 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double r = dist(rng);
    std::ostringstream oss;
    oss << std::setprecision(12) << r;
    calcInsertAtCursor(oss.str());
}

// Apply factorial: find n! patterns and replace with computed value
void calcApplyFactorial(std::string& expr) {
    size_t pos = 0;
    while ((pos = expr.find('!', pos)) != std::string::npos) {
        if (pos == 0) { pos++; continue; }

        // Find the number before '!'
        size_t start = pos;
        while (start > 0) {
            char c = expr[start - 1];
            if (std::isdigit(c) || c == '.') {
                start--;
            } else {
                break;
            }
        }

        std::string numStr = expr.substr(start, pos - start);
        if (!numStr.empty()) {
            double num = std::stod(numStr);
            // Only integer factorial
            long long n = (long long)num;
            if (n < 0 || n > 170 || std::abs(num - n) > 1e-10) {
                pos++;
                continue;
            }
            // Compute factorial
            long long result = 1;
            for (long long i = 2; i <= n; i++) {
                result *= i;
            }
            std::ostringstream oss;
            oss << result;
            expr.replace(start, pos - start + 1, oss.str());
            pos = start + oss.str().size();
        } else {
            pos++;
        }
    }
}

// Apply percentage: x% → x/100
void calcApplyPercent(std::string& expr) {
    size_t pos = 0;
    while ((pos = expr.find('%', pos)) != std::string::npos) {
        expr.replace(pos, 1, "/100");
        pos += 4;
    }
}

// Keyboard input
void calcKeyboardInput(int key, int mods) {
    calcTriggerKeyFlash();

    // Reset hasResult for digit/function input
    auto startFresh = [&]() {
        if (g_state.hasResult) {
            g_state.display.clear();
            g_state.inputCursorPos = 0;
            g_state.hasResult = false;
        }
    };

    // Digits
    if (key >= '0' && key <= '9') {
        startFresh();
        calcInsertAtCursor(std::string(1, (char)key));
        return;
    }

    // Decimal point
    if (key == '.') {
        startFresh();
        calcInsertAtCursor(".");
        return;
    }

    // Operators
    if (key == '+') {
        if (g_state.hasResult) {
            g_state.display = g_state.result;
            g_state.inputCursorPos = (int)g_state.display.size();
            g_state.hasResult = false;
        }
        calcInsertAtCursor("+");
        return;
    }
    if (key == '-') {
        if (g_state.hasResult) {
            g_state.display = g_state.result;
            g_state.inputCursorPos = (int)g_state.display.size();
            g_state.hasResult = false;
        }
        calcInsertAtCursor("-");
        return;
    }
    if (key == '*') {
        if (g_state.hasResult) {
            g_state.display = g_state.result;
            g_state.inputCursorPos = (int)g_state.display.size();
            g_state.hasResult = false;
        }
        calcInsertAtCursor("*");
        return;
    }
    if (key == '/') {
        if (g_state.hasResult) {
            g_state.display = g_state.result;
            g_state.inputCursorPos = (int)g_state.display.size();
            g_state.hasResult = false;
        }
        calcInsertAtCursor("/");
        return;
    }
    if (key == '^') {
        if (g_state.hasResult) {
            g_state.display = g_state.result;
            g_state.inputCursorPos = (int)g_state.display.size();
            g_state.hasResult = false;
        }
        calcInsertAtCursor("^");
        return;
    }

    // Parentheses
    if (key == '(') {
        startFresh();
        calcInsertAtCursor("(");
        return;
    }
    if (key == ')') {
        calcInsertAtCursor(")");
        return;
    }

    // Enter / = 
    if (key == 257 || key == '=') { // GLFW_KEY_ENTER = 257
        calcEvaluate();
        return;
    }

    // Backspace
    if (key == 259) { // GLFW_KEY_BACKSPACE = 259
        calcDelete();
        return;
    }

    // ESC = AC
    if (key == 256) { // GLFW_KEY_ESCAPE = 256
        calcClear();
        return;
    }

    // Arrow keys for cursor navigation in the manual input buffer
    if (key == 263) { // GLFW_KEY_LEFT
        if (g_state.inputCursorPos > 0) g_state.inputCursorPos--;
        return;
    }
    if (key == 262) { // GLFW_KEY_RIGHT
        if (g_state.inputCursorPos < (int)g_state.display.size()) g_state.inputCursorPos++;
        return;
    }
    if (key == 268) { // GLFW_KEY_HOME
        g_state.inputCursorPos = 0;
        return;
    }
    if (key == 269) { // GLFW_KEY_END
        g_state.inputCursorPos = (int)g_state.display.size();
        return;
    }

    // Letter keys for functions
    if (key == 's') { startFresh(); calcInsertAtCursor("sin("); return; }
    if (key == 'c') { startFresh(); calcInsertAtCursor("cos("); return; }
    if (key == 't') { startFresh(); calcInsertAtCursor("tan("); return; }
    if (key == 'l') { startFresh(); calcInsertAtCursor("log("); return; }
    if (key == 'n') { startFresh(); calcInsertAtCursor("ln("); return; }
    if (key == 'r') { startFresh(); calcInsertAtCursor("sqrt("); return; }
    // p and e now insert numeric constant values directly
    if (key == 'e') { startFresh(); calcInsertAtCursor("2.718281828459045"); return; }
    if (key == 'p') { startFresh(); calcInsertAtCursor("3.141592653589793"); return; }
}

// Handle button press from UI
void calcOnButton(const std::string& label) {
    calcTriggerKeyFlash();

    if (label == "AC") {
        calcClear();
        return;
    }
    if (label == "DEL") {
        calcDelete();
        return;
    }
    if (label == "=") {
        calcEvaluate();
        return;
    }
    if (label == "Ans") {
        calcInsertAns();
        return;
    }
    if (label == "rand") {
        calcInsertRandom();
        return;
    }
    if (label == "!") {
        if (g_state.hasResult) {
            g_state.display.clear();
            g_state.inputCursorPos = 0;
            g_state.hasResult = false;
        }
        calcInsertAtCursor("!");
        return;
    }
    if (label == "%") {
        if (g_state.hasResult) {
            g_state.display.clear();
            g_state.inputCursorPos = 0;
            g_state.hasResult = false;
        }
        calcInsertAtCursor("%");
        return;
    }

    // Number base indicator (for display only in the current implementation)
    if (label == "DEC" || label == "HEX" || label == "BIN" || label == "OCT") {
        return;
    }

    // Constants panel buttons
    if (label == "c" || label == "h" || label == "G" || label == "NA") {
        calcInsertConstant(label);
        return;
    }
    if (label == "π") {
        calcInsertConstant("pi");
        return;
    }

    // '(' and ')' as direct labels
    if (label == "(") {
        if (g_state.hasResult) {
            g_state.display.clear();
            g_state.inputCursorPos = 0;
            g_state.hasResult = false;
        }
        calcInsertAtCursor("(");
        return;
    }
    if (label == ")") {
        calcInsertAtCursor(")");
        return;
    }

    // 'e' button (euler's number)
    if (label == "e") {
        if (g_state.hasResult) {
            g_state.display.clear();
            g_state.inputCursorPos = 0;
            g_state.hasResult = false;
        }
        calcInsertAtCursor("2.718281828459045");
        return;
    }

    // Standard buttons: operators, digits, functions
    if (g_state.hasResult) {
        // After result, operators continue, everything else starts fresh
        if (label == "+" || label == "-" || label == "×" || label == "÷" || label == "^") {
            g_state.display = g_state.result;
            g_state.inputCursorPos = (int)g_state.display.size();
            g_state.hasResult = false;
        } else {
            g_state.display.clear();
            g_state.inputCursorPos = 0;
            g_state.hasResult = false;
            g_state.error.clear();
        }
    }

    // Map display symbols to evaluation symbols
    if (label == "×") {
        calcInsertAtCursor("*");
    } else if (label == "÷") {
        calcInsertAtCursor("/");
    } else if (label == "x²") {
        calcInsertAtCursor("^2");
    } else if (label == "x^y") {
        calcInsertAtCursor("^");
    } else if (label == "sin") {
        calcInsertAtCursor("sin(");
    } else if (label == "cos") {
        calcInsertAtCursor("cos(");
    } else if (label == "tan") {
        calcInsertAtCursor("tan(");
    } else if (label == "log") {
        calcInsertAtCursor("log(");
    } else if (label == "ln") {
        calcInsertAtCursor("ln(");
    } else if (label == "√") {
        calcInsertAtCursor("sqrt(");
    } else if (label == "exp") {
        calcInsertAtCursor("exp(");
    } else {
        calcInsertAtCursor(label);
    }
}

// Generate Fourier plot
void generateFourierPlot();

// Generate domain coloring
void generateDomainColoring();

// ============================================================
// Probability & Statistics implementations
// ============================================================

// Factorial for small integers (long long)
long long factorialLL(long long n) {
    if (n < 0) return 0;
    if (n <= 1) return 1;
    long long r = 1;
    for (long long i = 2; i <= n; i++) r *= i;
    return r;
}

// Double factorial for larger values (using double to avoid overflow)
static double factorialDouble(long long n) {
    if (n < 0) return 0;
    if (n <= 1) return 1.0;
    double r = 1.0;
    for (long long i = 2; i <= n; i++) r *= (double)i;
    return r;
}

// Permutation P(n,k) = n! / (n-k)!
long long permutationLL(long long n, long long k) {
    if (k < 0 || k > n) return 0;
    long long r = 1;
    for (long long i = n; i > n - k; i--) r *= i;
    return r;
}

// Combination C(n,k) = n! / (k! * (n-k)!)
long long combinationLL(long long n, long long k) {
    if (k < 0 || k > n) return 0;
    if (k == 0 || k == n) return 1;
    if (k > n - k) k = n - k; // Use symmetry
    long long r = 1;
    for (long long i = 0; i < k; i++) {
        r = r * (n - i) / (i + 1);
    }
    return r;
}

// Binomial probability
double binomialProb(long long n, long long k, double p) {
    if (k < 0 || k > n || p < 0 || p > 1) return 0;
    long long c = combinationLL(n, k);
    return (double)c * std::pow(p, (double)k) * std::pow(1.0 - p, (double)(n - k));
}

// Standard normal CDF using Abramowitz & Stegun approximation (error 1.5e-7)
double normalCDF(double x) {
    if (x < -8) return 0;
    if (x > 8) return 1;
    static const double a1 = 0.254829592;
    static const double a2 = -0.284496736;
    static const double a3 = 1.421413741;
    static const double a4 = -1.453152027;
    static const double a5 = 1.061405429;
    static const double p = 0.3275911;
    double sign = 1;
    if (x < 0) sign = -1;
    x = std::abs(x) / std::sqrt(2.0);
    double t = 1.0 / (1.0 + p * x);
    double y = 1.0 - (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * std::exp(-x * x);
    return 0.5 * (1.0 + sign * y);
}

// Inverse normal CDF (quantile function) using rational approximation
// Peter Acklam's algorithm
double normalInv(double p) {
    if (p <= 0.0) return -1e10;
    if (p >= 1.0) return 1e10;
    if (p == 0.5) return 0.0;

    static const double a[6] = {-3.969683028665376e+1, 2.209460984245205e+2,
                                -2.759285104469687e+2, 1.383577518672690e+2,
                                -3.066479806614716e+1, 2.506628277459239e+0};
    static const double b[5] = {-5.447609879822406e+1, 1.615858368580409e+2,
                                -1.556989798598866e+2, 6.680131188771972e+1,
                                -1.328068155288572e+1};
    static const double c[6] = {-7.784894002430293e-3, -3.223964580411365e-1,
                                -2.400758277161838e+0, -2.549732539343734e+0,
                                4.374664141464968e+0, 2.938163982698783e+0};
    static const double d[4] = {7.784695709041462e-3, 3.224671290700398e-1,
                                2.445134137142996e+0, 3.754408661907416e+0};

    double x, r;
    if (p < 0.02425) {
        // Lower region
        double q = std::sqrt(-2.0 * std::log(p));
        x = (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5])
            / ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
    } else if (p > 0.97575) {
        // Upper region
        double q = std::sqrt(-2.0 * std::log(1.0 - p));
        x = -(((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5])
             / ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
    } else {
        // Central region
        double q = p - 0.5;
        r = q * q;
        x = (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * q
            / (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1.0);
    }

    // Refine using one Newton iteration
    r = normalCDF(x) - p;
    double derv = std::exp(-x * x / 2.0) / std::sqrt(2.0 * std::numbers::pi);
    if (derv != 0) x -= r / derv;
    return x;
}

// Poisson distribution
double poissonProb(double lambda, long long k) {
    if (lambda <= 0 || k < 0) return 0;
    return std::exp(-lambda) * std::pow(lambda, (double)k) / factorialDouble(k);
}

// Parse comma-separated data string
std::vector<double> parseDataList(const char* str) {
    std::vector<double> data;
    std::string s = str;
    size_t start = 0, end;
    while ((end = s.find_first_of(",\n; ", start)) != std::string::npos) {
        if (end > start) {
            try {
                data.push_back(std::stod(s.substr(start, end - start)));
            } catch (...) {}
        }
        start = end + 1;
    }
    if (start < s.size()) {
        try {
            data.push_back(std::stod(s.substr(start)));
        } catch (...) {}
    }
    return data;
}

// Descriptive statistics
DescriptiveStats calcDescriptiveStats(const std::vector<double>& data) {
    DescriptiveStats s;
    s.count = (long long)data.size();
    if (data.empty()) return s;

    // Basic
    s.minVal = data[0];
    s.maxVal = data[0];
    s.sum = 0;
    for (double v : data) {
        s.sum += v;
        if (v < s.minVal) s.minVal = v;
        if (v > s.maxVal) s.maxVal = v;
    }
    s.mean = s.sum / s.count;
    s.range = s.maxVal - s.minVal;

    // Median
    std::vector<double> sorted = data;
    std::sort(sorted.begin(), sorted.end());
    if (s.count % 2 == 0) {
        s.median = (sorted[s.count/2 - 1] + sorted[s.count/2]) / 2.0;
    } else {
        s.median = sorted[s.count / 2];
    }

    // Variance & StdDev
    s.variance = 0;
    for (double v : data) {
        double d = v - s.mean;
        s.variance += d * d;
    }
    s.variance /= s.count; // population variance
    s.stddev = std::sqrt(s.variance);

    // Mode
    std::map<double, int> freq;
    int maxFreq = 0;
    for (double v : data) freq[v]++;
    for (auto& p : freq) {
        if (p.second > maxFreq) maxFreq = p.second;
    }
    if (maxFreq > 1) {
        for (auto& p : freq) {
            if (p.second == maxFreq) s.modeVals.push_back(p.first);
        }
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < s.modeVals.size(); i++) {
            if (i > 0) oss << ", ";
            oss << s.modeVals[i];
        }
        oss << "]";
        s.modeStr = oss.str();
    } else {
        s.modeStr = "无众数";
    }

    return s;
}

// Generate histogram bins
void calcHistogram(const std::vector<double>& data, int nBins,
                   std::vector<double>& counts,
                   double& dataMin, double& dataMax) {
    counts.clear();
    if (data.empty() || nBins < 1) return;

    dataMin = *std::min_element(data.begin(), data.end());
    dataMax = *std::max_element(data.begin(), data.end());

    if (dataMax == dataMin) {
        // All same value — one bin
        counts.push_back((double)data.size());
        return;
    }

    double binWidth = (dataMax - dataMin) / nBins;
    counts.resize(nBins, 0);
    for (double v : data) {
        int bin = (int)((v - dataMin) / binWidth);
        if (bin >= nBins) bin = nBins - 1;
        if (bin < 0) bin = 0;
        counts[bin]++;
    }
}

// ============================================================
// Volume calculations
// ============================================================
double sphereVolume(double r) {
    return (4.0 / 3.0) * std::numbers::pi * r * r * r;
}

double cylinderVolume(double r, double h) {
    return std::numbers::pi * r * r * h;
}

double coneVolume(double r, double h) {
    return (1.0 / 3.0) * std::numbers::pi * r * r * h;
}

double boxVolume(double a, double b, double c) {
    return a * b * c;
}

// ============================================================
// Surface Integral (Monte Carlo)
// ============================================================

// Evaluate scalar field expression at x,y,z using math_engine
double evalField(const std::string& expr, double x, double y, double z) {
    ExpressionEvaluator ee;
    ee.setExpression(expr);
    if (!ee.valid()) return 0;
    // We pass x as the variable, but need x,y,z as separate variables
    // The evaluator only has variable 'x' built-in
    // We'll use a trick: substitute x,y,z positions
    // Actually the math_engine evaluator only supports 'x' as the variable
    // So for surface integrals we'll use the expression with 'x' and substitute
    return ee.evaluate(x);
}

// Monte Carlo surface integral
double monteCarloSurfaceIntegral(
    std::function<double(double,double,double)> field,
    std::function<double(double,double)> px,
    std::function<double(double,double)> py,
    std::function<double(double,double)> pz,
    double uMin, double uMax,
    double vMin, double vMax,
    int nSamples)
{
    static std::mt19937_64 rngMC(std::random_device{}());
    std::uniform_real_distribution<double> distU(uMin, uMax);
    std::uniform_real_distribution<double> distV(vMin, vMax);

    double du = uMax - uMin;
    double dv = vMax - vMin;
    double area = du * dv;

    double sum = 0;
    double h = 1e-5;

    for (int i = 0; i < nSamples; i++) {
        double u = distU(rngMC);
        double v = distV(rngMC);

        // Evaluate surface point
        double x = px(u, v);
        double y = py(u, v);
        double z = pz(u, v);

        // Approximate dS = |∂r/∂u × ∂r/∂v| du dv using finite differences
        double xu = (px(u + h, v) - px(u - h, v)) / (2 * h);
        double yu = (py(u + h, v) - py(u - h, v)) / (2 * h);
        double zu = (pz(u + h, v) - pz(u - h, v)) / (2 * h);

        double xv = (px(u, v + h) - px(u, v - h)) / (2 * h);
        double yv = (py(u, v + h) - py(u, v - h)) / (2 * h);
        double zv = (pz(u, v + h) - pz(u, v - h)) / (2 * h);

        // Cross product
        double cx = yu * zv - zu * yv;
        double cy = zu * xv - xu * zv;
        double cz = xu * yv - yu * xv;
        double dS = std::sqrt(cx * cx + cy * cy + cz * cz);

        sum += field(x, y, z) * dS;
    }

    return area * sum / nSamples;
}

// ============================================================
// Derivative graph helpers
// ============================================================

std::vector<double> findZeroCrossings(const std::function<double(double)>& f,
                                       double xMin, double xMax, int steps) {
    std::vector<double> zeros;
    double dx = (xMax - xMin) / steps;
    double prev = f(xMin);
    for (int i = 1; i <= steps; i++) {
        double x = xMin + i * dx;
        double cur = f(x);
        if (prev * cur < 0) {
            // Refine with bisection
            double a = x - dx, b = x;
            for (int j = 0; j < 20; j++) {
                double m = (a + b) / 2;
                double fm = f(m);
                if (fm == 0) { a = b = m; break; }
                if (prev * fm < 0) { b = m; cur = fm; }
                else { a = m; prev = fm; }
            }
            zeros.push_back((a + b) / 2);
        } else if (std::abs(cur) < 1e-8) {
            zeros.push_back(x);
        }
        prev = cur;
    }
    return zeros;
}

std::vector<double> findLocalExtremes(const std::function<double(double)>& f,
                                       double xMin, double xMax, int steps) {
    // Find points where derivative changes sign (zero crossing of derivative)
    auto deriv = [&](double x) {
        double h = 1e-6;
        return (f(x + h) - f(x - h)) / (2 * h);
    };
    return findZeroCrossings(deriv, xMin, xMax, steps);
}

// ============================================================
// 3D Surface helper
// ============================================================
double eval3DFunc(const std::string& expr, double x, double y) {
    // Substitute into expression - the evaluator uses 'x' variable
    // For 3D surface we'll build a special expression replacing x and y
    // Since the evaluator only supports 'x', we build a composite expression
    std::string e = expr;
    // Replace 'y' with its value as text
    std::string yStr = std::to_string(y);
    // Simple approach: replace 'y' with numeric value (works for basic expressions)
    size_t pos = 0;
    while ((pos = e.find('y', pos)) != std::string::npos) {
        // Check it's standalone y
        bool standalone = true;
        if (pos > 0 && (std::isalnum((unsigned char)e[pos-1]) || e[pos-1] == '_'))
            standalone = false;
        if (pos + 1 < e.size() && (std::isalnum((unsigned char)e[pos+1]) || e[pos+1] == '_'))
            standalone = false;
        if (standalone) {
            e.replace(pos, 1, "(" + yStr + ")");
            pos += yStr.size() + 2;
        } else {
            pos++;
        }
    }
    // Now evaluate with the x value
    ExpressionEvaluator ee;
    ee.setExpression(e);
    if (!ee.valid()) return 0;
    double val = ee.evaluate(x);
    if (!std::isfinite(val)) return 0;
    return val;
}

// ============================================================
// Extension API implementations
// ============================================================

// Currency converter: input like "100 USD" or "650 CNY"
const char* extCurrency(const char* input) {
    static std::string result;
    result.clear();
    double amount = 0;
    std::string in = input;
    // Remove spaces and parse
    std::string numStr, currency, toCurrency;
    size_t i = 0;
    while (i < in.size() && (std::isspace((unsigned char)in[i]) || in[i] == '\"')) i++;
    while (i < in.size() && (std::isdigit((unsigned char)in[i]) || in[i] == '.')) {
        numStr += in[i]; i++;
    }
    while (i < in.size() && (std::isspace((unsigned char)in[i]) || in[i] == '\"')) i++;
    while (i < in.size() && std::isalpha((unsigned char)in[i])) {
        currency += std::toupper((unsigned char)in[i]); i++;
    }
    // Optional "to" target (e.g. "100 USD to CNY")
    while (i < in.size() && (std::isspace((unsigned char)in[i]) || in[i] == '\"')) i++;
    // Skip "to" or "in" or "->"
    if (i + 2 <= in.size() &&
        (in.substr(i, 2) == "to" || in.substr(i, 2) == "in")) {
        i += 2;
    } else if (i + 1 < in.size() && in[i] == '-' && in[i+1] == '>') {
        i += 2;
    }
    while (i < in.size() && (std::isspace((unsigned char)in[i]) || in[i] == '\"')) i++;
    while (i < in.size() && std::isalpha((unsigned char)in[i])) {
        toCurrency += std::toupper((unsigned char)in[i]); i++;
    }

    if (numStr.empty()) {
        result = "格式: 金额 货币 [to 目标货币], 如 \"100 USD to CNY\"";
        return result.c_str();
    }
    try { amount = std::stod(numStr); } catch (...) {
        result = "无效金额";
        return result.c_str();
    }

    // --- Use live rates if available ---
    bool useLive = !g_state.exchangeRates.empty();
    double inUSD = 0;

    if (useLive) {
        auto itUSD = g_state.exchangeRates.find(currency);
        if (itUSD != g_state.exchangeRates.end()) {
            inUSD = amount / itUSD->second;
        } else {
            useLive = false;
        }
    }

    // Fallback hardcoded rates
    const double usd_to_cny = 7.24;
    const double eur_to_usd = 1.08;
    const double jpy_to_usd = 0.0067;
    const double gbp_to_usd = 1.27;

    if (!useLive) {
        if (currency == "USD") inUSD = amount;
        else if (currency == "CNY") inUSD = amount / usd_to_cny;
        else if (currency == "EUR") inUSD = amount * eur_to_usd;
        else if (currency == "JPY") inUSD = amount * jpy_to_usd;
        else if (currency == "GBP") inUSD = amount * gbp_to_usd;
        else {
            result = "支持的货币: USD, CNY, EUR, JPY, GBP (或联网获取实时汇率)";
            return result.c_str();
        }
    }

    std::ostringstream oss;
    oss.precision(2);
    oss << std::fixed;
    oss << amount << " " << currency;
    if (!toCurrency.empty()) {
        // Convert to specific target
        if (useLive) {
            auto itTo = g_state.exchangeRates.find(toCurrency);
            if (itTo != g_state.exchangeRates.end()) {
                double conv = inUSD * itTo->second;
                oss << " = " << conv << " " << toCurrency;
                oss << "\n(1 " << currency << " = " << (itTo->second / g_state.exchangeRates[currency]) << " " << toCurrency << ")";
            } else {
                oss << ": 不支持 " << toCurrency;
            }
        } else {
            oss << " to " << toCurrency;
            // Fallback with approximate
            double rate = 1;
            if (currency == "USD" && toCurrency == "CNY") rate = usd_to_cny;
            else if (currency == "CNY" && toCurrency == "USD") rate = 1.0/usd_to_cny;
            else if (currency == "USD" && toCurrency == "EUR") rate = 1.0/eur_to_usd;
            else if (currency == "EUR" && toCurrency == "USD") rate = eur_to_usd;
            else { oss << "\n(使用近似汇率)"; }
            oss << "\n= " << (amount * rate) << " " << toCurrency;
        }
    } else {
        oss << " (USD $" << inUSD << ") =";
        if (currency == "USD") {
            if (useLive) {
                oss << "\n  CNY ¥" << (inUSD * g_state.exchangeRates["CNY"]);
                oss << "\n  EUR €" << (inUSD * g_state.exchangeRates["EUR"]);
                oss << "\n  JPY ¥" << (inUSD * g_state.exchangeRates["JPY"]);
                oss << "\n  GBP £" << (inUSD * g_state.exchangeRates["GBP"]);
                oss << "\n  KRW ₩" << (inUSD * g_state.exchangeRates["KRW"]);
                oss << "\n  HKD HK$" << (inUSD * g_state.exchangeRates["HKD"]);
            } else {
                oss << "\n  CNY ¥" << (inUSD * usd_to_cny);
                oss << "\n  EUR €" << (inUSD / eur_to_usd);
                oss << "\n  JPY ¥" << (inUSD / jpy_to_usd);
                oss << "\n  GBP £" << (inUSD / gbp_to_usd);
            }
        } else {
            oss << "\n  USD $" << inUSD;
            if (useLive && currency != "CNY")
                oss << "\n  CNY ¥" << (inUSD * g_state.exchangeRates["CNY"]);
            else if (!useLive)
                oss << "\n  CNY ¥" << (inUSD * usd_to_cny);
        }
    }
    if (useLive) {
        oss << "\n\n(实时汇率)";
    }
    result = oss.str();
    return result.c_str();
}

// BMI Calculator: input like "70 1.75" (kg, meters) or "154 5'9\""
const char* extBMI(const char* input) {
    static std::string result;
    result.clear();
    double weight = 0, height = 0;
    std::string in = input;
    // Parse two numbers
    std::vector<double> nums;
    std::string cur;
    for (char c : in) {
        if (std::isdigit((unsigned char)c) || c == '.' || c == '-') {
            cur += c;
        } else if (!cur.empty()) {
            try { nums.push_back(std::stod(cur)); } catch (...) {}
            cur.clear();
        }
    }
    if (!cur.empty()) {
        try { nums.push_back(std::stod(cur)); } catch (...) {}
    }
    if (nums.size() < 2) {
        result = "格式: 体重(kg) 身高(m), 如 \"70 1.75\"";
        return result.c_str();
    }
    weight = nums[0];
    height = nums[1];
    if (height > 3) height /= 100; // Assume cm if > 3

    double bmi = weight / (height * height);
    std::ostringstream oss;
    oss.precision(1);
    oss << std::fixed;
    oss << "BMI = " << bmi << "\n";
    const char* category;
    if (bmi < 18.5) category = "偏瘦 (体重过轻)";
    else if (bmi < 24.0) category = "正常 (健康体重)";
    else if (bmi < 28.0) category = "偏胖 (超重)";
    else category = "肥胖 (肥胖)";
    oss << "状态: " << category << "\n";
    oss << "健康体重范围: " << (18.5 * height * height) << " ~ "
        << (24.0 * height * height) << " kg";
    result = oss.str();
    return result.c_str();
}

// Temperature converter: input like "100C" or "212F" or "300K"
const char* extTemperature(const char* input) {
    static std::string result;
    result.clear();
    std::string in = input;
    // Extract number
    std::string numStr, unit;
    size_t i = 0;
    while (i < in.size() && (std::isspace((unsigned char)in[i]) || in[i] == '\"')) i++;
    while (i < in.size() && (std::isdigit((unsigned char)in[i]) || in[i] == '.' || in[i] == '-')) {
        numStr += in[i]; i++;
    }
    while (i < in.size() && (std::isspace((unsigned char)in[i]))) i++;
    if (i < in.size()) {
        char u = std::toupper((unsigned char)in[i]);
        if (u == 'C' || u == 'F' || u == 'K') unit = u;
        else if (in.substr(i, 2) == "°C" || in.substr(i, 2) == "℃") unit = "C";
        else if (in.substr(i, 2) == "°F" || in.substr(i, 2) == "℉") unit = "F";
    }
    if (numStr.empty() || unit.empty()) {
        result = "格式: 温度值+单位(C/F/K), 如 \"100C\" 或 \"212F\"";
        return result.c_str();
    }
    double temp;
    try { temp = std::stod(numStr); } catch (...) {
        result = "无效温度值";
        return result.c_str();
    }

    double celsius;
    if (unit == "C") celsius = temp;
    else if (unit == "F") celsius = (temp - 32) * 5.0 / 9.0;
    else if (unit == "K") celsius = temp - 273.15;
    else { result = "单位: C(摄氏), F(华氏), K(开尔文)"; return result.c_str(); }

    std::ostringstream oss;
    oss.precision(2);
    oss << std::fixed;
    oss << temp << "°" << unit << " =\n";
    oss << "  摄氏: " << celsius << " °C\n";
    oss << "  华氏: " << (celsius * 9.0 / 5.0 + 32) << " °F\n";
    oss << "  开尔文: " << (celsius + 273.15) << " K";
    result = oss.str();
    return result.c_str();
}

// Initialize built-in extensions
void initExtensions() {
    g_state.extensions.clear();
    g_state.extensions.push_back({
        "货币换算",
        extCurrency,
        "货币兑换: USD/CNY/EUR/JPY/GBP",
        "100 USD",
        "currency"
    });
    g_state.extensions.push_back({
        "BMI 计算器",
        extBMI,
        "体重指数计算: 输入体重(kg)和身高(m)",
        "70 1.75",
        "bmi"
    });
    g_state.extensions.push_back({
        "温度转换",
        extTemperature,
        "摄氏/华氏/开尔文温度转换",
        "100C",
        "temp"
    });
}

// ============================================================
// Currency display data
// ============================================================
const char* g_currencyCodes[] = {
    "USD", "EUR", "GBP", "JPY", "CNY", "KRW", "HKD", "TWD",
    "AUD", "CAD", "CHF", "SGD", "INR", "RUB", "BRL"
};
const char* g_currencyNames[] = {
    "美元", "欧元", "英镑", "日元", "人民币", "韩元", "港币", "台币",
    "澳元", "加元", "瑞士法郎", "新加坡元", "印度卢比", "卢布", "巴西雷亚尔"
};
const int g_currencyCount = 15;

// ============================================================
// Language toggle: EN → ZH → JA → EN
// ============================================================
void calcToggleLang() {
    switch (g_state.lang) {
        case LANG_EN: g_state.lang = LANG_ZH; break;
        case LANG_ZH: g_state.lang = LANG_JA; break;
        case LANG_JA: g_state.lang = LANG_EN; break;
    }
}

// ============================================================
// Currency exchange rate functions
// ============================================================

// Fetch exchange rates from open.er-api.com (free, no API key needed)
bool fetchExchangeRates() {
    std::string json = execShellAll(
        "curl -s --connect-timeout 5 --max-time 10 "
        "\"https://open.er-api.com/v6/latest/USD\""
    );

    if (json.empty()) {
        if (g_state.exchangeRates.empty()) {
            g_state.exchangeRateInfo = T(
                "Network unavailable - cannot fetch rates",
                "网络不可用 - 无法获取汇率",
                "ネットワーク不可 - レート取得不可"
            );
        } else {
            g_state.exchangeRateInfo = T(
                "Network unavailable - using cached rates",
                "网络不可用 - 使用缓存汇率",
                "ネットワーク不可 - キャッシュを使用"
            );
        }
        return false;
    }

    // Parse JSON: find "rates": { block
    auto ratesPos = json.find("\"rates\"");
    if (ratesPos == std::string::npos) {
        g_state.exchangeRateInfo = T(
            "Failed to parse exchange rate data",
            "解析汇率数据失败",
            "為替レートデータの解析に失敗"
        );
        return false;
    }

    auto bracePos = json.find('{', ratesPos);
    if (bracePos == std::string::npos) return false;

    // Clear old rates
    g_state.exchangeRates.clear();

    // Parse each "CODE": value pair
    size_t pos = bracePos + 1;
    while (pos < json.size()) {
        // Skip whitespace, commas, newlines
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == ',' ||
               json[pos] == '\n' || json[pos] == '\r' || json[pos] == '\t'))
            pos++;
        if (pos >= json.size() || json[pos] == '}') break;

        // Find opening quote of currency code
        if (json[pos] != '"') { pos++; continue; }
        pos++;
        size_t codeStart = pos;
        while (pos < json.size() && json[pos] != '"') pos++;
        if (pos >= json.size()) break;
        std::string code = json.substr(codeStart, pos - codeStart);
        pos++; // past closing quote

        // Skip colon and whitespace
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':')) pos++;

        // Parse value
        char* end = nullptr;
        double rate = std::strtod(json.c_str() + pos, &end);
        if (end != json.c_str() + pos) {
            g_state.exchangeRates[code] = rate;
            pos = end - json.c_str();
        } else {
            pos++;
        }
    }

    if (g_state.exchangeRates.empty()) {
        g_state.exchangeRateInfo = T(
            "No exchange rates found in response",
            "响应中未找到汇率数据",
            "応答に為替レートデータが見つかりません"
        );
        return false;
    }

    // Store timestamp
    g_state.exchangeRateTime = time(nullptr);

    // Format time string
    char timeBuf[64];
    struct tm* t = localtime(&g_state.exchangeRateTime);
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", t);

    g_state.exchangeRateInfo = T(
        "Rates updated at: ", "汇率更新时间: ", "レート更新時刻: "
    );
    g_state.exchangeRateInfo += timeBuf;

    return true;
}

// Convert currency: amount in 'from' currency → result in 'to' currency
double convertCurrency(double amount, const std::string& from, const std::string& to) {
    if (g_state.exchangeRates.empty()) return 0.0;

    auto itFrom = g_state.exchangeRates.find(from);
    auto itTo = g_state.exchangeRates.find(to);

    if (itFrom == g_state.exchangeRates.end() || itTo == g_state.exchangeRates.end())
        return 0.0;

    // Convert: from → USD → to
    // Rates are relative to USD: rate = value of 1 USD in that currency
    // So: amount in USD = amount_from / rate_from
    // Then: amount in to = usd_amount * rate_to
    double inUSD = amount / itFrom->second;
    return inUSD * itTo->second;
}

// Format exchange rate for display
std::string formatExchangeRate(double rate) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << rate;
    return oss.str();
}
