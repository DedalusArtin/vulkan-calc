#include "calc_logic.hpp"
#include "locale.hpp"
#include <string>
#include <cmath>
#include <numbers>
#include <random>
#include <sstream>

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

// Evaluate current expression
void calcEvaluate() {
    if (g_state.display.empty()) return;

    std::string expr = g_state.display;

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
}

// Delete last character
void calcDelete() {
    if (g_state.hasResult) {
        calcClear();
        return;
    }
    if (!g_state.display.empty()) {
        g_state.display.pop_back();
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
        g_state.hasResult = false;
    }
    g_state.display += g_state.lastAns;
}

// Insert a named constant (returns numeric value as string)
void calcInsertConstant(const std::string& name) {
    if (g_state.hasResult) {
        g_state.display.clear();
        g_state.hasResult = false;
    }

    if (name == "pi") {
        g_state.display += "pi";
    } else if (name == "e") {
        g_state.display += "e";
    } else if (name == "c") {
        g_state.display += "299792458";
    } else if (name == "h") {
        g_state.display += "6.62607015e-34";
    } else if (name == "G") {
        g_state.display += "6.67430e-11";
    } else if (name == "NA") {
        g_state.display += "6.02214076e23";
    }
}

// Random number
void calcInsertRandom() {
    if (g_state.hasResult) {
        g_state.display.clear();
        g_state.hasResult = false;
    }
    static std::mt19937_64 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double r = dist(rng);
    std::ostringstream oss;
    oss << std::setprecision(12) << r;
    g_state.display += oss.str();
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
    // Reset hasResult for digit/function input
    auto startFresh = [&]() {
        if (g_state.hasResult) {
            g_state.display.clear();
            g_state.hasResult = false;
        }
    };

    // Digits
    if (key >= '0' && key <= '9') {
        startFresh();
        // If last char was ')' or result was just shown, start fresh
        g_state.display += (char)key;
        return;
    }

    // Decimal point
    if (key == '.') {
        startFresh();
        g_state.display += '.';
        return;
    }

    // Operators
    if (key == '+') {
        if (g_state.hasResult) {
            g_state.display = g_state.result;
            g_state.hasResult = false;
        }
        g_state.display += '+';
        return;
    }
    if (key == '-') {
        if (g_state.hasResult) {
            g_state.display = g_state.result;
            g_state.hasResult = false;
        }
        g_state.display += '-';
        return;
    }
    if (key == '*') {
        if (g_state.hasResult) {
            g_state.display = g_state.result;
            g_state.hasResult = false;
        }
        g_state.display += '*';
        return;
    }
    if (key == '/') {
        if (g_state.hasResult) {
            g_state.display = g_state.result;
            g_state.hasResult = false;
        }
        g_state.display += '/';
        return;
    }
    if (key == '^') {
        if (g_state.hasResult) {
            g_state.display = g_state.result;
            g_state.hasResult = false;
        }
        g_state.display += '^';
        return;
    }

    // Parentheses
    if (key == '(') {
        startFresh();
        g_state.display += '(';
        return;
    }
    if (key == ')') {
        g_state.display += ')';
        return;
    }

    // Enter / = 
    if (key == '\n' || key == '\r' || key == '=') {
        calcEvaluate();
        return;
    }

    // Backspace
    if (key == 263) { // GLFW_KEY_BACKSPACE
        calcDelete();
        return;
    }

    // ESC = AC
    if (key == 256) { // GLFW_KEY_ESCAPE
        calcClear();
        return;
    }

    // Letter keys for functions
    if (key == 's') { startFresh(); g_state.display += "sin("; return; }
    if (key == 'c') { startFresh(); g_state.display += "cos("; return; }
    if (key == 't') { startFresh(); g_state.display += "tan("; return; }
    if (key == 'l') { startFresh(); g_state.display += "log("; return; }
    if (key == 'n') { startFresh(); g_state.display += "ln("; return; }
    if (key == 'r') { startFresh(); g_state.display += "sqrt("; return; }
    if (key == 'e') { startFresh(); g_state.display += "e"; return; }  // e constant
    if (key == 'p') { startFresh(); g_state.display += "pi"; return; } // pi
}

// Handle button press from UI
void calcOnButton(const std::string& label) {
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
            g_state.hasResult = false;
        }
        g_state.display += "!";
        return;
    }
    if (label == "%") {
        if (g_state.hasResult) {
            g_state.display.clear();
            g_state.hasResult = false;
        }
        g_state.display += "%";
        return;
    }

    // Number base indicator (for display only in the current implementation)
    if (label == "DEC" || label == "HEX" || label == "BIN" || label == "OCT") {
        // Toggle would go here — for now display only
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

    // 'e' button (euler's number)
    if (label == "e" && g_state.display.find("e") == std::string::npos) {
        // Simple check — if display ends with something that makes "e" a constant
        if (g_state.hasResult) {
            g_state.display.clear();
            g_state.hasResult = false;
        }
        g_state.display += "e";
        return;
    }

    // Standard buttons: operators, digits, functions
    if (g_state.hasResult) {
        // After result, operators continue, everything else starts fresh
        if (label == "+" || label == "-" || label == "×" || label == "÷" || label == "^") {
            g_state.display = g_state.result;
            g_state.hasResult = false;
        } else {
            g_state.display.clear();
            g_state.hasResult = false;
            g_state.error.clear();
        }
    }

    // Map display symbols to evaluation symbols
    if (label == "×") {
        g_state.display += "*";
    } else if (label == "÷") {
        g_state.display += "/";
    } else if (label == "x²") {
        g_state.display += "^2";
    } else if (label == "x^y") {
        g_state.display += "^";
    } else if (label == "sin") {
        g_state.display += "sin(";
    } else if (label == "cos") {
        g_state.display += "cos(";
    } else if (label == "tan") {
        g_state.display += "tan(";
    } else if (label == "log") {
        g_state.display += "log(";
    } else if (label == "ln") {
        g_state.display += "ln(";
    } else if (label == "√") {
        g_state.display += "sqrt(";
    } else if (label == "exp") {
        g_state.display += "exp(";
    } else {
        g_state.display += label;
    }
}

// Generate Fourier plot
void generateFourierPlot();

// Generate domain coloring
void generateDomainColoring();
