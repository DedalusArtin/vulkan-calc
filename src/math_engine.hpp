#ifndef MATH_ENGINE_HPP
#define MATH_ENGINE_HPP

#include <complex>
#include <vector>
#include <string>
#include <functional>
#include <cmath>
#include <sstream>
#include <iomanip>

// ============================================================
// Advanced Math Engine
// Supports: real/complex functions, numerical integration,
// differentiation, Cauchy integral formula, series expansion
// ============================================================

// -----------------------------------------------------------
// Complex number (wraps std::complex with extra utilities)
// -----------------------------------------------------------
using Complex = std::complex<double>;

inline Complex C(double re, double im = 0.0) { return Complex(re, im); }

inline std::string to_string(const Complex& z, int prec = 6) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(prec);
    if (std::abs(z.imag()) < 1e-15) {
        oss << z.real();
    } else if (std::abs(z.real()) < 1e-15) {
        oss << z.imag() << "i";
    } else {
        oss << z.real() << (z.imag() >= 0 ? " + " : " - ")
            << std::abs(z.imag()) << "i";
    }
    return oss.str();
}

// -----------------------------------------------------------
// Function types
// -----------------------------------------------------------
using RealFunc   = std::function<double(double)>;
using ComplexFunc = std::function<Complex(Complex)>;

// -----------------------------------------------------------
// Numerical Integration
// -----------------------------------------------------------
struct IntegrationResult {
    double value;
    double error_estimate;
    int    iterations;
};

class Integrator {
public:
    // Adaptive Simpson's rule
    static IntegrationResult adaptiveSimpson(
        const RealFunc& f, double a, double b,
        double tol = 1e-10, int max_depth = 50);

    // Romberg integration
    static IntegrationResult romberg(
        const RealFunc& f, double a, double b,
        int n = 20, double tol = 1e-10);

    // Gauss-Legendre quadrature (adaptive)
    static IntegrationResult gaussLegendre(
        const RealFunc& f, double a, double b,
        int n = 100, double tol = 1e-10);

    // Monte Carlo integration (for higher dimensions)
    static IntegrationResult monteCarlo(
        const RealFunc& f, double a, double b,
        int n_samples = 100000);

    // Contour integration (complex)
    static Complex contourIntegrate(
        const ComplexFunc& f,
        const std::function<Complex(double)>& contour,
        double t_start, double t_end,
        int n_steps = 1000);
};

// -----------------------------------------------------------
// Differentiation
// -----------------------------------------------------------
struct DerivativeResult {
    double value;
    double error_estimate;
};

class Differentiator {
public:
    // First derivative (central difference, adaptive step)
    static DerivativeResult derivative(
        const RealFunc& f, double x,
        double h = 1e-8);

    // Second derivative
    static DerivativeResult secondDerivative(
        const RealFunc& f, double x,
        double h = 1e-5);

    // Partial derivative placeholder (for multi-variable)
    static double partialDerivative(
        const std::function<double(double)>& f, double x,
        double h = 1e-8);

    // Complex derivative via Cauchy-Riemann check
    static Complex complexDerivative(
        const ComplexFunc& f, Complex z,
        double h = 1e-8);
};

// -----------------------------------------------------------
// Complex Analysis
// -----------------------------------------------------------
class ComplexAnalysis {
public:
    // Cauchy Integral Formula: f^(n)(a) = n!/(2πi) ∮ f(z)/(z-a)^(n+1) dz
    static Complex cauchyIntegral(
        const ComplexFunc& f, Complex a, double radius,
        int derivative_order = 0, int n_points = 1000);

    // Residue at a simple pole via limit
    static Complex residue(
        const ComplexFunc& f, Complex pole,
        double tol = 1e-10);

    // Laurent series coefficients (numerical)
    static Complex laurentCoefficient(
        const ComplexFunc& f, Complex a, int n,
        double radius = 1.0, int n_points = 1000);
};

// -----------------------------------------------------------
// Special Functions
// -----------------------------------------------------------
class SpecialFunctions {
public:
    static double gamma(double x);
    static double erf(double x);
    static double besselJ(int n, double x);
    static double besselY(int n, double x);
    static Complex gamma(const Complex& z);
};

// -----------------------------------------------------------
// Real Function Plot Data Generator
// -----------------------------------------------------------
struct PlotPoint {
    double x, y;
};

struct PlotData {
    std::vector<PlotPoint> points;
    double x_min, x_max;
    double y_min, y_max;
    std::string label;
};

class PlotGenerator {
public:
    static PlotData generate(
        const RealFunc& f, double x_min, double x_max,
        int n_points = 500, const std::string& label = "");

    // Generate multiple functions on same axes
    static std::vector<PlotData> generateMultiple(
        const std::vector<RealFunc>& funcs,
        double x_min, double x_max,
        int n_points = 500,
        const std::vector<std::string>& labels = {});

    // Complex function on complex plane -> color mapping
    // Domain coloring: hue = arg(f(z)), brightness = |f(z)|
    struct DomainColorPoint {
        double x, y;     // z-coordinate
        float r, g, b;   // color
    };
    static std::vector<DomainColorPoint> domainColoring(
        const ComplexFunc& f,
        double x_min, double x_max,
        double y_min, double y_max,
        int x_res = 400, int y_res = 400);
};

// -----------------------------------------------------------
// Expression evaluator (simple RPN-based)
// -----------------------------------------------------------
class ExpressionEvaluator {
public:
    enum TokenType { NUMBER, VARIABLE, OP_PLUS, OP_MINUS, OP_MULT,
                     OP_DIV, OP_POW, FUNC_SIN, FUNC_COS, FUNC_TAN,
                     FUNC_LOG, FUNC_LN, FUNC_EXP, FUNC_SQRT,
                     FUNC_ABS, PAREN_OPEN, PAREN_CLOSE, CONSTANT };

    struct Token {
        TokenType type;
        double    value;
        std::string name;
    };

    ExpressionEvaluator();

    // Parse expression string (e.g., "sin(x) + cos(x)")
    void setExpression(const std::string& expr);

    // Evaluate at x
    double evaluate(double x);

    // Parse error
    std::string lastError() const { return m_error; }
    bool valid() const { return m_valid; }

    // Evaluate complex variant
    Complex evaluateComplex(const Complex& z);

private:
    std::vector<Token> m_tokens;
    bool m_valid = false;
    std::string m_error;
    std::string m_expr;
};

#endif // MATH_ENGINE_HPP
